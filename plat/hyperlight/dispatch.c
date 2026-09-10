/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Guest function dispatch for Hyperlight.
 *
 * After the initial boot (evolve), the guest halts with the address
 * of hyperlight_dispatch_function in RAX.  The host can then invoke
 * guest functions by pushing a FunctionCall FlatBuffer onto the PEB
 * input stack, setting RIP to that address, and running the vCPU.
 *
 * The dispatch handler pops the FunctionCall from the input stack,
 * invokes the registered callback (if any), pushes a void result onto
 * the output stack, and halts via port 108.
 *
 * Callback registration:
 *   During boot, the kernel injects HL_DISPATCH_CALLBACK_PTR into the
 *   process environment — the hex address of g_dispatch_callback.
 *   A loaded ELF (e.g. hl_pydriver via app-elfloader) parses this env
 *   var and writes its callback function pointer there.  On each
 *   subsequent dispatch call, the kernel invokes that callback with
 *   the FunctionCall bytes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/arch/x86_64.h>
#include <uk/assert.h>
#include <uk/init.h>
#include <uk/print.h>

#include <hyperlight-x86/dispatch.h>
#include <hyperlight-x86/hcall.h>
#include <hyperlight-x86/mem.h>
#include <hyperlight-x86/outb.h>
#include <hyperlight-x86/peb.h>

/* ── PEB I/O stack pointers (cached from PEB at init) ─────────── */

static volatile __u8 *g_input_stack;
static __u64 g_input_stack_size;
static volatile __u8 *g_output_stack;
static __u64 g_output_stack_size;
static int g_dispatch_ready;

/*
 * Scratch exception stack top GVA — queried from the host at init
 * via GetExnStackTop.  The host reads the value from hyperlight-common
 * so the guest does not hardcode layout constants.
 */
__u64 g_exn_stack_top;

/* ── Callback registration ─────────────────────────────────────── */

typedef int (*hl_dispatch_fn_t)(const __u8 *fc, __u64 fc_len);
static volatile hl_dispatch_fn_t g_dispatch_callback;

/* Pointer/length of the current FunctionCall — set before the
 * callback is invoked so the application can read the raw bytes.
 */
static const __u8 *g_fc_bytes;
static __u64 g_fc_len;

const __u8 *hyperlight_dispatch_get_fc(__u64 *out_len)
{
	if (out_len)
		*out_len = g_fc_len;
	return g_fc_bytes;
}

/* ── Environment variable injection ───────────────────────────── */

/*
 * Inject kernel addresses into the process environment so loaded
 * ELFs (e.g. hl_pydriver via app-elfloader) can register their
 * dispatch callback and halt the VM without needing symbol
 * resolution against the kernel.
 *
 * Two env vars:
 *   HL_DISPATCH_CALLBACK_PTR  — address of g_dispatch_callback slot;
 *                               the driver writes its function pointer here.
 *   HL_DISPATCH_ENTRY         — address of hyperlight_dispatch_function;
 *                               the driver puts this in RAX before halting
 *                               so the host knows where to re-enter.
 *
 * Runs as a late init call — after posix-environ is initialised
 * but before boot.c calls main() (the elfloader).
 *
 * TODO: Replace this raw-address mechanism with a cleaner interface
 * (vDSO export, syscall, or device ioctl).
 */
static int hyperlight_dispatch_inject_env(struct uk_init_ctx *ictx __unused)
{
	static char env_cb[64];
	static char env_entry[64];
	static char env_getenv[64];

	snprintf(env_cb, sizeof(env_cb), "HL_DISPATCH_CALLBACK_PTR=0x%lx",
		 (unsigned long)&g_dispatch_callback);
	putenv(env_cb);

	snprintf(env_entry, sizeof(env_entry), "HL_DISPATCH_ENTRY=0x%lx",
		 (unsigned long)hyperlight_dispatch_function);
	putenv(env_entry);

	/*
	 * Export the address of hl_call_get_env_vars so the loaded ELF
	 * (driver) can call it directly.  The driver runs on glibc which
	 * has a separate environ from the kernel; calling this function
	 * is the only way to query host-provided env vars after boot.
	 */
	snprintf(env_getenv, sizeof(env_getenv),
		 "HL_GET_ENV_VARS_FN=0x%lx",
		 (unsigned long)hl_call_get_env_vars);
	putenv(env_getenv);

	return 0;
}

uk_late_initcall(hyperlight_dispatch_inject_env, 0x0);

/* ── Host-provided environment variables ─────────────────────────── */

/*
 * Check whether a KEY=VALUE entry uses a reserved key prefix.
 * Keys starting with "HL_" are reserved for Hyperlight internals
 * (HL_DISPATCH_CALLBACK_PTR, HL_DISPATCH_ENTRY, etc.) and must not
 * be overwritten by host-provided environment variables.
 */
static int is_reserved_env_key(const char *entry)
{
	return (entry[0] == 'H' && entry[1] == 'L' && entry[2] == '_');
}

/*
 * Inject host-provided environment variables into the guest process.
 *
 * The host registers a GetEnvVars function that returns a NUL-separated
 * string of KEY=VALUE pairs.  This init call queries that function and
 * injects each pair via putenv().
 *
 * The buffer is static so putenv() pointers remain valid for the
 * lifetime of the process.
 *
 * Keys starting with HL_ are silently skipped — they are reserved
 * for Hyperlight internal use.
 *
 * Runs right after the dispatch env injection (priority 0x1) so both
 * internal and host-provided env vars are set before main().
 */
static int hyperlight_dispatch_inject_host_env(struct uk_init_ctx *ictx __unused)
{
	static char env_buf[4096];
	int len;
	char *p, *end;

	len = hl_call_get_env_vars(env_buf, sizeof(env_buf));
	if (len <= 0)
		return 0; /* no env vars from host — not an error */

	p = env_buf;
	end = env_buf + len;
	while (p < end) {
		if (*p == '\0') {
			p++;
			continue;
		}
		if (!is_reserved_env_key(p))
			putenv(p);
		p += strlen(p) + 1;
	}

	return 0;
}

uk_late_initcall(hyperlight_dispatch_inject_host_env, 0x1);

/*
 * Re-inject host-provided environment variables.
 *
 * Called at the start of every dispatch (hyperlight_dispatch_inner).
 * On a normal boot this is a no-op (the initcall already ran and the
 * host typically returns the same vars).  After a snapshot restore the
 * initcall does NOT re-run, so this is the path that picks up env vars
 * set by the host after restore.
 *
 * Uses setenv() instead of putenv() because the buffer is on the stack
 * and must not outlive this call.  setenv() copies both key and value.
 */
static void hyperlight_dispatch_reinject_host_env(void)
{
	char buf[4096];
	int len;
	char *p, *end;

	len = hl_call_get_env_vars(buf, sizeof(buf));
	if (len <= 0)
		return;

	p = buf;
	end = buf + len;
	while (p < end) {
		char *eq;

		if (*p == '\0') {
			p++;
			continue;
		}
		if (is_reserved_env_key(p))
			goto next;

		eq = p;
		while (*eq && *eq != '=')
			eq++;
		if (*eq == '=') {
			*eq = '\0';
			setenv(p, eq + 1, 1);
			*eq = '=';
		}
next:
		/* Advance past the NUL separator.  We may have
		 * temporarily zeroed '=' above, but eq+1 points past
		 * the value; find the original terminator.
		 */
		while (p < end && *p != '\0')
			p++;
		p++;
	}
}

/*
 * Void FunctionCallResult — pre-encoded FlatBuffer.
 * Matches hyperlight-common 0.16.0.
 */
static const __u8 HL_VOID_RESULT[] = {
	0x2c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0xf4, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
	0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x00,
	0x07, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x09, 0x08, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
};

/* ── Public init ──────────────────────────────────────────────── */

void hyperlight_dispatch_init(const struct hyperlight_peb *peb)
{
	g_input_stack = (volatile __u8 *)peb->input_stack.ptr;
	g_input_stack_size = peb->input_stack.size;
	g_output_stack = (volatile __u8 *)peb->output_stack.ptr;
	g_output_stack_size = peb->output_stack.size;

	g_exn_stack_top = hl_call_get_exn_stack_top();
	if (!g_exn_stack_top)
		UK_CRASH("GetExnStackTop host function failed\n");

	g_dispatch_ready = 1;
}

/* ── Abort helper ──────────────────────────────────────────────── */

/*
 * Signal a dispatch error to the host via the Abort port (102).
 *
 * The abort protocol sends bytes in chunks of up to 3 per outb:
 *   u32 LE = [chunk_len, b1, b2, b3]
 * Byte sequence: [error_code, message..., 0xFF terminator].
 *
 * The host accumulates bytes until the 0xFF terminator, then returns
 * GuestAborted to the caller of sandbox.call().  On the next call()
 * the host resets RIP to hyperlight_dispatch_function, so the guest
 * resumes fresh.
 */
static void dispatch_send_abort(const char *msg)
{
	__u8 data[128];
	__sz len = 0;

	data[len++] = 1; /* error code: generic dispatch error */

	while (*msg && len < sizeof(data) - 1)
		data[len++] = (__u8)*msg++;

	data[len++] = 0xFF; /* terminator */

	/* Send in chunks of 3 bytes via port 102. */
	__sz i = 0;
	while (i < len) {
		__sz remaining = len - i;
		__sz chunk_len = remaining < 3 ? remaining : 3;
		__u32 val = (__u32)chunk_len;
		__sz j;
		for (j = 0; j < chunk_len; j++)
			val |= (__u32)data[i + j] << (8 * (j + 1));
		hyperlight_out32(HYPERLIGHT_PORT_ABORT, val);
		i += chunk_len;
	}
}

/* ── Dispatch inner ─────────────────────────────────────────────── */

void __attribute__((used))
hyperlight_dispatch_inner(void)
{
	/*
	 * Buffer for the FunctionCall FlatBuffer.
	 *
	 * The pop returns a pointer into the input stack.  Any subsequent
	 * host function call (including uk_pr_err via HostPrint) pushes a
	 * response onto the SAME input stack at the same offset, clobbering
	 * the data.  We must copy it out before doing anything that might
	 * trigger a host call.
	 *
	 * Static because dispatch is single-threaded (one vCPU) and the
	 * dispatch entry runs on the exception stack, which is too small
	 * for a 16 KiB stack allocation.
	 */
	static __u8 fc_buf[16384];
	const __u8 *fc_raw;
	__u64 fc_len;

	if (!g_dispatch_ready) {
		uk_pr_err("dispatch: not initialised\n");
		goto push_result;
	}

	/*
	 * Re-inject host-provided environment variables.
	 *
	 * On a normal boot this is redundant (the initcall already ran)
	 * but cheap — one host call that returns an empty string.
	 * After a snapshot restore the initcall does NOT re-run, so this
	 * is the only path that picks up env vars the host set after
	 * restore.
	 *
	 * Safe to call here: the GetEnvVars host call pushes/pops its
	 * own frame on top of the input stack; the FunctionCall for this
	 * dispatch remains below and is unaffected.
	 */
	hyperlight_dispatch_reinject_host_env();

	/* Pop the FunctionCall from the input stack.
	 * Copy immediately — the pointer is invalidated by any host call.
	 */
	if (hl_stack_pop(g_input_stack, &fc_raw, &fc_len) < 0) {
		uk_pr_err("dispatch: no function call on input stack\n");
		goto push_result;
	}

	if (fc_len > sizeof(fc_buf)) {
		uk_pr_err("dispatch: FunctionCall too large (%lu bytes)\n",
			  (unsigned long)fc_len);
		goto push_result;
	}

	memcpy(fc_buf, (const void *)fc_raw, fc_len);

	/* Make the FunctionCall bytes available to the callback */
	g_fc_bytes = fc_buf;
	g_fc_len = fc_len;

	if (g_dispatch_callback) {
		int dispatch_rc = g_dispatch_callback(fc_buf, fc_len);

		g_fc_bytes = NULL;
		g_fc_len = 0;

		if (dispatch_rc != 0) {
			/*
			 * Abort the VM so the host's run() returns
			 * Err(GuestAborted).  The host resets RIP on
			 * the next call(), so the guest resumes fresh.
			 */
			dispatch_send_abort("dispatch callback failed");
			return;
		}
	} else {
		g_fc_bytes = NULL;
		g_fc_len = 0;
		uk_pr_warn("dispatch: no callback registered (fc_len=%lu)\n",
			   (unsigned long)fc_len);
	}

push_result:
	hl_stack_push(g_output_stack, g_output_stack_size,
		      HL_VOID_RESULT, sizeof(HL_VOID_RESULT));
}

/* ── Snapshot pre-fault ─────────────────────────────────────────── */

/*
 * Pre-fault all writable kernel pages after snapshot restore.
 *
 * Snapshot/restore marks every writable page Copy-on-Write (PTE
 * read-only + AVL bit 9).  The first write triggers a page fault.
 * Unikraft's IDT uses IST stacks for the #PF handler — but those
 * stacks are themselves CoW after restore.  The CPU can't push the
 * exception frame onto a read-only IST stack, so a CoW fault while
 * IST stacks are still CoW causes:
 *
 *   #PF → push to IST2 (CoW) → #PF → #DF → push to IST3 (CoW)
 *       → #PF → triple fault → shutdown
 *
 * This function runs under a temporary IDT whose #PF entry uses
 * IST=0 (current RSP on the scratch stack, always writable).  It
 * pre-faults all kernel .data and .bss pages — including the IST
 * stacks, IDT, TSS, and GDT arrays — so the full Unikraft IDT/IST
 * mechanism works for subsequent CoW faults.
 *
 * Called from hyperlight_dispatch_function's snapshot-restore path.
 */
void __attribute__((used))
hyperlight_dispatch_prefault(void)
{
	extern char _data[], _end[];
	volatile __u8 *p;

	for (p = (volatile __u8 *)_data; p < (volatile __u8 *)_end;
	     p += __PAGE_SIZE) {
		__u8 tmp = *p;
		*(volatile __u8 *)p = tmp;
	}
}

/* ── SYSCALL MSR fixup ─────────────────────────────────────────── */

/*
 * Re-program SYSCALL MSRs after snapshot restore.
 *
 * Hyperlight's snapshot/restore preserves segment registers and
 * control registers (via CommonSpecialRegisters) but does NOT
 * save or restore the declared guest MSRs (LSTAR, STAR,
 * SYSCALL_MASK) — they are reset to their baseline (0) on restore.
 * The standard Hyperlight guest uses OUT/HLT instead of SYSCALL, so
 * this was never needed.  Unikraft programs these MSRs during boot
 * (setup.c) to support ring-3 ELFs via elfloader.  After
 * snapshot/restore they are 0, causing any SYSCALL to jump to RIP=0.
 *
 * EFER is NOT re-programmed here: hyperlight restores it (with SCE)
 * as part of the special-register state, and it cannot be written by
 * the guest under the 0.17.0 default-deny MSR filter — see setup.c.
 *
 * Called from the snapshot fixup path in hyperlight_dispatch_function.
 */
#ifdef CONFIG_HAVE_SYSCALL
extern void _ukplat_syscall(void);
#endif

void __attribute__((used))
hyperlight_dispatch_fixup_syscall(void)
{
#ifdef CONFIG_HAVE_SYSCALL
	uk_arch_x86_64_wrmsrl(UK_ARCH_X86_64_MSR_STAR,
		(0x08ULL << 48) | (0x08ULL << 32));
	uk_arch_x86_64_wrmsrl(UK_ARCH_X86_64_MSR_LSTAR,
		(__uptr)_ukplat_syscall);
	uk_arch_x86_64_wrmsrl(UK_ARCH_X86_64_MSR_SYSCALL_MASK,
		UK_ARCH_X86_64_RFLAGS_TF |
		UK_ARCH_X86_64_RFLAGS_DF |
		UK_ARCH_X86_64_RFLAGS_IF |
		UK_ARCH_X86_64_RFLAGS_AC |
		UK_ARCH_X86_64_RFLAGS_NT);
#endif
}

/* ── Dispatch entry point ───────────────────────────────────────── */

/*
 * Entry point for guest function calls.  The host sets RIP here
 * and runs the vCPU.  This function never returns — it halts the
 * vCPU after pushing the result.
 *
 * On entry, RFLAGS.ZF indicates whether a TLB flush is needed
 * (set by the host after snapshot/restore when page tables changed).
 *
 * RSP is loaded from g_exn_stack_top (the scratch exception stack
 * address queried from the host at init).  After evolve, the host
 * captures the guest's boot stack address (in BSS) as rsp_gva;
 * after snapshot/restore, that BSS page is Copy-on-Write (read-only).
 * Any push to a CoW page triggers #PF, but the IST stacks are also
 * CoW, causing a triple fault.  The scratch stack avoids this.
 *
 * Reading g_exn_stack_top (a BSS variable) before flushing the TLB
 * is safe: CoW pages are readable, and on a fresh-from-snapshot
 * vCPU the TLB is empty so the load fills from the new page tables.
 *
 * After snapshot/restore (ZF set), we must:
 *   1. Flush the TLB (page tables relocated to scratch)
 *   2. Install a temporary IDT with a #PF handler that uses IST=0
 *      (runs on the current scratch stack, always writable)
 *   3. Pre-fault all writable kernel pages so the IST stacks,
 *      IDT, TSS, and GDT become writable again
 *   4. Restore the full Unikraft IDT
 *   5. Continue with normal dispatch
 *
 * The temporary IDT reuses _cow_asm_pf_handler from entry64.S.
 */
void __attribute__((noreturn, naked))
hyperlight_dispatch_function(void)
{
	__asm__ volatile(
		/*
		 * Load RSP from the scratch exception stack top.
		 * The value was queried from the host at init.
		 *
		 * On the normal dispatch path (ZF=0) the TLB is valid,
		 * so this read of the BSS variable is correct.  On the
		 * snapshot/restore path (ZF=1) the vCPU may be reused
		 * with a STALE TLB (in-place MultiUseSandbox::restore
		 * relocates the page tables into fresh scratch but does
		 * not flush our TLB), so this read can return a stale
		 * value — we re-load it below after flushing the TLB.
		 */
		"movq g_exn_stack_top(%%rip), %%rsp\n\t"

		/*
		 * ZF=1: snapshot/restore path (needs fixup).
		 * ZF=0: normal dispatch (skip to 1f).
		 */
		"jnz 1f\n\t"

		/* ── Snapshot fixup ────────────────────────────── */

		/* Flush TLB: reload CR3.  Page tables are in new
		 * scratch; stale TLB entries must go.
		 */
		"movq %%cr3, %%rax\n\t"
		"movq %%rax, %%cr3\n\t"

		/*
		 * Re-load RSP from g_exn_stack_top now that the TLB is
		 * fresh: the load above may have used a stale entry that
		 * mapped the BSS to a pre-restore physical page, yielding
		 * 0 and a wild RSP (→ #PF on the sidt below → triple
		 * fault).  With the TLB flushed this reads the restored
		 * value from the new page tables.
		 */
		"movq g_exn_stack_top(%%rip), %%rsp\n\t"

		/* Save the original IDTR (10 bytes, 16 allocated). */
		"subq $16, %%rsp\n\t"
		"sidt (%%rsp)\n\t"

		/*
		 * Build a temporary 15-entry IDT on the stack.
		 * Only entry #14 (#PF) is populated — everything
		 * else is zeroed (absent → #GP on delivery, which
		 * is fine: we only need #PF during pre-fault).
		 *
		 * IST=0 means the handler runs on the current
		 * (scratch) stack, not the CoW IST stacks.
		 */
		"subq $240, %%rsp\n\t"
		"movq %%rsp, %%rbx\n\t"
		"movq %%rsp, %%rdi\n\t"
		"xorq %%rax, %%rax\n\t"
		"movq $30, %%rcx\n\t"
		"cld\n\t"
		"rep stosq\n\t"

		/* Populate entry #14: _cow_asm_pf_handler, IST=0 */
		"leaq _cow_asm_pf_handler(%%rip), %%rax\n\t"
		"movw %%ax, 224(%%rbx)\n\t"
		"movw $0x08, 226(%%rbx)\n\t"
		"movb $0, 228(%%rbx)\n\t"
		"movb $0x8E, 229(%%rbx)\n\t"
		"shrq $16, %%rax\n\t"
		"movw %%ax, 230(%%rbx)\n\t"
		"shrq $16, %%rax\n\t"
		"movl %%eax, 232(%%rbx)\n\t"

		/* Load the temporary IDTR. */
		"subq $16, %%rsp\n\t"
		"movw $239, (%%rsp)\n\t"
		"movq %%rbx, 2(%%rsp)\n\t"
		"lidt (%%rsp)\n\t"
		"addq $16, %%rsp\n\t"

		/*
		 * Pre-fault all writable kernel pages (.data/.bss).
		 * This resolves CoW for IST stacks, IDT, TSS, and
		 * GDT arrays so the full Unikraft IDT works again.
		 */
		"callq hyperlight_dispatch_prefault\n\t"

		/* Restore the original IDTR (saved above the IDT). */
		"lidt 240(%%rbx)\n\t"

		/* Pop temp IDT (240) + saved IDTR (16). */
		"addq $256, %%rsp\n\t"

		/*
		 * Re-program SYSCALL MSRs (LSTAR, STAR, SYSCALL_MASK).
		 * Hyperlight does not save/restore MSRs across snapshots.
		 */
		"callq hyperlight_dispatch_fixup_syscall\n\t"

		/*
		 * Re-initialise the paging frame allocator.
		 * The FA lives entirely in scratch, which was zeroed on
		 * restore.  Without this, demand paging (mmap, stack
		 * growth) crashes on NULL FA function pointers.
		 */
		"callq hyperlight_paging_reinit\n\t"

		"1:\n\t"
		/* ── Normal dispatch ───────────────────────────── */
		"callq hyperlight_dispatch_inner\n\t"

		/* Halt — port 108 = dispatch complete. */
		"movw $108, %%dx\n\t"
		"outl %%eax, %%dx\n\t"
		"cli\n\t"
		"hlt\n\t"
		: : : "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "memory"
	);
	__builtin_unreachable();
}

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Hyperlight guest function dispatch shim for Unikraft unikernels.
 *
 * Copyright (c) 2024 Microsoft Corporation
 */

#include <uk/arch/types.h>
#include <hyperlight-x86/outb.h>
#include <hyperlight-x86/peb.h>

static const __u8 HL_VOID_RESULT[] = {
	0x2c,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
	0xf4,0xff,0xff,0xff,0x00,0x00,0x00,0x01,
	0x0c,0x00,0x00,0x00,0x08,0x00,0x0c,0x00,
	0x07,0x00,0x08,0x00,0x08,0x00,0x00,0x00,
	0x00,0x00,0x00,0x09,0x08,0x00,0x00,0x00,
	0x04,0x00,0x04,0x00,0x04,0x00,0x00,0x00,
};

static void hl_buffer_push(volatile void *buf_ptr, __u64 buf_size,
			    const void *data, __u64 data_len)
{
	volatile __u64 *sp = (volatile __u64 *)buf_ptr;
	__u64 old_sp = *sp;
	__u8 *base = (__u8 *)buf_ptr;
	if (old_sp + data_len + 8 > buf_size)
		return;
	__builtin_memcpy(base + old_sp, data, data_len);
	*(volatile __u64 *)(base + old_sp + data_len) = old_sp;
	*sp = old_sp + data_len + 8;
}

static void hl_buffer_pop(volatile void *buf_ptr)
{
	volatile __u64 *sp = (volatile __u64 *)buf_ptr;
	__u64 cur_sp = *sp;
	if (cur_sp < 16)
		return;
	__u64 back_ptr = *(volatile __u64 *)((__u8 *)buf_ptr + cur_sp - 8);
	*sp = back_ptr;
}

/* Kernel FS_BASE for syscall context restore.
 * Set by the elfloader dispatch setup; extern'd in sysctx_auxsp.c.
 */
__uptr hyperlight_kernel_fsbase;

typedef void (*hl_run_fn)(void);
static volatile hl_run_fn g_run_callback;
static struct hyperlight_peb g_dispatch_peb;

/* MSR helpers */
#define MSR_KERNEL_GS_BASE 0xC0000102
#define MSR_STAR            0xC0000081
#define MSR_LSTAR           0xC0000082
#define MSR_SFMASK          0xC0000084

static inline __u64 rdmsr(__u32 msr) {
	__u32 lo, hi;
	__asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((__u64)hi << 32) | lo;
}
static inline void wrmsr(__u32 msr, __u64 val) {
	__asm__ volatile("wrmsr" : : "a"((__u32)val),
			 "d"((__u32)(val >> 32)), "c"(msr));
}

/* Saved state from init */
static __u64 g_elf_entry;
static __u64 g_saved_lstar;
static __u64 g_saved_star;
static __u64 g_saved_sfmask;
static __u64 g_saved_kernel_gs_base;

void hyperlight_dispatch_register(hl_run_fn fn) { g_run_callback = fn; }
void hyperlight_dispatch_init(const struct hyperlight_peb *peb) {
	__builtin_memcpy(&g_dispatch_peb, peb, sizeof(g_dispatch_peb));
}
void hyperlight_dispatch_set_elf_entry(__u64 e) { g_elf_entry = e; }
__u64 hyperlight_dispatch_get_elf_entry(void) { return g_elf_entry; }
__u64 hyperlight_dispatch_get_saved_kernel_gs_base(void) { return g_saved_kernel_gs_base; }

void hyperlight_dispatch_save_msrs(void)
{
	g_saved_lstar = rdmsr(MSR_LSTAR);
	g_saved_star = rdmsr(MSR_STAR);
	g_saved_sfmask = rdmsr(MSR_SFMASK);
	g_saved_kernel_gs_base = rdmsr(MSR_KERNEL_GS_BASE);
}

void hyperlight_dispatch_push_void_result(void)
{
	hl_buffer_push((void *)g_dispatch_peb.output_stack.ptr,
		       g_dispatch_peb.output_stack.size,
		       HL_VOID_RESULT, sizeof(HL_VOID_RESULT));
}

/**
 * Prepare the CPU exception environment for dispatch after restore.
 *
 * After snapshot/restore, the kernel's IST stacks (in .bss) are CoW
 * (read-only).  Any CPU exception would try to push to a CoW IST
 * stack, triggering a #PF during delivery → double fault → triple
 * fault (silent VM reset).
 *
 * Strategy:
 * 1. Save the kernel's IDTR
 * 2. Install a temporary IDT on the scratch stack with IST=0
 *    (exceptions push to current stack, which is scratch = writable)
 * 3. Restore MSRs (LSTAR, STAR, SFMASK, KERNEL_GS_BASE)
 * 4. Pre-fault the kernel's IST stacks by writing to them
 *    (CoW faults resolved by temp IDT handler on scratch stack)
 * 5. Reload the kernel's IDT (IST stacks now writable)
 */
static void hyperlight_dispatch_prepare(void)
{
	extern void _cow_asm_pf_handler(void);

	/* 1. Save the kernel's IDTR */
	struct {
		__u16 limit;
		__u64 base;
	} __attribute__((packed)) saved_idtr;
	__asm__ volatile("sidt %0" : "=m"(saved_idtr));

	/* 2. Build temporary IDT on stack with IST=0 */
	__u8 __attribute__((aligned(16))) tmp_idt[32 * 16];
	__builtin_memset(tmp_idt, 0, sizeof(tmp_idt));
	__u64 handler = (__u64)_cow_asm_pf_handler;
	for (int i = 0; i < 32; i++) {
		__u8 *e = &tmp_idt[i * 16];
		*(__u16 *)(e + 0) = (__u16)handler;
		*(__u16 *)(e + 2) = 0x08;
		*(e + 4) = 0; /* IST=0: use current stack */
		*(e + 5) = 0x8E;
		*(__u16 *)(e + 6) = (__u16)(handler >> 16);
		*(__u32 *)(e + 8) = (__u32)(handler >> 32);
		*(__u32 *)(e + 12) = 0;
	}
	struct {
		__u16 limit;
		__u64 base;
	} __attribute__((packed)) tmp_idtr = {
		.limit = sizeof(tmp_idt) - 1,
		.base = (__u64)tmp_idt,
	};
	__asm__ volatile("lidt %0" : : "m"(tmp_idtr));

	/* 3. Restore MSRs */
	wrmsr(MSR_LSTAR, g_saved_lstar);
	wrmsr(MSR_STAR, g_saved_star);
	wrmsr(MSR_SFMASK, g_saved_sfmask);
	wrmsr(MSR_KERNEL_GS_BASE, g_saved_kernel_gs_base);

	/* 4. Pre-fault kernel IST stacks.
	 * Read IST addresses from the TSS (found via GDT + TR).
	 * Write to the top 2 pages of each to trigger CoW resolution.
	 */
	{
		struct {
			__u16 limit;
			__u64 base;
		} __attribute__((packed)) gdtr;
		__asm__ volatile("sgdt %0" : "=m"(gdtr));

		__u16 tr;
		__asm__ volatile("str %0" : "=r"(tr));

		__u8 *gdt = (__u8 *)gdtr.base;
		__u8 *tss_desc = gdt + (tr & ~0x7);
		__u64 tss_base =
			((__u64)tss_desc[2]) |
			((__u64)tss_desc[3] << 8) |
			((__u64)tss_desc[4] << 16) |
			((__u64)tss_desc[7] << 24) |
			((__u64)*(__u32 *)(tss_desc + 8) << 32);

		/* TSS layout: IST1 @ +36, IST2 @ +44, IST3 @ +52 */
		__u64 ist1 = *(volatile __u64 *)(tss_base + 36);
		__u64 ist2 = *(volatile __u64 *)(tss_base + 44);
		__u64 ist3 = *(volatile __u64 *)(tss_base + 52);

		/* Pre-fault top 2 pages of each IST stack */
		if (ist1) {
			*(volatile __u64 *)(ist1 - 8) = 0;
			*(volatile __u64 *)(ist1 - 4096) = 0;
		}
		if (ist2) {
			*(volatile __u64 *)(ist2 - 8) = 0;
			*(volatile __u64 *)(ist2 - 4096) = 0;
		}
		if (ist3) {
			*(volatile __u64 *)(ist3 - 8) = 0;
			*(volatile __u64 *)(ist3 - 4096) = 0;
		}
	}

	/* 5. Reload the kernel's IDT (IST stacks now writable) */
	__asm__ volatile("lidt %0" : : "m"(saved_idtr));

	/* 6. Reset nested exception counter.
	 * Must happen AFTER step 5: this writes to .bss which is CoW.
	 * With the kernel's IDT restored (and IST stacks now writable),
	 * any CoW fault from this write is handled correctly.
	 */
	
	
}

/* Dispatch entry/exit */
void __attribute__((noreturn, naked))
hyperlight_dispatch_function(void)
{
	__asm__ volatile(
		"jnz 1f\n\t"
		"movq %%cr4, %%rdi\n\t"
		"xorq $0x80, %%rdi\n\t"
		"movq %%rdi, %%cr4\n\t"
		"xorq $0x80, %%rdi\n\t"
		"movq %%rdi, %%cr4\n\t"
		"1:\n\t"
		"callq hyperlight_dispatch_inner\n\t"
		"movw $108, %%dx\n\t"
		"outl %%eax, %%dx\n\t"
		"cli\n\t"
		"hlt\n\t"
		: : : "rdi", "rdx", "memory"
	);
	__builtin_unreachable();
}

void __attribute__((used))
hyperlight_dispatch_inner(void)
{
	hyperlight_dispatch_prepare();
	hl_buffer_pop((void *)g_dispatch_peb.input_stack.ptr);
	if (g_run_callback)
		g_run_callback();
	hyperlight_dispatch_push_void_result();
}

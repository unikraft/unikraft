/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/arch/x86_64.h>
#include <x86/traps.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/paging.h>
#include <uk/asm/cfi.h>
#include <uk/boot.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/intctlr.h>

#include <uk/lcpu.h>
#include <uk/plat/memory.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>

#include <hyperlight-x86/peb.h>
#include <hyperlight-x86/outb.h>
#include <hyperlight-x86/setup.h>
#include <hyperlight-x86/dispatch.h>

#if CONFIG_HAVE_SYSCALL
/* Syscall entrance provided by platform library */
void _ukplat_syscall(void);

static inline void _init_syscall(void)
{
	/* Skip CPUID check — x86-64 always has SYSCALL.
	 * Skip EFER write — Hyperlight sets EFER via SREGS.
	 * Program STAR, LSTAR, SFMASK which Hyperlight handles
	 * via MSR interception.
	 */
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

	uk_pr_info("SYSCALL entrance @ %p\n", _ukplat_syscall);
}
#endif /* CONFIG_HAVE_SYSCALL */

/* PKU check — Hyperlight doesn't use PKU, just set OSPKE to 0 */
static inline void _check_ospke(void)
{
	/* no-op */
}

/* Magic header for cmdline embedded in initrd from hyperlight-unikraft host */
#define HYPERLIGHT_CMDLINE_MAGIC "HLCMDLN\0"
#define HYPERLIGHT_CMDLINE_MAGIC_LEN 8

/* Magic for the optional hostfs-mount TLV that follows the cmdline. */
#define HYPERLIGHT_MOUNT_MAGIC "HLHSMNT\0"
#define HYPERLIGHT_MOUNT_MAGIC_LEN 8

/* Maximum combined cmdline length */
#define HYPERLIGHT_MAX_CMDLINE 1024

/* Maximum hostfs mount path advertised by the host. */
#define HYPERLIGHT_MAX_MOUNT_PATH 256

/* Forward declaration for console init */
void _ukplat_init_console(void);

/* Forward declaration for platform entry */
void _ukplat_entry(struct ukplat_bootinfo *bi);

/* Global PEB pointer */
static struct hyperlight_peb *g_peb;

/* Global entry args (defined in entry64.S) */
extern struct hyperlight_entry_args hyperlight_entry_args;

/* Static buffer for combined cmdline */
static char hyperlight_cmdline_buf[HYPERLIGHT_MAX_CMDLINE];

/* Static buffer for an optional hostfs mount path advertised by the host
 * via the HLHSMNT TLV. Empty string means "unset" (use kconfig default).
 */
static char hyperlight_hostfs_mountpoint[HYPERLIGHT_MAX_MOUNT_PATH];

/**
 * Return the runtime-configured hostfs mount path, or NULL if the host
 * didn't advertise one. Callable from lib/hostfs at auto-mount time.
 */
const char *hyperlight_hostfs_mountpoint_from_host(void)
{
	return hyperlight_hostfs_mountpoint[0] ? hyperlight_hostfs_mountpoint : 0;
}

/* Base cmdline with random seed (kernel args) */
static const char hyperlight_base_cmdline[] =
	"unikraft-hyperlight "
	"random.seed=[0x12345678 0x9abcdef0 "
	"0x13579bdf 0x2468ace0 0xfedcba98 0x76543210 "
	"0x0f1e2d3c 0x4b5a6978] --";

/**
 * Extract cmdline from initrd if it has magic header.
 *
 * Format (from hyperlight-unikraft host):
 * | Magic (8 bytes): "HLCMDLN\0" |
 * | Cmdline length (4 bytes LE)  |
 * | Cmdline data (null-term)     |
 * | Padding to UK_PAL_PAGE_SIZE boundary|
 * | Original initrd...           |
 *
 * @param init_ptr   Pointer to init_data pointer (updated if header found)
 * @param init_size  Pointer to init_data size (updated if header found)
 * @return Pointer to extracted cmdline (null-terminated), or NULL if no header
 */
static const char *extract_cmdline_from_initrd(__u64 *init_ptr, __u64 *init_size)
{
	const char *data = (const char *)(*init_ptr);
	__u64 size = *init_size;

	/* Check minimum size for header */
	if (size < HYPERLIGHT_CMDLINE_MAGIC_LEN + 4)
		return NULL;

	/* Check magic */
	if (memcmp(data, HYPERLIGHT_CMDLINE_MAGIC, HYPERLIGHT_CMDLINE_MAGIC_LEN) != 0)
		return NULL;

	/* Read cmdline length (4 bytes LE) */
	const unsigned char *len_ptr = (const unsigned char *)(data + HYPERLIGHT_CMDLINE_MAGIC_LEN);
	__u32 cmdline_len = len_ptr[0] | (len_ptr[1] << 8) |
			    (len_ptr[2] << 16) | (len_ptr[3] << 24);

	/* Calculate total TLV size: cmdline + optional hostfs mount TLV. */
	__u64 unpadded =
		HYPERLIGHT_CMDLINE_MAGIC_LEN + 4 + cmdline_len + 1;

	/* Optional HLHSMNT TLV immediately after the cmdline NUL. */
	if (unpadded + HYPERLIGHT_MOUNT_MAGIC_LEN + 4 <= size
	    && memcmp(data + unpadded, HYPERLIGHT_MOUNT_MAGIC,
		      HYPERLIGHT_MOUNT_MAGIC_LEN) == 0) {
		const unsigned char *ml_ptr = (const unsigned char *)
			(data + unpadded + HYPERLIGHT_MOUNT_MAGIC_LEN);
		__u32 mount_len = ml_ptr[0] | (ml_ptr[1] << 8) |
				  (ml_ptr[2] << 16) | (ml_ptr[3] << 24);

		if (mount_len < HYPERLIGHT_MAX_MOUNT_PATH &&
		    unpadded + HYPERLIGHT_MOUNT_MAGIC_LEN + 4
		    + mount_len + 1 <= size) {
			const char *mount = data + unpadded
				+ HYPERLIGHT_MOUNT_MAGIC_LEN + 4;
			memcpy(hyperlight_hostfs_mountpoint, mount,
			       mount_len);
			hyperlight_hostfs_mountpoint[mount_len] = '\0';
			unpadded += HYPERLIGHT_MOUNT_MAGIC_LEN + 4
				+ mount_len + 1;
		}
	}

	/* Pad whole TLV block to page boundary. */
	__u64 padded_header_size = (unpadded + UK_PAL_PAGE_SIZE - 1)
		& ~(UK_PAL_PAGE_SIZE - 1);

	if (padded_header_size > size)
		return NULL;

	/* Extract cmdline pointer */
	const char *cmdline = data + HYPERLIGHT_CMDLINE_MAGIC_LEN + 4;

	/* Update init_data to point past the header (page-aligned) */
	*init_ptr = *init_ptr + padded_header_size;
	*init_size = size - padded_header_size;

	return cmdline;
}

struct hyperlight_peb *hyperlight_get_peb(void)
{
	return g_peb;
}

const struct hyperlight_entry_args *hyperlight_get_entry_args(void)
{
	return &hyperlight_entry_args;
}

static void __noreturn ukplat_entry2(void)
{
	/* It's not possible to unwind past this function, because the stack
	 * pointer was overwritten in lcpu_arch_jump_to. Therefore, mark the
	 * previous instruction pointer as undefined, so that debuggers or
	 * profilers stop unwinding here.
	 */
	ukarch_cfi_unwind_end();

	uk_boot_entry();
	UK_BUG(); /* noreturn */
}

static void hyperlight_init_mem(struct ukplat_bootinfo *bi,
				struct hyperlight_peb *peb)
{
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	/* Add heap region as free memory */
	if (peb->guest_heap.size > 0) {
		mrd.pbase = peb->guest_heap.ptr;
		mrd.vbase = mrd.pbase; /* 1:1 mapping */
		mrd.pg_off = 0;
		mrd.len = peb->guest_heap.size;
		mrd.pg_count = UK_PAGING_PAGE_COUNT(mrd.len);
		mrd.type = UKPLAT_MEMRT_FREE;
		mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#ifdef CONFIG_UKPLAT_MEMRNAME
		memcpy(mrd.name, "heap", sizeof("heap"));
#endif
		rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
		if (unlikely(rc < 0))
			UK_CRASH("Unable to add heap region");
	}

	/* Add init data region if present */
	if (peb->init_data.size > 0) {
		uk_pr_info("Hyperlight: initrd at 0x%lx, size 0x%lx\n",
			   (unsigned long)peb->init_data.ptr,
			   (unsigned long)peb->init_data.size);
		mrd.pbase = peb->init_data.ptr;
		mrd.vbase = mrd.pbase;
		mrd.pg_off = 0;
		mrd.len = peb->init_data.size;
		mrd.pg_count = UK_PAGING_PAGE_COUNT(mrd.len);
		mrd.type = UKPLAT_MEMRT_INITRD;
		mrd.flags = UKPLAT_MEMRF_READ;
#ifdef CONFIG_UKPLAT_MEMRNAME
		memcpy(mrd.name, "initrd", sizeof("initrd"));
#endif
		rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
		if (unlikely(rc < 0))
			UK_CRASH("Unable to add init data region");
	} else {
		uk_pr_warn("Hyperlight: No initrd in init_data\n");
	}

	/* Check for initrd mapped via map_file_cow.
	 * When the host uses map_file_cow, init_data contains only the
	 * cmdline header with the mapped file size in the last 8 bytes.
	 * After cmdline extraction, init_data.size == 0 but the original
	 * init_data had the size at (ptr + original_size - 8).
	 */
#define INITRD_MAP_BASE 0xC0000000ULL  /* Must match host INITRD_MAP_BASE */
	if (peb->init_data.size == 0 && g_peb->init_data.size >= 8) {
		__u64 mapped_size = *(__u64 *)(
			(__u8 *)g_peb->init_data.ptr + g_peb->init_data.size - 8
		);
		if (mapped_size > 0) {
			uk_pr_info("Hyperlight: initrd mapped at 0x%lx, size 0x%lx (zero-copy)\n",
				   (unsigned long)INITRD_MAP_BASE,
				   (unsigned long)mapped_size);

			/* Register with demand-paging handler */
			extern void cow_register_mapped_file(__u64, __u64);
			cow_register_mapped_file(INITRD_MAP_BASE, mapped_size);

			/* Register as INITRD region for VFS */
			mrd.pbase = INITRD_MAP_BASE;
			mrd.vbase = INITRD_MAP_BASE;
			mrd.pg_off = 0;
			mrd.len = mapped_size;
			mrd.pg_count = UK_PAGING_PAGE_COUNT(mrd.len);
			mrd.type = UKPLAT_MEMRT_INITRD;
			mrd.flags = UKPLAT_MEMRF_READ;
#ifdef CONFIG_UKPLAT_MEMRNAME
			memcpy(mrd.name, "initrd", sizeof("initrd"));
#endif
			rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
			if (unlikely(rc < 0))
				UK_CRASH("Unable to add mapped initrd region");
		}
	}
}

/**
 * Entry function called from assembly after basic CPU initialization.
 *
 * @param lcpu Pointer to the LCPU structure
 * @param entry_args Pointer to hyperlight_entry_args structure
 */
void hyperlight_entry(struct uk_lcpu *lcpu,
		      struct hyperlight_entry_args *entry_args)
{
	struct ukplat_bootinfo *bi;
	const char *app_args = NULL;
	__u64 init_ptr, init_size;


	/* Debug: signal we reached C code */

	/* Initialize console */
	_ukplat_init_console();

	/* Store PEB globally */
	g_peb = (struct hyperlight_peb *)entry_args->peb_address;

	if (unlikely(!g_peb))
		UK_CRASH("PEB address is NULL");

	/* Cache PEB for dispatch (PEB is in read-only snapshot memory) */
	hyperlight_dispatch_init(g_peb);

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Incompatible or corrupted bootinfo");

	/* Check if init_data contains embedded cmdline from hyperlight-unikraft.
	 * If so, extract it and adjust init_data to point to actual initrd.
	 * NOTE: Do NOT write back to g_peb — the PEB lives in Hyperlight's
	 * snapshot memory which may be read-only.  Use a local copy instead.
	 */
	struct hyperlight_peb local_peb;
	memcpy(&local_peb, g_peb, sizeof(local_peb));

	if (local_peb.init_data.size > 0) {
		init_ptr = local_peb.init_data.ptr;
		init_size = local_peb.init_data.size;

		app_args = extract_cmdline_from_initrd(&init_ptr, &init_size);

		if (app_args) {
			uk_pr_info("Hyperlight: Extracted app args: %s\n",
				   app_args);
			local_peb.init_data.ptr = init_ptr;
			local_peb.init_data.size = init_size;
		}
	}

	/* Initialize memory regions from local PEB copy */
	hyperlight_init_mem(bi, &local_peb);

	memcpy(bi->bootprotocol, "hyperlight", sizeof("hyperlight"));

	/* Build combined cmdline: base args + app args (if any).
	 * Format: <kernel-name> <kernel-args> [-- <app-args>]
	 */
	{
		size_t base_len = sizeof(hyperlight_base_cmdline) - 1;
		size_t app_len = app_args ? strlen(app_args) : 0;
		size_t total_len = base_len + (app_len > 0 ? 1 + app_len : 0);

		if (total_len >= HYPERLIGHT_MAX_CMDLINE)
			UK_CRASH("Cmdline too long");

		memcpy(hyperlight_cmdline_buf, hyperlight_base_cmdline, base_len);

		if (app_args && app_len > 0) {
			hyperlight_cmdline_buf[base_len] = ' ';
			memcpy(hyperlight_cmdline_buf + base_len + 1,
			       app_args, app_len);
			hyperlight_cmdline_buf[base_len + 1 + app_len] = '\0';
		} else {
			hyperlight_cmdline_buf[base_len] = '\0';
		}

		uk_pr_info("Hyperlight: cmdline: %s\n", hyperlight_cmdline_buf);

		bi->cmdline = (__u64)hyperlight_cmdline_buf;
		bi->cmdline_len = strlen(hyperlight_cmdline_buf);
	}

	_ukplat_entry(bi);
}

/* At this point we expect that the C runtime is configured and that
 * bootcode has enabled all CPU features used by compiled code.
 */
void _ukplat_entry(struct ukplat_bootinfo *bi)
{
	void *bstack;
	int rc;


	/* Ensure interrupts are disabled. */
	__asm__ __volatile__("cli" ::: "memory");


	/* Pre-fault the IST exception stacks before setting up the IDT.
	 * The native PAL's except_init points IST to lcpu_except_stack (BSS),
	 * which is CoW on Hyperlight. If we don't pre-fault these pages,
	 * the first exception will triple-fault trying to push onto a
	 * read-only IST stack.
	 * At this point, the asm CoW handler (from entry64.S) is active.
	 */
	{
		extern __uptr uk_plat_native_except_get_except_stack_base(void);
		__uptr base = uk_plat_native_except_get_except_stack_base();
		volatile __u8 *p;
		__u8 tmp;
		/* Pre-fault the entire except module's BSS region.
		 * Layout relative to base (lcpu_except_stack):
		 *   base - 0xd0: idtptr, cpu_gdt64, cpu_tss
		 *   base + 0:    lcpu_except_stack (192KB)
		 *   base + 0x30008: cpu_idt (4KB)
		 * Start one page before base to cover GDT/TSS/idtptr.
		 * End at base + 0x31000 to cover IDT.
		 */
		for (__uptr off = 0; off < 0x32000; off += 0x1000) {
			p = (volatile __u8 *)(base - 0x1000 + off);
			tmp = *p;
			*p = tmp;
		}
	}

	/* Initialize LCPU of bootstrap processor.
	 * This also initializes the GDT, TSS, IDT (via uk_pal_except_init).
	 */
	rc = uk_lcpu_init(uk_pcpuvar_current_ptr_get(uk_lcpus));


	if (unlikely(rc))
		UK_CRASH("Bootstrap processor init failed: %d\n", rc);


	/* Initialize CoW handler (must be after IDT is set up) */
	extern void hyperlight_cow_init(void);
	hyperlight_cow_init();


	/* Execute early init */
	/* Register PM ops before early init (which may crash) */
	{
		extern int hyperlight_register_pm_ops(struct ukplat_bootinfo *);
		hyperlight_register_pm_ops(bi);
	}

	uk_boot_early_init(bi);



	/* Initialize IRQ controller */
	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Interrupt controller init failed: %d\n", rc);


	/* Initialize paging */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Failed to initialize memory: %d\n", rc);


	/* Allocate boot stack */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");

	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

#ifdef CONFIG_HAVE_SYSCALL
	_init_syscall();
#endif /* CONFIG_HAVE_SYSCALL */

#if CONFIG_HAVE_X86PKU
	_check_ospke();
#endif /* CONFIG_HAVE_X86PKU */


	/* Pre-fault the new boot stack for CoW before switching.
	 * The stack page must be writable before lcpu_arch_jump_to
	 * switches RSP, otherwise the first push would fault on a
	 * read-only CoW page with no way to handle it.
	 */
	{
		volatile __u8 *sp = (volatile __u8 *)bstack;
		__u8 tmp = *(sp - 8);
		*(volatile __u8 *)(sp - 8) = tmp;
	}

	/* Switch away from the bootstrap stack */
	uk_arch_x86_64_jump_to((__u64)bstack, (__u64)ukplat_entry2);
}

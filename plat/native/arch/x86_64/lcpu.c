/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>

#include <uk/arch/ctx.h>
#include <uk/ctors.h>
#include <uk/arch.h>
#include <uk/lcpu/core.h>
#include <uk/lcpu/pm.h>
#include <uk/plat/common/acpi.h>
#include <uk/plat/common/memory.h>
#include <uk/asm.h>
#include <x86/delay.h>

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX || CONFIG_LIBUKPLAT_NATIVE_LCPU
static void uk_arch_wrmsrgsbase(__u64 gsbase)
{
	uk_arch_wrmsrl(UK_ARCH_MSR_GS_BASE, gsbase);
}

static __u64 uk_arch_rdmsrgsbase(void)
{
	return uk_arch_rdmsrl(UK_ARCH_MSR_GS_BASE);
}

static __u64 rdgsbase_cr4fsgsbase(void)
{
	__u64 gsbase;

	__asm__ __volatile__(
		"rdgsbase	%0"
		: "=r" (gsbase)
		:
		: "memory"
	);

	return gsbase;
}

static void wrgsbase_cr4fsgsbase(__u64 gsbase)
{
	__asm__ __volatile__(
		"wrgsbase	%0"
		:
		: "r" (gsbase)
		: "memory"
	);
}

static void uk_arch_wrmsrkgsbase(__u64 kgsbase)
{
	uk_arch_wrmsrl(UK_ARCH_MSR_KERNEL_GS_BASE, kgsbase);
}

static void uk_arch_wrmsrfsbase(__u64 fsbase)
{
	uk_arch_wrmsrl(UK_ARCH_MSR_FS_BASE, fsbase);
}

static __u64 uk_arch_rdmsrfsbase(void)
{
	return uk_arch_rdmsrl(UK_ARCH_MSR_FS_BASE);
}

static __u64 rdfsbase_cr4fsgsbase(void)
{
	__u64 fsbase;

	__asm__ __volatile__(
		"rdfsbase	%0"
		: "=r" (fsbase)
		:
		: "memory"
	);

	return fsbase;
}

static void wrfsbase_cr4fsgsbase(__u64 fsbase)
{
	__asm__ __volatile__(
		"wrfsbase	%0"
		:
		: "r" (fsbase)
		: "memory"
	);
}

static void (*wrgsbasefn)(__u64) = &uk_arch_wrmsrgsbase;
static __u64 (*rdgsbasefn)(void) = &uk_arch_rdmsrgsbase;
void (*uk_plat_native_wrfsbasefn)(__u64) = &uk_arch_wrmsrfsbase;
__u64 (*uk_plat_native_rdfsbasefn)(void) = &uk_arch_rdmsrfsbase;
static void (*wrkgsbasefn)(__u64) = &uk_arch_wrmsrkgsbase;

static void init_fsgsbasefns(void)
{
	__u32 eax, ebx, ecx, edx;

	uk_arch_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (ebx & UK_ARCH_CPUID7_EBX_FSGSBASE) {
		wrgsbasefn = wrgsbase_cr4fsgsbase;
		rdgsbasefn = rdgsbase_cr4fsgsbase;
		uk_plat_native_wrfsbasefn = wrfsbase_cr4fsgsbase;
		uk_plat_native_rdfsbasefn = rdfsbase_cr4fsgsbase;
	}
}

UK_CTOR_PRIO(init_fsgsbasefns, 0);
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX || CONFIG_LIBUKPLAT_NATIVE_LCPU */

#if CONFIG_LIBUKPLAT_NATIVE_LCPU
void uk_plat_native_traps_init(struct uk_lcpu *this_lcpu);
void uk_plat_native_traps_table_init(void);

#if CONFIG_HAVE_SMP
static inline int x2apic_enable(void)
{
	__u32 eax, ebx, ecx, edx;

	/* Check for x2APIC support */
	uk_arch_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (!(ecx & UK_ARCH_CPUID1_ECX_X2APIC))
		return -ENOTSUP;

	/* Check if APIC is active */
	uk_arch_rdmsr(UK_ARCH_APIC_MSR_BASE, &eax, &edx);
	if (!(eax & UK_ARCH_APIC_BASE_EN))
		return -ENOTSUP;

	/* Switch to x2APIC mode */
	eax |= UK_ARCH_APIC_BASE_EXTD;
	uk_arch_wrmsr(UK_ARCH_APIC_MSR_BASE, eax, edx);

	/* Set APIC software enable flag if necessary */
	uk_arch_rdmsr(UK_ARCH_APIC_MSR_SVR, &eax, &edx);
	if ((eax & UK_ARCH_APIC_SVR_EN) == 0) {
		eax |= UK_ARCH_APIC_SVR_EN;
		uk_arch_wrmsr(UK_ARCH_APIC_MSR_SVR, eax, edx);
	}

	/*
	 * TODO: Configure spurious interrupt vector number
	 * After power-up or reset this is 0xff, which might not be
	 * configured in the trap table
	 */

	return 0;
}

static inline void x2apic_send_ipi(int irqno, int dest)
{
	__u32 eax;

	UK_ASSERT(((32 + irqno) & 0xff) == (32 + irqno));

	eax = UK_ARCH_APIC_ICR_TRIGGER_LEVEL | UK_ARCH_APIC_ICR_LEVEL_ASSERT |
	      UK_ARCH_APIC_ICR_DESTMODE_PHYSICAL |
	      UK_ARCH_APIC_ICR_DMODE_FIXED | (32 + irqno);

	uk_arch_wrmsr(UK_ARCH_APIC_MSR_ICR, eax, dest);
}

static inline void x2apic_send_sipi(__vaddr_t addr, int dest)
{
	__u32 eax;

	UK_ASSERT((addr &
		   (UK_ARCH_APIC_ICR_VECTOR_MASK << __PAGE_SHIFT)) == addr);

	eax = UK_ARCH_APIC_ICR_TRIGGER_LEVEL | UK_ARCH_APIC_ICR_LEVEL_ASSERT |
	      UK_ARCH_APIC_ICR_DESTMODE_PHYSICAL | UK_ARCH_APIC_ICR_DMODE_SUP |
	      (addr >> __PAGE_SHIFT);

	uk_arch_wrmsr(UK_ARCH_APIC_MSR_ICR, eax, dest);
}

static inline void x2apic_send_iipi(int dest)
{
	__u32 eax;

	eax = UK_ARCH_APIC_ICR_TRIGGER_LEVEL | UK_ARCH_APIC_ICR_LEVEL_ASSERT |
	      UK_ARCH_APIC_ICR_DESTMODE_PHYSICAL | UK_ARCH_APIC_ICR_DMODE_INIT;

	uk_arch_wrmsr(UK_ARCH_APIC_MSR_ICR, eax, dest);
}

/* Deassert only supported on Pentium and P6 familiy processors */
#define x2apic_send_iipi_deassert() {}

/* We only support x2APIC at the moment */
#define apic_enable		x2apic_enable
#define apic_send_ipi		x2apic_send_ipi
#define apic_send_sipi		x2apic_send_sipi
#define apic_send_iipi		x2apic_send_iipi
#define apic_send_iipi_deassert x2apic_send_iipi_deassert
#endif /* CONFIG_HAVE_SMP */

static int plat_native_lcpu_pm_ops_register(void);

int uk_plat_native_lcpu_init(struct uk_lcpu *this_lcpu)
{
	int rc;

#if CONFIG_HAVE_SMP
	rc = apic_enable();
	if (unlikely(rc))
		return rc;
#endif /* CONFIG_HAVE_SMP */

	rc = plat_native_lcpu_pm_ops_register();
	if (unlikely(rc))
		return rc;

	wrgsbasefn((__uptr)this_lcpu);
	wrkgsbasefn((__uptr)this_lcpu);

	uk_plat_native_traps_table_init();
	uk_plat_native_traps_init(this_lcpu);

	wrgsbasefn((__uptr)this_lcpu);
	wrkgsbasefn((__uptr)this_lcpu);

	return 0;
}

#if CONFIG_HAVE_SMP
/* Secondary cores start in 16-bit real-mode and we have to provide the
 * corresponding boot code somewhere in the first 1 MiB. We copy the trampoline
 * code to the target address during MP initialization.
 */
extern void *x86_start16_begin[];
extern void *x86_start16_end[];
extern __vaddr_t uk_plat_native_x86_64_start16_addr; /* target address */

static inline int memregion_alloc_sipi_vect(void)
{
#define X86_VIDEO_MEM_START	0xA0000UL
#define X86_VIDEO_MEM_LEN	0x20000UL
	__sz len;

	len = (__sz)((__uptr)x86_start16_end - (__uptr)x86_start16_begin);
	len = UK_PAGING_PAGE_ALIGN_UP(len);
	uk_plat_native_x86_64_start16_addr = (__uptr)ukplat_memregion_alloc(len,
							  UKPLAT_MEMRT_RESERVED,
							  UKPLAT_MEMRF_READ  |
							  UKPLAT_MEMRF_WRITE);
	if (unlikely(!uk_plat_native_x86_64_start16_addr ||
		     uk_plat_native_x86_64_start16_addr >= X86_VIDEO_MEM_START))
		return -ENOMEM;

	return 0;
}

#define UK_ARCH_START16_SIZE						\
	((__uptr)x86_start16_end - (__uptr)x86_start16_begin)

#define START16_MOV_SYM(sym, sz)					\
	sym##_imm##sz##_start16

#define START16_DATA_SYM(sym, sz)					\
	sym##_data##sz##_start16

#define IMPORT_START16_SYM(sym, sz, type)				\
	extern void *sym[];						\
	extern void *START16_##type##_SYM(sym, sz)[]

#define START16_MOV_OFF(sym, sz)					\
	((void *)START16_MOV_SYM(sym, sz) -				\
	(void *)x86_start16_begin)

#define START16_DATA_OFF(sym, sz)					\
	((void *)START16_DATA_SYM(sym, sz) -				\
	(void *)x86_start16_begin)

IMPORT_START16_SYM(gdt32_ptr, 2, MOV);
IMPORT_START16_SYM(gdt32, 4, DATA);
IMPORT_START16_SYM(lcpu_start16, 2, MOV);
IMPORT_START16_SYM(jump_to32, 2, MOV);
IMPORT_START16_SYM(lcpu_start32, 4, MOV);

#define START16_RELOC_ENTRY(sym, sz, type)				\
	{								\
		.r_mem_off = START16_##type##_OFF(sym, sz),		\
		.r_addr = (void *)(sym) - (void *)x86_start16_begin,	\
		.r_sz = (sz),						\
	}

static void apply_start16_reloc(__u64 baddr, __u64 r_mem_off,
				__u64 r_addr, __u32 r_sz)
{
	switch (r_sz) {
	case 2:
		*(__u16 *)((__u8 *)baddr + r_mem_off) = (__u16)(baddr + r_addr);
		break;
	case 4:
		*(__u32 *)((__u8 *)baddr + r_mem_off) = (__u32)(baddr + r_addr);
		break;
	}
}

static void start16_reloc_mp_init(void)
{
	struct {
		__u64 r_mem_off;
		__u64 r_addr;
		__u32 r_sz;
	} x86_start16_relocs[] = {
		START16_RELOC_ENTRY(lcpu_start16, 2, MOV),
		START16_RELOC_ENTRY(gdt32_ptr, 2, MOV),
		START16_RELOC_ENTRY(gdt32, 4, DATA),
		START16_RELOC_ENTRY(jump_to32, 2, MOV),
		START16_RELOC_ENTRY(lcpu_start32, 4, MOV),
	};
	__sz i;

	for (i = 0; i < ARRAY_SIZE(x86_start16_relocs); i++)
		apply_start16_reloc((__u64)uk_plat_native_x86_64_start16_addr,
				    x86_start16_relocs[i].r_mem_off,
				    x86_start16_relocs[i].r_addr,
				    x86_start16_relocs[i].r_sz);

	/* Unlike the other entries, lcpu_start32 must stay the same
	 * as it is not part of the start16 section.
	 */
	apply_start16_reloc((__u64)uk_plat_native_x86_64_start16_addr,
			    START16_MOV_OFF(lcpu_start32, 4),
			    (__u64)lcpu_start32 -
			    (__u64)uk_plat_native_x86_64_start16_addr, 4);
}

int uk_plat_native_lcpu_mp_init(void *arg __unused)
{
	__u64 bsp_cpu_id = uk_lcpu_get(0)->id;
	union {
		struct acpi_madt_x2apic *x2apic;
		struct acpi_madt_lapic *lapic;
		struct acpi_subsdt_hdr *h;
	} m;
	int bsp_found __maybe_unused = 0;
	struct acpi_madt *madt;
	struct uk_lcpu *lcpu;
	__u64 cpu_id;
	__sz off, len;
	int rc;

	uk_pr_info("Bootstrapping processor has the ID %ld\n", bsp_cpu_id);

	/* Enumerate all other CPUs */
	madt = acpi_get_madt();
	UK_ASSERT(madt);

	len = madt->hdr.tab_len - sizeof(*madt);
	for (off = 0; off < len; off += m.h->len) {
		m.h = (struct acpi_subsdt_hdr *)(madt->entries + off);

		switch (m.h->type) {
		case ACPI_MADT_LAPIC:
			if (!(m.lapic->flags & ACPI_MADT_LAPIC_FLAGS_EN) &&
			    !(m.lapic->flags & ACPI_MADT_LAPIC_FLAGS_ON_CAP))
				continue; /* goto next MADT entry */

			cpu_id = m.lapic->lapic_id;
			break;

		case ACPI_MADT_LX2APIC:
			if (!(m.x2apic->flags & ACPI_MADT_X2APIC_FLAGS_EN) &&
			    !(m.x2apic->flags & ACPI_MADT_X2APIC_FLAGS_ON_CAP))
				continue; /* goto next MADT entry */

			cpu_id = m.x2apic->lapic_id;
			break;

		default:
			continue; /* goto next MADT entry */
		}

		if (bsp_cpu_id == cpu_id) {
			UK_ASSERT(!bsp_found);

			bsp_found = 1;
			continue;
		}

		lcpu = uk_lcpu_alloc(cpu_id);
		if (unlikely(!lcpu)) {
			/* If we cannot allocate another LCPU, we probably have
			 * reached the maximum number of supported CPUs. So
			 * just stop here.
			 */
			uk_pr_warn("Maximum number of cores exceeded.\n");
			return 0;
		}
	}
	UK_ASSERT(bsp_found);

	/* Allocate an mrd for the SIPI vector */
	rc = memregion_alloc_sipi_vect();
	if (unlikely(rc)) {
		uk_pr_err("Could not allocate mrd for the SIPI vector(%d)", rc);
		return rc;
	}

	/* Copy AP startup code to target address in first 1MiB */
	UK_ASSERT(uk_plat_native_x86_64_start16_addr < 0x100000);
	memcpy((void *)uk_plat_native_x86_64_start16_addr, &x86_start16_begin,
	       UK_ARCH_START16_SIZE);

	start16_reloc_mp_init();

	uk_pr_debug("Copied AP 16-bit boot code to 0x%"__PRIvaddr"\n",
		    uk_plat_native_x86_64_start16_addr);

	return 0;
}

static int plat_native_lcpu_start(struct uk_lcpu *lcpu)
{
	/* Send INIT IPI */
	apic_send_iipi(lcpu->id);

	/* Deassert */
	apic_send_iipi_deassert();

	return 0;
}

#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
static int plat_native_lcpu_post_start(const __u32 lcpuidx[], unsigned int *num)
{
	__u64 this_cpu_id = uk_lcpu_id();
	unsigned int i, n, j;
	struct uk_lcpu *lcpu;

	/* wait 10 msec (according to Intel manual 8.4.4.1) */
	mdelay(10);

	uk_lcpu_lcpuidx_list_foreach(lcpuidx, num, n, i, lcpu) {
		if (lcpu->id == this_cpu_id)
			continue;

		for (j = 0; j < 2; j++) {
			/* Send STARTUP IPI */
			apic_send_sipi(uk_plat_native_x86_64_start16_addr,
				       lcpu->id);

			/* wait 200 usec (according to Intel manual 8.4.4.1) */
			udelay(200);
		}
	}

	return 0;
}
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */

int uk_plat_native_send_ipi(__u64 id, unsigned long irq)
{
	apic_send_ipi(irq, id);
	return 0;
}
#endif /* CONFIG_HAVE_SMP */

static void plat_native_lcpu_halt(void)
{
	uk_arch_halt();
}

static void plat_native_lcpu_halt_irq(void)
{
	/*
	 * We have to be careful when enabling interrupts before entering a
	 * halt state. If we want to wait for an interrupt (e.g., a timer)
	 * the interrupt may fire in the short window between sti and hlt and
	 * we are going to halt forever. As sti only enables interrupts after
	 * the following instruction, we can avoid the race condition by
	 * ensuring that hlt immediately follows sti. There must be no
	 * instruction in between.
	 */
	asm volatile (
		"sti\n\t"
		"hlt\n\t"
		"cli\n\t"
		:
		:
		: "memory"
	);
}

static const struct uk_lcpu_pm_ops plat_native_pm_ops = {
#if CONFIG_HAVE_SMP
	.start = plat_native_lcpu_start,
#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
	.post_start = plat_native_lcpu_post_start,
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */
#endif /* CONFIG_HAVE_SMP */
	.halt = plat_native_lcpu_halt,
	.halt_irq = plat_native_lcpu_halt_irq,
};

static int plat_native_lcpu_pm_ops_register(void)
{
	return uk_lcpu_pm_ops_register(&plat_native_pm_ops);
}
#endif /* CONFIG_LIBUKPLAT_NATIVE_LCPU */

#if CONFIG_LIBUKPLAT_NATIVE_SYSCTX
__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	sysctx->gsbase = rdgsbasefn();
	sysctx->fsbase = uk_plat_native_rdfsbasefn();
}

__isr void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);

	wrgsbasefn(sysctx->gsbase);
	uk_plat_native_wrfsbasefn(sysctx->fsbase);
}
#endif /* CONFIG_LIBUKPLAT_NATIVE_SYSCTX */

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/boot/earlytab.h>
#include <uk/arch/util.h>
#include <uk/essentials.h>
#include <uk/lcpu/pm.h>
#include <uk/prio.h>

#if CONFIG_LIBUKACPI
#include <uk/acpi.h>
#endif /* CONFIG_LIBUKACPI */
#include <uk/plat/common/memory.h>
#include <x86/delay.h>

#if CONFIG_HAVE_SMP
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
#define apic_send_ipi		x2apic_send_ipi
#define apic_send_sipi		x2apic_send_sipi
#define apic_send_iipi		x2apic_send_iipi
#define apic_send_iipi_deassert x2apic_send_iipi_deassert

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

#if CONFIG_LIBUKACPI
int uk_plat_native_lcpu_mp_init(void *arg __unused)
{
	__u64 bsp_cpu_id = uk_pcpuvar_lval(0, uk_pcpuvar_cpu_id);
	union {
		struct uk_acpi_madt_x2apic *x2apic;
		struct uk_acpi_madt_lapic *lapic;
		struct uk_acpi_subsdt_hdr *h;
	} m;
	int bsp_found __maybe_unused = 0;
	struct uk_acpi_madt *madt;
	__sz off, len;
	__u64 cpu_id;
	__u32 idx;
	int rc;

	uk_pr_info("Bootstrapping processor has the ID %ld\n", bsp_cpu_id);

	/* Enumerate all other CPUs */
	madt = uk_acpi_get_madt();
	UK_ASSERT(madt);

	len = madt->hdr.tab_len - sizeof(*madt);
	idx = 1;
	for (off = 0; off < len; off += m.h->len) {
		m.h = (struct uk_acpi_subsdt_hdr *)(madt->entries + off);

		switch (m.h->type) {
		case UK_ACPI_MADT_LAPIC:
			if (!(m.lapic->flags & UK_ACPI_MADT_LAPIC_FLAGS_EN) &&
			    !(m.lapic->flags & UK_ACPI_MADT_LAPIC_FLAGS_ON_CAP))
				continue; /* goto next MADT entry */

			cpu_id = m.lapic->lapic_id;
			break;

		case UK_ACPI_MADT_LX2APIC:
			if (!(m.x2apic->flags & UK_ACPI_MADT_X2APIC_FLAGS_EN) &&
			    !(m.x2apic->flags &
			      UK_ACPI_MADT_X2APIC_FLAGS_ON_CAP))
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

		uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_id) = cpu_id;
		uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_idx) = idx;
		idx++;
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
#endif /* CONFIG_LIBUKACPI */

static int plat_native_lcpu_start(__u64 idx)
{
	/* Send INIT IPI */
	apic_send_iipi(uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_id));

	/* Deassert */
	apic_send_iipi_deassert();

	return 0;
}

#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
static int plat_native_lcpu_post_start(const __u64 lcpuidx[], unsigned int *num)
{
	__u64 id, this_cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);
	unsigned int i, j;

	/* wait 10 msec (according to Intel manual 8.4.4.1) */
	mdelay(10);

	for (i = 0; i < *num; i++) {
		id = uk_pcpuvar_lval(lcpuidx[i], uk_pcpuvar_cpu_id);
		if (id == this_cpu_id)
			continue;

		for (j = 0; j < 2; j++) {
			/* Send STARTUP IPI */
			apic_send_sipi(uk_plat_native_x86_64_start16_addr, id);

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

__isr static
int plat_native_lcpu_pm_ops_register(struct ukplat_bootinfo *bi __unused)
{
	return uk_lcpu_pm_ops_register(&plat_native_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(plat_native_lcpu_pm_ops_register, UK_PRIO_EARLIEST);

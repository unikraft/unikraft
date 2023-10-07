/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <x86/irq.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <x86/delay.h>
#include <uk/plat/common/acpi.h>

#include <uk/plat/lcpu.h>
#include <uk/plat/common/lcpu.h>
#include <uk/reloc.h>

#include <string.h>
#include <errno.h>

#if CONFIG_LIBUKINTCTLR_APIC
#include <uk/intctlr/apic.h>
#endif /* CONFIG_LIBUKINTCTLR_APIC */

#include "start16_helpers.h"

__lcpuid lcpu_arch_id(void)
{
	__u32 eax, ebx, ecx, edx;

	cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	return (ebx >> 24);
}

int lcpu_arch_init(struct lcpu *this_lcpu)
{
#ifdef CONFIG_HAVE_SMP
	int rc;

	rc = apic_enable();
	if (unlikely(rc))
		return rc;
#endif /* CONFIG_HAVE_SMP */

	traps_lcpu_init(this_lcpu);

	return 0;
}

void __noreturn lcpu_arch_jump_to(void *sp, ukplat_lcpu_entry_t entry)
{
	__asm__ (
		"movq	%0, %%rsp\n"

		/* According to System V AMD64 the stack pointer must be
		 * aligned to 16-bytes. In other words, the value (RSP+8) must
		 * be a multiple of 16 when control is transferred to the
		 * function entry point (i.e., the compiler expects a
		 * misalignment due to the return address having been pushed
		 * onto the stack).
		 */
		"andq	$~0xf, %%rsp\n"
		"subq	$0x8, %%rsp\n"

#if !__OMIT_FRAMEPOINTER__
		"xorq	%%rbp, %%rbp\n"
#endif /* __OMIT_FRAMEPOINTER__ */

		"jmp	*%1\n"
		:
		: "r"(sp), "r"(entry)
		: /* clobbers not needed */);

	/* just make the compiler happy about returning function */
	__builtin_unreachable();
}

#ifdef CONFIG_HAVE_SMP
/* Secondary cores start in 16-bit real-mode and we have to provide the
 * corresponding boot code somewhere in the first 1 MiB. We copy the trampoline
 * code to the target address during MP initialization.
 */
#if CONFIG_LIBUKRELOC
IMPORT_START16_UKRELOC_SYM(gdt32_ptr, 2, MOV);
IMPORT_START16_UKRELOC_SYM(gdt32, 4, DATA);
IMPORT_START16_UKRELOC_SYM(lcpu_start16, 2, MOV);
IMPORT_START16_UKRELOC_SYM(jump_to32, 2, MOV);
IMPORT_START16_UKRELOC_SYM(lcpu_start32, 4, MOV);

static void uk_reloc_mp_init(void)
{
	size_t i;
	struct uk_reloc x86_start16_relocs[] = {
		START16_UKRELOC_ENTRY(lcpu_start16, 2, MOV),
		START16_UKRELOC_ENTRY(gdt32, 4, DATA),
		START16_UKRELOC_ENTRY(gdt32_ptr, 2, MOV),
		START16_UKRELOC_ENTRY(jump_to32, 2, MOV),
	};

	for (i = 0; i < ARRAY_SIZE(x86_start16_relocs); i++)
		apply_uk_reloc(&x86_start16_relocs[i],
			      (__u64)x86_start16_addr +
			      x86_start16_relocs[i].r_addr,
			      (void *)x86_start16_addr);

	/* Unlike the other entries, lcpu_start32 must stay the same
	 * as it is not part of the start16 section
	 */
	apply_uk_reloc(&(struct uk_reloc)UKRELOC_ENTRY(
				START16_UKRELOC_MOV_OFF(lcpu_start32, 4),
				(__u64)lcpu_start32, 4,
				UKRELOC_FLAGS_PHYS_REL),
		       (__u64)lcpu_start32,
		       (void *)x86_start16_addr);
}
#else  /* CONFIG_LIBUKRELOC */
static void uk_reloc_mp_init(void) { }
#endif /* !CONFIG_LIBUKRELOC */

int lcpu_arch_mp_init(void *arg __unused)
{
	__lcpuid bsp_cpu_id = lcpu_get(0)->id;
	union {
		struct acpi_madt_x2apic *x2apic;
		struct acpi_madt_lapic *lapic;
		struct acpi_subsdt_hdr *h;
	} m;
	int bsp_found __maybe_unused = 0;
	struct acpi_madt *madt;
	struct lcpu *lcpu;
	__lcpuid cpu_id;
	__sz off, len;

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

		lcpu = lcpu_alloc(cpu_id);
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

	/* Copy AP startup code to target address in first 1MiB */
	UK_ASSERT(x86_start16_addr < 0x100000);
	memcpy((void *)x86_start16_addr, &x86_start16_begin, X86_START16_SIZE);

	uk_reloc_mp_init();

	uk_pr_debug("Copied AP 16-bit boot code to 0x%"__PRIvaddr"\n",
		    x86_start16_addr);

	return 0;
}

int lcpu_arch_start(struct lcpu *lcpu, unsigned long flags __unused)
{
	UK_ASSERT(lcpu->state == LCPU_STATE_INIT);

	/* Send INIT IPI */
	apic_send_iipi(lcpu->id);

	/* Deassert */
	apic_send_iipi_deassert();

	return 0;
}

int lcpu_arch_post_start(const __lcpuidx lcpuidx[], unsigned int *num)
{
	__lcpuid this_cpu_id = ukplat_lcpu_id();
	struct lcpu *lcpu;
	unsigned int i, n, j;

	/* wait 10 msec (according to Intel manual 8.4.4.1) */
	mdelay(10);

	lcpu_lcpuidx_list_foreach(lcpuidx, num, n, i, lcpu) {
		if (lcpu->id == this_cpu_id)
			continue;

		for (j = 0; j < 2; j++) {
			/* Send STARTUP IPI */
			apic_send_sipi(x86_start16_addr, lcpu->id);

			/* wait 200 usec (according to Intel manual 8.4.4.1) */
			udelay(200);
		}
	}

	return 0;
}

int lcpu_arch_run(struct lcpu *lcpu, const struct ukplat_lcpu_func *fn,
		  unsigned long flags __unused)
{
	int rc;

	UK_ASSERT(lcpu->id != lcpu_arch_id());

	rc = lcpu_fn_enqueue(lcpu, fn);
	if (unlikely(rc))
		return rc;

	apic_send_ipi(*lcpu_run_irqv, lcpu->id);

	return 0;
}

int lcpu_arch_wakeup(struct lcpu *lcpu)
{
	UK_ASSERT(lcpu->id != lcpu_arch_id());

	apic_send_ipi(*lcpu_wakeup_irqv, lcpu->id);

	return 0;
}
#endif /* CONFIG_HAVE_SMP */

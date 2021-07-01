/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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
 *
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <x86/apic.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <x86/acpi/madt.h>
#include <kvm-x86/smp_defs.h>
#include <kvm-x86/smp.h>
#include <kvm-x86/delay.h>
#include <uk/print.h>
#include <uk/arch/types.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>
#include <uk/plat/irq.h>
#include <uk/plat/config.h>

struct x86_lcpu_startup_args {
	ukplat_lcpu_entry_t entry;
	void *stackp;
};

/*
 * If this struct is to be changed, the values in include/uk/plat/smp_defs.h
 * must be updated
 */
struct x86_cpu {
	struct x86_lcpu_startup_args s_args;

	volatile int state;
	__u32 apic_id;
	char *intr_stack;
	char *trap_stack;
	char *panic_stack;
	struct uk_list_head fnlist;
	struct uk_list_head nmi_fnlist;
};

struct x86_cpu cpus[CONFIG_UKPLAT_LCPU_MAXCOUNT];

extern char cpu_intr_stack[CONFIG_UKPLAT_LCPU_MAXCOUNT][STACK_SIZE];
extern char cpu_trap_stack[CONFIG_UKPLAT_LCPU_MAXCOUNT][STACK_SIZE];
extern char cpu_nmi_stack[CONFIG_UKPLAT_LCPU_MAXCOUNT][4096];

__u8 bspdone;
__u64 cr3_val, idt_addr;
volatile int aps_halted;

static __lcpuid bspid;
static __u32 smp_numcores;
static struct MADT *madt;

extern struct desc_table_ptr64 idtptr;

/*
 * With the current SMP implementation, we use a different NMI handler,
 * not just crash the system
 */
void do_nmi(struct __regs *regs, unsigned long error_code)
{
	struct ukplat_lcpu_func *i;
	struct uk_list_head *fnlist;
	__lcpuid id = ukplat_lcpu_id();

	fnlist = &cpus[id].nmi_fnlist;

	if (!uk_list_empty(fnlist)) {
		uk_list_for_each_entry(i, fnlist, lentry) {
			i->fn(regs, i);
			if (i == uk_list_first_entry(fnlist,
				struct ukplat_lcpu_func, lentry))
				break;
		}

		x2apic_ack_interrupt();
	} else
		do_unhandled_trap(TRAP_nmi, "NMI", regs, error_code);
}

static int x86_lcpu_ipi_run_handler(void *args __unused, struct __regs *regs)
{
	struct ukplat_lcpu_func *i;
	struct uk_list_head *fnlist;

	int id = ukplat_lcpu_id();

	fnlist = &cpus[id].fnlist;

	uk_list_for_each_entry(i, fnlist, lentry) {
		i->fn(regs, i);
		if (i == uk_list_first_entry(fnlist,
			struct ukplat_lcpu_func, lentry))
			break;
	}

	x2apic_ack_interrupt();
	return 1;
}

__lcpuid ukplat_lcpu_count(void)
{
	__u8 *ptr = madt->Entries, numcores = 0;
	unsigned int i;
	struct MADTEntryHeader *mh;
	struct MADTType0Entry *madt_entry;

	if (smp_numcores > 0)
		return smp_numcores;

	UK_ASSERT(madt);

	for (i = 0; i < madt->h.Length; i += mh->Length) {
		mh = (struct MADTEntryHeader *)(ptr + i);
		switch (mh->Type) {
		case 0:
			madt_entry = (struct MADTType0Entry *)mh;
			if (madt_entry->Flags & 1 || madt_entry->Flags & 2)
				numcores++;
			break;
			/*
			 * TODO: search for Type 9 entries, as well, if a CPU
			 * with more than 256 cores is used
			 */
		default:
			break;
		}
	}

	if (numcores > CONFIG_UKPLAT_LCPU_MAXCOUNT)
		numcores = CONFIG_UKPLAT_LCPU_MAXCOUNT;
	smp_numcores = numcores;

	return numcores;
}

extern void *_sec_start16_begin[];
extern void *_sec_start16_end[];
#define _SEC_START16_SIZE                                                      \
	((__uptr)_sec_start16_end - (__uptr)_sec_start16_begin)

int ukplat_lcpu_start(__lcpuid lcpuid[], void *sp[],
		       ukplat_lcpu_entry_t entry[], unsigned int num)
{
	unsigned int i, j, ncores = ukplat_lcpu_count();
	int ret;
	__lcpuid id;

	if (num > ncores) {
		uk_pr_warn("More cores are selected, but not available. Limiting to %d", ncores);
		num = ncores;
	}

	if (num > 256) {
		uk_pr_warn("Starting more than 256 cores is not impemented. Limiting to 256!");
		num = 256;
	}

	memcpy((void *)0x8000, &_sec_start16_begin, _SEC_START16_SIZE);
	uk_pr_debug("Copied AP 16 bit boot code to 0x8000\n");

	/* Store the CR3 register */
	__asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3_val));

	ret = ukplat_irq_register(UKPLAT_X86_IPI_RUN_IRQ,
				  x86_lcpu_ipi_run_handler, NULL);
	if (ret != 0) {
		uk_pr_crit("Interrupt handler for IRQ %d failed\n",
			   UKPLAT_X86_IPI_RUN_IRQ);
		return ret;
	}

	for (i = 0; i < num; i++) {
		id = (lcpuid) ? lcpuid[i] : (__lcpuid)i;

		/* Don't restart the BootStrapping Processor */
		if (id == bspid)
			continue;

		/*
		 * Unitialized apic_id; either it wasn't found, or we don't
		 * want to start it
		 */
		if (cpus[id].apic_id == 0)
			continue;

		UK_ASSERT(sp[i]);

		/* Store the entry address and stack pointer */
		cpus[id].s_args.entry = entry[i];
		cpus[id].s_args.stackp = sp[i];

		/* Store the interrupt stacks */
		cpus[id].intr_stack = cpu_intr_stack[id];
		cpus[id].trap_stack = cpu_trap_stack[id];
		cpus[id].panic_stack = cpu_nmi_stack[id];

		/* Initialize the lists */
		UK_INIT_LIST_HEAD(&cpus[id].fnlist);
		UK_INIT_LIST_HEAD(&cpus[id].nmi_fnlist);

		/* clear APIC errors */
		x2apic_clear_errors();

		/* select AP and trigger INIT IPI */
		x2apic_send_iipi(cpus[id].apic_id);

		/* deassert */
		x2apic_send_iipi_deassert(cpus[id].apic_id);
	}

	/* wait 10 msec */
	mdelay(10);

	for (i = 0; i < num; i++) {
		id = (lcpuid) ? lcpuid[i] : (__lcpuid)i;

		if (id == bspid)
			continue;

		if (cpus[id].apic_id == 0)
			continue;

		for (j = 0; j < 2; j++) {
			/* clear APIC errors */
			x2apic_clear_errors();

			/**
			 * select AP and trigger STARTUP IPI with entry code
			 * at 0x8000
			 */
			x2apic_send_sipi(0x8000, cpus[id].apic_id);

			/* wait 200 usec */
			udelay(200);
		}
	}
	/* wait 10 msec */
	mdelay(10);

	/* Signal the APs that the BSP has finished */
	/* NOTE: without scheduling implemented, everything works fine without
	 * this part (from here, until return), but everyone recommends that the
	 * APs wait for the BSP to finish.
	 */
	bspdone = 1;

	return 0;
}

__lcpuid ukplat_lcpu_id(void)
{
	__u32 eax, ebx, ecx, edx;

	/*
	 * Until we implement per-CPU storage, we must use cpuid to find out
	 * the id of the current CPU
	 */
	cpuid(1, 0, &eax, &ebx, &ecx, &edx);

	return (ebx >> 24);
}

int __attribute__((optimize("O0")))
ukplat_lcpu_wait(__lcpuid lcpuid[], unsigned int num, __nsec timeout)
{
	unsigned int i, ncores = ukplat_lcpu_count();
	__lcpuid id;
	__nsec end;

	num = (lcpuid) ? num : ncores;
	if (num > ncores) {
		uk_pr_warn("More cores are selected, but not available\n");
		num = ncores;
	}

	if (timeout > 0)
		end = ukplat_monotonic_clock() + timeout;

	for (i = 0; i < num; i++) {
		id = (lcpuid) ? lcpuid[i] : (__lcpuid)i;

		/* Don't wait for the current CPU */
		if (id == ukplat_lcpu_id())
			continue;

		while (cpus[id].state != LCPU_STATE_IDLE) {
			if (timeout && (ukplat_monotonic_clock() > end))
				return -ECANCELED;
		}
	}

	return 0;
}

int ukplat_lcpu_run(__lcpuid lcpuid[], unsigned int num,
		    struct ukplat_lcpu_func *fn, int flags)
{
	unsigned int i, ncores = ukplat_lcpu_count();
	__lcpuid id;

	UK_ASSERT(fn);

	num = (lcpuid) ? num : ncores;
	if (num > ncores) {
		uk_pr_warn("More cores are selected, but not available\n");
		num = ncores;
	}

	for (i = 0; i < num; i++) {
		id = (lcpuid) ? lcpuid[i] : (__lcpuid)i;

		/*
		 * Don't schedule functions or send interrupts to
		 * the current CPU
		 */
		if (id == bspid)
			continue;

		cpus[id].state = LCPU_STATE_BUSY;
		if (flags == X86_LCPU_RUN_FLAG_NMI)
			uk_list_add(&fn->lentry, &cpus[id].nmi_fnlist);
		else
			uk_list_add(&fn->lentry, &cpus[id].fnlist);

		x2apic_clear_errors();

		/*
		 * If it isn't the NMI flag, we assume a simple IPI
		 * is wanted
		 */
		if (flags == X86_LCPU_RUN_FLAG_NMI)
			x2apic_send_nmi(id);
		else
			x2apic_send_ipi(UKPLAT_X86_IPI_RUN_IRQ, id);
	}

	return 0;
}

int ukplat_lcpu_wakeup(__lcpuid lcpuid[], unsigned int num)
{
	unsigned int i, ncores = ukplat_lcpu_count();
	__lcpuid id;

	num = (lcpuid) ? num : ukplat_lcpu_count();
	if (num > ncores) {
		uk_pr_warn("More cores are selected, but not available\n");
		num = ncores;
	}

	for (i = 0; i < num; i++) {
		id = (lcpuid) ? lcpuid[i] : (__lcpuid)i;

		if (id == ukplat_lcpu_id())
			continue;

		x2apic_send_ipi(UKPLAT_X86_IPI_WAKEUP_IRQ, id);
	}

	return 0;
}

static void get_lapicid(void)
{
	__u8 *ptr;
	unsigned int i;
	struct MADTEntryHeader *mh;
	struct MADTType0Entry *madt_entry0;
	struct MADTType9Entry *madt_entry9;

	UK_ASSERT(madt);

	ptr = madt->Entries;

	for (i = 0; i < madt->h.Length; i += mh->Length) {
		mh = (struct MADTEntryHeader *)(ptr + i);
		switch (mh->Type) {
		case 0:
			madt_entry0 = (struct MADTType0Entry *)mh;
			if (madt_entry0->ACPIProcessorID
			    >= CONFIG_UKPLAT_LCPU_MAXCOUNT)
				return;
			if (madt_entry0->Flags & 1 || madt_entry0->Flags & 2) {
				cpus[madt_entry0->ACPIProcessorID].apic_id =
				    madt_entry0->APICID;
			} else
				uk_pr_info("Core %d is not available\n",
					   madt_entry0->ACPIProcessorID);
			break;
		/* Unless more than 256 cores are used, no Type 9 entry will be
		 * found. But they are searched, just in case.
		 */
		case 9:
			uk_pr_debug("Found type 9 MADT entry\n");
			madt_entry9 = (struct MADTType9Entry *)mh;
			if (madt_entry9->ACPIProcessorUID
			    >= CONFIG_UKPLAT_LCPU_MAXCOUNT)
				return;
			if (madt_entry9->Flags & 1 || madt_entry9->Flags & 2)
				cpus[madt_entry9->ACPIProcessorUID].apic_id =
				    madt_entry9->X2APICID;
			else {
				uk_pr_info("Core %d is not available\n",
					   madt_entry9->ACPIProcessorUID);
			}
			break;
		default:
			break;
		}
	}
}

static int enable_x2apic(void)
{
	__u32 eax, ebx, ecx, edx;

	cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (ecx & (1 << x2APIC_CPUID_BIT))
		uk_pr_debug("x2APIC is supported; enabling\n");
	else {
		uk_pr_err("x2APIC is not supported\n");
		return -ENOTSUP;
	}

	rdmsr(IA32_APIC_BASE, &eax, &edx);
	uk_pr_debug(
	    "IA32_APIC_BASE has the value %x; EN bit: %d, EXTD bit: %d\n", eax,
	    (eax & (1 << APIC_BASE_EN)) != 0,
	    (eax & (1 << APIC_BASE_EXTD)) != 0);

	/* set the x2APIC enable bit */
	eax |= (1 << APIC_BASE_EXTD);
	wrmsr(IA32_APIC_BASE, eax, edx);
	uk_pr_debug("x2APIC is enabled\n");

	return 0;
}

void __noreturn _lcpu_entry_default(void)
{
	int ret;
	unsigned int eax, edx;
	__lcpuid id = ukplat_lcpu_id();

	uk_pr_debug("Core with the ID %d has started\n", id);

	ret = enable_x2apic();
	if (ret != 0) {
		uk_pr_err("x2APIC could not be enabled!\n");
		halt();
	}

	rdmsr(x2APIC_SPUR, &eax, &edx);
	if ((eax & (1 << APIC_SPUR_EN)) == 0) {
		eax |= (1 << APIC_SPUR_EN);
		wrmsr(x2APIC_SPUR, eax, edx);
		uk_pr_debug("Spurious interrupt enabled\n");
	}

	/* Initialize interrupts */
	traps_init_ap();
	ukplat_lcpu_enable_irq();

	/* Halt the LCPU */
	while (1) {
		cpus[id].state = LCPU_STATE_IDLE;
		halt();
	}
}

int smp_init(void)
{
	int ret;
	__u32 eax, edx;

	bspid = ukplat_lcpu_id();
	uk_pr_info("Bootstrapping processor has the ID %d\n", bspid);

	cpus[bspid].state = LCPU_STATE_BUSY;

	if ((ret = enable_x2apic()) != 0) {
		uk_pr_err("x2APIC could not be enabled!\n");
		return ret;

		/* TODO: Use xAPIC mode */
	}

	rdmsr(x2APIC_SPUR, &eax, &edx);
	uk_pr_debug(
	    "Spurious Interrupt Register has the values %x; EN bit: %d\n", eax,
	    (eax & (1 << APIC_SPUR_EN)) != 0);

	if ((eax & (1 << APIC_SPUR_EN)) == 0) {
		eax |= (1 << APIC_SPUR_EN);
		wrmsr(x2APIC_SPUR, eax, edx);
		uk_pr_debug("Spurious interrupt enabled\n");
	}

	madt = acpi_get_madt();
	UK_ASSERT(madt);

	get_lapicid();

	return 0;
}

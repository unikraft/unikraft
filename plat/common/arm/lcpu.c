/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Răzvan Vîrtan <virtanrazvan@gmail.com>
 *
 * Copyright (c) 2022, University Politehnica of Bucharest.
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
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/acpi.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/io.h>
#include <uk/ofw/fdt.h>
#include <arm/smccc.h>
#include <arm/arm64/cpu.h>
#include <uk/intctlr/gic.h>
#include <libfdt.h>

#define CPU_ID_MASK 0xff00ffffffUL

__lcpuid lcpu_arch_id(void)
{
	__u64 mpidr_reg;

	mpidr_reg = SYSREG_READ64(mpidr_el1);

	/* return the affinity bits for the current core */
	return mpidr_reg & CPU_ID_MASK;
}

void __noreturn lcpu_arch_jump_to(void *sp, ukplat_lcpu_entry_t entry)
{
	__asm__ __volatile__ (
		"mov	sp, %0\n" /* set the sp  */
		"br	%1\n"     /* branch to the entry function */
		:
		: "r"(sp), "r"(entry)
		: /* sp not needed */);

	/* just make the compiler happy about returning function */
	__builtin_unreachable();
}

/**
 * The number of cells for the size field should be 0 in cpu nodes.
 * The number of cells in the address field is set by default to 2 in cpu
 * nodes. This is also the maximum number of cells for the address field that
 * we can currently support.
 */
#define FDT_SIZE_CELLS_DEFAULT 0
#define FDT_ADDR_CELLS_DEFAULT 2

void lcpu_start(struct lcpu *cpu);
extern struct _gic_dev *gic;

int lcpu_arch_init(struct lcpu *this_lcpu)
{
	int ret = 0;

	/* Initialize the interrupt controller for non-bsp cores */
	if (!lcpu_is_bsp(this_lcpu)) {
		ret = gic->ops.initialize();
		if (unlikely(ret))
			return ret;
	}

	SYSREG_WRITE64(tpidr_el1, (__uptr)this_lcpu);

	return ret;
}

#ifdef CONFIG_HAVE_SMP
static __paddr_t lcpu_start_paddr;

__lcpuidx lcpu_arch_idx(void)
{
	struct lcpu *this_lcpu = SYSREG_READ64(tpidr_el1);

	UK_ASSERT(IS_LCPU_PTR(this_lcpu));

	return this_lcpu->idx;
}

#ifdef CONFIG_UKPLAT_ACPI
static int do_arch_mp_init(void *arg __unused)
{
	__lcpuid bsp_cpu_id = lcpu_get(0)->id;
	int bsp_found __maybe_unused = 0;
	union {
		struct acpi_madt_gicc *gicc;
		struct acpi_subsdt_hdr *h;
	} m;
	struct lcpu *lcpu;
	struct acpi_madt *madt;
	__lcpuid cpu_id;
	__sz off, len;

	uk_pr_info("Bootstrapping processor has the ID %ld\n", bsp_cpu_id);

	/* Enumerate all other CPUs */
	madt = acpi_get_madt();
	UK_ASSERT(madt);

	len = madt->hdr.tab_len - sizeof(*madt);
	for (off = 0; off < len; off += m.h->len) {
		m.h = (struct acpi_subsdt_hdr *)(madt->entries + off);

		if (m.h->type != ACPI_MADT_GICC ||
		    (!(m.gicc->flags & ACPI_MADT_GICC_FLAGS_EN) &&
		     !(m.gicc->flags & ACPI_MADT_GICC_FLAGS_ON_CAP)))
			continue;

		cpu_id = m.gicc->mpidr & CPU_ID_MASK;

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

	return 0;
}
#else
static int do_arch_mp_init(void *arg)
{
	int fdt_cpu;
	const fdt32_t *naddr_prop, *nsize_prop, *id_reg;
	const struct fdt_property *cpu_prop;
	int naddr, nsize = 0, cell;
	int subnode;
	__lcpuid cpu_id;
	__lcpuid bsp_cpu_id;
	char bsp_found __maybe_unused = 0;
	struct lcpu *lcpu;
	void *dtb;

	/* MP support is dependent on an initialized GIC */
	UK_ASSERT(gic);

	dtb = arg;

	bsp_cpu_id = lcpu_arch_id();
	uk_pr_info("Bootstrapping processor has the ID %ld\n",
		   bsp_cpu_id);

	/* Search for CPUS node in DTB */
	fdt_cpu = fdt_path_offset(dtb, "/cpus");
	if (unlikely(fdt_cpu < 0)) {
		uk_pr_err("cpus node is not found in device tree\n");
		return -EINVAL;
	}

	/* Get address cells */
	naddr_prop = fdt_getprop(dtb, fdt_cpu, "#address-cells", &cell);
	if (naddr_prop == NULL) {
		naddr = FDT_ADDR_CELLS_DEFAULT;
		uk_pr_warn("Using default value 2 for cpu address-cells.\n");
	} else {
		naddr = fdt32_to_cpu(naddr_prop[0]);
	}
	if (unlikely(naddr < 0 || naddr >= FDT_MAX_NCELLS)) {
		uk_pr_err("Invalid address-cells size.\n");
		return -EINVAL;
	}
	if (unlikely(naddr > FDT_ADDR_CELLS_DEFAULT)) {
		uk_pr_err("Address-cells size greater than 2 not supported yet.\n");
		return -EINVAL;
	}

	/* Get size cells */
	nsize_prop = fdt_getprop(dtb, fdt_cpu, "#size-cells", &cell);
	if (nsize_prop == NULL) {
		nsize = FDT_SIZE_CELLS_DEFAULT;
		uk_pr_warn("Using default value of 0 for cpu size-cells.\n");
	} else {
		nsize = fdt32_to_cpu(nsize_prop[0]);
	}
	/**
	 * According to the device-tree v0.3 specification,
	 * for the `size-cells` property of the `/cpus` node:
	 * "Value shall be 0. Specifies that no size is
	 * required in the reg property in children of this node."
	 */
	if (unlikely(nsize != 0)) {
		uk_pr_err("Invalid size-cells value for cpu node.\n");
		return -EINVAL;
	}

	/* Search all the CPU nodes in DTB */
	fdt_for_each_subnode(subnode, dtb, fdt_cpu) {
		int prop_len = 0;

		cpu_prop = fdt_get_property(dtb, subnode,
					    "device_type", &prop_len);
		if (!cpu_prop)
			continue;
		if (strcmp(cpu_prop->data, "cpu"))
			continue;

		cpu_prop = fdt_get_property(dtb, subnode, "reg", &prop_len);
		if (unlikely(cpu_prop == NULL || prop_len <= 0)) {
			uk_pr_err("CPU node does not contain reg field "
				  "(cpu_id), this core will not be enabled.\n");
			continue;
		}
		id_reg = (const fdt32_t *) cpu_prop->data;
		cpu_id = fdt_reg_read_number(id_reg, naddr);

		cpu_prop = fdt_get_property(dtb, subnode,
					    "enable-method", NULL);
		if (!cpu_prop) {
			if (cpu_id != bsp_cpu_id) {
				uk_pr_err("enable-method for core 0x%lx not "
					  "set, core will not be enabled.",
					  cpu_id);
				continue;
			} else {
				uk_pr_warn("enable-method for core 0x%lx not "
					   "set\n", cpu_id);
				bsp_found = 1;
				continue;
			}
		} else if (strcmp(cpu_prop->data, "psci")) {
			uk_pr_err("enable-method for core 0x%lx is not PSCI : "
				  "(%s), core will not be enabled.\n", cpu_id,
				  cpu_prop->data);
			continue;
		}

		if (cpu_id != bsp_cpu_id) {
			lcpu = lcpu_alloc(cpu_id);
			if (unlikely(!lcpu)) {
				uk_pr_warn("Maximum number of cores exceeded.\n");
				return 0;
			}
		} else
			bsp_found = 1;
	}
	UK_ASSERT(bsp_found);

	return 0;
}
#endif

int lcpu_arch_mp_init(void *arg)
{
	/**
	 * We have to provide the physical address of the start routine when
	 * starting secondary CPUs. We thus do the translation here once and
	 * cache the result.
	 */
	lcpu_start_paddr = ukplat_virt_to_phys(lcpu_start);

	return do_arch_mp_init(arg);
}

int lcpu_arch_start(struct lcpu *lcpu, unsigned long flags __unused)
{
	return cpu_on(lcpu->id, lcpu_start_paddr, lcpu);
}

int lcpu_arch_run(struct lcpu *lcpu, const struct ukplat_lcpu_func *fn,
		  unsigned long flags __unused)
{
	int rc;

	UK_ASSERT(lcpu->id != lcpu_arch_id());

	rc = lcpu_fn_enqueue(lcpu, fn);
	if (unlikely(rc))
		return rc;

	gic->ops.gic_sgi_gen(*lcpu_run_irqv, lcpu->id);

	return 0;
}

int lcpu_arch_wakeup(struct lcpu *lcpu)
{
	UK_ASSERT(lcpu->id != lcpu_arch_id());

	gic->ops.gic_sgi_gen(*lcpu_wakeup_irqv, lcpu->id);

	return 0;
}
#endif /* CONFIG_HAVE_SMP */

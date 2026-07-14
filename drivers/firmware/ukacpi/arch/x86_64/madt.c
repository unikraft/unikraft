/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University POLITEHNICA of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/acpi.h>
#include <uk/acpi/prio.h>
#include <uk/assert.h>
#include <uk/pcpuvar.h>
#include <uk/print.h>
#include <uk/prio.h>

#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#endif /* CONFIG_LIBUKBOOT */

int uk_acpi_madt_fill_cpu_idmap(void)
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

		/* Ignore cores that exceed max configured value */
		if (unlikely(idx == CONFIG_UKPLAT_CPU_MAXCOUNT)) {
			uk_pr_warn("Maximum number of cores exceeded.\n");
			break;
		}
	}
	UK_ASSERT(bsp_found);

	return 0;
}

#if CONFIG_LIBUKBOOT
static int boot_acpi_madt_fill_cpu_idmap(struct ukplat_bootinfo *bi __unused)
{
	return uk_acpi_madt_fill_cpu_idmap();
}

UK_BOOT_EARLYTAB_ENTRY(boot_acpi_madt_fill_cpu_idmap,
		       UK_PRIO_AFTER(UK_ACPI_INIT_PRIO));
#endif /* CONFIG_LIBUKBOOT */

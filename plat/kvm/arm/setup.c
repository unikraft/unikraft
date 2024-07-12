/* SPDX-License-Identifier: ISC */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Sergiu Moga <sergiu.moga@protonmail.com>
 *
 * Copyright (c) 2018 Arm Ltd.
 * Copyright (c) 2023 University Politehnica of Bucharest.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <uk/config.h>
#include <libfdt.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/acpi.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/common/lcpu.h>
#include <uk/assert.h>
#include <uk/boot.h>
#include <uk/intctlr.h>
#include <arm/cpu.h>
#include <arm/arm64/cpu.h>
#include <arm/smccc.h>
#include <uk/arch/limits.h>

#if CONFIG_ENFORCE_W_XOR_X && CONFIG_PAGING
#include <uk/plat/common/w_xor_x.h>
#endif /* CONFIG_ENFORCE_W_XOR_X && CONFIG_PAGING */

#ifdef CONFIG_ARM64_FEAT_PAUTH
#include <arm/arm64/pauth.h>
#endif /* CONFIG_ARM64_FEAT_PAUTH */

#ifdef CONFIG_HAVE_MEMTAG
#include <uk/arch/memtag.h>
#endif /* CONFIG_HAVE_MEMTAG */

smccc_conduit_fn_t smccc_psci_call;

#ifdef CONFIG_UKPLAT_ACPI
static int get_psci_method(struct ukplat_bootinfo *bi __unused)
{
	struct acpi_fadt *acpi_fadt = acpi_get_fadt();

	if (unlikely(!acpi_fadt))
		return -ENOTSUP;

	if (unlikely(!(acpi_fadt->arm_bflags & ACPI_FADT_ARM_BFLAGS_PSCI)))
		return -ENOTSUP;

	if (acpi_fadt->arm_bflags & ACPI_FADT_ARM_BFLAGS_PSCI_HVC)
		smccc_psci_call = smccc_hvc;
	else
		smccc_psci_call = smccc_smc;

	return 0;
}
#else
static int get_psci_method(struct ukplat_bootinfo *bi)
{
	const char *fdtmethod;
	int fdtpsci, len;
	void *fdt;

	fdt = (void *)bi->dtb;
	UK_ASSERT(bi->dtb);

	/*
	 * We just support PSCI-0.2 and PSCI-1.0, the PSCI-0.1 would not
	 * be supported.
	 */
	fdtpsci = fdt_node_offset_by_compatible(fdt, -1, "arm,psci-1.0");
	if (unlikely(fdtpsci < 0))
		fdtpsci = fdt_node_offset_by_compatible(fdt, -1,
							"arm,psci-0.2");

	if (unlikely(fdtpsci < 0)) {
		uk_pr_info("No PSCI conduit found in DTB\n");
		goto enomethod;
	}

	fdtmethod = fdt_getprop(fdt, fdtpsci, "method", &len);
	if (unlikely(!fdtmethod || len <= 0)) {
		uk_pr_info("No PSCI method found\n");
		goto enomethod;
	}

	if (!strcmp(fdtmethod, "hvc"))
		smccc_psci_call = smccc_hvc;
	else if (!strcmp(fdtmethod, "smc"))
		smccc_psci_call = smccc_smc;
	else {
		uk_pr_info("Invalid PSCI conduit method: %s\n", fdtmethod);
		goto enomethod;
	}
	uk_pr_info("PSCI method: %s\n", fdtmethod);

	return 0;

enomethod:
	uk_pr_info("Support PSCI from PSCI-0.2\n");
	smccc_psci_call = NULL;

	return -ENOENT;
}
#endif

/* At this point we expect that the C runtime is configured and that
 * bootcode has enabled all CPU features used by compiled code.
 */
void __no_pauth _ukplat_entry(void)
{
	struct ukplat_bootinfo *bi;
	void *bstack;
	int rc;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Could not retrieve bootinfo\n");

	uk_boot_early_init(bi);

	/* Allocate boot stack */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");
	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	/* Initialize paging */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize paging (%d)\n", rc);

#if CONFIG_ENFORCE_W_XOR_X && CONFIG_PAGING
	enforce_w_xor_x();
#endif /* CONFIG_ENFORCE_W_XOR_X && CONFIG_PAGING */

#ifdef CONFIG_ARM64_FEAT_PAUTH
	rc = ukplat_pauth_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize PAuth (%d)\n", rc);
#endif /* CONFIG_ARM64_FEAT_PAUTH */

#ifdef CONFIG_HAVE_MEMTAG
	rc = ukarch_memtag_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize MTE (%d)\n", rc);
#endif /* CONFIG_HAVE_MEMTAG */

#if defined(CONFIG_UKPLAT_ACPI)
	rc = acpi_init();
	if (unlikely(rc < 0))
		uk_pr_err("ACPI init failed: %d\n", rc);
#endif /* CONFIG_UKPLAT_ACPI */

	/* Initialize interrupt controller */
	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Could not initialize the IRQ controller: %d\n", rc);

	/* Initialize logical boot CPU */
	rc = lcpu_init(lcpu_get_bsp());
	if (unlikely(rc))
		UK_CRASH("Failed to initialize bootstrapping CPU: %d\n", rc);

#ifdef CONFIG_HAVE_SMP
	rc = lcpu_mp_init(CONFIG_UKPLAT_LCPU_RUN_IRQ,
			  CONFIG_UKPLAT_LCPU_WAKEUP_IRQ,
			  (void *)bi->dtb);
	if (unlikely(rc))
		UK_CRASH("SMP initialization failed: %d.\n", rc);
#endif /* CONFIG_HAVE_SMP */

	rc = get_psci_method(bi);
	if (unlikely(rc < 0))
		UK_CRASH("Failed to get PSCI method: %d.\n", rc);

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n", bstack);

	lcpu_arch_jump_to(bstack, uk_boot_entry);
	ukplat_lcpu_halt();
}

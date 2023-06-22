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
#include <uk/plat/lcpu.h>
#include <uk/plat/common/lcpu.h>
#include <uart/pl011.h>
#ifdef CONFIG_RTC_PL031
#include <rtc/pl031.h>
#endif /* CONFIG_RTC_PL031 */
#include <uk/assert.h>
#include <kvm/intctrl.h>
#include <arm/cpu.h>
#include <arm/arm64/cpu.h>
#include <arm/smccc.h>
#include <uk/arch/limits.h>

#ifdef CONFIG_ARM64_FEAT_PAUTH
#include <arm/arm64/pauth.h>
#endif /* CONFIG_ARM64_FEAT_PAUTH */

#ifdef CONFIG_HAVE_MEMTAG
#include <uk/arch/memtag.h>
#endif /* CONFIG_HAVE_MEMTAG */

smccc_conduit_fn_t smccc_psci_call;

static void _dtb_get_psci_method(void *fdt)
{
	int fdtpsci, len;
	const char *fdtmethod;

	/*
	 * We just support PSCI-0.2 and PSCI-1.0, the PSCI-0.1 would not
	 * be supported.
	 */
	fdtpsci = fdt_node_offset_by_compatible(fdt, -1,
						"arm,psci-1.0");
	if (fdtpsci < 0)
		fdtpsci = fdt_node_offset_by_compatible(fdt,
							-1, "arm,psci-0.2");

	if (fdtpsci < 0) {
		uk_pr_info("No PSCI conduit found in DTB\n");
		goto enomethod;
	}

	fdtmethod = fdt_getprop(fdt, fdtpsci, "method", &len);
	if (!fdtmethod || (len <= 0)) {
		uk_pr_info("No PSCI method found\n");
		goto enomethod;
	}

	if (!strcmp(fdtmethod, "hvc"))
		smccc_psci_call = smccc_hvc;
	else if (!strcmp(fdtmethod, "smc"))
		smccc_psci_call = smccc_smc;
	else {
		uk_pr_info("Invalid PSCI conduit method: %s\n",
			   fdtmethod);
		goto enomethod;
	}
	uk_pr_info("PSCI method: %s\n", fdtmethod);
	return;

enomethod:
	uk_pr_info("Support PSCI from PSCI-0.2\n");
	smccc_psci_call = NULL;
}

static char *cmdline;
static __sz cmdline_len;

static inline int cmdline_init(struct ukplat_bootinfo *bi)
{
	char *cmdl;

	if (bi->cmdline_len) {
		cmdl = (char *)bi->cmdline;
		cmdline_len = bi->cmdline_len;
	} else {
		cmdl = CONFIG_UK_NAME;
		cmdline_len = sizeof(CONFIG_UK_NAME) - 1;
	}

	/* This is not the original command-line, but one that will be thrashed
	 * by `ukplat_entry_argp` to obtain argc/argv. So mark it as a kernel
	 * resource instead.
	 */
	cmdline = ukplat_memregion_alloc(cmdline_len + 1, UKPLAT_MEMRT_KERNEL,
					 UKPLAT_MEMRF_READ |
					 UKPLAT_MEMRF_WRITE |
					 UKPLAT_MEMRF_MAP);
	if (unlikely(!cmdline))
		return -ENOMEM;

	memcpy(cmdline, cmdl, cmdline_len);
	cmdline[cmdline_len] = 0;

	return 0;
}

static void __noreturn _ukplat_entry2(void)
{
	ukplat_entry_argp(NULL, cmdline, cmdline_len);

	ukplat_lcpu_halt();
}

void __no_pauth _ukplat_entry(struct ukplat_bootinfo *bi)
{
	void *bstack;
	void *fdt;
	int rc;

	fdt = (void *)bi->dtb;

	pl011_console_init(fdt);

	rc = cmdline_init(bi);
	if (unlikely(rc < 0))
		UK_CRASH("Failed to initialize command-line\n");

	/* Allocate boot stack */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE |
					UKPLAT_MEMRF_MAP);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");
	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	/* Get PSCI method from DTB */
	_dtb_get_psci_method(fdt);

	/* Initialize paging */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize paging (%d)\n", rc);

#if defined(CONFIG_ENFORCE_W_XOR_X) && defined(CONFIG_PAGING)
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

#ifdef CONFIG_RTC_PL031
	/* Initialize RTC */
	pl031_init_rtc(fdt);
#endif /* CONFIG_RTC_PL031 */

	/* Initialize interrupt controller */
	intctrl_init();

	/* Initialize logical boot CPU */
	rc = lcpu_init(lcpu_get_bsp());
	if (unlikely(rc))
		UK_CRASH("Failed to initialize bootstrapping CPU: %d\n", rc);

#ifdef CONFIG_HAVE_SMP
	rc = lcpu_mp_init(CONFIG_UKPLAT_LCPU_RUN_IRQ,
			  CONFIG_UKPLAT_LCPU_WAKEUP_IRQ,
			  fdt);
	if (unlikely(rc))
		UK_CRASH("SMP initialization failed: %d.\n", rc);
#endif /* CONFIG_HAVE_SMP */

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n", bstack);

	lcpu_arch_jump_to(bstack, _ukplat_entry2);
}

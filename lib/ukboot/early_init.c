/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/argparse.h>
#include <uk/assert.h>
#include <uk/boot/earlytab.h>
#include <uk/essentials.h>
#include <uk/libparam.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/prio.h>

#if CONFIG_LIBUKCFI_PAUTH
#include <uk/cfi.h>
#endif /* CONFIG_LIBUKCFI_PAUTH */

/* Mutable version of the cmdline. This is tokenized by uk_argnparse().
 * Keep this as a global as `argv` points to tokens in this string.
 */
static char cmdline[CONFIG_LIBUKBOOT_CMDLINE_MAX_LEN];
static __sz cmdline_len;

char *arg_vect[CONFIG_LIBUKBOOT_MAXNBARGS];
char **boot_argv = arg_vect; /* updated if CONFIG_LIBUKPARAM */
int boot_argc;

#define uk_boot_earlytab_foreach(itr)			\
	for ((itr) = (struct uk_boot_earlytab_entry *)	\
			uk_boot_earlytab_start;		\
	     (itr) < &(uk_boot_earlytab_end);		\
	     (itr)++)

static int uk_boot_early_init_parse_cmdl(struct ukplat_bootinfo *bi)
{
	int rc __maybe_unused;
	const char *cmdl;

	UK_ASSERT(bi);

	if (!bi->cmdline_len) {
		cmdl = CONFIG_UK_NAME;
		cmdline_len = sizeof(CONFIG_UK_NAME);
	} else {
		cmdl = (const char *)bi->cmdline;
		cmdline_len = bi->cmdline_len;
	}

	if (unlikely(cmdline_len > ARRAY_SIZE(cmdline))) {
		rc = -E2BIG;
		uk_pr_err("Could not parse cmdline (%d)\n", rc);
		return rc;
	}

	/* Break down cmdline to argv */
	strncpy(cmdline, cmdl, ARRAY_SIZE(cmdline));
	cmdline[cmdline_len] = '\0';

	boot_argc = uk_argnparse(cmdline, cmdline_len, boot_argv,
				 CONFIG_LIBUKBOOT_MAXNBARGS);

#if CONFIG_LIBUKLIBPARAM
	/*
	 * First, we scan if we can find the stop sequence in the kernel
	 * cmdline. If not, we assume that there are no uklibparam arguments in
	 * the command line.
	 * NOTE: argv[0] contains the kernel/program name that we need to
	 *       hide from the parser.
	 */
	rc = uk_libparam_parse(boot_argc - 1, &boot_argv[1],
			       UK_LIBPARAM_F_SCAN);
	if (rc > 0 && rc < (boot_argc - 1)) {
		/* In this case, we did successfully scan for uklibparam
		 * arguments and stop sequence is at rc < (argc - 1).
		 */
		/* Run a second pass for parsing */
		rc = uk_libparam_parse(boot_argc - 1, &boot_argv[1],
				       UK_LIBPARAM_F_USAGE);
		if (unlikely(rc < 0)) /* go down on errors (including USAGE) */
			return -EINVAL;

		/* Drop uklibparam parameters from argv but keep argv[0].
		 * We are going to replace the stop sequence with argv[0].
		 */
		rc += 1; /* include argv[0]; we use rc as idx to stop seq */
		boot_argc -= rc;
		boot_argv[rc] = boot_argv[0];
		boot_argv = &boot_argv[rc];
	}
#endif /* CONFIG_LIBUKLIBPARAM */

	/* Prevent bi->cmdline from being accessed later
	 * as, depending on the boot protocol, it may be
	 * in memory that is unmapped at memory_init().
	 */
	bi->cmdline = (__u64)NULL;

	return 0;
}

static int uk_boot_early_init_mrd_coalesce(struct ukplat_bootinfo *bi)
{
	UK_ASSERT(bi);
	ukplat_memregion_list_coalesce(&bi->mrds);
	return 0;
}

#if CONFIG_LIBUKCFI_PAUTH
void __no_pauth uk_boot_early_init(struct ukplat_bootinfo *bi)
#else /* CONFIG_LIBUKCFI_PAUTH */
void uk_boot_early_init(struct ukplat_bootinfo *bi)
#endif /* CONFIG_LIBUKCFI_PAUTH */
{
	struct uk_boot_earlytab_entry *entry;
	int rc;

	UK_ASSERT(bi);

	ukplat_bootinfo_print();

	uk_boot_earlytab_foreach(entry) {
		UK_ASSERT(entry);
		rc = (*entry->init)(bi);
		if (unlikely(rc))
			UK_CRASH("Early init call @ %p failed with %d",
				 entry->init, rc);
	}
}

/* Parse cmdline parameters at the earliest priority
 * to make them available to the rest of early init
 */
UK_BOOT_EARLYTAB_ENTRY(uk_boot_early_init_parse_cmdl, UK_PRIO_EARLIEST);

/* Reserve UK_PRIO_LATEST for post-coalesce operations */
UK_BOOT_EARLYTAB_ENTRY(uk_boot_early_init_mrd_coalesce,
		       UK_PRIO_BEFORE(UK_PRIO_LATEST));

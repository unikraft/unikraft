/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <uk/assert.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/file.h>
#include <uk/file-console.h>
#include <uk/init.h>

#if CONFIG_LIBUKLIBPARAM
#include <uk/libparam.h>
#endif /* CONFIG_LIBUKLIBPARAM */

static char *tty_cons_arg;

#if CONFIG_LIBUKLIBPARAM
UK_LIBPARAM_PARAM_ALIAS(console, &tty_cons_arg, charp,
			"Default application stdio console device based on devfs node");
#endif /* CONFIG_LIBUKLIBPARAM */

const struct uk_file *tty_f;

static int init_tty_cons(struct uk_init_ctx *ictx __unused)
{
	__sz i, ccount, idx, cclass_str_len = 0;
	enum uk_console_devclass tty_dclass;
	struct uk_console *cons, *tty_cons;
	int rc;

	/* If no argument provided, assume the first registered one */
	if (!tty_cons_arg) {
		tty_cons = uk_console_get(0);
		goto setup_tty_globals;
	}

	if (!strncmp(tty_cons_arg, "hvc", sizeof("hvc") - 1)) {
		tty_dclass = UK_CONSOLE_CLASS_HVC;
		cclass_str_len = sizeof("hvc") - 1;
	} else if (!strncmp(tty_cons_arg, "ttyS", sizeof("ttyS") - 1)) {
		tty_dclass = UK_CONSOLE_CLASS_UART;
		cclass_str_len = sizeof("ttyS") - 1;
	} else if (!strncmp(tty_cons_arg, "tty", sizeof("tty") - 1)) {
		tty_dclass = UK_CONSOLE_CLASS_FB;
		cclass_str_len = sizeof("tty") - 1;
	} else {
		uk_pr_err("Invalid console argument\n");
		return -EINVAL;
	}

	rc = sscanf(tty_cons_arg + cclass_str_len, "%lu", &idx);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to parse tty_console arg\n");
		return rc;
	}

	/**
	 * Now go through each registered console and find the idx'th console
	 * of class tty_cclass.
	 */
	ccount = uk_console_count();
	for (i = 0; i < ccount; i++) {
		cons = uk_console_get(i);
		UK_ASSERT(cons);

		 /* If not the class we care about, ignore and move on */
		if (cons->dclass != tty_dclass)
			continue;

		/**
		 * If idx is 0 and this is the first console of class
		 * tty_cclass then we are done - assign it.
		 * Otherwise we keep decrementing idx for each tty_cclass
		 * class registration we find and when it hits 0 then we know
		 * we found the user-intended idx'th console registration
		 * of this class.
		 */
		if (!idx) {
			tty_cons = cons;
			break;
		}
		idx--;
	}

	if (unlikely(!tty_cons)) {
		uk_pr_err("Requested console not found\n");
		return -ENOENT;
	}

setup_tty_globals:
	tty_f = uk_file_console_create(tty_cons);
	if (unlikely(!tty_f)) {
		uk_pr_err("Failed to create main console-backed file\n");
		return -ENOMEM;
	}

	return 0;
}

uk_rootfs_initcall_prio(init_tty_cons, 0x0, UK_FS_PRIO_FSAVAIL);

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/init.h>
#include <uk/posix-fd.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-pseudofile.h>
#include <uk/posix-serialfile.h>

static int init_posix_tty(struct uk_init_ctx *ictx __unused)
{
	const struct uk_file *in;
	const struct uk_file *out;
	int r;

#if CONFIG_LIBPOSIX_TTY_STDIN_NULL
	in = uk_nullfile_create();
#elif CONFIG_LIBPOSIX_TTY_STDIN_VOID
	in = uk_voidfile_create();
#elif CONFIG_LIBPOSIX_TTY_STDIN_SERIAL
	in = uk_serialfile_create();
#else /* !CONFIG_LIBPOSIX_TTY_STDIN_* */
	#error Nonexistent stdin file
#endif

#if CONFIG_LIBPOSIX_TTY_STDOUT_NULL
	out = uk_nullfile_create();
#elif CONFIG_LIBPOSIX_TTY_STDOUT_SERIAL
	out = uk_serialfile_create();
#else /* !CONFIG_LIBPOSIX_TTY_STDOUT_* */
	#error Nonexistent stdout file
#endif

	r = uk_fdtab_open(in, O_RDONLY|UKFD_O_NOSEEK);
	if (unlikely(r != 0)) {
		uk_pr_err("Failed to allocate fd for stdin: %d\n", r);
		return (r < 0) ? r : -EBADF;
	}

	r = uk_fdtab_open(out, O_WRONLY|UKFD_O_NOSEEK);
	if (unlikely(r != 1)) {
		uk_pr_err("Failed to allocate fd for stdout: %d\n", r);
		return (r < 0) ? r : -EBADF;
	}

	r = uk_sys_dup2(1, 2);
	if (unlikely(r != 2)) {
		uk_pr_err("Failed to allocate fd for stderr: %d\n", r);
		return (r < 0) ? r : -EBADF;
	}

	return 0;
}

uk_rootfs_initcall_prio(init_posix_tty, 0x0, UK_PRIO_LATEST);

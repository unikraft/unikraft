/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/config.h>
#include <uk/init.h>
#include <uk/posix-fd.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-serialfile.h>

#if CONFIG_LIBPOSIX_TTY_PSEUDO
#include <uk/file-pseudo.h>
#endif /* CONFIG_LIBPOSIX_TTY_PSEUDO */

#if CONFIG_LIBPOSIX_TTY_DEVFS
#include <uk/devfs.h>
#include <uk/fs.h>
#include <uk/fs/prio.h>

static int init_posix_tty_devfs(const struct uk_file *in,
				const struct uk_file *out,
				const struct uk_file *err)
{
	const void *r;

	/* We borrow the singleton static reference, no refcounting needed */
	UK_ASSERT(uk_fs_devfs_root);

	/* We do not clean up created files on error, as they will be dropped
	 * when the devfs root is released on system shutdown.
	 */
	r = uk_fs_createat(uk_fs_devfs_root, "stdin", 4, 0444, O_EXCL,
			   (union uk_fs_create_target){ .file = in });
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/stdin: %d\n", PTR2ERR(r));
		return PTR2ERR(r);
	}
	r = uk_fs_createat(uk_fs_devfs_root, "stdout", 4, 0222, O_EXCL,
			   (union uk_fs_create_target){ .file = out });
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/stdout: %d\n", PTR2ERR(r));
		return PTR2ERR(r);
	}
	r = uk_fs_createat(uk_fs_devfs_root, "stderr", 4, 0222, O_EXCL,
			   (union uk_fs_create_target){ .file = err });
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/stderr: %d\n", PTR2ERR(r));
		return PTR2ERR(r);
	}
	return 0;
}
#endif /* CONFIG_LIBPOSIX_TTY_DEVFS */

#define STDIN_FNAME_NULL "stdin:null"
#define STDIN_FNAME_LEN_NULL (sizeof(STDIN_FNAME_NULL) - 1)

#define STDIN_FNAME_VOID "stdin:void"
#define STDIN_FNAME_LEN_VOID (sizeof(STDIN_FNAME_VOID) - 1)

#define STDIN_FNAME_SERIAL "stdin:serial"
#define STDIN_FNAME_LEN_SERIAL (sizeof(STDIN_FNAME_SERIAL) - 1)

#define STDOUT_FNAME_NULL "stdout:null"
#define STDOUT_FNAME_LEN_NULL (sizeof(STDOUT_FNAME_NULL) - 1)

#define STDOUT_FNAME_SERIAL "stdout:serial"
#define STDOUT_FNAME_LEN_SERIAL (sizeof(STDOUT_FNAME_SERIAL) - 1)

static int init_posix_tty(struct uk_init_ctx *ictx __unused)
{
	const struct uk_file *in;
	const struct uk_file *out;
	const char *in_fname;
	const char *out_fname;
	size_t in_fnamelen;
	size_t out_fnamelen;
	int r;

#if CONFIG_LIBPOSIX_TTY_STDIN_NULL
	in = &uk_file_null;
	in_fname = STDIN_FNAME_NULL;
	in_fnamelen = STDIN_FNAME_LEN_NULL;
#elif CONFIG_LIBPOSIX_TTY_STDIN_VOID
	in = &uk_file_void;
	in_fname = STDIN_FNAME_VOID;
	in_fnamelen = STDIN_FNAME_LEN_VOID;
#elif CONFIG_LIBPOSIX_TTY_STDIN_SERIAL
	in = &uk_file_serial;
	in_fname = STDIN_FNAME_SERIAL;
	in_fnamelen = STDIN_FNAME_LEN_SERIAL;
#else /* !CONFIG_LIBPOSIX_TTY_STDIN_* */
#error Nonexistent stdin file
#endif /* !CONFIG_LIBPOSIX_TTY_STDIN_* */

#if CONFIG_LIBPOSIX_TTY_STDOUT_NULL
	out = &uk_file_null;
	out_fname = STDOUT_FNAME_NULL;
	out_fnamelen = STDOUT_FNAME_LEN_NULL;
#elif CONFIG_LIBPOSIX_TTY_STDOUT_SERIAL
	out = &uk_file_serial;
	out_fname = STDOUT_FNAME_SERIAL;
	out_fnamelen = STDOUT_FNAME_LEN_SERIAL;
#else /* !CONFIG_LIBPOSIX_TTY_STDOUT_* */
#error Nonexistent stdout file
#endif /* !CONFIG_LIBPOSIX_TTY_STDOUT_* */

	r = uk_fdtab_open_named(in, O_RDONLY | UKFD_O_NOSEEK,
				in_fname, in_fnamelen);
	if (unlikely(r != 0)) {
		uk_pr_err("Failed to allocate fd for stdin: %d\n", r);
		return (r < 0) ? r : -EBADF;
	}

	r = uk_fdtab_open_named(out, O_WRONLY | UKFD_O_NOSEEK,
				out_fname, out_fnamelen);
	if (unlikely(r != 1)) {
		uk_pr_err("Failed to allocate fd for stdout: %d\n", r);
		return (r < 0) ? r : -EBADF;
	}

	r = uk_sys_dup2(1, 2);
	if (unlikely(r != 2)) {
		uk_pr_err("Failed to allocate fd for stderr: %d\n", r);
		return (r < 0) ? r : -EBADF;
	}

#if CONFIG_LIBPOSIX_TTY_DEVFS
	return init_posix_tty_devfs(in, out, out);
#else /* !CONFIG_LIBPOSIX_TTY_DEVFS */
	return 0;
#endif /* !CONFIG_LIBPOSIX_TTY_DEVFS */
}

#if CONFIG_LIBPOSIX_TTY_DEVFS
uk_rootfs_initcall_prio(init_posix_tty, 0x0, UK_FS_PRIO_FSAVAIL);
#else /* !CONFIG_LIBPOSIX_TTY_DEVFS */
uk_rootfs_initcall(init_posix_tty, 0x0);
#endif /* !CONFIG_LIBPOSIX_TTY_DEVFS */

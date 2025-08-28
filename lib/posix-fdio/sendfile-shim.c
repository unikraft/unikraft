/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Generic implementation for sendfile using two I/O calls and a temp buffer. */

#include <unistd.h>

#include <uk/assert.h>
#include <uk/posix-fd.h>
#include <uk/posix-fdio.h>
#include <uk/posix-fdtab.h>
#include <uk/syscall.h>

#if CONFIG_LIBVFSCORE
#include <vfscore/file.h>
#include <vfscore/syscalls.h>
#endif /* CONFIG_LIBVFSCORE */

/* We define sendfile as a syscall shimming between vfscore and ukfiles for now.
 * Once vfscore is removed, the implementation should be moved into fdio.c
 * where it belongs, with a corresponding `uk_sys_*` equivalent.
 *
 * Shim implementations involving vfscore are not synchronized.
 */

static inline
int shim_get(int fd, union uk_shim_file *sf)
{
	const int r = uk_fdtab_shim_get(fd, sf);

	switch (r) {
	case UK_SHIM_OFILE:
#if CONFIG_LIBVFSCORE
	case UK_SHIM_LEGACY:
#endif /* CONFIG_LIBVFSCORE */
		return r;
	default:
		return -1;
	}
}

static inline
void shim_release(int type, union uk_shim_file sf)
{
	switch (type) {
	case UK_SHIM_OFILE:
		uk_ofile_release(sf.ofile);
		break;
#if CONFIG_LIBVFSCORE
	case UK_SHIM_LEGACY:
		fdrop(sf.vfile);
		break;
#endif /* CONFIG_LIBVFSCORE */
	default:
		UK_BUG(); /* Do validation before */
	}
}

/* Convenience for all possible combinations of in/out and offset */
enum shim_combo {
	SHIM_OFILE_OFILE,
	SHIM_OFILE_OFILE_OFF,
#if CONFIG_LIBVFSCORE
	SHIM_OFILE_LEGACY,
	SHIM_OFILE_LEGACY_OFF,
	SHIM_LEGACY_OFILE,
	SHIM_LEGACY_OFILE_OFF,
	SHIM_LEGACY_LEGACY,
	SHIM_LEGACY_LEGACY_OFF
#endif /* CONFIG_LIBVFSCORE */
};

#if CONFIG_LIBVFSCORE
static inline
enum shim_combo shim_combo_of(int in_type, int out_type, void *op)
{
	switch (in_type) {
	case UK_SHIM_OFILE:
		switch (out_type) {
		case UK_SHIM_OFILE:
			return op ? SHIM_OFILE_OFILE_OFF : SHIM_OFILE_OFILE;
		case UK_SHIM_LEGACY:
			return op ? SHIM_OFILE_LEGACY_OFF : SHIM_OFILE_LEGACY;
		default:
			UK_BUG(); /* Do validation before */
		}
	case UK_SHIM_LEGACY:
		switch (out_type) {
		case UK_SHIM_OFILE:
			return op ? SHIM_LEGACY_OFILE_OFF : SHIM_LEGACY_OFILE;
		case UK_SHIM_LEGACY:
			return op ? SHIM_LEGACY_LEGACY_OFF : SHIM_LEGACY_LEGACY;
		default:
			UK_BUG(); /* Do validation before */
		}
	default:
		UK_BUG(); /* Do validation before */
	}
}
#else /* !CONFIG_LIBVFSCORE */
static inline
enum shim_combo shim_combo_of(int in_type __unused, int out_type __unused,
			      void *op)
{
	return op ? SHIM_OFILE_OFILE_OFF : SHIM_OFILE_OFILE;
}
#endif /* !CONFIG_LIBVFSCORE */

#if CONFIG_LIBVFSCORE

/* HACK: Both uk_sys_ and vfscore_ ops share signatures, except for typeof f.
 *
 * Function type punning acceptable only because shim w/ vfscore; remove ASAP.
 * Probably not compatible with very strictly-typed CFI either.
 */
typedef ssize_t (*sendfile_readf)(void *f, void *buf, size_t count);
typedef ssize_t (*sendfile_preadf)(void *f, void *buf, size_t count, off_t off);
typedef ssize_t (*sendfile_writef)(void *f, const void *buf, size_t count);
typedef off_t (*sendfile_lseekf)(void *f, off_t offset, int whence);

/* Arbitrary; small enough to fit on stacks, big enough to be efficient. */
#define SENDFILE_BUFSZ 0x1000UL

static
ssize_t sendfile_compat(sendfile_readf readf, sendfile_writef writef,
			sendfile_lseekf lseekf,
			void *infile, void *outfile, size_t count)
{
	char tmp[SENDFILE_BUFSZ];
	size_t sent = 0;
	ssize_t r;

	uk_pr_info("Fallback sendfile of 0x%zx bytes from stateful file offset\n",
		   count);
	do {
		const size_t len = MIN(count, SENDFILE_BUFSZ);
		size_t remain;
		size_t written = 0;

		r = readf(infile, tmp, len);
		if (r <= 0)
			break;
		remain = r;
		do {
			r = writef(outfile, &tmp[written], remain);
			if (r <= 0)
				break;
			remain -= r;
			written += r;
			sent += r;
			count -= r;
		} while (remain);
		/* Rewind infile offset to before last read & break on error */
		if (unlikely(remain)) {
			off_t rc;

			rc = lseekf(infile, -remain, SEEK_CUR);
			if (unlikely(rc < 0))
				uk_pr_warn("Unable to revert input file as a result of sendfile write error %zd: %zd\n",
					   r, rc);
			break;
		}
	} while (count);
	UK_ASSERT(sent || r <= 0); /* Return only error or nop through r */
	return sent ? (ssize_t)sent : r;
}

static
ssize_t psendfile_compat(sendfile_preadf preadf, sendfile_writef writef,
			 void *infile, void *outfile, off_t *offset,
			 size_t count)
{
	char tmp[SENDFILE_BUFSZ];
	size_t sent = 0;
	off_t off = *offset;
	ssize_t r;

	uk_pr_info("Fallback sendfile of 0x%zx bytes from offset 0x%zx\n",
		   count, (size_t)off);
	do {
		const size_t len = MIN(count, SENDFILE_BUFSZ);
		size_t remain;
		size_t written = 0;

		r = preadf(infile, tmp, len, off);
		if (r <= 0)
			break;
		remain = r;
		do {
			r = writef(outfile, &tmp[written], remain);
			if (r <= 0)
				goto out;
			remain -= r;
			written += r;
			sent += r;
			count -= r;
			off += r;
		} while (remain);
	} while (count);
out:
	*offset = off;
	UK_ASSERT(sent || r <= 0); /* Return only error or nop through r */
	return sent ? (ssize_t)sent : r;
}

#endif /* CONFIG_LIBVFSCORE */

UK_SYSCALL_R_DEFINE(ssize_t, sendfile, int, out_fd, int, in_fd, off_t *, offp,
		    size_t, count)
{
	union uk_shim_file out_sf;
	union uk_shim_file in_sf;
	int out_type;
	int in_type;
	ssize_t ret;

	if (unlikely(offp && *offp < 0))
		return -EINVAL;

	in_type = shim_get(in_fd, &in_sf);
	if (unlikely(in_type < 0))
		return -EBADF;
	out_type = shim_get(out_fd, &out_sf);
	if (unlikely(out_type < 0)) {
		ret = -EBADF;
		goto out_insf;
	}

	count = MIN(count, 0x7ffff000U); /* Documented in sendfile(2) NOTES */
	switch (shim_combo_of(in_type, out_type, offp)) {
	case SHIM_OFILE_OFILE:
	case SHIM_OFILE_OFILE_OFF:
		/* Only true sendfile */
		ret = uk_sys_sendfile(out_sf.ofile, in_sf.ofile, offp, count);
		break;
#if CONFIG_LIBVFSCORE
	case SHIM_OFILE_LEGACY:
		ret = sendfile_compat((sendfile_readf)uk_sys_read,
				      (sendfile_writef)vfscore_write,
				      (sendfile_lseekf)uk_sys_lseek,
				      in_sf.ofile, out_sf.vfile, count);
		break;
	case SHIM_OFILE_LEGACY_OFF:
		ret = psendfile_compat((sendfile_preadf)uk_sys_pread,
				       (sendfile_writef)vfscore_write,
				       in_sf.ofile, out_sf.vfile, offp, count);
		break;
	case SHIM_LEGACY_OFILE:
		ret = sendfile_compat((sendfile_readf)vfscore_read,
				      (sendfile_writef)uk_sys_write,
				      (sendfile_lseekf)vfscore_lseek,
				      in_sf.vfile, out_sf.ofile, count);
		break;
	case SHIM_LEGACY_OFILE_OFF:
		ret = psendfile_compat((sendfile_preadf)vfscore_pread64,
				       (sendfile_writef)uk_sys_write,
				       in_sf.vfile, out_sf.ofile, offp, count);
		break;
	case SHIM_LEGACY_LEGACY:
		ret = sendfile_compat((sendfile_readf)vfscore_read,
				      (sendfile_writef)vfscore_write,
				      (sendfile_lseekf)vfscore_lseek,
				      in_sf.vfile, out_sf.vfile, count);
		break;
	case SHIM_LEGACY_LEGACY_OFF:
		ret = psendfile_compat((sendfile_preadf)vfscore_pread64,
				       (sendfile_writef)vfscore_write,
				       in_sf.vfile, out_sf.vfile, offp, count);
		break;
#endif /* CONFIG_LIBVFSCORE */
	}

	shim_release(out_type, out_sf);
out_insf:
	shim_release(in_type, in_sf);
	return ret;
}

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include <uk/assert.h>
#include <uk/atomic.h>
#include <uk/print.h>

#include "virtiofs-fuse.h"

#include "fuse.h"

/* This driver assumes FUSE major version 7 */
UK_CTASSERT(FUSE_KERNEL_VERSION == 7);

/* Log here all features that we *require* and their minimum version:
 * - FUSE_DESTROY			7.8
 * - fuse_getattr_in arg to GETATTR	7.9
 * - FUSE_FALLOCATE			7.19
 * - FATTR_CTIME			7.23
 * - FUSE_RENAME2?			7.23
 * - FUSE_*MAPPING (DAX)?		7.31
 * - FUSE_SYNCFS?			7.34
 * - FUSE_STATX?			7.39
 * Entries with ? are optional features that we gracefully handle.
 * Minimum supported minor version is the highest among mandatory features.
 */
#define VIRTIOFS_MIN_FUSE_MINOR 23

/* We assume struct iovec and struct uk_virtiofs_iovec are bit-compatible */
UK_CTASSERT(sizeof(struct iovec) == sizeof(struct uk_virtiofs_iovec));
UK_CTASSERT(__alignof__(struct iovec) == __alignof__(struct uk_virtiofs_iovec));
UK_CTASSERT(__offsetof(struct iovec, iov_base) ==
	    __offsetof(struct uk_virtiofs_iovec, iov_base));
UK_CTASSERT(__offsetof(struct iovec, iov_len) ==
	    __offsetof(struct uk_virtiofs_iovec, iov_len));

/* Maximum payload for a single virtiofs FUSE operation */
#define VIRTIOFS_MAX_PAYLOAD (UINT32_MAX - sizeof(struct fuse_in_header))

/* Minimum length of data to completely cover `field` */
#define FIELDLEN(t, field) \
	(__offsetof(t, field) + sizeof(((t *)NULL)->field))

/* Singleton NUL for terminating zero-copy strings in requests */
static const char NULCHR = '\0';

/* Overflow-safe helpers to compute total payload length & check against max */

static inline
bool payload_ok(size_t s)
{
	return s <= VIRTIOFS_MAX_PAYLOAD;
}

static inline
bool payload_sum(size_t *s, size_t l0, size_t l1)
{
	return !__builtin_add_overflow(l0, l1, s) &&
	       payload_ok(*s);
}

static inline
bool payload_sum3(size_t *s, size_t l0, size_t l1, size_t l2)
{
	return !__builtin_add_overflow(l0, l1, s) &&
	       !__builtin_add_overflow(*s, l2, s) &&
	       payload_ok(*s);
}

static inline
bool payload_sumv(size_t *s, size_t l0, size_t nelem, size_t elsz)
{
	return !__builtin_mul_overflow(nelem, elsz, s) &&
	       !__builtin_add_overflow(*s, l0, s) &&
	       payload_ok(*s);
}

/* Atomically fetch+inc (and thus acquire) the next unique number on a device */
static inline
uint64_t next_unique(struct virtiofs_fuse_dev *fdev)
{
	return uk_inc(&fdev->unique);
}

/* TODO: use real values from calling context */
static inline
void set_credentials(struct fuse_in_header *hdr)
{
	hdr->uid = 0;
	hdr->gid = 0;
	hdr->pid = 0;
}

static inline
void init_hdr(struct virtiofs_fuse_dev *fdev, struct fuse_in_header *hdr,
	      enum fuse_opcode opcode, uint32_t paylen)
{
	hdr->len = sizeof(*hdr) + paylen;
	hdr->opcode = opcode;
	hdr->unique = next_unique(fdev);
	hdr->total_extlen = 0;
	set_credentials(hdr);
}

/* Validate a basic reply message, extracting the error if one occurred */
static inline
int check_reply(ssize_t ret, struct fuse_out_header *ohdr)
{
	if (unlikely(ret < 0))
		return ret;
	if (ohdr) {
		if (unlikely((size_t)ret < sizeof(*ohdr)))
			return -EIO; /* Reply too short */
		if (unlikely((size_t)ret != ohdr->len))
			uk_pr_err("Response size mismatch: %zd != %u\n",
				  ret, ohdr->len);
		if (unlikely(ohdr->error))
			return ohdr->error;
	} else if (unlikely(ret)) {
		return -EIO; /* Unexpected write */
	}
	return 0;
}

static inline
size_t payload_len(ssize_t ret)
{
	UK_ASSERT(ret >= 0);
	UK_ASSERT((size_t)ret >= sizeof(struct fuse_out_header));
	return (size_t)ret - sizeof(struct fuse_out_header);
}

static inline
int check_reply_payload(ssize_t ret, struct fuse_out_header *ohdr,
			size_t expected)
{
	const int err = check_reply(ret, ohdr);

	if (unlikely(err))
		return err;
	if (unlikely(payload_len(ret) != expected))
		return -EIO;
	return 0;
}

/* Lib-internal "public" API for FUSE ops */

int virtiofs_fuse_destroy(struct virtiofs_fuse_dev *fdev)
{
	struct fuse_in_header hdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_DESTROY, 0);
	rlen = uk_virtiofs_request(fdev->dev, iov, 1, 0);
	return check_reply(rlen, NULL);
}

#define VIRTIOFS_FUSE_FLAGS (FUSE_BIG_WRITES)

int virtiofs_fuse_init(struct virtiofs_fuse_dev *fdev)
{
	struct fuse_in_header hdr;
	const struct fuse_init_in in = {
		.major = FUSE_KERNEL_VERSION,
		.minor = FUSE_KERNEL_MINOR_VERSION,
		.flags = VIRTIOFS_FUSE_FLAGS,
	};
	struct fuse_out_header ohdr;
	struct fuse_init_out out;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) },
		{ &out, sizeof(out) }
	};
	int err;
	ssize_t rlen;
	size_t paylen;
	uint32_t minor;

	init_hdr(fdev, &hdr, FUSE_INIT, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2);
	if (unlikely((err = check_reply(rlen, &ohdr))))
		return err;

	paylen = payload_len(rlen);
	/* Check major version */
	if (unlikely(paylen < FIELDLEN(struct fuse_init_out, major)))
		return -ENODEV; /* Reply w/o major number; bad device */
	if (unlikely(out.major != FUSE_KERNEL_VERSION)) {
		/* Device must not reply with a larger version than asked */
		if (unlikely(out.major > FUSE_KERNEL_VERSION))
			return -ENODEV;
		/* We only support the current major version; this may change */
		return -EPROTO;
	}
	/* Check minor version */
	if (unlikely(paylen < FIELDLEN(struct fuse_init_out, minor)))
		return -ENODEV; /* Correct major but no minor; bad device */
	if (unlikely(out.minor < VIRTIOFS_MIN_FUSE_MINOR)) {
		/* Init valid, but device is too old; destroy & error */
		err = -EPROTO;
		goto err_destroy;
	}
	minor = MIN(out.minor, (uint32_t)FUSE_KERNEL_MINOR_VERSION);

	uk_pr_debug("Negotiated FUSE version %u.%u\n",
		    FUSE_KERNEL_VERSION, minor);

	/* Check flags */
	if (unlikely(paylen < FIELDLEN(struct fuse_init_out, flags)))
		return -ENODEV;
	if (unlikely(out.flags != VIRTIOFS_FUSE_FLAGS)) {
		uk_pr_err("Flags mismatch: %x vs. %x\n",
			  VIRTIOFS_FUSE_FLAGS, out.flags);
		err = -EPROTO;
		goto err_destroy;
	}

	return 0;

err_destroy:
	virtiofs_fuse_destroy(fdev);
	return err;
}

int virtiofs_fuse_forget(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, uint64_t nlookup)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_forget_in in = {
		.nlookup = nlookup
	};
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_FORGET, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 0);
	return check_reply(rlen, NULL);
}

int virtiofs_fuse_batch_forget(struct virtiofs_fuse_dev *fdev,
			       const struct fuse_forget_one *arg,
			       uint32_t count)
{
	struct fuse_in_header hdr;
	const struct fuse_batch_forget_in in = {
		.count = count
	};
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ (void *)arg, count * sizeof(*arg) }
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sumv(&paylen, sizeof(in), count, sizeof(*arg))))
		return -EDOM;

	init_hdr(fdev, &hdr, FUSE_BATCH_FORGET, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 3, 0);
	return check_reply(rlen, NULL);
}

int virtiofs_fuse_interrupt(struct virtiofs_fuse_dev *fdev, uint64_t unique)
{
	struct fuse_in_header hdr;
	const struct fuse_interrupt_in in = {
		.unique = unique
	};
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_INTERRUPT, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 0);
	return check_reply(rlen, NULL);
}

#define OUT_IOV(out, out_x)					\
	(out_x ?							\
	 (struct uk_virtiofs_iovec){ (out).trunc, sizeof(*(out).trunc) } : \
	 (struct uk_virtiofs_iovec){ (out).full, sizeof(*(out).full) }	\
	)

#define OUTX_IOV(out_x) \
	{ (out_x), (out_x) ? sizeof(*(out_x)) : 0 }

int virtiofs_fuse_lookup(struct virtiofs_fuse_dev *fdev, uint64_t node,
			 const char *fname, size_t len,
			 union virtiofs_fuse_entry_outp out,
			 struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)fname, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_LOOKUP, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 3, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_entry_out));
}

int virtiofs_fuse_getattr(struct virtiofs_fuse_dev *fdev,
			  uint64_t node, uint64_t fh, uint32_t getattr_flags,
			  union virtiofs_fuse_attr_outp out,
			  struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_getattr_in in = {
		.fh = fh,
		.getattr_flags = getattr_flags
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_GETATTR, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_attr_out));
}

int virtiofs_fuse_setattr(struct virtiofs_fuse_dev *fdev, uint64_t node,
			  const struct fuse_setattr_in *in,
			  union virtiofs_fuse_attr_outp out,
			  struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_SETATTR, sizeof(*in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_attr_out));
}

ssize_t virtiofs_fuse_readlink(struct virtiofs_fuse_dev *fdev, uint64_t node,
			       char *buf, size_t len)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ &ohdr, sizeof(ohdr) },
		{ buf, len }
	};
	ssize_t rlen;
	int err;

	init_hdr(fdev, &hdr, FUSE_READLINK, 0);
	rlen = uk_virtiofs_request(fdev->dev, iov, 1, 2);
	if (unlikely((err = check_reply(rlen, &ohdr))))
		return err;
	return payload_len(rlen);
}

int virtiofs_fuse_symlink(struct virtiofs_fuse_dev *fdev, uint64_t node,
			  const char *name, size_t namelen,
			  const char *target, size_t targlen,
			  union virtiofs_fuse_entry_outp out,
			  struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)name, namelen },
		{ (void *)&NULCHR, 1 },
		{ (void *)target, targlen },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum3(&paylen, namelen, targlen, 2)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_SYMLINK, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 5, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_entry_out));
}

int virtiofs_fuse_mknod(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const char *name, size_t len,
			const struct fuse_mknod_in *in,
			union virtiofs_fuse_entry_outp out,
			struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ (void *)name, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, sizeof(*in) + 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_MKNOD, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 4, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_entry_out));
}

int virtiofs_fuse_mkdir(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const char *name, size_t len,
			const struct fuse_mkdir_in *in,
			union virtiofs_fuse_entry_outp out,
			struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ (void *)name, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, sizeof(*in) + 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_MKDIR, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 4, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_entry_out));
}

int virtiofs_fuse_unlink(struct virtiofs_fuse_dev *fdev, uint64_t node,
			 const char *name, size_t len)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)name, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) }
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_UNLINK, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 3, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_rmdir(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const char *name, size_t len)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)name, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) }
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_RMDIR, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 3, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_rename(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, const char *name, size_t nlen,
			 uint64_t dest, const char *dname, size_t dlen)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_rename_in in = {
		.newdir = dest
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ (void *)name, nlen },
		{ (void *)&NULCHR, 1 },
		{ (void *)dname, dlen },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) }
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum3(&paylen, nlen, dlen, sizeof(in) + 2)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_RENAME, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 6, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_link(struct virtiofs_fuse_dev *fdev, uint64_t node,
		       uint64_t target, const char *name, size_t len,
		       union virtiofs_fuse_entry_outp out,
		       struct fuse_attr *out_attr)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_link_in in = {
		.oldnodeid = target
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ (void *)name, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_attr),
		OUTX_IOV(out_attr)
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, sizeof(in) + 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_LINK, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 4, 2 + !!out_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_entry_out));
}

int virtiofs_fuse_open(struct virtiofs_fuse_dev *fdev, uint64_t node,
		       uint32_t flags, struct fuse_open_out *out)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_open_in in = {
		.flags = flags
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) },
		{ out, sizeof(*out) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_OPEN, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2);
	return check_reply_payload(rlen, &ohdr, sizeof(*out));
}

ssize_t virtiofs_fuse_read(struct virtiofs_fuse_dev *fdev, uint64_t node,
			   const struct fuse_read_in *in,
			   const struct iovec *read_iov, size_t iovcnt)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec hdr_iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ &ohdr, sizeof(ohdr) }
	};
	const struct uk_virtiofs_iovecs iovecs[] = {
		{ hdr_iov, 3 },
		{ (const struct uk_virtiofs_iovec *)read_iov, iovcnt }
	};
	ssize_t rlen;
	int err;

	init_hdr(fdev, &hdr, FUSE_READ, sizeof(*in));
	rlen = uk_virtiofs_request_vec(fdev->dev, iovecs, 2, 2, iovcnt + 1);
	if (unlikely((err = check_reply(rlen, &ohdr))))
		return err;
	return payload_len(rlen);
}

ssize_t virtiofs_fuse_write(struct virtiofs_fuse_dev *fdev, uint64_t node,
			    const struct fuse_write_in *in,
			    const struct iovec *write_iov, size_t iovcnt)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_write_out out;
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec ihdr_iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
	};
	const struct uk_virtiofs_iovec ohdr_iov[] = {
		{ &ohdr, sizeof(ohdr) },
		{ &out, sizeof(out) }
	};
	const struct uk_virtiofs_iovecs iovecs[] = {
		{ ihdr_iov, 2 },
		{ (const struct uk_virtiofs_iovec *)write_iov, iovcnt },
		{ ohdr_iov, 2 }
	};
	size_t paylen;
	ssize_t rlen;
	int err;

	if (unlikely(!payload_sum(&paylen, sizeof(*in), in->size)))
		return -EDOM;

	init_hdr(fdev, &hdr, FUSE_WRITE, paylen);
	rlen = uk_virtiofs_request_vec(fdev->dev, iovecs, 3, 2 + iovcnt, 2);
	if (unlikely((err = check_reply_payload(rlen, &ohdr, sizeof(out)))))
		return err;
	return out.size;
}

int virtiofs_fuse_statfs(struct virtiofs_fuse_dev *fdev,
			 struct fuse_statfs_out *out)
{
	struct fuse_in_header hdr = {
		.nodeid = FUSE_ROOT_ID
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ &ohdr, sizeof(ohdr) },
		{ out, sizeof(out) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_STATFS, 0);
	rlen = uk_virtiofs_request(fdev->dev, iov, 1, 2);
	return check_reply_payload(rlen, &ohdr, sizeof(*out));
}

int virtiofs_fuse_release(struct virtiofs_fuse_dev *fdev,
			  uint64_t node, uint64_t fh)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_release_in in = {
		.fh = fh
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_RELEASE, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_fsync(struct virtiofs_fuse_dev *fdev,
			uint64_t node, uint64_t fh, uint32_t fsync_flags)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_fsync_in in = {
		.fh = fh,
		.fsync_flags = fsync_flags
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_FSYNC, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

/* No xattr support */

int virtiofs_fuse_flush(struct virtiofs_fuse_dev *fdev,
			uint64_t node, uint64_t fh)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_flush_in in = {
		.fh = fh
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_FLUSH, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

ssize_t virtiofs_fuse_readdir(struct virtiofs_fuse_dev *fdev, uint64_t node,
			      uint64_t fh, uint64_t off,
			      void *buf, uint64_t len)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_read_in in = {
		.fh = fh,
		.offset = off,
		.size = len
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) },
		{ buf, len }
	};
	ssize_t rlen;
	int err;

	init_hdr(fdev, &hdr, FUSE_READDIR, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2);
	if (unlikely((err = check_reply(rlen, &ohdr))))
		return err;
	return payload_len(rlen);
}

/* No lock ops support */

int virtiofs_fuse_access(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, uint32_t mask)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_access_in in = {
		.mask = mask
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_ACCESS, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_create(struct virtiofs_fuse_dev *fdev,
			 uint64_t node, const char *name, size_t len,
			 const struct fuse_create_in *in,
			 union virtiofs_fuse_entry_outp entry,
			 struct fuse_attr *entry_attr,
			 struct fuse_open_out *out)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ (void *)name, len },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(entry, entry_attr),
		(entry_attr ?
		 (struct uk_virtiofs_iovec){ entry_attr, sizeof(*entry_attr) } :
		 (struct uk_virtiofs_iovec){ out, sizeof(*out) }
		),
		(entry_attr ?
		 (struct uk_virtiofs_iovec){ out, sizeof(*out) } :
		 (struct uk_virtiofs_iovec){ NULL, 0 }
		)
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum(&paylen, len, sizeof(*in) + 1)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_CREATE, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 4, 3 + !!entry_attr);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_entry_out) +
						sizeof(*out));
}

/* No bmap support */

/* No ioctl support */

/* No poll/notify_reply */

int virtiofs_fuse_fallocate(struct virtiofs_fuse_dev *fdev,
			    uint64_t node, uint64_t fh,
			    uint64_t offset, uint64_t len, uint32_t mode)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_fallocate_in in = {
		.fh = fh,
		.offset = offset,
		.length = len,
		.mode = mode
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_FALLOCATE, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

/* No readdirplus */

int virtiofs_fuse_rename2(struct virtiofs_fuse_dev *fdev,
			  uint64_t node, const char *name, size_t nlen,
			  uint64_t dest, const char *dname, size_t dlen,
			  uint32_t flags)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	const struct fuse_rename2_in in = {
		.newdir = dest,
		.flags = flags
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ (void *)name, nlen },
		{ (void *)&NULCHR, 1 },
		{ (void *)dname, dlen },
		{ (void *)&NULCHR, 1 },
		{ &ohdr, sizeof(ohdr) }
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sum3(&paylen, nlen, dlen, sizeof(in) + 2)))
		return -ENAMETOOLONG;

	init_hdr(fdev, &hdr, FUSE_RENAME2, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 6, 1);
	return check_reply(rlen, &ohdr);
}

/* No lseek (we keep our own file position) */

ssize_t virtiofs_fuse_copy_file_range(struct virtiofs_fuse_dev *fdev,
				      uint64_t node,
				      const struct fuse_copy_file_range_in *in)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	struct fuse_write_out out;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ &ohdr, sizeof(ohdr) },
		{ &out, sizeof(out) }
	};
	ssize_t rlen;
	int err;

	init_hdr(fdev, &hdr, FUSE_COPY_FILE_RANGE, sizeof(*in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2);
	if (unlikely((err = check_reply_payload(rlen, &ohdr, sizeof(out)))))
		return err;
	return out.size;
}

int virtiofs_fuse_setupmapping(struct virtiofs_fuse_dev *fdev, uint64_t node,
			       const struct fuse_setupmapping_in *in)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_SETUPMAPPING, sizeof(*in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_removemapping(struct virtiofs_fuse_dev *fdev,
				const struct fuse_removemapping_one *arg,
				uint32_t count)
{
	struct fuse_in_header hdr;
	const struct fuse_removemapping_in in = {
		.count = count
	};
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ (void *)arg, count * sizeof(*arg) },
		{ &ohdr, sizeof(ohdr) }
	};
	size_t paylen;
	ssize_t rlen;

	if (unlikely(!payload_sumv(&paylen, sizeof(in), count, sizeof(*arg))))
		return -EDOM;

	init_hdr(fdev, &hdr, FUSE_REMOVEMAPPING, paylen);
	rlen = uk_virtiofs_request(fdev->dev, iov, 3, 1);
	return check_reply(rlen, &ohdr);
}

int virtiofs_fuse_syncfs(struct virtiofs_fuse_dev *fdev)
{
	struct fuse_in_header hdr = {
		.nodeid = FUSE_ROOT_ID
	};
	const struct fuse_syncfs_in in = {}; /* All padding */
	struct fuse_out_header ohdr;
	const struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)&in, sizeof(in) },
		{ &ohdr, sizeof(ohdr) }
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_SYNCFS, sizeof(in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 1);
	return check_reply(rlen, &ohdr);
}

/* No tmpfile support */

int virtiofs_fuse_statx(struct virtiofs_fuse_dev *fdev, uint64_t node,
			const struct fuse_statx_in *in,
			union virtiofs_fuse_statx_outp out,
			struct fuse_statx *out_statx)
{
	struct fuse_in_header hdr = {
		.nodeid = node
	};
	struct fuse_out_header ohdr;
	struct uk_virtiofs_iovec iov[] = {
		{ &hdr, sizeof(hdr) },
		{ (void *)in, sizeof(*in) },
		{ &ohdr, sizeof(ohdr) },
		OUT_IOV(out, out_statx),
		OUTX_IOV(out_statx)
	};
	ssize_t rlen;

	init_hdr(fdev, &hdr, FUSE_STATX, sizeof(*in));
	rlen = uk_virtiofs_request(fdev->dev, iov, 2, 2 + !!out_statx);
	return check_reply_payload(rlen, &ohdr, sizeof(struct fuse_statx_out));
}

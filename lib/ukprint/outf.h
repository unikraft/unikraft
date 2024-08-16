/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_OUTF_H__
#define __UK_PRINT_OUTF_H__

#include <uk/config.h>
#include <inttypes.h>
#include <stdio.h>
#include <uk/libid.h>

enum out_dev_type {
	OUTDEV_FILE = 0,
	OUTDEV_BUFFER,
#if CONFIG_LIBUKPRINT_PRINTK
	OUTDEV_KERN,
#endif
};

struct out_dev {
	enum out_dev_type type;

	union {
		/* OUTDEV_KERN */
		struct {
			int flags; /* OUTDEV_KERN only */
			__u16 libid;
			const char *srcname;
			unsigned int srcline;
		} uk_pr;

		/* UK_HXDOUT_FILE */
		struct {
			FILE *fp;
		} file;

		/* UK_HXDOUT_BUFFER */
		struct {
			char *pos;
			size_t left;
		} buffer;
	};
};

/**
 * Sends a formatted string to a given output device
 */
int outf(struct out_dev *dev, const char *fmt, ...);

#define out_dev_init_file(dev, fp)				\
	do {							\
		(dev)->type          = OUTDEV_FILE;		\
		(dev)->file.fp       = (fp);			\
	} while (0)

#define out_dev_init_buffer(dev, addr, len)			\
	do {							\
		(dev)->type          = OUTDEV_BUFFER;		\
		(dev)->buffer.pos    = (addr);			\
		(dev)->buffer.left   = (len);			\
	} while (0)

#if CONFIG_LIBUKPRINT_PRINTK
#define out_dev_init_kern(dev, flgs, libid, srcname, srcline)	\
	do {							\
		(dev)->type          = OUTDEV_KERN;		\
		(dev)->uk_pr.flags   = (flgs);			\
		(dev)->uk_pr.libid   = (libid);			\
		(dev)->uk_pr.srcname = (srcname);		\
		(dev)->uk_pr.srcline = (srcline);		\
	} while (0)
#endif

#endif /* __UK_PRINT_OUTF_H__ */

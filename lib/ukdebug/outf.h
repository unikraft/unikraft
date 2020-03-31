/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Internal helper for text output redirection
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UKDEBUG_INTERNAL_OUTF_H__
#define __UKDEBUG_INTERNAL_OUTF_H__

#include <uk/config.h>
#include <inttypes.h>
#include <stdio.h>

enum out_dev_type {
	OUTDEV_FILE = 0,
	OUTDEV_BUFFER,
#if CONFIG_LIBUKDEBUG_PRINTK
	OUTDEV_KERN,
#endif
	OUTDEV_DEBUG,
};

struct out_dev {
	enum out_dev_type type;

	union {
		/* OUTDEV_KERN, OUTDEV_DEBUG */
		struct {
			int lvl; /* OUTDEV_KERN only */
			const char *libname;
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

#if CONFIG_LIBUKDEBUG_PRINTK
#define out_dev_init_kern(dev, lvl, libname, srcname, srcline)	\
	do {							\
		(dev)->type          = OUTDEV_KERN;		\
		(dev)->uk_pr.lvl     = (lvl);			\
		(dev)->uk_pr.libname = (libname);		\
		(dev)->uk_pr.srcname = (srcname);		\
		(dev)->uk_pr.srcline = (srcline);		\
	} while (0)
#endif

#define out_dev_init_debug(dev, libname, srcname, srcline)	\
	do {							\
		(dev)->type          = OUTDEV_DEBUG;		\
		(dev)->uk_pr.libname = (libname);		\
		(dev)->uk_pr.srcname = (srcname);		\
		(dev)->uk_pr.srcline = (srcline);		\
	} while (0)

#endif /* __UKDEBUG_INTERNAL_OUTF_H__ */

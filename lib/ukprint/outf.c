/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "outf.h"

#include <string.h>
#include <stdarg.h>
#include <uk/assert.h>
#include "snprintf.h"

int outf(struct out_dev *dev, const char *fmt, ...)
{
	int ret = 0;
	size_t rem;
	va_list ap;

	UK_ASSERT(dev);

	va_start(ap, fmt);
	switch (dev->type) {
	case OUTDEV_FILE:
		/* Use standard libc approach when printing to a file */
		ret = vfprintf(dev->file.fp, fmt, ap);
		break;
	case OUTDEV_BUFFER:
		ret = uk_vsnprintf(dev->buffer.pos, dev->buffer.left, fmt, ap);
		if (ret > 0) {
			rem = MIN(dev->buffer.left, (size_t)ret);
			dev->buffer.pos  += rem;
			dev->buffer.left -= rem;
		}
		break;
#if CONFIG_LIBUKPRINT_PRINTK
	case OUTDEV_KERN:
		_uk_vprintk(dev->uk_pr.flags, dev->uk_pr.libid,
			    dev->uk_pr.srcname, dev->uk_pr.srcline,
			    fmt, ap);
		break;
#endif
	default:
		break;
	}
	va_end(ap);

	return ret;
}

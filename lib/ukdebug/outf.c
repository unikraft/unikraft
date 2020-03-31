/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Internal helper for text output redirection
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include "outf.h"

#include <string.h>
#include <stdarg.h>
#include <uk/assert.h>
#include "snprintf.h"

int outf(struct out_dev *dev, const char *fmt, ...)
{
	int ret = 0;
	va_list ap;

	UK_ASSERT(dev);

	va_start(ap, fmt);
	switch (dev->type) {
	case OUTDEV_FILE:
		/* Use standard libc approach when printing to a file */
		ret = vfprintf(dev->file.fp, fmt, ap);
		break;
	case OUTDEV_BUFFER:
		ret = __uk_vsnprintf(dev->buffer.pos, dev->buffer.left, fmt, ap);

		if (ret > 0) {
			/* in order to overwrite '\0' by successive calls,
			 * we move the buffer pointer by (ret-1) characters
			 */
			dev->buffer.pos  += (ret - 1);
			dev->buffer.left -= (ret - 1);
		}
		break;
	case OUTDEV_DEBUG:
		_uk_vprintd(dev->uk_pr.libname,
			    dev->uk_pr.srcname, dev->uk_pr.srcline,
			    fmt, ap);
		break;
#if CONFIG_LIBUKDEBUG_PRINTK
	case OUTDEV_KERN:
		_uk_vprintk(dev->uk_pr.lvl, dev->uk_pr.libname,
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

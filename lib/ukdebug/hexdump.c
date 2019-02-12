/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Hexdump-like routines
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/hexdump.h>

#define UK_HXDF_GRPFLAGS                                                       \
	(UK_HXDF_GRPBYTE | UK_HXDF_GRPWORD | UK_HXDF_GRPDWORD                  \
	 | UK_HXDF_GRPQWORD)

enum _hxd_output_type {
	UK_HXDOUT_FILE = 0,
	UK_HXDOUT_BUFFER,
#if CONFIG_LIBUKDEBUG_PRINTK
	UK_HXDOUT_KERN,
#endif
	UK_HXDOUT_DEBUG,
};

struct _hxd_output {
	enum _hxd_output_type type;

	union {
		/* UK_HXDOUT_KERN, UK_HXDOUT_DEBUG */
		struct {
			int lvl; /* UK_HXDOUT_KERN only */
			const char *libname;
			const char *srcname;
			unsigned int srcline;
		} ukprint;

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
 * Send a formatted string to an output device
 */
static int _hxd_outf(struct _hxd_output *o, const char *fmt, ...)
{
	int ret = 0;
	va_list ap;

	va_start(ap, fmt);
	switch (o->type) {
	case UK_HXDOUT_FILE:
		ret = vfprintf(o->file.fp, fmt, ap);
		break;
	case UK_HXDOUT_BUFFER:
		ret = vsnprintf(o->buffer.pos, o->buffer.left, fmt, ap);

		if (ret > 0) {
			/* in order to overwrite '\0' by successive calls,
			 * we move the buffer pointer by (ret-1) characters
			 */
			o->buffer.pos += (ret - 1);
			o->buffer.left -= (ret - 1);
		}
		break;
	case UK_HXDOUT_DEBUG:
		_uk_vprintd(o->ukprint.libname,
			    o->ukprint.srcname, o->ukprint.srcline,
			    fmt, ap);
		break;
#if CONFIG_LIBUKDEBUG_PRINTK
	case UK_HXDOUT_KERN:
		_uk_vprintk(o->ukprint.lvl, o->ukprint.libname,
			    o->ukprint.srcname, o->ukprint.srcline,
			    fmt, ap);
		break;
#endif
	default:
		break;
	}
	va_end(ap);

	return ret;
}

/**
 * Plot one hexdump data line
 * This function is called by _hxd()
 */
static inline size_t _hxd_line(struct _hxd_output *o, const unsigned char *data,
			       size_t len, size_t linelen, int flags)
{
	size_t i, grplen = 0;
	int iret, ret = 0;
	char c;

	if (flags & UK_HXDF_GRPBYTE)
		grplen = 1;
	else if (flags & UK_HXDF_GRPWORD)
		grplen = 2;
	else if (flags & UK_HXDF_GRPDWORD)
		grplen = 4;
	else if (flags & UK_HXDF_GRPQWORD)
		grplen = 8;

	/* hex section */
	for (i = 0; i < len; ++i) {
		c = *(data + i);
		iret = _hxd_outf(o, "%02x ", (unsigned char)c);
		if (iret < 0)
			return iret;
		ret += iret;

		if (i && grplen && ((i + 1) % grplen == 0)) {
			iret = _hxd_outf(o, " ");
			if (iret < 0)
				return iret;
			ret += iret;
		}
	}

	/* ascii section */
	if (flags & UK_HXDF_ASCIISEC) {
		/* fillup to align ascii section */
		for (; i < linelen; ++i) {
			iret = _hxd_outf(o, "   ");
			if (iret < 0)
				return iret;
			ret += iret;

			if (i && grplen && ((i + 1) % grplen == 0)) {
				iret = _hxd_outf(o, " ");
				if (iret < 0)
					return iret;
				ret += iret;
			}
		}
		if (!grplen) {
			iret = _hxd_outf(o, " ");
			if (iret < 0)
				return iret;
			ret += iret;
		}

		/* print ascii characters */
		iret = _hxd_outf(o, "|");
		if (iret < 0)
			return iret;
		ret += iret;
		for (i = 0; i < len; ++i) {
			c = *(data + i);
			iret = _hxd_outf(o, "%c",
					 (c >= ' ' && c <= '~') ? c : '.');
			if (iret < 0)
				return iret;
			ret += iret;
		}
		iret = _hxd_outf(o, "|");
		if (iret < 0)
			return iret;
		ret += iret;
	}

	iret = _hxd_outf(o, "\n");
	if (iret < 0)
		return iret;
	ret += iret;
	return ret;
}

/**
 * Plots an hexdump for a given data region
 *
 * @param o Output definition
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param addr0 Address offset to be added to address plot (see UK_HXDF_ADDR),
 *        because otherwise the bytes are counted from 0 onwards
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @param line_prefix String to be prepended to each line, can be NULL
 * @return Returns the number of printed characters to output o
 */
static int _hxd(struct _hxd_output *o, const void *data, size_t len,
		size_t addr0, int flags, unsigned int grps_per_line,
		const char *line_prefix)
{
	size_t i, linebytes, rembytes, linelen;
	int iret, ret = 0;
	int prevc = 0;

	UK_ASSERT(grps_per_line >= 1);

	/* ensure that at most only one grouping flag is enabled */
	UK_ASSERT(((flags & UK_HXDF_GRPFLAGS) == 0)
		  || ((flags & UK_HXDF_GRPFLAGS) == UK_HXDF_GRPBYTE)
		  || ((flags & UK_HXDF_GRPFLAGS) == UK_HXDF_GRPWORD)
		  || ((flags & UK_HXDF_GRPFLAGS) == UK_HXDF_GRPDWORD)
		  || ((flags & UK_HXDF_GRPFLAGS) == UK_HXDF_GRPQWORD));

	rembytes = len;
	linelen = grps_per_line;

	if (flags & UK_HXDF_GRPWORD)
		linelen *= 2;
	else if (flags & UK_HXDF_GRPDWORD)
		linelen *= 4;
	else if (flags & UK_HXDF_GRPQWORD)
		linelen *= 8;

	for (i = 0; i < len; i += linelen) {
		linebytes = MIN(rembytes, linelen);
		rembytes -= linebytes;

		if (i && (flags & UK_HXDF_COMPRESS)) {
			/* do a memcmp with previous linebytes and
			 * skip printing when line is equal
			 */
			if (memcmp(((const unsigned char *)data) + i - linelen,
				   ((const unsigned char *)data) + i, linebytes)
			    == 0) {
				if (!prevc) {
					iret = _hxd_outf(o, "*\n");
					if (iret < 0)
						return iret;
					ret += iret;
					prevc = 1;
				}
				continue;
			}
		}
		prevc = 0;

		if (line_prefix) {
			iret = _hxd_outf(o, "%s", line_prefix);
			if (iret < 0)
				return iret;
			ret += iret;
		}

		if (flags & UK_HXDF_ADDR) {
			iret = _hxd_outf(o, "%08"__PRIuptr
					    "  ",
					 (__uptr)(i + addr0));
			if (iret < 0)
				return iret;
			ret += iret;
		}

		/* data */
		iret = _hxd_line(o, ((const unsigned char *)data) + i,
				 linebytes, linelen, flags);
		if (iret < 0)
			return iret;
		ret += iret;
	}

	return ret;
}

int uk_hexdumpsn(char *str, size_t size, const void *data, size_t len,
		 size_t addr0, int flags, unsigned int grps_per_line,
		 const char *line_prefix)
{
	struct _hxd_output o = {.type = UK_HXDOUT_BUFFER,
				.buffer.pos = str,
				.buffer.left = size};
	UK_ASSERT(str != NULL);

	return _hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}

int uk_hexdumpf(FILE *fp, const void *data, size_t len, size_t addr0, int flags,
		unsigned int grps_per_line, const char *line_prefix)
{
	struct _hxd_output o = {.type = UK_HXDOUT_FILE,
				.file.fp = fp};
	UK_ASSERT(fp != NULL);

	return _hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}

void _uk_hexdumpd(const char *libname, const char *srcname,
		  unsigned int srcline, const void *data, size_t len,
		  size_t addr0, int flags, unsigned int grps_per_line,
		  const char *line_prefix)
{
	struct _hxd_output o = {.type = UK_HXDOUT_DEBUG,
				.ukprint.libname = libname,
				.ukprint.srcname = srcname,
				.ukprint.srcline = srcline};

	_hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}

#if CONFIG_LIBUKDEBUG_PRINTK
void _uk_hexdumpk(int lvl, const char *libname, const char *srcname,
		  unsigned int srcline, const void *data, size_t len,
		  size_t addr0, int flags, unsigned int grps_per_line,
		  const char *line_prefix)
{
	struct _hxd_output o = {.type = UK_HXDOUT_KERN,
				.ukprint.lvl = lvl,
				.ukprint.libname = libname,
				.ukprint.srcname = srcname,
				.ukprint.srcline = srcline};

	_hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}
#endif

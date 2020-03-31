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
#include "outf.h"

#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/hexdump.h>

#define UK_HXDF_GRPFLAGS                                                       \
	(UK_HXDF_GRPBYTE | UK_HXDF_GRPWORD | UK_HXDF_GRPDWORD                  \
	 | UK_HXDF_GRPQWORD)

/**
 * Plot one hexdump data line
 * This function is called by _hxd()
 */
static inline size_t _hxd_line(struct out_dev *o, const unsigned char *data,
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
		iret = outf(o, "%02x ", (unsigned char)c);
		if (iret < 0)
			return iret;
		ret += iret;

		if (i && grplen && ((i + 1) % grplen == 0)) {
			iret = outf(o, " ");
			if (iret < 0)
				return iret;
			ret += iret;
		}
	}

	/* ascii section */
	if (flags & UK_HXDF_ASCIISEC) {
		/* fillup to align ascii section */
		for (; i < linelen; ++i) {
			iret = outf(o, "   ");
			if (iret < 0)
				return iret;
			ret += iret;

			if (i && grplen && ((i + 1) % grplen == 0)) {
				iret = outf(o, " ");
				if (iret < 0)
					return iret;
				ret += iret;
			}
		}
		if (!grplen) {
			iret = outf(o, " ");
			if (iret < 0)
				return iret;
			ret += iret;
		}

		/* print ascii characters */
		iret = outf(o, "|");
		if (iret < 0)
			return iret;
		ret += iret;
		for (i = 0; i < len; ++i) {
			c = *(data + i);
			iret = outf(o, "%c",
				    (c >= ' ' && c <= '~') ? c : '.');
			if (iret < 0)
				return iret;
			ret += iret;
		}
		iret = outf(o, "|");
		if (iret < 0)
			return iret;
		ret += iret;
	}

	iret = outf(o, "\n");
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
static int _hxd(struct out_dev *o, const void *data, size_t len,
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
					iret = outf(o, "*\n");
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
			iret = outf(o, "%s", line_prefix);
			if (iret < 0)
				return iret;
			ret += iret;
		}

		if (flags & UK_HXDF_ADDR) {
			iret = outf(o, "%08"__PRIuptr"  ",
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
	struct out_dev o;

	UK_ASSERT(str != NULL);
	out_dev_init_buffer(&o, str, size);
	return _hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}

int uk_hexdumpf(FILE *fp, const void *data, size_t len, size_t addr0, int flags,
		unsigned int grps_per_line, const char *line_prefix)
{
	struct out_dev o;

	UK_ASSERT(fp != NULL);
	out_dev_init_file(&o, fp);
	return _hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}

void _uk_hexdumpd(const char *libname, const char *srcname,
		  unsigned int srcline, const void *data, size_t len,
		  size_t addr0, int flags, unsigned int grps_per_line,
		  const char *line_prefix)
{
	struct out_dev o;

	out_dev_init_debug(&o, libname, srcname, srcline);
	_hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}

#if CONFIG_LIBUKDEBUG_PRINTK
void _uk_hexdumpk(int lvl, const char *libname, const char *srcname,
		  unsigned int srcline, const void *data, size_t len,
		  size_t addr0, int flags, unsigned int grps_per_line,
		  const char *line_prefix)
{
	struct out_dev o;

	out_dev_init_kern(&o, lvl, libname, srcname, srcline);
	_hxd(&o, data, len, addr0, flags, grps_per_line, line_prefix);
}
#endif

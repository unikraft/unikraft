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

#ifndef __UKDEBUG_HEXDUMP_H__
#define __UKDEBUG_HEXDUMP_H__

#include <stdio.h>
#include <uk/print.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_HXDF_ADDR (1)     /* show address column */
#define UK_HXDF_ASCIISEC (2) /* show ascii section */

#define UK_HXDF_GRPBYTE (4)   /* group bytes */
#define UK_HXDF_GRPWORD (8)   /* group 2 bytes */
#define UK_HXDF_GRPDWORD (16) /* group 4 bytes */
#define UK_HXDF_GRPQWORD (32) /* group 8 bytes */

#define UK_HXDF_COMPRESS (64) /* suppress repeated lines */

/*
 * HEXDUMP ON DEBUG CONSOLE
 */
#ifdef __IN_LIBUKDEBUG__
/*
 * This redefinition of CONFIG_LIBUKDEBUG_PRINTD is doing the trick to
 * switch on the correct declaration of uk_hexdumpd() when we are compiling
 * this library and have the global debug switch CONFIG_LIBUKDEBUG_PRINTD
 * not enabled.
 */
#if !defined CONFIG_LIBUKDEBUG_PRINTD || !CONFIG_LIBUKDEBUG_PRINTD
#undef CONFIG_LIBUKDEBUG_PRINTD
#define CONFIG_LIBUKDEBUG_PRINTD 1
#endif
#endif /* __IN_LIBUKDEBUG__ */

#if (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD
/* Please use uk_hexdumpd() instead */
void _uk_hexdumpd(const char *libname, const char *srcname,
		  unsigned int srcline, const void *data, size_t len,
		  size_t addr0, int flags, unsigned int grps_per_line,
		  const char *line_prefix);

/**
 * Plots an hexdump for a given data region to kernel output
 * The absolute address is plotted when UK_HXDF_ADDR is set
 *
 * @param lvl Debug level
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @return Returns the number of printed characters to output fp
 */
#define uk_hexdumpd(data, len, flags, grps_per_line)			\
	_uk_hexdumpd(__STR_LIBNAME__, __STR_BASENAME__,			\
		     __LINE__, (data), (len),				\
		     ((size_t)(data)), (flags),				\
		     (grps_per_line), STRINGIFY(data) ": ")
#else /* (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD */
static inline void uk_hexdumpd(const void *data __unused, size_t len __unused,
			       int flags __unused,
			       unsigned int grps_per_line __unused)
{}
#endif /* (defined UK_DEBUG) || CONFIG_LIBUKDEBUG_PRINTD */

#if CONFIG_LIBUKDEBUG_PRINTK
/* Please use uk_hexdumpk() instead */
void _uk_hexdumpk(int lvl, const char *libname, const char *srcname,
		  unsigned int srcline, const void *data, size_t len,
		  size_t addr0, int flags, unsigned int grps_per_line,
		  const char *line_prefix);

/**
 * Plots an hexdump for a given data region to kernel output
 * The absolute address is plotted when UK_HXDF_ADDR is set
 *
 * @param lvl Debug level
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @return Returns the number of printed characters to output fp
 */
#define uk_hexdumpk(lvl, data, len, flags, grps_per_line)                      \
	do {                                                                   \
		if ((lvl) <= KLVL_MAX)                                         \
			_uk_hexdumpk((lvl), __STR_LIBNAME__, __STR_BASENAME__, \
				     __LINE__, (data), (len),                  \
				     ((size_t)(data)), (flags),                \
				     (grps_per_line), STRINGIFY(data) ": ");   \
	} while (0)
#else /* CONFIG_LIBUKDEBUG_PRINTK */
static inline void uk_hexdumpk(int lvl __unused, const void *data __unused,
			       size_t len __unused, int flags __unused,
			       unsigned int grps_per_line __unused)
{}
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

/**
 * Plots an hexdump for a given data region to a file descriptor
 *
 * @param fp File descriptor for output
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param addr0 Address offset to be added to address plot (see UK_HXDF_ADDR),
 *        because otherwise the bytes are counted from 0 onwards
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @param line_prefix String to be prepended to each line, can be NULL
 * @return Returns the number of printed characters to output fp
 */
int uk_hexdumpf(FILE *fp, const void *data, size_t len, size_t addr0, int flags,
		unsigned int grps_per_line, const char *line_prefix);

/**
 * Plots an hexdump for a given data region to a string buffer
 *
 * @param str Buffer for output string
 * @param size Size of buffer str
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param addr0 Address offset to be added to address plot (see UK_HXDF_ADDR),
 *        because otherwise the bytes are counted from 0 onwards
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @param line_prefix String to be prepended to each line, can be NULL
 * @return Returns the number of printed characters to output str
 */
int uk_hexdumpsn(char *str, size_t size, const void *data, size_t len,
		 size_t addr0, int flags, unsigned int grps_per_line,
		 const char *line_prefix);

/**
 * Plots an hexdump for a given data region to a string buffer of unlimited size
 *
 * @param str Buffer for output string
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param addr0 Address offset to be added to address plot (see UK_HXDF_ADDR),
 *        because otherwise the bytes are counted from 0 onwards
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @param line_prefix String to be prepended to each line, can be NULL
 * @return Returns the number of printed characters to output str
 */
#define uk_hexdumps(str, data, len, addr0, flags, grps_per_line, line_prefix)  \
	uk_hexdumpsn((str), SIZE_MAX, (data), (len), (addr0), (flags),         \
		     (grps_per_line), (line_prefix))

/**
 * Plots an hexdump for a given data region to stdout
 *
 * @param data Start of data region to plot
 * @param len Length of data region (number of bytes)
 * @param addr0 Address offset to be added to address plot (see UK_HXDF_ADDR),
 *        because otherwise the bytes are counted from 0 onwards
 * @param flags Format flags, see UK_HXDF_*
 * @param grps_per_line Defines the number of bytes shown per line:
 *        Number of groups (UK_HXDF_GRP*) shown per line
 * @param line_prefix String to be prepended to each line, can be NULL
 * @return Returns the number of printed characters to output str
 */
#define uk_hexdump(data, len, addr0, flags, grps_per_line, line_prefix)        \
	uk_hexdumpf(stdout, (data), (len), (addr0), (flags), (grps_per_line),  \
		    (line_prefix))

/**
 * Shortcuts for all hexdump variants ahead. The shortcuts use a similar style
 * as the hexdump Unix command using -C parameter: hexdump -C
 */
#define uk_hexdumpCd(data, len)                                                \
	uk_hexdumpd((data), (len), (UK_HXDF_ADDR | UK_HXDF_ASCIISEC            \
				    | UK_HXDF_GRPQWORD | UK_HXDF_COMPRESS),    \
		    2)

#define uk_hexdumpCk(lvl, data, len)                                           \
	uk_hexdumpk((lvl), (data), (len),                                      \
		    (UK_HXDF_ADDR | UK_HXDF_ASCIISEC | UK_HXDF_GRPQWORD        \
		     | UK_HXDF_COMPRESS),                                      \
		    2)

#define uk_hexdumpCf(fp, data, len)                                            \
	uk_hexdumpf((fp), (data), (len), ((size_t)(data)),                     \
		    (UK_HXDF_ADDR | UK_HXDF_ASCIISEC | UK_HXDF_GRPQWORD        \
		     | UK_HXDF_COMPRESS),                                      \
		    2, NULL)

#define uk_hexdumpCsn(str, size, data, len)                                    \
	uk_hexdumpsn((str), (size), (data), (len), ((size_t)(data)),           \
		     (UK_HXDF_ADDR | UK_HXDF_ASCIISEC | UK_HXDF_GRPQWORD       \
		      | UK_HXDF_COMPRESS),                                     \
		     2, NULL)

#define uk_hexdumpCs(str, data, len)                                           \
	uk_hexdumps((str), (data), (len), ((size_t)(data)),                    \
		    (UK_HXDF_ADDR | UK_HXDF_ASCIISEC | UK_HXDF_GRPQWORD        \
		     | UK_HXDF_COMPRESS),                                      \
		    2, NULL)

#define uk_hexdumpC(data, len)                                                 \
	uk_hexdump((data), (len), ((size_t)(data)),			       \
		    (UK_HXDF_ADDR | UK_HXDF_ASCIISEC | UK_HXDF_GRPQWORD        \
		     | UK_HXDF_COMPRESS),                                      \
		    2, NULL)

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_HEXDUMP_H__ */

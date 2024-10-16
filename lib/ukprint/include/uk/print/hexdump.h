/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_HEXDUMP_H__
#define __UK_PRINT_HEXDUMP_H__

#include <stdio.h>
#include <uk/arch/types.h>
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

#if CONFIG_LIBUKPRINT_PRINTK
/* Please use uk_hexdumpk() instead */
void _uk_hexdumpk(int lvl, __u16 libid, const char *srcname,
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
		if ((lvl) <= UK_PRINT_KLVL_MAX)                                \
			_uk_hexdumpk((lvl), uk_libid_self(), __STR_BASENAME__, \
				     __LINE__, (data), (len),                  \
				     ((size_t)(data)), (flags),                \
				     (grps_per_line), STRINGIFY(data) ": ");   \
	} while (0)
#else /* CONFIG_LIBUKPRINT_PRINTK */
static inline void uk_hexdumpk(int lvl __unused, const void *data __unused,
			       size_t len __unused, int flags __unused,
			       unsigned int grps_per_line __unused)
{}
#endif /* CONFIG_LIBUKPRINT_PRINTK */

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

#endif /* __UK_PRINT_HEXDUMP_H__ */

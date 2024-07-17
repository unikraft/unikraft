/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *          Simon Kuenzer <simon@unikraft.io>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH. All rights reserved.
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
#ifndef __UK_LIBPARAM_H
#define __UK_LIBPARAM_H

#include <uk/config.h>
#ifndef __ASSEMBLY__
#include <uk/ctors.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/print.h>

#ifdef __cplusplus
extern C {
#endif /* __cplusplus */
#endif /* !__ASSEMBLY__ */

/* We need to define an own version here in order to avoid (indirect) header
 * dependencies in our linker script `libparam.lds.S`. These headers are likely
 * not prepared or available in the scope of linker scripts.
 */
#define _UK_LIBPARAM_CONCAT(a, b) a ## b
#define UK_LIBPARAM_CONCAT(a, b) _UK_LIBPARAM_CONCAT(a, b)

/* Fallback to default library prefix if not set by compiler flag */
#ifndef UK_LIBPARAM_LIBPREFIX
#define UK_LIBPARAM_LIBPREFIX __LIBNAME__
#endif /* !UK_LIBPARAM_PREFIX */

/*
 * Name (prefixes) used for a per-library section that stores all references
 * to parameters (struct uk_libparam_param) available for one library. This
 * section is basically compiled into an array of pointers.
 * A library parameter descriptor (struct uk_libparam_desc) is referencing to
 * the section's base address for iteration.
 */
#define UK_LIBPARAM_PARAM_NAMEPREFIX          __uk_libparam_param_
#define UK_LIBPARAM_PDATA_NAMEPREFIX          __uk_libparam_pdata_

#define UK_LIBPARAM_PARAMSECTION_NAME       uk_libparam_params
#define UK_LIBPARAM_PARAMSECTION_STARTSYM   \
	UK_LIBPARAM_CONCAT(UK_LIBPARAM_PARAMSECTION_NAME, _start)
#define UK_LIBPARAM_PARAMSECTION_ENDSYM     \
	UK_LIBPARAM_CONCAT(UK_LIBPARAM_PARAMSECTION_NAME, _end)

#ifndef __ASSEMBLY__
#ifdef CONFIG_LIBUKLIBPARAM
/* The following symbols are provided by the per-library linker script.
 * They define the start and the end of the library parameters reference array
 */
extern struct uk_libparam_param * const UK_LIBPARAM_PARAMSECTION_STARTSYM[];
extern struct uk_libparam_param * const UK_LIBPARAM_PARAMSECTION_ENDSYM;

/*
 * Library parameter data types
 */
enum  uk_libparam_param_type {
	_UK_LIBPARAM_PARAM_TYPE___undef = 0,
	_UK_LIBPARAM_PARAM_TYPE_bool,
	_UK_LIBPARAM_PARAM_TYPE___s8,
#define _UK_LIBPARAM_PARAM_TYPE_char _UK_LIBPARAM_PARAM_TYPE___s8
	_UK_LIBPARAM_PARAM_TYPE___u8,
#define _UK_LIBPARAM_PARAM_TYPE_uchar _UK_LIBPARAM_PARAM_TYPE___u8
	_UK_LIBPARAM_PARAM_TYPE___s16,
	_UK_LIBPARAM_PARAM_TYPE___u16,
	_UK_LIBPARAM_PARAM_TYPE___s32,
#define _UK_LIBPARAM_PARAM_TYPE_int _UK_LIBPARAM_PARAM_TYPE___s32
	_UK_LIBPARAM_PARAM_TYPE___u32,
#define _UK_LIBPARAM_PARAM_TYPE_uint _UK_LIBPARAM_PARAM_TYPE___u32
	_UK_LIBPARAM_PARAM_TYPE___s64,
	_UK_LIBPARAM_PARAM_TYPE___u64,
	_UK_LIBPARAM_PARAM_TYPE___uptr,
	_UK_LIBPARAM_PARAM_TYPE_charp
};

/*
 * Library parameter descriptor
 */

struct uk_libparam_pdata { /* Non-const members of `struct uk_libparam_param` */
	/* Internally used by parser for array parameters */
	__sz __widx;
};

struct uk_libparam_param {
	/* Library prefix */
	const char * const prefix;
	const char * const name;
	const char * const desc;
	const enum uk_libparam_param_type type;
	/* Number of elements (>1 means we have an array of the given type) */
	const __sz count;
	/* Reference to corresponding variable */
	void * const addr;
	/* Non-const members */
	struct uk_libparam_pdata *pdata;
};

/* -------------------------------------------------------------------------- */
/* Parameter registration                                                     */

#define __UK_LIBPARAM_PARAM_NAME(varname)				\
	UK_LIBPARAM_CONCAT(UK_LIBPARAM_PARAM_NAMEPREFIX, varname)

#define __UK_LIBPARAM_DATA_NAME(varname)				\
	UK_LIBPARAM_CONCAT(UK_LIBPARAM_PDATA_NAMEPREFIX, varname)

#define __UK_LIBPARAM_PARAM_DEFINE(arg_var, arg_addr, arg_type, arg_count,\
				   arg_desc)				\
	static struct uk_libparam_pdata __UK_LIBPARAM_DATA_NAME(arg_var);\
	static const struct uk_libparam_param				\
	__used __section("." STRINGIFY(UK_LIBPARAM_PARAMSECTION_NAME))	\
	__align(8) __UK_LIBPARAM_PARAM_NAME(arg_var) = {		\
		.prefix = STRINGIFY(UK_LIBPARAM_LIBPREFIX),		\
		.name   = STRINGIFY(arg_var),				\
		.desc   = arg_desc,					\
		.type   = _UK_LIBPARAM_PARAM_TYPE_##arg_type,		\
		.count  = arg_count,					\
		.addr   = arg_addr,					\
		.pdata  = &__UK_LIBPARAM_DATA_NAME(arg_var)		\
	}

#define _UK_LIBPARAM_PARAM_DEFINE(name, var, type, count, desc) \
	__UK_LIBPARAM_PARAM_DEFINE(name, var, type, count, desc)

/* -------------------------------------------------------------------------- */
/* Parser                                                                     */

/*
 * Flag bits for defining parsing behavior
 */
/* Scan only, do not parse and set values */
#define UK_LIBPARAM_F_SCAN   0x1
/* Don't skip on unknown arguments, exit with a parsing error */
#define UK_LIBPARAM_F_STRICT 0x2
/* Print usage when 'help' is found as parameter and return with -EINTR */
#define UK_LIBPARAM_F_USAGE  0x4

/**
 * Parse given parameter list. The parsing mode can be defined with flags
 * (see: `UK_LIBPARAM_F_*`). Parsing will stop if the end of the argument list
 * is reached or if the stop sequence `---` is detected.
 * NOTE: The parser is designed to be alloc-free, errno-free, and TLS-free so
 *       that it can be used in early boot code. Because of this, please note
 *       that registered parameters that expect a C string will be filled with
 *       a reference to the according argv object. argv must not be free'd
 *       after the parser was called (except scan mode).
 *
 * @param argc
 *      The number of arguments
 * @param argv
 *      Reference to the command line arguments
 *      NOTE: In strict mode, program name (typically `argv[0]`) should not be
 *            handed over because the parser tries to parse `argv[0]` as well.
 * @param flags
 *      UK_LIBPARAM_F_* to influence the behavior
 * @return
 *      On success, the argument index is return where the parser stopped.
 *      A negative errno code is returned (<0) on failure.
 */
int uk_libparam_parse(int argc, char **argv, int flags);

#else /* !CONFIG_LIBUKLIBPARAM */

/* Removes library parameter instrumentation if the library is unselected
 * WARNING: Do not use directly.
 */
#define _UK_LIBPARAM_PARAM_DEFINE(name, var, type, count, desc)

#endif /* !CONFIG_LIBUKLIBPARAM */

/*
 * Register a single parameter
 *
 * @param var
 *      Variable to export as library parameter
 * @param type
 *      Data type: bool, char, uchar, int, uint, charp, __s8, __u8, __s16,
 *                 __u16, __s32, __u32, __s64, __u64, __uptr
 * @param desc
 *      C string with parameter description. Optional, can be __NULL.
 */
#define UK_LIBPARAM_PARAM(var, type, desc) \
	_UK_LIBPARAM_PARAM_DEFINE(var, &var, type, 1, desc)

/*
 * Register a single parameter with a custom name
 *
 * @param name
 *      Name used for library parameter
 * @param addr
 *      Reference to variable/memory address of parameter
 * @param type
 *      Data type: bool, char, uchar, int, uint, charp, __s8, __u8, __s16,
 *                 __u16, __s32, __u32, __s64, __u64, __uptr
 * @param desc
 *      C string with parameter description. Optional, can be __NULL.
 */
#define UK_LIBPARAM_PARAM_ALIAS(name, addr, type, desc)		\
	_UK_LIBPARAM_PARAM_DEFINE(name, (addr), type, 1, desc)

/*
 * Register a parameter array
 *
 * @param var
 *      Array to export as library parameter
 * @param type
 *      Data type of array elements: bool, char, uchar, int, uint, charp, __s8,
 *                                   __u8, __s16, __u16, __s32, __u32, __s64,
 *                                   __u64, __uptr
 * @param count
 *      Number of elements in the array that can be filled (<= array size)
 * @param desc
 *      C string with parameter description. Optional, can be __NULL.
 */
#define UK_LIBPARAM_PARAM_ARR(var, type, count, desc) \
	_UK_LIBPARAM_PARAM_DEFINE(var, &var, type, (count), desc)

/*
 * Register a parameter array with a custom name
 *
 * @param name
 *      Name used for library parameter
 * @param addr
 *      Reference to first array element/memory address of parameter
 * @param type
 *      Data type of array elements: bool, char, uchar, int, uint, charp, __s8,
 *                                   __u8, __s16, __u16, __s32, __u32, __s64,
 *                                   __u64, __uptr
 * @param count
 *      Number of elements in the array that can be filled (<= array size)
 * @param desc
 *      C string with parameter description. Optional, can be __NULL.
 */
#define UK_LIBPARAM_PARAM_ARR_ALIAS(name, addr, type, count, desc)	\
	_UK_LIBPARAM_PARAM_DEFINE(name, (addr), type, (count), desc)

/* Deprecated registration macros */
/* WARNING: These interfaces are here for backwards compatibility and will be
 *          removed in the near future.
 */
#define UK_LIB_PARAM(var, type) \
	_UK_LIBPARAM_PARAM_DEFINE(var, &var, type, 1, __NULL)
#define UK_LIB_PARAM_STR(var) \
	UK_LIB_PARAM(var, charp)

#define UK_LIB_PARAM_ARR(var, type) \
	_UK_LIBPARAM_PARAM_DEFINE(var, &var, type, ARRAY_SIZE(var), __NULL)
#define UK_LIB_PARAM_ARR_STR(var) \
	UK_LIB_PARAM_ARR(var, charp)

#endif /* !__ASSEMBLY */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __UK_LIBPARAM_H */

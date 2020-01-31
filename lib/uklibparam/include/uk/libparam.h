/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

/**
 * Variable name prefix/suffix
 */
#define UK_LIBPARAM_SECTION	uk_lib_arg
/**
 * Library: section suffix for the name and the
 * parameter.
 */
#define LIB_PARAM_SUFFIX	__lib_param
#define LIB_NAME_SUFFIX		__lib_str
/**
 * Library variable names for the name and the
 * parameter.
 */
#define LIB_PARAMVAR_PREFIX	_lib_param_
#define LIB_NAMEVAR_PREFIX	_lib_name_
/**
 * Parameter within a library: section suffix for the name and the
 * parameter.
 */
#define PARAM_SECTION_SUFFIX	__param_arg
#define PARAM_NAME_SUFFIX	__param_str
/**
 * Parameter within a library: variable name prefix for the name and the
 * parameter.
 */
#define PARAM_PARAMVAR_PREFIX	_param_param_
#define PARAM_NAMEVAR_PREFIX	_param_name_

#define __STRINGCONCAT(x, y)	x ## y

/**
 * Create a section name.
 * @param libname
 *	The library name
 * @param section
 *	The section suffix for the library
 */
#define _LIB_PARAM_SECTION_NAME(libname, section_name)		\
				__STRINGCONCAT(libname, section_name)

/**
 * Macros to denote the start / stop of a section.
 */
#define _SECTION_START(name)	__STRINGCONCAT(__start_, name)
#define _SECTION_STOP(name)	__STRINGCONCAT(__stop_, name)

/**
 * Make sure there is a dummy implementation for the UK_PARAM family of
 * functions.
 */
#ifndef CONFIG_LIBUKLIBPARAM
/**
 * Declare a library param.
 * @param name
 *	The name of the library param.
 * @param type
 *	The type of the param.
 */
#define UK_LIB_PARAM(name, type)

/**
 * Declare a string library param. This is a dummy implementation.
 * @param name
 *	The name of the parameter.
 */
#define UK_LIB_PARAM_STR(name)

/**
 * Declare an array of primitive.
 * @param name
 *	The name of the parameter.
 * @param type
 *	The type of the parameter.
 */
#define UK_LIB_PARAM_ARR(name, type)

#else /* !CONFIG_LIBUKLIBPARAM */
/**
 * Each parameter is bit-mapped as follows:
 * ---------------------------------------
 * | sign | copy | size of the parameter |
 * ---------------------------------------
 * 7     6      5                       0
 */
/**
 * Sign bit: Shift & Mask
 */
#define PARAM_SIGN_SHIFT	(7)
#define PARAM_SIGN_MASK		(0x1)
/**
 * Shallow copy: Shift & Mask
 */
#define PARAM_SCOPY_SHIFT	(6)
#define PARAM_SCOPY_MASK	(0x1)
/**
 * Size of the param: Shift & Mask
 */
#define PARAM_SIZE_SHIFT	(0x0)
#define PARAM_SIZE_MASK         (0x3F)

#ifndef __ASSEMBLY__
/**
 * Get the parameter type.
 * @param sign
 *	The sign of the data type.
 * @param scopy
 *	Flag to indicate shallow copy.
 *	1 - shallow copy.
 *	0 - data copy.
 * @param size
 *	The size of the parameter.
 */
#define PARAM_TYPE(sign, scopy, size)				\
		(						\
			((((__u8) (sign & PARAM_SIGN_MASK)) <<	\
				  PARAM_SIGN_SHIFT) |		\
			(((__u8) (scopy & PARAM_SCOPY_MASK)) <<	\
				  PARAM_SCOPY_SHIFT) |		\
			(((__u8) (size & PARAM_SIZE_MASK)) <<	\
				  PARAM_SIZE_SHIFT))		\
		)

/**
 * Support data types as parameters
 */
#define _LIB_PARAM___s8		PARAM_TYPE(1, 0, sizeof(__s8))
#define _LIB_PARAM_char		_LIB_PARAM___s8
#define _LIB_PARAM___u8		PARAM_TYPE(0, 0, sizeof(__u8))
#define _LIB_PARAM___s16	PARAM_TYPE(1, 0, sizeof(__s16))
#define _LIB_PARAM___u16	PARAM_TYPE(0, 0, sizeof(__u16))
#define _LIB_PARAM___s32	PARAM_TYPE(1, 0, sizeof(__s32))
#define _LIB_PARAM_int		_LIB_PARAM___s32
#define _LIB_PARAM___u32	PARAM_TYPE(0, 0, sizeof(__u32))
#define _LIB_PARAM___s64	PARAM_TYPE(1, 0, sizeof(__s64))
#define _LIB_PARAM___u64	PARAM_TYPE(0, 0, sizeof(__u64))
#define _LIB_PARAM___uptr	PARAM_TYPE(0, 1, sizeof(__uptr))
#define _LIB_PARAM_charp	_LIB_PARAM___uptr

struct uk_param {
	/* The name of the param */
	const char *name;
	/* Type information for the param */
	const __u8 param_type;
	/* Type information for the variable size param */
	const __u8 param_size;
	/* Define a reference to location of the parameter */
	__uptr addr;
};

struct uk_lib_section {
	/* Library name */
	const char *lib_name;
	/* Section header of the uk_param args */
	struct uk_param *sec_addr_start;
	/* Length of the section */
	__u32	len;
	/* Next section entry */
	struct uk_list_head next;
};

/**
 * Parse through the kernel parameter
 * @param progname
 *	The application name
 * @param argc
 *	The number of arguments
 * @param argv
 *	Reference to the command line arguments
 * @return
 *	On success, return the number of argument parsed.
 *	On Failure, return the error code.
 */
int uk_libparam_parse(const char *progname, int argc, char **argv);

/**
 * Register the library containing kernel parameter.
 *
 * @param lib_sec
 *	A reference to the uk_lib_section.
 */
void _uk_libparam_lib_add(struct uk_lib_section *lib_sec);

/**
 * Add a variable to a specific section.
 * @param section_name
 *	The name of the section.
 * @param align_type
 *	The alignment requirements for the variable definitions.
 */
#define _LIB_PARAM_SECTION_ADD(section_name, align_type)		\
				__attribute__ ((used,			\
						section(		\
					__STRINGIFY(section_name)),	\
					aligned(sizeof(align_type))	\
					     ))
/**
 * Create a constructor name.
 * @param libname
 *	The library name.
 * @param suffix
 *	The suffix appended to the library name.
 */
#define _LIB_UK_CONSTRUCT_NAME(libname, suffix)			\
	       __STRINGCONCAT(libname, suffix)

/**
 * Create a variable name
 * @param prefix
 *	The prefix to the variable name.
 * @param name
 *	The name of the variable
 */
#define _LIB_VARNAME_SET(prefix, name)				\
			 __STRINGCONCAT(prefix, name)

/**
 * Import the section header.
 * @param libname
 *	The library name.
 * @param section_suffix
 *	The suffix string for the section name
 */
#define UK_LIB_IMPORT_SECTION_PARAMS(libname, section_suffix)		\
	extern char *_SECTION_START(					\
			_LIB_PARAM_SECTION_NAME(libname,		\
						section_suffix));	\
	extern char *_SECTION_STOP(					\
			_LIB_PARAM_SECTION_NAME(libname,		\
						section_suffix))	\

/**
 * Create a library name variable and uk_lib_section for each library.
 * @param libname
 *	The library name.
 */
#define UK_LIB_SECTION_CREATE(section, libname)				\
	static const char						\
		_LIB_VARNAME_SET(LIB_NAMEVAR_PREFIX, libname)[] =	\
						__STRINGIFY(libname);	\
	static _LIB_PARAM_SECTION_ADD(					\
				      _LIB_PARAM_SECTION_NAME(section,	\
						LIB_PARAM_SUFFIX),	\
						void *)			\
		struct uk_lib_section					\
			_LIB_VARNAME_SET(LIB_PARAMVAR_PREFIX, libname) = \
			{ .lib_name = __NULL,				\
			  .sec_addr_start = __NULL, .len = 0		\
			}

#define UK_LIB_CTOR_PRIO	1

#define UK_LIB_CONSTRUCTOR_SETUP(prio, name)				\
	UK_CTOR_PRIO(name, prio)

/**
 * Create a constructor to initialize the parameters in the library.
 */
#define UK_LIB_CONSTRUCTOR_CREATE(libname)				\
	static void _LIB_UK_CONSTRUCT_NAME(libname, process_arg)(void)	\
	{								\
		int len = (__uptr) &_SECTION_STOP(			\
				_LIB_PARAM_SECTION_NAME(		\
					libname, PARAM_SECTION_SUFFIX)	\
					) -				\
			  (__uptr) &_SECTION_START(			\
				_LIB_PARAM_SECTION_NAME(		\
					libname, PARAM_SECTION_SUFFIX)	\
					 );				\
		if (len > 0) {						\
			_LIB_VARNAME_SET(LIB_PARAMVAR_PREFIX, libname).	\
					sec_addr_start =		\
						(struct uk_param *)	\
						ALIGN_UP((__uptr)	\
						&_SECTION_START(	\
						_LIB_PARAM_SECTION_NAME(\
						libname,		\
						PARAM_SECTION_SUFFIX)),	\
						sizeof(void *));	\
			_LIB_VARNAME_SET(LIB_PARAMVAR_PREFIX, libname).	\
						len =	len;		\
			_LIB_VARNAME_SET(LIB_PARAMVAR_PREFIX, libname).	\
					 lib_name =		\
						&_LIB_VARNAME_SET(	\
						LIB_NAMEVAR_PREFIX,	\
						libname)[0];		\
			_uk_libparam_lib_add(&_LIB_VARNAME_SET(		\
						LIB_PARAMVAR_PREFIX,	\
						libname)		\
					    );				\
		}							\
	}								\

#define UK_LIB_CONSTRUCTOR_INIT(libname)				\
		UK_LIB_IMPORT_SECTION_PARAMS(libname,			\
					     PARAM_SECTION_SUFFIX);	\
		UK_LIB_SECTION_CREATE(UK_LIBPARAM_SECTION, libname);	\
		UK_LIB_CONSTRUCTOR_CREATE(libname)			\
		UK_LIB_CONSTRUCTOR_SETUP(UK_LIB_CTOR_PRIO,		\
			_LIB_UK_CONSTRUCT_NAME(libname, process_arg))


/**
 * Create a constructor to fill in the parameter.
 */
#ifdef UK_LIBPARAM_PREFIX
	UK_LIB_CONSTRUCTOR_INIT(UK_LIBPARAM_PREFIX);
#endif /* UK_LIBPARAM_PREFIX */

/**
 * Create the fully qualified name of a parameter.
 *
 * @param libname
 *	The name of the library
 * @param name
 *	The name of the parameter
 */
#define _LIB_PARAM_STRING(libname, name)			\
			libname.name

/**
 * Initialize the parameter string in a variable. The name of the
 * parameter is stored in a separate linker section.
 *
 * @param name
 *	The name of the variable
 * @param value
 *	The string representation of the parameter.
 */
#define _LIB_PARAM_NAME_SET(name, value)				\
	static const							\
	char _LIB_VARNAME_SET(PARAM_NAMEVAR_PREFIX, name)[] =		\
						__STRINGIFY(value)


/**
 * Initialize the parameter structure.
 *
 * @param param_name
 *	The name of the parameter
 * @param type
 *	The type of the parameter
 * @param cnt
 *	The number of the elements of that type.
 */
#define _LIB_UK_PARAM_SET(param_name, type, cnt)			\
	static const							\
	_LIB_PARAM_SECTION_ADD(						\
				_LIB_PARAM_SECTION_NAME(		\
						UK_LIBPARAM_PREFIX,	\
						PARAM_SECTION_SUFFIX),	\
						void *			\
				)					\
	struct uk_param _LIB_VARNAME_SET(PARAM_SECTION_SUFFIX,		\
					 param_name) = {		\
		.name = _LIB_VARNAME_SET(PARAM_NAMEVAR_PREFIX,		\
					  param_name),			\
		.param_type = _LIB_PARAM_##type,			\
		.param_size = cnt,					\
		.addr       = (__uptr) &param_name,			\
	}

/**
 * Declare a library param.
 * @param name
 *	The name of the library param.
 * @param type
 *	The type of the param.
 */
#define UK_LIB_PARAM(name, type)					\
	_LIB_PARAM_NAME_SET(name, _LIB_PARAM_STRING(UK_LIBPARAM_PREFIX,	\
						    name));		\
	_LIB_UK_PARAM_SET(name, type, 1)

/**
 * Declare an array of primitive.
 * @param name
 *	The name of the parameter.
 * @param type
 *	The type of the parameter.
 */
#define UK_LIB_PARAM_ARR(name, type)					\
	_LIB_PARAM_NAME_SET(name, _LIB_PARAM_STRING(UK_LIBPARAM_PREFIX,	\
						    name));		\
	_LIB_UK_PARAM_SET(name, type, sizeof(name)/sizeof(type))	\

/**
 * Declare a string library param.
 * @param name
 *	The name of the parameter.
 */
#define UK_LIB_PARAM_STR(name)						\
	UK_LIB_PARAM(name, __uptr)

#endif /* !__ASSEMBLY__ */
#endif /* CONFIG_LIBUKLIBPARAM */

#ifndef __ASSEMBLY__
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !__ASSEMBLY */

#endif /* __UK_LIBPARAM_H */

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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <uk/list.h>
#include <uk/arch/limits.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/libparam.h>
#include <uk/version.h>

#define ARRAY_SEP	 ' '
#define LIB_ARG_SEP	 "--"
#define NUMBER_SET(fn, type, value, addr, max, min, errcode, result_type, fmt)\
	do {								\
		errno = 0;						\
		result_type result = (result_type)fn(value, NULL, 10);	\
		unsigned long long maxvalue =				\
				(sizeof(type) == sizeof(maxvalue)) ?	\
				(result_type)-1 :			\
				(1ULL << ((sizeof(type) << 3))) - 1;	\
		uk_pr_debug("max value: 0x%llx\n", maxvalue);		\
		if (errno != 0)						\
			errcode = -errno;				\
		else if (result >= maxvalue) {				\
			errcode = 1;					\
			*((type *)addr) = (type)(result & maxvalue);	\
		} else {						\
			errcode = 0;					\
			*((type *)addr) = (type)(result & maxvalue);	\
		}							\
		uk_pr_debug("Converting value %s to %"fmt" %"fmt"\n",	\
			    value, *(type *)addr, result);		\
	} while (0)

#define PARGS_PARAM_SET(pargs, parameter, len)				\
	do {								\
		if ((pargs)->param_len)					\
			uk_pr_warn("Found no value. Parameter %s skipped\n",\
				   (pargs)->param);			\
		(pargs)->param = (parameter);				\
		(pargs)->param_len = (len);				\
	} while (0)

struct param_args {
	/* Reference to the start of the library */
	char *lib;
	/* Reference to the start of the parameter */
	char *param;
	/* Reference to the start of the value */
	char *value;
	/* length of the library name */
	__u32 lib_len;
	/* length of the parameter */
	__u32 param_len;
	/* length of the value */
	__u32 value_len;
};

static UK_LIST_HEAD(uk_libsections);

/**
 * Local functions
 */
static int kernel_arg_range_fetch(int argc, char **argv);
static void uk_usage(const char *progname);
static int kernel_arg_fetch(char **args, int nr_args,
			    struct param_args *pargs, int *rewind);
static int kernel_lib_fetch(struct param_args *pargs,
			    struct uk_lib_section **section);
static int kernel_parse_arg(struct param_args *pargs,
			    struct uk_lib_section *section,
			    struct uk_param **param);
static int kernel_arg_set(void *addr, char *value, int size, int sign);
static int kernel_args_set(struct param_args *pargs,
			   struct uk_param *param);
static int kernel_value_sanitize(struct param_args *pargs);

void _uk_libparam_lib_add(struct uk_lib_section *lib_sec)
{
	uk_pr_info("libname: %s, %d\n", lib_sec->lib_name, lib_sec->len);
	uk_list_add_tail(&lib_sec->next, &uk_libsections);
}

static void uk_usage(const char *progname)
{
	printf("Usage: %s\n", progname);
	printf(" [[UNIKRAFT KERNEL ARGUMENT]].. -- [[APPLICATION ARGUMENT]]..\n\n");
	printf("Unikraft library arguments:\n");
	printf("The library arguments are represented as [LIBPARAM_PREFIX].[PARAMNAME]\n\n");
	printf("  -h, --help                 display this help and exit\n");
	printf("  -V, --version              display Unikraft version and exit\n");
}

static int kernel_arg_range_fetch(int argc, char **argv)
{
	int i = 0;

	while (i < argc) {
		/* Separate the kernel param from the application parameters */
		if (strcmp(LIB_ARG_SEP, argv[i]) == 0)
			return i;
		i++;
	}

	return -1;
}

static int kernel_arg_fetch(char **args, int nr_args,
			    struct param_args *pargs, int *rewind)
{
	int i = 0;
	int rc = 0;
	char *equals_ptr = NULL, *dupl_ptr = NULL;
	int len, cnt = 0, equals = -1;

	UK_ASSERT(rewind && pargs);

	pargs->param = NULL;
	pargs->value = NULL;
	pargs->param_len = 0;
	pargs->value_len = 0;

	for (i = 0; (!pargs->value_len ||
		     !pargs->param_len) && i < nr_args; i++) {
		uk_pr_debug("at index:%d user args %s\n", i, args[i]);
		len = strlen(args[i]);
		/* if the equals character is present */
		if (!equals_ptr)
			equals_ptr = strchr(args[i], '=');
		cnt++;
		/* Check for multiple '=' */
		dupl_ptr = strrchr(args[i], '=');
		if (equals_ptr && dupl_ptr && equals_ptr !=  dupl_ptr) {
			uk_pr_err("Multiple '=' character found. Skipping argument %s\n",
				   args[i]);
			rc = -EINVAL;
			goto exit;
		} else if (equals < 0) {
			/* Searching for the parameters */
			if (equals_ptr && (len > 1) &&
			   (equals_ptr - args[i]) == (len - 1)) {
				/* [libname_prefix].[parameter]= value */
				uk_pr_debug("Expecting parameter with equals %s\n",
					     args[i]);
				PARGS_PARAM_SET(pargs, args[i], len - 1);
				equals = i;
			} else if (equals_ptr && (len > 1) &&
				   equals_ptr == args[i]) {
				/* [libname_prefix].[parameter] =value */
				uk_pr_debug("Expecting equals followed by value %s\n",
					    args[i]);
				pargs->value =  equals_ptr + 1;
				pargs->value_len = len - 1;
				equals = i;
			} else if (equals_ptr && len == 1) {
				/* Contains only equals */
				equals = i;
				continue;
			} else if (equals_ptr) {
				/* [libname_prefix].[parameter]=value */
				uk_pr_debug("Expecting entire argument %s\n",
					    args[i]);
				PARGS_PARAM_SET(pargs, args[i],
						equals_ptr - args[i]);
				equals = i;
				pargs->value = equals_ptr + 1;
				pargs->value_len = len - (pargs->param_len + 1);
			} else if (!equals_ptr) {
				/* [libname_prefix].[parameter] = value */
				uk_pr_debug("Expecting parameter alone%s\n",
					    args[i]);
				PARGS_PARAM_SET(pargs, args[i], len);
				pargs->param = args[i];
				pargs->param_len = len;
			} else {
				uk_pr_err("Failed to parse the argument %s\n",
					  args[i]);
				rc = -EINVAL;
				goto exit;
			}
		} else if (equals >= 0) {
			uk_pr_debug("Expecting value only %s\n",
				    args[i]);
			pargs->value = args[i];
			pargs->value_len = len;
		} else {
			/* Error case */
			uk_pr_err("Failed to parse the argument:%s\n", args[i]);
			rc = -EINVAL;
			goto exit;

		}
	}

	uk_pr_debug("pargs->param: %p, pargs->value: %p\n", pargs->param,
		    pargs->value);
	if (pargs->param_len != 0 && pargs->value_len == 0) {
		uk_pr_err("Failed to completely parse the user argument\n");
		rc = -EINVAL;
		goto exit;
	}

exit:
	*rewind = cnt;
	return rc;
}

/**
 * Kernel Parameter are passed in this format
 * [libname_prefix].[parameter]
 */
static int kernel_lib_fetch(struct param_args *pargs,
			    struct uk_lib_section **section)
{
	char *libparam;
	struct uk_lib_section *iter;

	UK_ASSERT(section && pargs);
	pargs->lib_len = 0;
	libparam = memchr(pargs->param, '.', pargs->param_len);
	if (!libparam) {
		uk_pr_err("Failed to identify the library\n");
		goto error_exit;
	}

	uk_list_for_each_entry(iter, &uk_libsections, next) {
		uk_pr_debug("Lib: %s, libname: %s %ld\n", iter->lib_name,
			    pargs->param, libparam - pargs->param);
		/**
		 * Compare the length of the library names to avoid having
		 * library with a similar prefix wrongly matching.
		 */
		if ((strlen(iter->lib_name) ==
		    (size_t) (libparam - pargs->param)) &&
		    memcmp(pargs->param, iter->lib_name,
			   (libparam - pargs->param)) == 0) {
			*section = iter;
			pargs->lib_len = libparam - pargs->param;
			return 0;
		}
	}
	uk_pr_err("Failed to fetch the library\n");

error_exit:
	*section = NULL;
	pargs->lib_len = 0;
	return -EINVAL;
}

static int kernel_parse_arg(struct param_args *pargs,
			    struct uk_lib_section *section,
			    struct uk_param **param)
{
	int i = 0;
	struct uk_param *iter;
	int len = 0;

	UK_ASSERT(section && param && pargs);

	len = section->len / sizeof(struct uk_param);
	iter = section->sec_addr_start;
	uk_pr_debug("Section length %d section@%p, uk_param: %lu\n", len, iter,
		    sizeof(*iter));

	for (i = 0; i < len; i++, iter++) {
		UK_ASSERT(iter->name);
		uk_pr_debug("Param name: %s at address: %p\n", iter->name,
			    iter);
		/**
		 * Compare the length of the library names to avoid having
		 * library with a similar prefix wrongly matching.
		 */
		if ((strlen(iter->name) == pargs->param_len) &&
		     memcmp(iter->name, pargs->param, pargs->param_len) == 0) {
			*param = iter;
			return 0;
		}
	}

	uk_pr_err("Failed to identify the parameter\n");
	*param = NULL;
	return -EINVAL;
}

static int kernel_arg_set(void *addr, char *value, int size, int sign)
{
	int error = 0;

	/**
	 * Check for the output address instead of UK_ASSERT because this is
	 * a user provided input.
	 */
	if (!addr) {
		uk_pr_err("Invalid output buffer\n");
		goto error_exit;
	}

	switch (size) {
	case 1:
		if (sign) {
			*((__s8 *)addr) = *value;
			if (strnlen(value, 2) > 1)
				error = 1;
		} else
			NUMBER_SET(strtoul, __u8, value, addr, __U8_MAX,
				   __U8_MIN, error, __u32, __PRIu8);
		break;
	case 2:
		if (sign)
			NUMBER_SET(strtol, __s16, value, addr, __S16_MAX,
				   __S16_MIN, error, __u32, __PRIs16);
		else
			NUMBER_SET(strtoul, __u16, value, addr, __U16_MAX,
				   __U16_MIN, error, __u32, __PRIu16);
		break;
	case 4:
		if (sign)
			NUMBER_SET(strtol, __s32, value, addr, __S32_MAX,
				   __S32_MIN, error, __u32, __PRIs32);
		else
			NUMBER_SET(strtoul, __u32, value, addr, __U32_MAX,
				   __U32_MIN, error, __u32, __PRIu32);
		break;
	case 8:
		if (sign)
			NUMBER_SET(strtoll, __s64, value, addr, __S64_MAX,
				   __S64_MIN, error, __u64, __PRIs64);
		else
			NUMBER_SET(strtoull, __u64, value, addr, __U64_MAX,
				   __U64_MIN, error, __u64, __PRIu64);
		break;
	default:
		uk_pr_err("Cannot understand type of size %d\n", size);
		goto error_exit;
	}
	if (error < 0)
		goto error_exit;
	else if (error == 1)
		uk_pr_warn("Overflow/Underflow detected in value %s\n", value);
	return 0;

error_exit:
	uk_pr_err("Failed to convert value %s\n", value);
	return -EINVAL;
}

static int kernel_args_set(struct param_args *pargs,
			   struct uk_param *param)
{
	int rc = 0;
	int i  = 0;
	char *start, *value;
	int sign = (param->param_type >> PARAM_SIGN_SHIFT) & PARAM_SIGN_MASK;
	int scopy = (param->param_type >> PARAM_SCOPY_SHIFT) & PARAM_SCOPY_MASK;
	int param_type = (param->param_type >> PARAM_SIZE_SHIFT)
				& PARAM_SIZE_MASK;
	uk_pr_debug("Parameter value %s, type: %d, sign: %d scopy: %d\n",
		    pargs->value, param_type, sign, scopy);

	if (scopy == 1)
		/* Reference the pointer instead of copying the value */
		*((__uptr *)param->addr) = (__uptr) pargs->value;
	else {
		if (param->param_size > 1) {
			/* Adding support for array */
			i = 0;
			value = &pargs->value[i];
			uk_pr_debug("Value:%s length: %d\n", value,
				     pargs->value_len);
			while (value && i < param->param_size) {
				start = value;
				value = strchr(value, ARRAY_SEP);
				if (value) {
					uk_pr_debug("Delimiter: %p\n", value);
					*value = '\0';
					/* Search from the next index */
					value++;
				}
				uk_pr_debug("Array index: %d contains %s\n",
					    i, start);
				rc = kernel_arg_set((void *)(param->addr +
						    (i * param_type)),
						    start, param_type, sign);
				if (rc < 0)
					break;
				i++;
			}
			if (rc < 0)
				uk_pr_err("Failed to read element at index: %d\n",
					   i);
			else if (value && i == param->param_size)
				uk_pr_warn("Overflow detected! Max array size:%d\n",
					   param->param_size);
			else
				uk_pr_debug("Converted value: %s into an array containing %d elements\n",
					    pargs->value, i);
		} else if (param->param_size == 1) {
			rc = kernel_arg_set((void *)param->addr,
					    pargs->value, param_type, sign);
		} else {
			uk_pr_err("Error: Cannot find the parameter\n");
			rc = -EINVAL;
		}
	}

	return rc;
}

/**
 * The function removes parse for quotes around the value.
 * TODO: We do not support nested '"'.
 */
static int kernel_value_sanitize(struct param_args *pargs)
{
	int rc = 0;
	char *ptr;
	char *start_idx = NULL;
	char *end_idx = NULL;
	int qcnt = 0;

	UK_ASSERT(pargs && pargs->value);
	ptr = pargs->value;
	uk_pr_debug("Sanitizing value %s (length %d)\n", pargs->value,
		    pargs->value_len);

	do {
		switch (*ptr) {
		case ' ':
		case '\r':
		case '\n':
		case '\t':
		case '\v':
			ptr++;
			break;
		case'\'':
		case '"':
			if (start_idx)
				end_idx = ptr;
			else if (!start_idx)
				start_idx = ptr + 1;
			ptr++;
			qcnt++;
			break;
		default:
			if (!start_idx)
				start_idx = ptr;
			ptr++;
			break;
		}
	} while (*ptr != '\0' && !(end_idx && start_idx));
	if (!end_idx)
		end_idx =  ptr;

	uk_pr_debug("Adjusting start to %p & end to %p #quotes: %d\n",
		    start_idx, end_idx, qcnt);

	if (qcnt == 1) {
		uk_pr_err("Value %s not quoted properly\n", pargs->value);
		rc = -EINVAL;
	} else if (start_idx && end_idx) {
		memset(pargs->value, '\0', start_idx - pargs->value);
		memset(end_idx, '\0',
		       (pargs->value + pargs->value_len) - end_idx);
		pargs->value = start_idx;
		pargs->value_len = end_idx - start_idx;
	}
	uk_pr_debug("Sanitized value %s (length %d)\n", pargs->value,
		    pargs->value_len);

	return rc;
}

int uk_libparam_parse(const char *progname, int argc, char **argv)
{
	int keindex = 0;
	int rc = 0, cnt = 0, args_read, i;
	struct param_args pargs = {0};
	struct uk_lib_section *section = NULL;
	struct uk_param *param = NULL;

	keindex = kernel_arg_range_fetch(argc, argv);
	if (keindex < 0) {
		uk_pr_info("No library arguments found\n");
		return 0;
	}

	uk_pr_debug("Library argument ends at %d\n", keindex);

	while (cnt < keindex) {
		/* help and version */
		if (strcmp(argv[cnt], "-h") == 0 ||
		    strcmp(argv[cnt], "--help") == 0) {
			uk_usage(progname);
			ukplat_halt();
		} else if (strcmp(argv[cnt], "-V") == 0 ||
			   strcmp(argv[cnt], "--version") == 0) {
			uk_version();
			ukplat_halt();
		}

		args_read = 0;
		/* Fetch the argument from the input */
		rc = kernel_arg_fetch(&argv[cnt], (keindex - cnt),
				      &pargs, &args_read);
		if (rc < 0) {
			uk_pr_err("Failed to fetch arg between index %d and %d\n",
				  cnt, (cnt + args_read));
			uk_pr_err("Skipping Args:");
			for ( i = cnt; i < cnt + args_read; i++)
				uk_pr_err(" %s", argv[i]);
			uk_pr_err("\n");
			cnt += args_read;
			continue;
		}
		uk_pr_debug("Processing argument %s\n", pargs.param);
		cnt += args_read;

		/* Fetch library for the argument */
		rc = kernel_lib_fetch(&pargs, &section);
		if (rc < 0 || !section) {
			uk_pr_err("Failed to identify the library\n");
			continue;
		}

		/* Fetch the parameter for the argument */
		rc = kernel_parse_arg(&pargs, section, &param);
		if (rc < 0 || !param) {
			uk_pr_err("Failed to parse arg\n");
			continue;
		}

		rc = kernel_value_sanitize(&pargs);
		if (rc  < 0) {
			uk_pr_err("Failed to sanitize %s param\n", pargs.param);
			continue;
		}

		rc = kernel_args_set(&pargs, param);
		uk_pr_info("Parsed %d args\n", cnt);
	}

	/* Replacing the -- with progname */
	argv[keindex] = DECONST(char *, progname);

	return keindex + 1;
}

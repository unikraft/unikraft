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
#include <uk/config.h>
#include <uk/libparam.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <uk/list.h>
#include <uk/arch/limits.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/version.h>
#include <uk/essentials.h>

static UK_LIST_HEAD(ld_head);

#define PARSE_PARAM_SEP    '.'
#define PARSE_VALUE_SEP    '='
#define PARSE_LIST_START   '['
#define PARSE_LIST_END     ']'
#define PARSE_STOP         "--"
#define PARSE_USAGE        "help"

void _uk_libparam_libsec_register(struct uk_libparam_libdesc *ld)
{
	uk_list_add_tail(&ld->next, &ld_head);
}

static const char *str_param_type(enum uk_libparam_param_type pt)
{
	switch (pt) {
	case _UK_LIBPARAM_PARAM_TYPE_bool:
		return "bool";
	case _UK_LIBPARAM_PARAM_TYPE___s8:
		return "s8";
	case _UK_LIBPARAM_PARAM_TYPE___u8:
		return "u8";
	case _UK_LIBPARAM_PARAM_TYPE___s16:
		return "s16";
	case _UK_LIBPARAM_PARAM_TYPE___u16:
		return "u16";
	case _UK_LIBPARAM_PARAM_TYPE___s32:
		return "s32";
	case _UK_LIBPARAM_PARAM_TYPE___u32:
		return "u32";
	case _UK_LIBPARAM_PARAM_TYPE___s64:
		return "s64";
	case _UK_LIBPARAM_PARAM_TYPE___u64:
		return "u64";
	case _UK_LIBPARAM_PARAM_TYPE___uptr:
		return "uptr";
	case _UK_LIBPARAM_PARAM_TYPE_charp:
		return "charp";
	default:
		break;
	}
	return "<?>";
}

#define UK_LIBPARAM_FOREACH_LIBDESC(ld_iter) \
	uk_list_for_each_entry((ld_iter), &ld_head, next)
#define UK_LIBPARAM_FOREACH_PARAMIDX(ld, p_iter) \
	for ((p_iter) = 0; (p_iter) < (ld)->params_len; (p_iter)++)
#define UK_LIBPARAM_PARAM_GET(libdesc, idx) \
	(((idx) >= (libdesc)->params_len) ? __NULL : ((libdesc)->params)[idx])

static void uk_usage(void)
{
	struct uk_libparam_libdesc *ld;
	struct uk_libparam_param *p;
	__sz p_i;

	/*
	 * FIXME: Use a console print variant without context prefix
	 *        (Update as soon as it is available)
	 */
	uk_pr_info("Usage of command line:\n"
		   "   [%s] [PREFIX%cPARAMETER%cVALUE]... %s [APPLICATION ARGUMENT]...\n\n",
		   PARSE_USAGE, PARSE_PARAM_SEP, PARSE_VALUE_SEP, PARSE_STOP);
	uk_pr_info("Special commands:\n"
		   "%12s                   Print this help summary\n\n",
		   PARSE_USAGE);
	uk_pr_info("Available parameters:\n");
	UK_LIBPARAM_FOREACH_LIBDESC(ld) {
		UK_LIBPARAM_FOREACH_PARAMIDX(ld, p_i) {
			p = UK_LIBPARAM_PARAM_GET(ld, p_i);
			UK_ASSERT(p);

			if (p->count == 0)
				continue;
			uk_pr_info("%12s.%-18s%s (",
				   ld->prefix, p->name,
				   p->desc ? p->desc : p->name);
			if (p->count > 1)
				uk_pr_info("array[%"__PRIsz"] of ",
				p->count);
			uk_pr_info("%s)\n", str_param_type(p->type));
		}
	}
	uk_pr_info("\n"
		   "Numbers can be passed in decimal, octal (\"0\" as prefix), or hexadecimal (\"0x\" as prefix).\n");
	uk_pr_info("Valid boolean values for 'true' are: \"true\", \"on\", \"yes\", a non-zero number.\n"
		   "Valid boolean values for 'false' are: \"false\", \"off\", \"no\", a zero number (e.g., \"0\").\n");
	uk_pr_info("Boolean parameters that are passed without a value will be set to 'true'.\n");
	uk_pr_info("Array parameters can be passed with multiple 'PREFIX%cPARAMETER%cVALUE' tokens,\n",
		   PARSE_PARAM_SEP, PARSE_VALUE_SEP);
	uk_pr_info("using a list: 'PREFIX%cPARAMETER%c%c VALUE0 VALUE1 ... %c', or a combination of both.\n",
		   PARSE_PARAM_SEP, PARSE_VALUE_SEP, PARSE_LIST_START,
		   PARSE_LIST_END);
	uk_pr_info("Please refer the application manual or application help for application arguments.\n");
}

enum parse_arg_state {
	PAS_PARAM,	/* Parsing parameters and single value assignments */
	PAS_LIST,	/* Parsing list of values */
};

/* Parser context/state */
struct parse_arg_ctx {
	int hit_stop;
	int hit_usage;
	enum parse_arg_state state;

	struct uk_libparam_libdesc *ld;
	struct uk_libparam_param *p;
};

static struct uk_libparam_libdesc *find_libdesc(const char *libname,
						__sz libname_len)
{
	struct uk_libparam_libdesc *ld_iter;

	UK_LIBPARAM_FOREACH_LIBDESC(ld_iter) {
		/*
		 * We want an exact match, because libname might not
		 * be zero terminated, we need to compare library name and the
		 * lengths separately.
		 */
		if ((strncmp(ld_iter->prefix, libname, libname_len) == 0)
		    && (ld_iter->prefix[libname_len] == '\0'))
			return ld_iter;
	}
	return NULL;
}

static struct uk_libparam_param *find_libparam(struct uk_libparam_libdesc *ld,
					       const char *paramname,
					       __sz paramname_len)
{
	struct uk_libparam_param *p;
	__sz i;

	UK_ASSERT(ld);

	UK_LIBPARAM_FOREACH_PARAMIDX(ld, i) {
		p = UK_LIBPARAM_PARAM_GET(ld, i);

		/*
		 * We look for an exact match, because `paramname` might not
		 * be zero terminated. First, we need to compare strings and
		 * then their lengths. Because we know the length of `paramname`
		 * we can simply check if the position of the zero termination
		 * symbol matches.
		 */
		if ((strncmp(p->name, paramname, paramname_len) == 0)
		    && (p->name[paramname_len] == '\0'))
			return p;
	}
	return NULL;
}

/*
 * Internal and stripped down version of strtoull that does not use `errno`.
 * The parsed integer value is returned on `result` and its sign on
 * `result_is_neg`. This function is derived from nolibc.
 */
static __ssz __safe_strntoull(const char *nptr, __sz maxlen, char **endptr,
			      int base, unsigned long long *result,
			      int *result_is_neg)
{
	const char *s = nptr;
	unsigned long long acc = 0;
	unsigned char c;
	unsigned long long qbase, cutoff;
	int neg, any, cutlim;
	__sz len = 0;
	int err = 0;

	UK_ASSERT(nptr);
	UK_ASSERT(result);
	UK_ASSERT(result_is_neg);
	UK_ASSERT((base >= 0) && (base <= 36));

	if (maxlen == 0) {
		err = -EINVAL;
		goto exit;
	}

	c = *s++;
	while (isspace(c) && len < maxlen) {
		c = *s++;
		len++;
	}

	/* Detect sign */
	if (c == '-') {
		neg = 1;
		c = *s++;
		len++;
	} else {
		neg = 0;
		if (c == '+') {
			c = *s++;
			len++;
		}
	}
	/* Detect base */
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
		len += 2;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	qbase = (unsigned int)base;
	cutoff = (unsigned long long) __ULL_MAX / qbase;
	cutlim = (unsigned long long) __ULL_MAX % qbase;
	for (acc = 0, any = 0; len < maxlen && c != '\0'; c = *s++, len++) {
		if (!isascii(c)) {
			err = -EINVAL;
			break;
		}
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else {
			err = -EINVAL;
			break;
		}
		if (c >= base) {
			err = -EINVAL;
			break;
		}
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= qbase;
			acc += c;
		}
	}
	if (any < 0)
		err = -ERANGE;

	/* Trailing whitespaces */
	while (isspace(c) && len < maxlen) {
		c = *s++;
		len++;
	}

exit:
	if (endptr != 0)
		*endptr = DECONST(char *, any ? s - 1 : nptr);
	if (err < 0)
		return err;

	*result = acc;
	*result_is_neg = neg;
	return (__ssz) len;
}

#define do_write_value(p, type_val, widx, raw_val)			\
	do {								\
		uk_pr_debug("Writing '0x%"__PRIx64"' to '%s' "		\
			    "(%s, %p[%"__PRIsz"]=%p)\n",		\
			    (__s64) raw_value, (p)->name,		\
			    str_param_type((p)->type),			\
			    (p)->addr, widx,				\
			    &(((type_val *) (p)->addr)[widx]));		\
		((type_val *) (p)->addr)[widx] = (type_val) (raw_val);	\
	} while (0)

/* WARNING: Since we do not have a __maxint type, we take __uptr as generic
 *          value container for all int types and charp pointer
 */
static int write_value(struct uk_libparam_param *p, __uptr raw_value)
{
	UK_ASSERT(p);
	__sz widx = 0;

	if (unlikely(p->count == 0)) {
		/* If count is equal to 0, we never have space */
		return -ENOSPC;
	} else if (p->count == 1) {
		/* Always overwrite single value parameters */
		widx = 0;
	} else {
		/* Take next index within array bounds,
		 * otherwise return -ENOSPC
		 */
		if (unlikely(p->__widx >= p->count))
			return -ENOSPC;
		widx = p->__widx++;
	}

	switch (p->type) {
	case _UK_LIBPARAM_PARAM_TYPE_bool:
		if (raw_value != 0)
			raw_value = 1; /* Normalization */
		do_write_value(p, int, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s8:
		do_write_value(p, __s8, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u8:
		do_write_value(p, __u8, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s16:
		do_write_value(p, __s16, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u16:
		do_write_value(p, __u16, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s32:
		do_write_value(p, __s32, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u32:
		do_write_value(p, __u32, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s64:
		do_write_value(p, __s64, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u64:
		do_write_value(p, __u64, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___uptr:
		do_write_value(p, __uptr, widx, raw_value);
		break;
	case _UK_LIBPARAM_PARAM_TYPE_charp:
		do_write_value(p, __uptr, widx, raw_value);
		break;
	default:
		break;
	}
	return 0;
}

static int parse_value(struct parse_arg_ctx *ctx, char *value, __sz value_len)
{
	int is_negative;
	unsigned long long num;
	__ssz len;

	UK_ASSERT(ctx);
	UK_ASSERT(ctx->ld);
	UK_ASSERT(ctx->p);

	/* Handle special string cases for boolean and charp */
	switch (ctx->p->type) {
	case _UK_LIBPARAM_PARAM_TYPE_bool:
		if ((!value) ||
		    (value_len >= 2 &&
		     strncmp("on", value, value_len) == 0) ||
		    (value_len >= 3 &&
		     strncmp("yes", value, value_len) == 0) ||
		    (value_len >= 4 &&
		     strncmp("true", value, value_len) == 0)) {
			return write_value(ctx->p, 1); /* Write "true" */
		} else if ((value_len >= 3 &&
			    strncmp("off", value, value_len) == 0) ||
			   (value_len >= 2 &&
			    strncmp("no", value, value_len) == 0) ||
			   (value_len >= 5 &&
			    strncmp("false", value, value_len) == 0)) {
			return write_value(ctx->p, 0);  /* Write "false" */
		}
		break; /* Continue treating value as a number */

	case _UK_LIBPARAM_PARAM_TYPE_charp:
		if (unlikely(!value)) {
			uk_pr_warn("No value given to %s.%s (charp)\n",
				   ctx->ld->prefix, ctx->p->name);
			return -EINVAL;
		}

		/* Ensure '\0'-termination and store reference to
		 * parameter variable
		 */
		value[value_len] = '\0';
		return write_value(ctx->p, (__uptr) value);

	default:
		break;
	}

	/*
	 * Convert string to number
	 * NOTE: We should never enter here with _UK_LIBPARAM_PARAM_TYPE_charp,
	 *       _UK_LIBPARAM_PARAM_TYPE_bool may arrive here if we need to
	 *       parse a number value for it.
	 */
	UK_ASSERT(ctx->p->type != _UK_LIBPARAM_PARAM_TYPE_charp);

	/* From here on, we cannot continue without having a value to convert */
	if (!value || (value_len == 0))
		goto novalue;

	/* Parse `value` and store result in `num` */
	len = __safe_strntoull(value, value_len, __NULL,
			       0 /* auto-detect base */,
			       &num, &is_negative);
	if ((len < 0) || ((__sz) len < value_len)) {
		if (len == -ERANGE)
			goto outofrange;

		uk_pr_debug("len:%"__PRIssz" value_len:%"__PRIsz"\n",
			    len, value_len);
		goto malformed;
	}

	/* Check if resulting number fits */
#define do_check_num(num, is_negative, min, max)			\
	do {								\
		if (num > (unsigned long long) max)			\
			goto outofrange;				\
		if (is_negative) {					\
			num = -num;					\
			if ((long long) num < (long long) min)		\
				goto outofrange;			\
		}							\
	} while (0)

	switch (ctx->p->type) {
	case _UK_LIBPARAM_PARAM_TYPE___s8:
		do_check_num(num, is_negative, __S8_MIN, __S8_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s16:
		do_check_num(num, is_negative, __S16_MIN, __S16_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s32:
		do_check_num(num, is_negative, __S32_MIN, __S32_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___s64:
		do_check_num(num, is_negative, __S64_MIN, __S64_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u8:
		do_check_num(num, is_negative, 0, __U8_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u16:
		do_check_num(num, is_negative, 0, __U16_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u32:
		do_check_num(num, is_negative, 0, __U32_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___u64:
		do_check_num(num, is_negative, 0, __U64_MAX);
		break;
	case _UK_LIBPARAM_PARAM_TYPE___uptr:
		do_check_num(num, is_negative, 0, __PTR_MAX);
		break;
	default:
		/* No checks or modification needed for bool */
		break;
	}
#undef do_check_num

	/* Write number to target parameter */
	return write_value(ctx->p, (__uptr) num);

novalue:
	uk_pr_err("Parameter %s.%s requires a %s value\n",
		  ctx->ld->prefix, ctx->p->name, str_param_type(ctx->p->type));
	return -EINVAL;

outofrange:
	uk_pr_err("Parameter %s.%s (%s): Given number is out of range\n",
		  ctx->ld->prefix, ctx->p->name, str_param_type(ctx->p->type));
	return -ERANGE;

malformed:
	uk_pr_err("Parameter %s.%s (%s): Given number is malformed\n",
		  ctx->ld->prefix, ctx->p->name, str_param_type(ctx->p->type));
	return -EINVAL;
}

static int parse_arg(struct parse_arg_ctx *ctx, char *strbuf, int scan_only)
{
	UK_ASSERT(ctx);
	UK_ASSERT(strbuf);

	char *value;
	__sz value_len;
	int ret = 0;

	if (strbuf[0] == '\0') {
		 /* Empty string: We skip this snippet */
		return 0;
	}

	uk_pr_debug("Parsing snippet: \"%s\"\n", strbuf);
	switch (ctx->state) {
	case PAS_PARAM:
		/*
		 * Catch stop sequence ('--') or usage command ('help')
		 */
		if (strcmp(strbuf, PARSE_STOP) == 0) {
			ctx->hit_stop = 1;
			return 0;
		}
		if (strcmp(strbuf, PARSE_USAGE) == 0) {
			ctx->hit_usage = 1;
			return 0;
		}

		/* At this stage we parse the library and parameter name.
		 * We expect the input to be in the following form:
		 *   libname.parameter=value
		 */
		do {
			char *libname = strbuf;
			__sz libname_len;
			char *paramname;
			__sz paramname_len;

			paramname = strchr(libname, PARSE_PARAM_SEP);
			if (unlikely(!paramname)) {
				uk_pr_debug(" Failed to determine library and parameter names (separator '%c' not found)\n",
					    PARSE_PARAM_SEP);
				return -EINVAL;
			}
			libname_len = (__sz)((__uptr) paramname -
					     (__uptr) libname);
			paramname += 1; /* skip leading char (= separator) */

			/*
			 * Check if a value was given (by looking for '='
			 * separator). Note, a value is optional; it sets
			 * boolean values to TRUE
			 */
			value = strchr(paramname, PARSE_VALUE_SEP);
			if (!value) {
				uk_pr_debug(" No value given (separator '%c' not found), trying without...\n",
					    PARSE_VALUE_SEP);
				paramname_len = strlen(paramname);
				value_len = 0;
			} else {
				paramname_len = (__sz)((__uptr) value -
						       (__uptr) paramname);
				value += 1; /* skip leading char (separator) */
				value_len = strlen(value);
			}
			uk_pr_debug(" Parsed token:\n");
			uk_pr_debug("  libprefix: \"%.*s\"\n",
				    (int) libname_len, libname);
			uk_pr_debug("  paramname: \"%.*s\"\n",
				    (int) paramname_len, paramname);

			/* Search database for corresponding libname.paramname
			 * entry.
			 * NOTE: Because we accept only exact matches, this
			 *       automatically filters wrongly detected libname
			 *       and paramname pairs (e.g., having whitespaces,
			 *       not allowed characters, or empty strings).
			 */
			if (!scan_only) {
				ctx->ld = find_libdesc(libname, libname_len);
				if (ctx->ld)
					ctx->p = find_libparam(ctx->ld,
							       paramname,
							       paramname_len);
				if (!ctx->ld || !ctx->p) {
					uk_pr_warn("Parameter %.*s.%.*s: Unknown or invalid\n",
						(int) libname_len, libname,
						(int) paramname_len, paramname);
					ret = -ENOENT;
					ctx->p = NULL;
				}
			}

			/* Do we have a value list? (first char must be '[') */
			if (value && value[0] == PARSE_LIST_START) {
				ctx->state = PAS_LIST;
				value += 1; /* skip list start character */
				value_len -= 1;

				if (value_len > 0)
					goto parse_list;
				break;
			}

			/* Parse value (ctx->param points to current value) */
			uk_pr_debug("  value: \"%s\"\n",
				    value ? value : "<NULL>");
			if (ctx->p && !scan_only)
				ret = parse_value(ctx, value, value_len);
		} while (0);
		break;

	case PAS_LIST:
		value = strbuf;
		value_len = strlen(value);
parse_list:
		/* Parse values until the end of the values list.
		 * For the end, we only need to check if we have a terminating
		 * ']' character.
		 */
		if ((value_len >= 1)
		    && (value[value_len - 1] == PARSE_LIST_END)) {
			/* Because we reached the end of the list,
			 * we will stop processing the list
			 */
			ctx->state = PAS_PARAM;
			value_len -= 1; /* Hide terminating char */

			if (value_len == 0) {
				/* We had an end of list character, only */
				ctx->state = PAS_PARAM;
				break;
			}
		}

		uk_pr_debug("  list value: \"%.*s\"\n", (int) value_len, value);
		if (ctx->p && !scan_only)
			ret = parse_value(ctx, value, value_len);
		break;
	}

	return ret;
}

static inline void __reset_p_widx(void)
{
	struct uk_libparam_libdesc *ld;
	struct uk_libparam_param *p;
	__sz p_i;

	UK_LIBPARAM_FOREACH_LIBDESC(ld) {
		UK_LIBPARAM_FOREACH_PARAMIDX(ld, p_i) {
			p = UK_LIBPARAM_PARAM_GET(ld, p_i);
			UK_ASSERT(p);
			p->__widx = 0;
		}
	}
}

int uk_libparam_parse(int argc, char **argv, int flags)
{
	struct parse_arg_ctx c;
	int rc;
	int i;

	memset(&c, 0x0, sizeof(c));
	/* Reset __widx fields */
	__reset_p_widx();

	/* Feed arguments from vector to parse_arg() */
	for (i = 0; i < argc; ++i) {
		rc = parse_arg(&c, argv[i], (flags & UK_LIBPARAM_F_SCAN));
		if (rc < 0 && (flags & UK_LIBPARAM_F_STRICT))
			return rc;

		if (c.hit_stop) {
			/* Stop sequence detected */
			break;
		}
	}

	if (unlikely(c.hit_usage && (flags & UK_LIBPARAM_F_USAGE))) {
		uk_usage();
		return -EINTR;
	}
	return i;
}

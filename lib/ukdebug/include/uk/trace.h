/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
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

#ifndef _UK_TRACE_H_
#define _UK_TRACE_H_
#include <uk/essentials.h>
#include <stdint.h>
#include <stddef.h>
#include <uk/plat/time.h>
#include <string.h>
#include <uk/arch/lcpu.h>
#include <uk/plat/lcpu.h>

/* There is no justification of the limit of 80 symbols. But there
 * must be a limit anyways. Just number from the coding standard is
 * used. Feel free to change this limit if you have the actual reason
 * for another number
 */
#define __UK_TRACE_MAX_STRLEN 80
#define UK_TP_HEADER_MAGIC 0x64685254 /* TRhd */
#define UK_TP_DEF_MAGIC 0x65645054 /* TPde */

enum __uk_trace_arg_type {
	__UK_TRACE_ARG_INT = 0,
	__UK_TRACE_ARG_STRING = 1,
};

struct uk_tracepoint_header {
	uint32_t magic;
	uint32_t size;
	__nsec time;
	void *cookie;
};

extern size_t uk_trace_buffer_free;
extern char *uk_trace_buffer_writep;

#define __UK_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define UK_NARGS(...)  __UK_NARGS_X(, ##__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)


static inline void __uk_trace_save_arg(char **pbuff,
				      size_t *pfree,
				      enum __uk_trace_arg_type type,
				      int size,
				      long arg)
{
	char *buff = *pbuff;
	size_t free = *pfree;
	int len;

	if (type == __UK_TRACE_ARG_STRING) {
		len = strnlen((char *) arg, __UK_TRACE_MAX_STRLEN);
		/* The '+1' is for storing length of the string */
		size = len + 1;
	}

	if (free < (size_t) size) {
		/* Block the next invocations of trace points */
		*pfree = 0;
		uk_trace_buffer_free = 0;
		return;
	}

	switch (type) {
	case __UK_TRACE_ARG_INT:
		/* for simplicity we do not care about alignment */
		memcpy(buff, &arg, size);
		break;
	case __UK_TRACE_ARG_STRING:
		*((uint8_t *) buff) = len;
		memcpy(buff + 1, (char *) arg, len);
		break;
	}

	*pbuff = buff + size;
	*pfree -= size;
}

#define __UK_TRACE_GET_TYPE(arg) (					\
	__builtin_types_compatible_p(typeof(arg), const char *) *	\
		__UK_TRACE_ARG_STRING +					\
	__builtin_types_compatible_p(typeof(arg), char *) *		\
		__UK_TRACE_ARG_STRING +					\
	0)

#define __UK_TRACE_SAVE_ONE(arg) __uk_trace_save_arg(	\
		&buff,					\
		&free,					\
		__UK_TRACE_GET_TYPE(arg),		\
		sizeof(arg),				\
		(long) arg)

#define __UK_TRACE_SAVE_ARGS0()
#define __UK_TRACE_SAVE_ARGS1() __UK_TRACE_SAVE_ONE(arg1)
#define __UK_TRACE_SAVE_ARGS2() __UK_TRACE_SAVE_ARGS1(); __UK_TRACE_SAVE_ONE(arg2)
#define __UK_TRACE_SAVE_ARGS3() __UK_TRACE_SAVE_ARGS2(); __UK_TRACE_SAVE_ONE(arg3)
#define __UK_TRACE_SAVE_ARGS4() __UK_TRACE_SAVE_ARGS3(); __UK_TRACE_SAVE_ONE(arg4)
#define __UK_TRACE_SAVE_ARGS5() __UK_TRACE_SAVE_ARGS4(); __UK_TRACE_SAVE_ONE(arg5)
#define __UK_TRACE_SAVE_ARGS6() __UK_TRACE_SAVE_ARGS5(); __UK_TRACE_SAVE_ONE(arg6)
#define __UK_TRACE_SAVE_ARGS7() __UK_TRACE_SAVE_ARGS6(); __UK_TRACE_SAVE_ONE(arg7)

#define __UK_GET_ARG1(a1, ...) a1
#define __UK_GET_ARG2(a1, a2, ...) a2
#define __UK_GET_ARG3(a1, a2, a3, ...) a3
#define __UK_GET_ARG4(a1, a2, a3, a4, ...) a4
#define __UK_GET_ARG5(a1, a2, a3, a4, a5, ...) a5
#define __UK_GET_ARG6(a1, a2, a3, a4, a5, a6, ...) a6
#define __UK_GET_ARG7(a1, a2, a3, a4, a5, a6, a7) a7
/* Returns argument of a given number */
#define __UK_GET_ARG_N(n, ...)				\
	UK_CONCAT(__UK_GET_ARG, n)(__VA_ARGS__)

/* Calls given function with argument Nr n */
#define UK_1ARG_MAP(n, f, ...)		\
	f(n, __UK_GET_ARG_N(n, __VA_ARGS__))
#define UK_FOREACH0(f, ...)
#define UK_FOREACH1(f, ...) UK_1ARG_MAP(1, f, __VA_ARGS__)
#define UK_FOREACH2(f, ...) UK_FOREACH1(f, __VA_ARGS__), UK_1ARG_MAP(2, f, __VA_ARGS__)
#define UK_FOREACH3(f, ...) UK_FOREACH2(f, __VA_ARGS__), UK_1ARG_MAP(3, f, __VA_ARGS__)
#define UK_FOREACH4(f, ...) UK_FOREACH3(f, __VA_ARGS__), UK_1ARG_MAP(4, f, __VA_ARGS__)
#define UK_FOREACH5(f, ...) UK_FOREACH4(f, __VA_ARGS__), UK_1ARG_MAP(5, f, __VA_ARGS__)
#define UK_FOREACH6(f, ...) UK_FOREACH5(f, __VA_ARGS__), UK_1ARG_MAP(6, f, __VA_ARGS__)
#define UK_FOREACH7(f, ...) UK_FOREACH6(f, __VA_ARGS__), UK_1ARG_MAP(7, f, __VA_ARGS__)

/* TODO: FOREACH could be useful for other Unikraft modules. Consider
 * moving it to public headers */

/* Apply function/macro f to every argument passed. Resulting
 * arguments are separated with commas
 */
#define UK_FOREACH_N(n, f, ...)				\
	UK_CONCAT(UK_FOREACH, n)(f, __VA_ARGS__)

#define UK_FOREACH(f, ...)					\
	UK_FOREACH_N(UK_NARGS(__VA_ARGS__), f, __VA_ARGS__)

#define UK_FOREACH_SIZEOF(a, b) sizeof(b)
#define __UK_TRACE_ARG_SIZES(n, ...)				\
	{ UK_FOREACH(UK_FOREACH_SIZEOF, __VA_ARGS__) }

#define __UK_TRACE_GET_TYPE_FOREACH(n, arg)	\
	__UK_TRACE_GET_TYPE(arg)
#define __UK_TRACE_ARG_TYPES(n, ...)					\
	{ UK_FOREACH(__UK_TRACE_GET_TYPE_FOREACH, __VA_ARGS__) }

#define __UK_TRACE_REG(NR, regname, trace_name, fmt, ...)	\
	UK_CTASSERT(sizeof(#trace_name) < 255);			\
	UK_CTASSERT(sizeof(fmt) < 255);				\
	__attribute((__section__(".uk_tracepoints_list")))	\
	static struct {						\
		uint32_t magic;					\
		uint32_t size;					\
		uint64_t cookie;				\
		uint8_t args_nr;				\
		uint8_t name_len;				\
		uint8_t format_len;				\
		uint8_t sizes[NR];				\
		uint8_t types[NR];				\
		char name[sizeof(#trace_name)];			\
		char format[sizeof(fmt)];			\
	} regname __used = {					\
		UK_TP_DEF_MAGIC,				\
		sizeof(regname),				\
		(uint64_t) &regname,				\
		NR,						\
		sizeof(#trace_name), sizeof(fmt),		\
		__UK_TRACE_ARG_SIZES(NR, __VA_ARGS__),		\
		__UK_TRACE_ARG_TYPES(NR, __VA_ARGS__),		\
		#trace_name, fmt }

static inline char *__uk_trace_get_buff(size_t *free)
{
	struct uk_tracepoint_header *ret;

	if (uk_trace_buffer_free < sizeof(*ret))
		return 0;
	ret = (struct uk_tracepoint_header *) uk_trace_buffer_writep;

	/* In case we fail to fill the tracepoint for any reason, make
	 * sure we do not confuse parser. We fill the header only
	 * after the full tracepoint is completed
	 */
	ret->magic = 0;
	*free = uk_trace_buffer_free - sizeof(*ret);
	return (char *) (ret + 1);
}

static inline void __uk_trace_finalize_buff(char *new_buff_pos, void *cookie)
{
	uint32_t size;
	struct uk_tracepoint_header *head =
		(struct uk_tracepoint_header *) uk_trace_buffer_writep;

	size = new_buff_pos - uk_trace_buffer_writep;
	uk_trace_buffer_writep += size;
	uk_trace_buffer_free -= size;

	head->time = ukplat_monotonic_clock();
	head->size = size - sizeof(*head);
	head->cookie = cookie;
	barrier();
	head->magic = UK_TP_HEADER_MAGIC;
}

/* Makes from "const char*" "const char* arg1".
 */
#define __UK_ARGS_MAP_FN(n, t) t UK_CONCAT(arg, n)
#define __UK_TRACE_ARGS_MAP(n, ...) \
	UK_FOREACH(__UK_ARGS_MAP_FN, __VA_ARGS__)

#define __UK_ARGS_MAP_FN_UNUSED(n, t) t UK_CONCAT(arg, n) __unused
#define __UK_TRACE_ARGS_MAP_UNUSED(n, ...) \
	UK_FOREACH(__UK_ARGS_MAP_FN_UNUSED, __VA_ARGS__)


#if (defined(CONFIG_LIBUKDEBUG_TRACEPOINTS) &&				\
	(defined(UK_DEBUG_TRACE) || defined(CONFIG_LIBUKDEBUG_ALL_TRACEPOINTS)))
#define ____UK_TRACEPOINT(n, regdata_name, trace_name, fmt, ...)	\
	__UK_TRACE_REG(n, regdata_name, trace_name, fmt,		\
		       __VA_ARGS__);					\
	static inline void trace_name(__UK_TRACE_ARGS_MAP(n, __VA_ARGS__)) \
	{								\
		unsigned long flags = ukplat_lcpu_save_irqf();		\
		size_t free __maybe_unused;				\
		char *buff = __uk_trace_get_buff(&free);		\
		if (buff) {						\
			__UK_TRACE_SAVE_ARGS ## n();			\
			__uk_trace_finalize_buff(			\
				buff, &regdata_name);			\
		}							\
		ukplat_lcpu_restore_irqf(flags);			\
	}
#else
#define ____UK_TRACEPOINT(n, regdata_name, trace_name, fmt, ...)	\
	static inline void trace_name(					\
		__UK_TRACE_ARGS_MAP_UNUSED(n, __VA_ARGS__))		\
	{								\
	}
#endif

#define __UK_TRACEPOINT(n, regdata_name, trace_name, fmt, ...)	\
	____UK_TRACEPOINT(n, regdata_name, trace_name, fmt,	\
			  __VA_ARGS__)
#define UK_TRACEPOINT(trace_name, fmt, ...)				\
	__UK_TRACEPOINT(UK_NARGS(__VA_ARGS__),				\
			__ ## trace_name ## _regdata,			\
			trace_name, fmt, __VA_ARGS__)


#endif /* _UK_TRACE_H_ */

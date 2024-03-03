/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
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
 */

#ifndef __UK_SYSCALL_H__
#define __UK_SYSCALL_H__

#include <uk/arch/ctx.h>
#include <arch/syscall_prologue.h>

#if !__ASSEMBLY__
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <errno.h>
#include <stdarg.h>
#include <uk/print.h>
#include "legacy_syscall.h"
#include "../../arch/regmap_linuxabi.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Whenever the hidden Config.uk option LIBSYSCALL_SHIM_NOWRAPPER
 * is set, the creation of libc-style wrappers are disable by the
 * UK_SYSCALL_DEFINE() and UK_SYSCALL_R_DEFINE() macros. Alternatively,
 * UK_LIBC_SYSCALLS can be set to 0 through compilation flags.
 */
#ifndef UK_LIBC_SYSCALLS
#if CONFIG_LIBSYSCALL_SHIM && CONFIG_LIBSYSCALL_SHIM_NOWRAPPER
#define UK_LIBC_SYSCALLS (0)
#else
#define UK_LIBC_SYSCALLS (1)
#endif /* CONFIG_LIBSYSCALL_SHIM && CONFIG_LIBSYSCALL_SHIM_NOWRAPPER */
#endif /* UK_LIBC_SYSCALLS */

#define __uk_scc(X) ((long) (X))
typedef long uk_syscall_arg_t;

#define __UK_NAME2SCALLE_FN(name) UK_CONCAT(uk_syscall_e_, name)
#define __UK_NAME2SCALLR_FN(name) UK_CONCAT(uk_syscall_r_, name)

#define  UK_ARG_MAP0(...)
#define  UK_ARG_MAP2(m, type, arg) m(type, arg)
#define  UK_ARG_MAP4(m, type, arg, ...) m(type, arg), UK_ARG_MAP2(m, __VA_ARGS__)
#define  UK_ARG_MAP6(m, type, arg, ...) m(type, arg), UK_ARG_MAP4(m, __VA_ARGS__)
#define  UK_ARG_MAP8(m, type, arg, ...) m(type, arg), UK_ARG_MAP6(m, __VA_ARGS__)
#define UK_ARG_MAP10(m, type, arg, ...) m(type, arg), UK_ARG_MAP8(m, __VA_ARGS__)
#define UK_ARG_MAP12(m, type, arg, ...) m(type, arg), UK_ARG_MAP10(m, __VA_ARGS__)
#define UK_ARG_MAP14(m, type, arg, ...) m(type, arg), UK_ARG_MAP12(m, __VA_ARGS__)
#define UK_ARG_MAPx(nr_args, ...) UK_CONCAT(UK_ARG_MAP, nr_args)(__VA_ARGS__)

#define UK_EXECENV_CALLMAP0_0(...)
#define UK_EXECENV_CALLMAP2_2(m, type, arg)				\
	, (type)execenv->regs.__syscall_rarg0

#define UK_EXECENV_CALLMAP2_4(m, type, arg)				\
	, (type)execenv->regs.__syscall_rarg1
#define UK_EXECENV_CALLMAP4_4(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg0 UK_EXECENV_CALLMAP2_4(m, __VA_ARGS__)

#define UK_EXECENV_CALLMAP2_6(m, type, arg)				\
	, (type)execenv->regs.__syscall_rarg2
#define UK_EXECENV_CALLMAP4_6(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg1 UK_EXECENV_CALLMAP2_6(m, __VA_ARGS__)
#define UK_EXECENV_CALLMAP6_6(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg0 UK_EXECENV_CALLMAP4_6(m, __VA_ARGS__)

#define UK_EXECENV_CALLMAP2_8(m, type, arg)				\
	, (type)execenv->regs.__syscall_rarg3
#define UK_EXECENV_CALLMAP4_8(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg2 UK_EXECENV_CALLMAP2_8(m, __VA_ARGS__)
#define UK_EXECENV_CALLMAP6_8(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg1 UK_EXECENV_CALLMAP4_8(m, __VA_ARGS__)
#define UK_EXECENV_CALLMAP8_8(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg0 UK_EXECENV_CALLMAP6_8(m, __VA_ARGS__)

#define UK_EXECENV_CALLMAP2_10(m, type, arg)				\
	, (type)execenv->regs.__syscall_rarg4
#define UK_EXECENV_CALLMAP4_10(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg3 UK_EXECENV_CALLMAP2_10(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP6_10(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg2 UK_EXECENV_CALLMAP4_10(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP8_10(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg1 UK_EXECENV_CALLMAP6_10(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP10_10(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg0 UK_EXECENV_CALLMAP8_10(m,	__VA_ARGS__)

#define UK_EXECENV_CALLMAP2_12(m, type, arg)				\
	, (type)execenv->regs.__syscall_rarg5
#define UK_EXECENV_CALLMAP4_12(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg4 UK_EXECENV_CALLMAP2_12(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP6_12(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg3 UK_EXECENV_CALLMAP4_12(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP8_12(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg2 UK_EXECENV_CALLMAP6_12(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP10_12(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg1 UK_EXECENV_CALLMAP8_12(m,	__VA_ARGS__)
#define UK_EXECENV_CALLMAP12_12(m, type, arg, ...)			\
	, (type)execenv->regs.__syscall_rarg0 UK_EXECENV_CALLMAP10_12(m, __VA_ARGS__)
#define UK_EXECENV_CALLMAPx(nr_args, ...)				\
	execenv UK_CONCAT(UK_CONCAT(UK_EXECENV_CALLMAP, nr_args),	\
			  _##nr_args)(__VA_ARGS__)

#define UK_EXECENV_EMAP0_0(...)
#define UK_EXECENV_EMAP2_2(m, type, arg) , (long)execenv->regs.__syscall_rarg0

#define UK_EXECENV_EMAP2_4(m, type, arg) , (long)execenv->regs.__syscall_rarg1
#define UK_EXECENV_EMAP4_4(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg0 UK_EXECENV_EMAP2_4(m, __VA_ARGS__)

#define UK_EXECENV_EMAP2_6(m, type, arg) , (long)execenv->regs.__syscall_rarg2
#define UK_EXECENV_EMAP4_6(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg1 UK_EXECENV_EMAP2_6(m, __VA_ARGS__)
#define UK_EXECENV_EMAP6_6(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg0 UK_EXECENV_EMAP4_6(m, __VA_ARGS__)

#define UK_EXECENV_EMAP2_8(m, type, arg) , (long)execenv->regs.__syscall_rarg3
#define UK_EXECENV_EMAP4_8(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg2 UK_EXECENV_EMAP2_8(m, __VA_ARGS__)
#define UK_EXECENV_EMAP6_8(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg1 UK_EXECENV_EMAP4_8(m, __VA_ARGS__)
#define UK_EXECENV_EMAP8_8(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg0 UK_EXECENV_EMAP6_8(m, __VA_ARGS__)

#define UK_EXECENV_EMAP2_10(m, type, arg) , (long)execenv->regs.__syscall_rarg4
#define UK_EXECENV_EMAP4_10(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg3 UK_EXECENV_EMAP2_10(m, __VA_ARGS__)
#define UK_EXECENV_EMAP6_10(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg2 UK_EXECENV_EMAP4_10(m, __VA_ARGS__)
#define UK_EXECENV_EMAP8_10(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg1 UK_EXECENV_EMAP6_10(m, __VA_ARGS__)
#define UK_EXECENV_EMAP10_10(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg0 UK_EXECENV_EMAP8_10(m, __VA_ARGS__)

#define UK_EXECENV_EMAP2_12(m, type, arg) , (long)execenv->regs.__syscall_rarg5
#define UK_EXECENV_EMAP4_12(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg4 UK_EXECENV_EMAP2_12(m, __VA_ARGS__)
#define UK_EXECENV_EMAP6_12(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg3 UK_EXECENV_EMAP4_12(m, __VA_ARGS__)
#define UK_EXECENV_EMAP8_12(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg2 UK_EXECENV_EMAP6_12(m, __VA_ARGS__)
#define UK_EXECENV_EMAP10_12(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg1 UK_EXECENV_EMAP8_12(m, __VA_ARGS__)
#define UK_EXECENV_EMAP12_12(m, type, arg, ...)				\
	, (long)execenv->regs.__syscall_rarg0 UK_EXECENV_EMAP10_12(m, __VA_ARGS__)
#define UK_EXECENV_EMAPx(nr_args, ...)					\
	(long)execenv UK_CONCAT(UK_CONCAT(UK_EXECENV_EMAP, nr_args),		\
		      _##nr_args)(__VA_ARGS__)

/* Variant of UK_ARG_MAPx() but prepends a comma if nr_args > 0 */
#define  UK_ARG_EMAP0(...)
#define  UK_ARG_EMAP2(m, type, arg) , m(type, arg)
#define  UK_ARG_EMAP4(m, type, arg, ...) , m(type, arg), UK_ARG_MAP2(m, __VA_ARGS__)
#define  UK_ARG_EMAP6(m, type, arg, ...) , m(type, arg), UK_ARG_MAP4(m, __VA_ARGS__)
#define  UK_ARG_EMAP8(m, type, arg, ...) , m(type, arg), UK_ARG_MAP6(m, __VA_ARGS__)
#define UK_ARG_EMAP10(m, type, arg, ...) , m(type, arg), UK_ARG_MAP8(m, __VA_ARGS__)
#define UK_ARG_EMAP12(m, type, arg, ...) , m(type, arg), UK_ARG_MAP10(m, __VA_ARGS__)
#define UK_ARG_EMAP14(m, type, arg, ...) , m(type, arg), UK_ARG_MAP12(m, __VA_ARGS__)
#define UK_ARG_EMAPx(nr_args, ...) UK_CONCAT(UK_ARG_EMAP, nr_args)(__VA_ARGS__)

#define UK_S_ARG_LONG(type, arg)   long arg
#define UK_S_ARG_ACTUAL(type, arg) type arg
#define UK_S_ARG_LONG_MAYBE_UNUSED(type, arg)   long arg __maybe_unused
#define UK_S_ARG_ACTUAL_MAYBE_UNUSED(type, arg) type arg __maybe_unused
#define UK_S_ARG_CAST_LONG(type, arg)   (long) arg
#define UK_S_ARG_CAST_ACTUAL(type, arg) (type) arg
#define UK_S_EXECENV_ARG_ACTUAL	struct ukarch_execenv *execenv
#define UK_S_EXECENV_ARG_ACTUAL_MAYBE_UNUSED				\
	struct ukarch_execenv *execenv __maybe_unused

#define UK_EXECENV_DECLMAPx(__syscall_rarg, nr_args, ...)			\
	__syscall_rarg UK_ARG_EMAPx(nr_args, __VA_ARGS__)

#if CONFIG_LIBSYSCALL_SHIM_DEBUG_SYSCALLS || CONFIG_LIBUKDEBUG_PRINTD
#define UK_ARG_FMT_MAP0(...)
#define UK_ARG_FMT_MAP2(m, type, arg) m(type, arg)
#define UK_ARG_FMT_MAP4(m, type, arg, ...) m(type, arg) ", " UK_ARG_FMT_MAP2(m, __VA_ARGS__)
#define UK_ARG_FMT_MAP6(m, type, arg, ...) m(type, arg) ", " UK_ARG_FMT_MAP4(m, __VA_ARGS__)
#define UK_ARG_FMT_MAP8(m, type, arg, ...) m(type, arg) ", " UK_ARG_FMT_MAP6(m, __VA_ARGS__)
#define UK_ARG_FMT_MAP10(m, type, arg, ...) m(type, arg) ", " UK_ARG_FMT_MAP8(m, __VA_ARGS__)
#define UK_ARG_FMT_MAP12(m, type, arg, ...) m(type, arg) ", " UK_ARG_FMT_MAP10(m, __VA_ARGS__)
#define UK_ARG_FMT_MAP14(m, type, arg, ...) m(type, arg) ", " UK_ARG_FMT_MAP12(m, __VA_ARGS__)
#define UK_ARG_FMT_MAPx(nr_args, ...) UK_CONCAT(UK_ARG_FMT_MAP, nr_args)(__VA_ARGS__)

#define UK_S_ARG_FMT_LONG(type, arg)  "(" STRINGIFY(type) ") %ld"
#define UK_S_ARG_FMT_LONGX(type, arg)  "(" STRINGIFY(type) ") 0x%lx"

#define __UK_SYSCALL_PRINTD(x, rtype, fname, ...)			\
	_uk_printd(uk_libid_self(), __STR_BASENAME__, __LINE__,		\
		   "(" STRINGIFY(rtype) ") " STRINGIFY(fname)		\
		   "(" UK_ARG_FMT_MAPx(x, UK_S_ARG_FMT_LONGX, __VA_ARGS__) ")\n" \
		   UK_ARG_EMAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__) )

#define __UK_SYSCALL_EXECENV_PRINTD(x, rtype, fname, ...)			\
	uk_printd("\nInvoking context saving %s system call.\n",	\
		  STRINGIFY(fname));					\
	_uk_printd(uk_libid_self(), __STR_BASENAME__, __LINE__,		\
		   "(" STRINGIFY(rtype) ") " STRINGIFY(fname)		\
		   "( execenv 0x%lx, " UK_ARG_FMT_MAPx(x,		\
						       UK_S_ARG_FMT_LONGX,\
						       __VA_ARGS__) ")\n",\
		   UK_EXECENV_EMAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__))
#else
#define __UK_SYSCALL_PRINTD(...) do {} while(0)
#define __UK_SYSCALL_EXECENV_PRINTD(...) do {} while(0)
#endif /* CONFIG_LIBSYSCALL_SHIM_DEBUG || CONFIG_LIBUKDEBUG_PRINTD */

/* System call implementation that uses errno and returns -1 on errors */
/* TODO: `void` as return type is currently not supported.
 * NOTE: Workaround is to use `int` instead.
 */
/*
 * UK_LLSYSCALL_DEFINE()
 * Low-level variant, does not provide a libc-style wrapper
 */
#define __UK_LLSYSCALL_DEFINE(x, rtype, name, ename, rname, ...)	\
	long ename(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__));		\
	long rname(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__))		\
	{								\
		int _errno = errno;					\
		long ret;						\
									\
		errno = 0;						\
		ret = ename(						\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
		if (ret == -1)						\
			ret = errno ? -errno : -EFAULT;			\
		errno = _errno;						\
		return ret;						\
	}								\
	static inline rtype __##ename(UK_ARG_MAPx(x,			\
					UK_S_ARG_ACTUAL, __VA_ARGS__)); \
	long ename(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__))		\
	{								\
		long ret;						\
									\
		__UK_SYSCALL_PRINTD(x, rtype, ename, __VA_ARGS__);	\
		ret = (long) __##ename(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_ACTUAL, __VA_ARGS__)); \
		return ret;						\
	}								\
	static inline rtype __##ename(UK_ARG_MAPx(x,			\
						  UK_S_ARG_ACTUAL_MAYBE_UNUSED,\
						  __VA_ARGS__))
#define _UK_LLSYSCALL_DEFINE(...) __UK_LLSYSCALL_DEFINE(__VA_ARGS__)
#define UK_LLSYSCALL_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_DEFINE(UK_NARGS(__VA_ARGS__),			\
			     rtype,					\
			     name,					\
			     __UK_NAME2SCALLE_FN(name),			\
			     __UK_NAME2SCALLR_FN(name),			\
			     __VA_ARGS__)

/*
 * UK_SYSCALL_DEFINE()
 * Based on UK_LLSYSCALL_DEFINE and provides a libc-style wrapper
 * in case UK_LIBC_SYSCALLS is enabled
 */
#if UK_LIBC_SYSCALLS
#define __UK_SYSCALL_DEFINE(x, rtype, name, ename, rname, ...)		\
	long ename(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__));		\
	rtype name(UK_ARG_MAPx(x, UK_S_ARG_ACTUAL, __VA_ARGS__))	\
	{								\
		rtype ret;						\
									\
		ret = (rtype) ename(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
		return ret;						\
	}								\
	__UK_LLSYSCALL_DEFINE(x, rtype, name, ename, rname, __VA_ARGS__)
#define _UK_SYSCALL_DEFINE(...) __UK_SYSCALL_DEFINE(__VA_ARGS__)
#define UK_SYSCALL_DEFINE(rtype, name, ...)				\
	_UK_SYSCALL_DEFINE(UK_NARGS(__VA_ARGS__),			\
			   rtype,					\
			   name,					\
			   __UK_NAME2SCALLE_FN(name),			\
			   __UK_NAME2SCALLR_FN(name),			\
			   __VA_ARGS__)
#else
#define UK_SYSCALL_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_DEFINE(UK_NARGS(__VA_ARGS__),			\
			     rtype,					\
			     name,					\
			     __UK_NAME2SCALLE_FN(name),			\
			     __UK_NAME2SCALLR_FN(name),			\
			     __VA_ARGS__)
#endif /* UK_LIBC_SYSCALLS */

/* Raw system call implementation that is returning negative codes on errors */
/* TODO: `void` as return type is currently not supported.
 * NOTE: Workaround is to use `int` instead.
 */
/*
 * UK_LLSYSCALL_R_DEFINE()
 * Low-level variant, does not provide a libc-style wrapper
 */
#define __UK_LLSYSCALL_R_DEFINE(x, rtype, name, ename, rname, ...)	\
	long rname(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__));		\
	long ename(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__))		\
	{								\
		long ret;						\
									\
		ret = rname(						\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
		if (ret < 0 && PTRISERR(ret)) {				\
			errno = -(int) PTR2ERR(ret);			\
			return -1;					\
		}							\
		return ret;						\
	}								\
	static inline rtype __##rname(UK_ARG_MAPx(x, UK_S_ARG_ACTUAL,	\
						 __VA_ARGS__));		\
	long rname(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__))		\
	{								\
		long ret;						\
									\
		__UK_SYSCALL_PRINTD(x, rtype, rname, __VA_ARGS__);	\
		ret = (long) __##rname(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_ACTUAL, __VA_ARGS__)); \
		return ret;						\
	}								\
	static inline rtype __##rname(UK_ARG_MAPx(x,			\
						  UK_S_ARG_ACTUAL_MAYBE_UNUSED,\
						  __VA_ARGS__))
#define _UK_LLSYSCALL_R_DEFINE(...) __UK_LLSYSCALL_R_DEFINE(__VA_ARGS__)
#define UK_LLSYSCALL_R_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_R_DEFINE(UK_NARGS(__VA_ARGS__),			\
			       rtype,					\
			       name,					\
			       __UK_NAME2SCALLE_FN(name),		\
			       __UK_NAME2SCALLR_FN(name),		\
			       __VA_ARGS__)

#define __UK_LLSYSCALL_R_E_DEFINE(x, rtype, name, ename, rname, ...)	\
	long rname(long _execenv);					\
	long __used ename(long _execenv)				\
	{								\
		long ret;						\
									\
		ret = rname(_execenv);					\
		if (ret < 0 && PTRISERR(ret)) {				\
			errno = -(int) PTR2ERR(ret);			\
			return -1;					\
		}							\
		return ret;						\
	}								\
	static inline rtype __##rname(UK_EXECENV_DECLMAPx(UK_S_EXECENV_ARG_ACTUAL,\
						      x, UK_S_ARG_ACTUAL,\
						      __VA_ARGS__));	\
	long __used rname(long _execenv)					\
	{								\
		struct ukarch_execenv *execenv;				\
		long ret;						\
									\
		execenv = (struct ukarch_execenv *)_execenv;		\
		__UK_SYSCALL_EXECENV_PRINTD(x, rtype, rname,		\
					__VA_ARGS__);			\
		ret = (long) __##rname(UK_EXECENV_CALLMAPx(x,		\
						   UK_S_ARG_ACTUAL,	\
						   __VA_ARGS__));	\
		return ret;						\
	}								\
	static inline rtype __used __##rname(UK_EXECENV_DECLMAPx(		\
					     UK_S_EXECENV_ARG_ACTUAL_MAYBE_UNUSED,\
					     x, UK_S_ARG_ACTUAL_MAYBE_UNUSED,\
					     __VA_ARGS__))
#define _UK_LLSYSCALL_R_E_DEFINE(...) __UK_LLSYSCALL_R_E_DEFINE(__VA_ARGS__)
#define UK_LLSYSCALL_R_E_DEFINE(rtype, name, ...)			\
	UK_SYSCALL_EXECENV_PROLOGUE_DEFINE(__UK_NAME2SCALLE_FN(name),	\
				       __UK_NAME2SCALLE_FN(e_##name),	\
				       UK_NARGS(__VA_ARGS__),		\
				       __VA_ARGS__)			\
	UK_SYSCALL_EXECENV_PROLOGUE_DEFINE(__UK_NAME2SCALLR_FN(name),	\
				       __UK_NAME2SCALLR_FN(e_##name),	\
				       UK_NARGS(__VA_ARGS__),		\
				       __VA_ARGS__)			\
	_UK_LLSYSCALL_R_E_DEFINE(UK_NARGS(__VA_ARGS__),			\
				 rtype,					\
				 name,					\
				 __UK_NAME2SCALLE_FN(e_##name),		\
				 __UK_NAME2SCALLR_FN(e_##name),		\
				 __VA_ARGS__)

/*
 * UK_SYSCALL_R_DEFINE()
 * Based on UK_LLSYSCALL_R_DEFINE and provides a libc-style wrapper
 * in case UK_LIBC_SYSCALLS is enabled
 */
#if UK_LIBC_SYSCALLS
#define __UK_SYSCALL_R_DEFINE(x, rtype, name, ename, rname, ...)	\
	long ename(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__));		\
	rtype name(UK_ARG_MAPx(x, UK_S_ARG_ACTUAL, __VA_ARGS__))	\
	{								\
		rtype ret;						\
									\
		ret = (rtype) ename(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
		return ret;						\
	}								\
	__UK_LLSYSCALL_R_DEFINE(x, rtype, name, ename, rname, __VA_ARGS__)
#define _UK_SYSCALL_R_DEFINE(...) __UK_SYSCALL_R_DEFINE(__VA_ARGS__)
#define UK_SYSCALL_R_DEFINE(rtype, name, ...)				\
	_UK_SYSCALL_R_DEFINE(UK_NARGS(__VA_ARGS__),			\
			     rtype,					\
			     name,					\
			     __UK_NAME2SCALLE_FN(name),			\
			     __UK_NAME2SCALLR_FN(name),			\
			     __VA_ARGS__)
#else
#define UK_SYSCALL_R_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_R_DEFINE(UK_NARGS(__VA_ARGS__),			\
			       rtype,					\
			       name,					\
			       __UK_NAME2SCALLE_FN(name),		\
			       __UK_NAME2SCALLR_FN(name),		\
			       __VA_ARGS__)
#endif /* UK_LIBC_SYSCALLS */


#define __UK_SPROTO_ARGS_TYPE long
#define __UK_SPROTO_ARGS0()  void
#define __UK_SPROTO_ARGS1()  __UK_SPROTO_ARGS_TYPE a
#define __UK_SPROTO_ARGS2()  __UK_SPROTO_ARGS1(), __UK_SPROTO_ARGS_TYPE b
#define __UK_SPROTO_ARGS3()  __UK_SPROTO_ARGS2(), __UK_SPROTO_ARGS_TYPE c
#define __UK_SPROTO_ARGS4()  __UK_SPROTO_ARGS3(), __UK_SPROTO_ARGS_TYPE d
#define __UK_SPROTO_ARGS5()  __UK_SPROTO_ARGS4(), __UK_SPROTO_ARGS_TYPE e
#define __UK_SPROTO_ARGS6()  __UK_SPROTO_ARGS5(), __UK_SPROTO_ARGS_TYPE f
#define __UK_SPROTO_ARGS7()  __UK_SPROTO_ARGS6(), __UK_SPROTO_ARGS_TYPE g
#define __UK_SPROTO_ARGSx(args_nr)  \
	UK_CONCAT(__UK_SPROTO_ARGS, args_nr)()

#define UK_SYSCALL_E_PROTO(args_nr, syscall_name)			\
	long __UK_NAME2SCALLE_FN(syscall_name)(__UK_SPROTO_ARGSx(args_nr))
#define UK_SYSCALL_R_PROTO(args_nr, syscall_name)			\
	long __UK_NAME2SCALLR_FN(syscall_name)(__UK_SPROTO_ARGSx(args_nr))

#define uk_syscall_e_stub(syscall_name) ({				\
			uk_pr_debug("System call \"" syscall_name	\
				    "\" is not available (-ENOSYS)\n");	\
			errno = -ENOSYS;				\
			-1;						\
		})

#define uk_syscall_r_stub(syscall_name) ({				\
			uk_pr_debug("System call \"" syscall_name	\
				    "\" is not available (-ENOSYS)\n");	\
			-ENOSYS;					\
		})


#ifdef CONFIG_LIBSYSCALL_SHIM
#include <uk/bits/syscall_nrs.h>
#include <uk/bits/syscall_map.h>
#include <uk/bits/syscall_provided.h>
#include <uk/bits/syscall_stubs.h>
#include <uk/bits/syscall_static.h>
#include <uk/bits/syscall_r_static.h>

/* System call, returns -1 and sets errno on errors */
long uk_syscall(long nr, ...);
long uk_vsyscall(long nr, va_list arg);
long uk_syscall6(long nr, long arg1, long arg2, long arg3,
		 long arg4, long arg5, long arg6);

/*
 * Use this variant instead of `uk_syscall()` whenever the system call number
 * is a constant. This macro maps the function call directly to the target
 * handler instead of doing a look-up at runtime
 */
#define uk_syscall_static0(syscall_nr, ...) \
	UK_CONCAT(uk_syscall0_fn, syscall_nr)()
#define uk_syscall_static1(syscall_nr, a) \
	UK_CONCAT(uk_syscall1_fn, syscall_nr)(a)
#define uk_syscall_static2(syscall_nr, a, b) \
	UK_CONCAT(uk_syscall2_fn, syscall_nr)(a, b)
#define uk_syscall_static3(syscall_nr, a, b, c) \
	UK_CONCAT(uk_syscall3_fn, syscall_nr)(a, b, c)
#define uk_syscall_static4(syscall_nr, a, b, c, d) \
	UK_CONCAT(uk_syscall4_fn, syscall_nr)(a, b, c, d)
#define uk_syscall_static5(syscall_nr, a, b, c, d, e) \
	UK_CONCAT(uk_syscall5_fn, syscall_nr)(a, b, c, d, e)
#define uk_syscall_static6(syscall_nr, a, b, c, d, e, f) \
	UK_CONCAT(uk_syscall6_fn, syscall_nr)(a, b, c, d, e, f)

#define uk_syscall_static(syscall_nr, ...)			\
	UK_CONCAT(uk_syscall_static,				\
		  UK_NARGS(__VA_ARGS__))(syscall_nr, __VA_ARGS__)

/* Raw system call, returns negative codes on errors */
long uk_syscall_r(long nr, ...);
long uk_vsyscall_r(long nr, va_list arg);
long uk_syscall6_r_e(struct ukarch_execenv *execenv);
long uk_syscall6_r(long nr, long arg1, long arg2, long arg3,
		   long arg4, long arg5, long arg6);

/*
 * Use this variant instead of `uk_syscall_r()` whenever the system call number
 * is a constant. This macro maps the function call directly to the target
 * handler instead of doing a look-up at runtime
 */
#define uk_syscall_r_static0(syscall_nr, ...) \
	UK_CONCAT(uk_syscall_r0_fn, syscall_nr)()
#define uk_syscall_r_static1(syscall_nr, a) \
	UK_CONCAT(uk_syscall_r1_fn, syscall_nr)(a)
#define uk_syscall_r_static2(syscall_nr, a, b) \
	UK_CONCAT(uk_syscall_r2_fn, syscall_nr)(a, b)
#define uk_syscall_r_static3(syscall_nr, a, b, c) \
	UK_CONCAT(uk_syscall_r3_fn, syscall_nr)(a, b, c)
#define uk_syscall_r_static4(syscall_nr, a, b, c, d) \
	UK_CONCAT(uk_syscall_r4_fn, syscall_nr)(a, b, c, d)
#define uk_syscall_r_static5(syscall_nr, a, b, c, d, e) \
	UK_CONCAT(uk_syscall_r5_fn, syscall_nr)(a, b, c, d, e)
#define uk_syscall_r_static6(syscall_nr, a, b, c, d, e, f) \
	UK_CONCAT(uk_syscall_r6_fn, syscall_nr)(a, b, c, d, e, f)

#define uk_syscall_r_static(syscall_nr, ...)			\
	UK_CONCAT(uk_syscall_r_static,				\
		  UK_NARGS(__VA_ARGS__))(syscall_nr, __VA_ARGS__)

/**
 * Returns a string with the name of the system call number `nr`.
 *
 * @param nr
 *  System call number of current architecture
 * @return
 *  - (const char *): name of system call
 *  - (NULL): if system call number is unknown
 */
const char *uk_syscall_name(long nr);

/**
 * Returns a string with the name of the system call number `nr`.
 * This function is similar to `uk_syscall_name` but it uses
 * a smaller lookup table internally. This table contains only
 * system call name mappings of provided calls.
 *
 * @param nr
 *  System call number of current architecture
 * @return
 *  - (const char *): name of provided system call
 *  - (NULL): if system call is not provided
 */
const char *uk_syscall_name_p(long nr);

/*
 * Format flags for system call print functions `uk_snprsyscall()`  and
 * `uk_vsnprsyscall()`
 */
/* Append a newline at the end of the generated string */
#define UK_PRSYSCALL_FMTF_NEWLINE   0x1
/* Apply syntax highlighting with ANSI color sequences */
#define UK_PRSYSCALL_FMTF_ANSICOLOR 0x2

/**
 * Pretty prints a system call request and response to a given C-string buffer.
 * The function ensures that the generated string is NULL terminated. The
 * function truncates the output string if there is not enough space on the
 * target buffer.
 *
 * @param buf
 *  Reference to a buffer where the resulting string is stored
 * @param maxlen
 *  Maximum length that can be used omn the buffer. If it is shorter than
 *  the generated string, the string will be truncated but NULL-terminated
 *  to fit into the buffer
 * @param fmtf
 *  Format flags that influence the generated string
 * @param syscall_num
 *  The system call number
 * @param sysret
 *  The return code of the system call
 * @param ...
 *  The system call arguments, each of it has to be passed as `long`
 * @return
 *  Number of characters of the generated string written to the buffer
 *  (excluding terminating '\0')
 */
int uk_snprsyscall(char *buf, __sz maxlen, int fmtf, long syscall_num,
		   long sysret, ...);

/**
 * Pretty prints a system call request and response to a given C-string buffer.
 * The function ensures that the generated string is NULL terminated. The
 * function truncates the output string if there is not enough space on the
 * target buffer.
 *
 * @param buf
 *  Reference to a buffer where the resulting string is stored
 * @param maxlen
 *  Maximum length that can be used omn the buffer. If it is shorter than
 *  the generated string, the string will be truncated but NULL-terminated
 *  to fit into the buffer
 * @param fmtf
 *  Format flags that influence the generated string
 * @param syscall_num
 *  The system call number
 * @param sysret
 *  The return code of the system call
 * @param args
 *  Variadic list of the system call arguments, each of it has to be passed
 *  as `long`
 * @return
 *  Number of bytes of the generated string written to the buffer
 *  (excluding terminating '\0')
 */
int uk_vsnprsyscall(char *buf, __sz maxlen, int fmtf, long syscall_num,
		    long sysret, va_list args);

#endif /* CONFIG_LIBSYSCALL_SHIM */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_SYSCALL_H__ */

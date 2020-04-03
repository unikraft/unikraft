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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_SYSCALL_H__
#define __UK_SYSCALL_H__

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <errno.h>
#include <stdarg.h>
#include <uk/print.h>

/*
 * Whenever the hidden Config.uk option LIBSYSCALL_SHIM_NOWRAPPER
 * is set, the creation of libc-style wrappers are disable by the
 * UK_SYSCALL_DEFINE() and UK_SYSCALL_R_DEFINE() macros. Alternatively,
 * UK_LIBC_SYSCALLS can be set to 0 through compilation flags.
 */
#ifndef UK_LIBC_SYSCALLS
#if CONFIG_LIBSYSCALL_SHIM_NOWRAPPER
#define UK_LIBC_SYSCALLS (0)
#else
#define UK_LIBC_SYSCALLS (1)
#endif /* CONFIG_LIBSYSCALL_SHIM_NOWRAPPER */
#endif /* UK_LIBC_SYSCALLS */

#define __uk_scc(X) ((long) (X))
typedef long uk_syscall_arg_t;

#define __uk_syscall_fn(syscall_nr, ...) \
	UK_CONCAT(uk_syscall_fn_, syscall_nr) (__VA_ARGS__)
#define __uk_syscall_r_fn(syscall_nr, ...) \
	UK_CONCAT(uk_syscall_r_fn_, syscall_nr) (__VA_ARGS__)

#define __uk_syscall0(n) __uk_syscall_fn(n)
#define __uk_syscall1(n,a) __uk_syscall_fn(n,__uk_scc(a))
#define __uk_syscall2(n,a,b) __uk_syscall_fn(n,__uk_scc(a),__uk_scc(b))
#define __uk_syscall3(n,a,b,c) __uk_syscall_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c))
#define __uk_syscall4(n,a,b,c,d) __uk_syscall_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d))
#define __uk_syscall5(n,a,b,c,d,e) __uk_syscall_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e))
#define __uk_syscall6(n,a,b,c,d,e,f) __uk_syscall_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e),__uk_scc(f))
#define __uk_syscall7(n,a,b,c,d,e,f,g) __uk_syscall_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e),__uk_scc(f),__uk_scc(g))

#define __uk_syscall0_r(n) __uk_syscall_r_fn(n)
#define __uk_syscall1_r(n,a) __uk_syscall_r_fn(n,__uk_scc(a))
#define __uk_syscall2_r(n,a,b) __uk_syscall_r_fn(n,__uk_scc(a),__uk_scc(b))
#define __uk_syscall3_r(n,a,b,c) __uk_syscall_r_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c))
#define __uk_syscall4_r(n,a,b,c,d) __uk_syscall_r_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d))
#define __uk_syscall5_r(n,a,b,c,d,e) __uk_syscall_r_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e))
#define __uk_syscall6_r(n,a,b,c,d,e,f) __uk_syscall_r_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e),__uk_scc(f))
#define __uk_syscall7_r(n,a,b,c,d,e,f,g) __uk_syscall_r_fn(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e),__uk_scc(f),__uk_scc(g))

#define __UK_SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
#define __UK_SYSCALL_NARGS(...) __UK_SYSCALL_NARGS_X(__VA_ARGS__,7,6,5,4,3,2,1,0,)

#define __UK_SYSCALL_DEF_NARGS_X(z, a1,a2, b1,b2, c1,c2, d1,d2, e1,e2, f1,f2, g1,g2, nr, ...) nr
#define __UK_SYSCALL_DEF_NARGS(...) __UK_SYSCALL_DEF_NARGS_X(__VA_ARGS__, 7,7, 6,6, 5,5, 4,4, 3,3, 2,2, 1,1,0)

#define __UK_NAME2SCALLE_FN(name) UK_CONCAT(uk_syscall_e_, name)
#define __UK_NAME2SCALLR_FN(name) UK_CONCAT(uk_syscall_r_, name)

#define UK_ARG_MAP1(m, type, arg) m(type, arg)
#define UK_ARG_MAP2(m, type, arg, ...) m(type, arg), UK_ARG_MAP1(m, __VA_ARGS__)
#define UK_ARG_MAP3(m, type, arg, ...) m(type, arg), UK_ARG_MAP2(m, __VA_ARGS__)
#define UK_ARG_MAP4(m, type, arg, ...) m(type, arg), UK_ARG_MAP3(m, __VA_ARGS__)
#define UK_ARG_MAP5(m, type, arg, ...) m(type, arg), UK_ARG_MAP4(m, __VA_ARGS__)
#define UK_ARG_MAP6(m, type, arg, ...) m(type, arg), UK_ARG_MAP5(m, __VA_ARGS__)
#define UK_ARG_MAP7(m, type, arg, ...) m(type, arg), UK_ARG_MAP6(m, __VA_ARGS__)
#define UK_ARG_MAPx(nr_args, ...) UK_CONCAT(UK_ARG_MAP, nr_args)(__VA_ARGS__)

#define UK_S_ARG_LONG(type, arg)   long arg
#define UK_S_ARG_ACTUAL(type, arg) type arg
#define UK_S_ARG_CAST_LONG(type, arg)   (long) arg
#define UK_S_ARG_CAST_ACTUAL(type, arg) (type) arg

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
		return (long) __##ename(				\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_ACTUAL, __VA_ARGS__)); \
	}								\
	static inline rtype __##ename(UK_ARG_MAPx(x,			\
						  UK_S_ARG_ACTUAL, __VA_ARGS__))
#define _UK_LLSYSCALL_DEFINE(...) __UK_LLSYSCALL_DEFINE(__VA_ARGS__)
#define UK_LLSYSCALL_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_DEFINE(__UK_SYSCALL_DEF_NARGS(__VA_ARGS__),	\
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
		return (rtype) ename(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
	}								\
	__UK_LLSYSCALL_DEFINE(x, rtype, name, ename, rname, __VA_ARGS__)
#define _UK_SYSCALL_DEFINE(...) __UK_SYSCALL_DEFINE(__VA_ARGS__)
#define UK_SYSCALL_DEFINE(rtype, name, ...)				\
	_UK_SYSCALL_DEFINE(__UK_SYSCALL_DEF_NARGS(__VA_ARGS__),		\
			   rtype,					\
			   name,					\
			   __UK_NAME2SCALLE_FN(name),			\
			   __UK_NAME2SCALLR_FN(name),			\
			   __VA_ARGS__)
#else
#define UK_SYSCALL_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_DEFINE(__UK_SYSCALL_DEF_NARGS(__VA_ARGS__),	\
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
		long ret = rname(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
		if (ret < 0 && PTRISERR(ret)) {				\
			errno = (int) PTR2ERR(ret);			\
			return -1;					\
		}							\
		return ret;						\
	}								\
	static inline rtype __##rname(UK_ARG_MAPx(x, UK_S_ARG_ACTUAL,	\
						 __VA_ARGS__));		\
	long rname(UK_ARG_MAPx(x, UK_S_ARG_LONG, __VA_ARGS__))		\
	{								\
		return (long) __##rname(				\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_ACTUAL, __VA_ARGS__)); \
	}								\
	static inline rtype __##rname(UK_ARG_MAPx(x, UK_S_ARG_ACTUAL,	\
						 __VA_ARGS__))
#define _UK_LLSYSCALL_R_DEFINE(...) __UK_LLSYSCALL_R_DEFINE(__VA_ARGS__)
#define UK_LLSYSCALL_R_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_R_DEFINE(__UK_SYSCALL_DEF_NARGS(__VA_ARGS__),	\
			       rtype,					\
			       name,					\
			       __UK_NAME2SCALLE_FN(name),		\
			       __UK_NAME2SCALLR_FN(name),		\
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
		return (rtype) ename(					\
			UK_ARG_MAPx(x, UK_S_ARG_CAST_LONG, __VA_ARGS__)); \
	}								\
	__UK_LLSYSCALL_R_DEFINE(x, rtype, name, ename, rname, __VA_ARGS__)
#define _UK_SYSCALL_R_DEFINE(...) __UK_SYSCALL_R_DEFINE(__VA_ARGS__)
#define UK_SYSCALL_R_DEFINE(rtype, name, ...)				\
	_UK_SYSCALL_R_DEFINE(__UK_SYSCALL_DEF_NARGS(__VA_ARGS__),	\
			     rtype,					\
			     name,					\
			     __UK_NAME2SCALLE_FN(name),			\
			     __UK_NAME2SCALLR_FN(name),			\
			     __VA_ARGS__)
#else
#define UK_SYSCALL_R_DEFINE(rtype, name, ...)				\
	_UK_LLSYSCALL_R_DEFINE(__UK_SYSCALL_DEF_NARGS(__VA_ARGS__),	\
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
#include <uk/bits/provided_syscalls.h>
#include <uk/bits/syscall_stubs.h>

/* System call, returns -1 and sets errno on errors */
long uk_syscall(long n, ...);
long uk_vsyscall(long n, va_list arg);

/*
 * Use this variant instead of `uk_syscall()` whenever the system call number
 * is a constant. This macro maps the function call directly to the target
 * handler instead of doing a look-up at runtime
 */
#define uk_syscall_static(...)						\
	UK_CONCAT(__uk_syscall, __UK_SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

/* Raw system call, returns negative codes on errors */
long uk_syscall_r(long n, ...);
long uk_vsyscall_r(long n, va_list arg);

/*
 * Use this variant instead of `uk_syscall_r()` whenever the system call number
 * is a constant. This macro maps the function call directly to the target
 * handler instead of doing a look-up at runtime
 */
#define uk_syscall_r_static(...)					\
	UK_CONCAT(__uk_syscall,						\
		  UK_CONCAT(__UK_SYSCALL_NARGS(__VA_ARGS__)), _r)(__VA_ARGS__)

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

#endif /* CONFIG_LIBSYSCALL_SHIM */

#endif /* __UK_SYSCALL_H__ */

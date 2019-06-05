/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
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

#include <uk/essentials.h>
#include <errno.h>
#include <uk/print.h>

#define __uk_scc(X) ((long) (X))
typedef long syscall_arg_t;

#define __uk_syscall(syscall_nr, ...) \
	UK_CONCAT(uk_syscall_fn_, syscall_nr) (__VA_ARGS__)

#define __uk_syscall0(n) __uk_syscall(n)
#define __uk_syscall1(n,a) __uk_syscall(n,__uk_scc(a))
#define __uk_syscall2(n,a,b) __uk_syscall(n,__uk_scc(a),__uk_scc(b))
#define __uk_syscall3(n,a,b,c) __uk_syscall(n,__uk_scc(a),__uk_scc(b),__uk_scc(c))
#define __uk_syscall4(n,a,b,c,d) __uk_syscall(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d))
#define __uk_syscall5(n,a,b,c,d,e) __uk_syscall(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e))
#define __uk_syscall6(n,a,b,c,d,e,f) __uk_syscall(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e),__uk_scc(f))
#define __uk_syscall7(n,a,b,c,d,e,f,g) (__uk_syscall)(n,__uk_scc(a),__uk_scc(b),__uk_scc(c),__uk_scc(d),__uk_scc(e),__uk_scc(f),__uk_scc(g))


#define __SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
#define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__,7,6,5,4,3,2,1,0,)

#define __SYSCALL_DEF_NARGS_X(z, a1,a2, b1,b2, c1,c2, d1,d2, e1,e2, f1,f2, g1,g2, nr, ...) nr
#define __SYSCALL_DEF_NARGS(...) __SYSCALL_DEF_NARGS_X(__VA_ARGS__, 7,7, 6,6, 5,5, 4,4, 3,3, 2,2, 1,1,0)

#define __UK_NAME2SCALL_FN(name) UK_CONCAT(uk_syscall_, name)

#define UK_ARG_MAP1(m, type, arg) m(type, arg)
#define UK_ARG_MAP2(m, type, arg, ...) m(type, arg), UK_ARG_MAP1(m, __VA_ARGS__)
#define UK_ARG_MAP3(m, type, arg, ...) m(type, arg), UK_ARG_MAP2(m, __VA_ARGS__)
#define UK_ARG_MAP4(m, type, arg, ...) m(type, arg), UK_ARG_MAP3(m, __VA_ARGS__)
#define UK_ARG_MAP5(m, type, arg, ...) m(type, arg), UK_ARG_MAP4(m, __VA_ARGS__)
#define UK_ARG_MAP6(m, type, arg, ...) m(type, arg), UK_ARG_MAP5(m, __VA_ARGS__)
#define UK_ARG_MAP7(m, type, arg, ...) m(type, arg), UK_ARG_MAP6(m, __VA_ARGS__)
#define UK_ARG_MAPx(nr_args, ...) UK_CONCAT(UK_ARG_MAP, nr_args)(__VA_ARGS__)

#define S_ARG_LONG(type, arg) unsigned long arg
#define S_ARG_ACTUAL(type, arg) type arg
#define S_ARG_CAST(type, arg) (type) arg


/* NOTE and TODO:
 *	Currently all the functions in unikraft which are mimicking
 *	libc behavior (writev, open, mount, etc) are handling 'errno'
 *	on their own. To comply with that syscall_shim expects syscall
 *	implementation to set errno, and return 0 in case of success
 *	or -1 (or whatever the man page says) in case of failure.
 *
 *	This has to be reconsidered later. The corresponding functions
 *	in unikraft should not touch errno, and syscall_shim should
 *	provide 2 syscall functions - one that handles return and the
 *	one which does not.
 *
 *	A good reference would be musl implementation.
 */
#ifdef CONFIG_LIBSYSCALL_SHIM
#define __UK_SYSCALL_DEFINE(x, name, ...)				\
	static inline long __##name(UK_ARG_MAPx(x, S_ARG_ACTUAL, __VA_ARGS__)); \
	long name(UK_ARG_MAPx(x, S_ARG_LONG, __VA_ARGS__))			\
	{								\
		long ret = __##name(					\
			UK_ARG_MAPx(x, S_ARG_CAST, __VA_ARGS__));		\
		return ret;						\
	}								\
	static inline long __##name(UK_ARG_MAPx(x, S_ARG_ACTUAL, __VA_ARGS__))
#else
#define __UK_SYSCALL_DEFINE(x, name, ...)				\
	static inline long name(UK_ARG_MAPx(x, S_ARG_ACTUAL, __VA_ARGS__))
#endif

#define _UK_SYSCALL_DEFINE(...) __UK_SYSCALL_DEFINE(__VA_ARGS__)
#define UK_SYSCALL_DEFINE(name, ...)				\
	_UK_SYSCALL_DEFINE(__SYSCALL_DEF_NARGS(__VA_ARGS__),	\
			    __UK_NAME2SCALL_FN(name),		\
			    __VA_ARGS__)

#define __UK_SPROTO_ARGS_TYPE unsigned long
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

#define UK_SYSCALL_PROTO(args_nr, syscall_name)			\
	long UK_CONCAT(uk_syscall_, syscall_name)(	\
		__UK_SPROTO_ARGSx(args_nr))

#define uk_syscall_stub(syscall_name) ({			\
			uk_pr_debug("syscall \"" syscall_name	\
				    "\" is not implemented");	\
			errno = -ENOSYS;			\
			-1;					\
		})


#ifdef CONFIG_LIBSYSCALL_SHIM
#include <uk/bits/syscall_nrs.h>
#include <uk/bits/syscall_map.h>
#include <uk/bits/provided_syscalls.h>
#include <uk/bits/syscall_stubs.h>

long uk_syscall(long n, ...);

#define syscall(...)							\
	UK_CONCAT(__uk_syscall, __SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)
#endif

#endif /* __UK_SYSCALL_H__ */

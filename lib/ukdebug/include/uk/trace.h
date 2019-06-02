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
/* TODO: consider to move UK_CONCAT into public headers */
#define __UK_CONCAT_X(a, b) a##b
#define UK_CONCAT(a, b) __UK_CONCAT_X(a, b)

#define __UK_NARGS_X(a, b, c, d, e, f, g, h, n, ...) n
#define UK_NARGS(...)  __UK_NARGS_X(, ##__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)

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

#endif /* _UK_TRACE_H_ */

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Assertions
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKDEBUG_ASSERT_H__
#define __UKDEBUG_ASSERT_H__

#include <uk/plat/bootstrap.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
#define UK_ASSERT(x)							\
	do {								\
		if (unlikely(!(x))) {					\
			uk_pr_crit("Assertion failure: %s\n",		\
				   STRINGIFY(x));			\
			/* TODO: stack trace */				\
			ukplat_terminate(UKPLAT_CRASH);			\
		}							\
	} while (0)

#define UK_WARNIF(x)							\
	do {								\
		if (unlikely(x)) {					\
			uk_pr_warn("Condition warning: %s\n",		\
				   STRINGIFY(x));			\
		}							\
	} while (0)

#else
#define UK_WARNIF(x)							\
	do {								\
	} while (0)

#define UK_ASSERT(x)							\
	do {								\
	} while (0)
#endif

#define UK_BUGON(x)							\
	UK_ASSERT(!(x))

#define UK_BUG()							\
	UK_BUGON(1)

#define UK_CRASH(fmt, ...)						\
	do {								\
		uk_pr_crit((fmt), ##__VA_ARGS__);			\
		/* TODO: stack trace */					\
		ukplat_terminate(UKPLAT_CRASH);				\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __UKDEBUG_ASSERT_H__ */

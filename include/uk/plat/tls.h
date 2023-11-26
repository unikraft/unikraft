/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2021, NEC Laboratories GmbH, NEC Corporation.
 *                     All rights reserved.
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

#ifndef __UKPLAT_TLS_H__
#define __UKPLAT_TLS_H__

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/arch/tls.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>
#if CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

/**
 * Gets the current thread local storage register value
 * NOTE: The returned value is not necessarily the base address of the TLS
 *       region (see `ukarch_tls_tlsp()`).
 */
__uptr ukplat_tlsp_get(void);

/**
 * Sets the thread local storage register
 * WARNING: Setting of the TLS pointer should happen outside of a function that
 *          accesses TLS variables because of potential compiler optimizations.
 *          Due to performance optimizations, it can happen that a TLS variable
 *          is loaded and cached before the actual TLS pointer is set.
 *          When working with multiple TLS, we recommend using
 *          `ukplat_tlsp_exec()`.
 *
 * @param tlsp TLS pointer value (see `ukarch_tls_tlsp()`)
 */
void ukplat_tlsp_set(__uptr tlsp);

/**
 * Helper for setting the thread local storage register
 * to a given base address of a TLS area
 *
 * @param tls_area Base pointer of the TLS area to set. Please note that
 *                 it has to be of size `ukarch_tls_area_size()`
 */
static inline void ukplat_tls_set(void *tls_area)
{
#if CONFIG_LIBUKDEBUG
	UK_ASSERT(tls_area);
	UK_ASSERT(IS_ALIGNED((__sz) tls_area, ukarch_tls_area_align()));
#endif /* CONFIG_LIBUKDEBUG */

	ukplat_tlsp_set(ukarch_tls_tlsp(tls_area));
}

typedef int (*ukplat_tlsp_exec_fn)(void *);

/**
 * Executes a function with a different TLS pointer activated.
 * This wrapper function will apply a given TLS pointer before executing
 * a given function. After the execution, the original TLS pointer is
 * restored.
 * NOTE: This function is declared as `__noinline`  so that we keep
 *       a dedicated function context to load and store the TLS pointer
 *       register of the CPU. No TLS variable is accessed within this
 *       function wrapper.
 *
 * @param tlsp
 *   TLS pointer to set for the execution of `fn()`
 * @param fn
 *   Function to execute
 * @param argp
 *   Argument pointer that will be handed over to `fn`
 * @return
 *   The return value from `fn()`
 */
static __noinline __maybe_unused int
ukplat_tlsp_exec(__uptr tlsp, ukplat_tlsp_exec_fn fn, void *argp)
{
	__uptr orig_tlsp;
	int ret;

#if CONFIG_LIBUKDEBUG
	UK_ASSERT(fn);
#endif

	orig_tlsp = ukplat_tlsp_get();
	barrier();
	ukplat_tlsp_set(tlsp);
	ret = (*fn)(argp);
	ukplat_tlsp_set(orig_tlsp);
	return ret;
}

#endif /* __UKPLAT_TLS_H__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
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
#ifndef __UK_THREAD_TCB_IMPL_H__
#define __UK_THREAD_TCB_IMPL_H__

#include <uk/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header provides prototypes and helpers for integrating a custom
 * thread control block to `libuksched` threads.
 * A library (e.g., libc, pthread) that enables the integration sets
 * CONFIG_LIBUKSCHED_TCB_INIT through its Makefile.uk:
 *
 *  select LIBUKSCHED_TCB_INIT
 *
 * NOTE: Please note that the space needed for the TCB needs to be configured
 *       with the architecture library libcontext.
 * HINT: The configured size of the TCB area can be queried with
 *       `ukarch_tls_tcb_size()`.
 */

#if CONFIG_LIBUKSCHED_TCB_INIT
/**
 * Called by `libuksched` as soon as a thread is created or initialized
 * that has a Unikraft TLS allocated.
 * It is intended that a libc or a thread abstraction library that require
 * a TCB will provide this function symbol.
 *
 * @param thread thread that is created or initialized
 * @param tcb Reference to reserved space to custom thread control block
 *            that should be initialized. See tls.h
 * @return
 *   - (>=0): Success, tcb is initialized.
 *   - (<0): Negative error code, thread creation is canceled.
 */
int uk_thread_uktcb_init(struct uk_thread *thread, void *tcb);

/**
 * Called by `libuksched` as soon as a thread is released or uninitialized
 * that has a Unikraft TLS allocated.
 * It is intended that a libc or a thread abstraction library that require
 * a TCB will provide this function symbol.
 *
 * @param thread thread that is released or uninitialized
 * @param tcb Reference to reserved space to custom thread control block
 *            that should be uninitialized.
 */
void uk_thread_uktcb_fini(struct uk_thread *thread, void *tcb);
#endif /* CONFIG_LIBUKSCHED_TCB_INIT */

/**
 * Macro that returns the TCB of a (foreign) thread
 */
#define uk_thread_uktcb(thread)						\
	({								\
		UK_ASSERT((thread)->flags & UK_THREADF_UKTLS);		\
		(ukarch_tls_tcb_get((thread)->uktlsp));			\
	})

#ifdef __cplusplus
}
#endif

#endif /* __UK_THREAD_TCB_IMPL_H__ */

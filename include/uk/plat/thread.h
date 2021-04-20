/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
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
 */

/*
 * Platform specific thread functions
 */
#ifndef __UKPLAT_THREAD_H__
#define __UKPLAT_THREAD_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/assert.h>

enum ukplat_ctx_type {
	ukplat_ctx_none,
	ukplat_ctx_hw,
	ukplat_ctx_sw,
};

typedef __sz (*ukplat_ctx_size_func_t)(void);
typedef void  (*ukplat_ctx_init_func_t)
		(void *ctx, unsigned long sp, unsigned long tlsp);
typedef void  (*ukplat_ctx_start_func_t)
		(void *ctx);
typedef void  (*ukplat_ctx_switch_func_t)
		(void *prevctx, void *nextctx);

struct ukplat_ctx_callbacks {
	/* callback for returning the needed size for a thread context */
	ukplat_ctx_size_func_t size_cb;
	/* callback for initializing a new thread context */
	ukplat_ctx_init_func_t init_cb;
	/* callback for starting thread context */
	ukplat_ctx_start_func_t start_cb __noreturn;
	/* callback for switching contexts */
	ukplat_ctx_switch_func_t switch_cb;
};

int ukplat_ctx_callbacks_init(struct ukplat_ctx_callbacks *ctx_cbs,
		enum ukplat_ctx_type ctx_type);


static inline
__sz ukplat_thread_ctx_size(struct ukplat_ctx_callbacks *cbs)
{
	UK_ASSERT(cbs != __NULL);

	return cbs->size_cb();
}

static inline
void ukplat_thread_ctx_init(struct ukplat_ctx_callbacks *cbs,
			    void *ctx,
			    unsigned long sp, unsigned long tlsp)
{
	UK_ASSERT(cbs != __NULL);
	UK_ASSERT(ctx != __NULL);

	return cbs->init_cb(ctx, sp, tlsp);
}

static inline
void ukplat_thread_ctx_start(struct ukplat_ctx_callbacks *cbs,
		void *ctx) __noreturn;

static inline
void ukplat_thread_ctx_start(struct ukplat_ctx_callbacks *cbs,
		void *ctx)
{
	UK_ASSERT(cbs != __NULL);
	UK_ASSERT(ctx != __NULL);

	cbs->start_cb(ctx);
}

static inline
void ukplat_thread_ctx_switch(struct ukplat_ctx_callbacks *cbs,
		void *prevctx, void *nextctx)
{
	UK_ASSERT(cbs != __NULL);
	UK_ASSERT(prevctx != __NULL);
	UK_ASSERT(nextctx != __NULL);

	cbs->switch_cb(prevctx, nextctx);
}

#endif /* __UKPLAT_THREAD_H__ */

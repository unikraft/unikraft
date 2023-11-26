/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
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

#ifndef __UKARCH_CTX_H__
#define __UKARCH_CTX_H__

#include <uk/arch/types.h>
#ifndef __ASSEMBLY__
#include <uk/config.h>
#if CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#else /* !CONFIG_LIBUKDEBUG */
#define UK_ASSERT(...) do {} while (0)
#endif /* !CONFIG_LIBUKDEBUG */
#include <uk/essentials.h>
#include <uk/asm/ctx.h>
#endif /*!__ASSEMBLY__*/

#define UKARCH_CTX_OFFSETOF_IP 0
#if (defined __PTR_IS_16)
#define UKARCH_CTX_OFFSETOF_SP 2
#elif (defined __PTR_IS_32)
#define UKARCH_CTX_OFFSETOF_SP 4
#elif (defined __PTR_IS_64)
#define UKARCH_CTX_OFFSETOF_SP 8
#endif

#ifndef __ASSEMBLY__
struct ukarch_ctx {
	__uptr ip;	/**< instruction pointer */
	__uptr sp;	/**< stack pointer */
} __packed;

/*
 * Context functions are not allowed to return
 */
typedef void (*ukarch_ctx_entry0)(void) __noreturn;
typedef void (*ukarch_ctx_entry1)(long) __noreturn;
typedef void (*ukarch_ctx_entry2)(long, long) __noreturn;

/**
 * Initializes a context struct with stack pointer and
 * instruction pointer. The standard register set is
 * __not__ zero'ed when a CPU switches to such a context.
 * This is the most minimal context initialization.
 * Please note that also the frame pointer register is
 * not cleared.
 *
 * @param ctx
 *   Reference to context to initialize
 * @param sp
 *   Stack pointer
 *   The caller has to make sure that `sp` is aligned to the
 *   requirements of the code executed starting from `ip`.
 * @param ip
 *   Instruction pointer to start execution
 */
static inline void ukarch_ctx_init_bare(struct ukarch_ctx *ctx,
					__uptr sp, __uptr ip)
{
	UK_ASSERT(ctx);

	/* NOTE: We are not checking if SP is given or if SP is aligned because
	 *       execution does not have to start with a function entry.
	 */
	(*ctx) = (struct ukarch_ctx){ .ip = ip, .sp = sp };
}

/**
 * Initializes a context struct with stack pointer and
 * instruction pointer. A potential frame pointer register
 * is always zero'ed when a CPU switches to such a context.
 *
 * @param ctx
 *   Reference to context to initialize
 * @param sp
 *   Stack pointer (required)
 *   The caller has to make sure that `sp` is aligned to the
 *   requirements of the code executed starting from `ip`.
 * @param keep_regs
 *   If set to 0, the standard register set is zero'ed
 *    before execution starts at `ip`.
 *   Otherwise, no clearing of the standard register set
 *    happens on context switch with `ukarch_ctx_switch()`.
 * @param ip
 *   Instruction pointer to start execution (required)
 */
void ukarch_ctx_init(struct ukarch_ctx *ctx,
		     __uptr sp, int keep_regs,
		     __uptr ip);

/**
 * Initializes a context struct with stack pointer and
 * entrance function. A potential frame pointer register
 * is always zero'ed when a CPU switches to such a context.
 *
 * @param ctx
 *   Reference to context to initialize
 * @param sp
 *   Stack pointer (required and must be aligned
 *                  to `UKARCH_SP_ALIGN`)
 * @param keep_regs
 *   If set to 0, the standard register set is zero'ed
 *    before `entry` is executed.
 *   Otherwise, no clearing of the standard register set
 *    happens on context switch with `ukarch_ctx_switch()`.
 * @param entry
 *   Entry function to execute (required)
 */
void ukarch_ctx_init_entry0(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry0 entry);

/**
 * Similar to `ukarch_ctx_init_entry0()` but with an entry function accepting
 * one argument.
 */
void ukarch_ctx_init_entry1(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry1 entry, long arg);

/**
 * Similar to `ukarch_ctx_init_entry0()` but with an entry function accepting
 * two arguments.
 */
void ukarch_ctx_init_entry2(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry2 entry, long arg0, long arg1);

/**
 * Pushes a value to the stack of a remote context that should not be executed
 * at the moment. This macro reserves required space (including potential
 * architecture size requirements to ensure alignments) to store `value`'s
 * datatype on the stack.
 *
 * @param ctx
 *   Reference to context to which stack a value should be pushed
 * @param value
 *   Value to push on the stack of `ctx`.
 */
#define ukarch_rctx_stackpush(ctx, value)				\
	({								\
		(ctx)->sp = ukarch_rstack_push((ctx)->sp, (value));	\
	})

/**
 * Similar to `ukarch_rctx_stackpush()` but without alignment.
 */
#define ukarch_rctx_stackpush_packed(ctx, value)			\
	({								\
		(ctx)->sp = ukarch_rstack_push_packed((ctx)->sp, (value)); \
	})

/**
 * Switch the current logical CPU to context `load`. The current context
 * is stored to `store`. The standard register set is saved to `store`'s
 * stack and will be restored when the context will be loaded again.
 *
 * @param store
 *   Reference to context struct to save the current context to
 * @param load
 *   Reference to context that shall be executed
 */
void ukarch_ctx_switch(struct ukarch_ctx *store, struct ukarch_ctx *load);

/**
 * State of extended context, like additional CPU registers and units
 * (e.g., floating point, vector registers)
 * Note: The layout is architecture specific. The helpers `ukarch_ectx_size()`
 *       and `ukarch_ectx_align()` provide constraints needed for allocating
 *       memory `struct ukarch_ectx`.
 */
struct ukarch_ectx;

/**
 * Size needed to allocate memory to store an extended context state
 */
__sz ukarch_ectx_size(void);

/**
 * Alignment requirement for allocated memory to store an
 * extended context state
 */
__sz ukarch_ectx_align(void);

/**
 * Perform the minimum necessary to ensure the memory at `state`
 * is appropriate for passing to `ukarch_ectx_save()`.
 *
 * @param state
 *   Reference to extended context
 */
void ukarch_ectx_sanitize(struct ukarch_ectx *state);

/**
 * Initializes an extended context so that it can be loaded
 * into a logical CPU with `ukarch_ectx_load()`.
 *
 * @param state
 *   Reference to extended context to initialize
 */
void ukarch_ectx_init(struct ukarch_ectx *state);

/**
 * Stores the extended context of the currently executing CPU to `state`.
 * Such an extended context can be restored with `ukarch_ectx_load()`.
 *
 * @param state
 *   Reference to extended context to save to
 */
void ukarch_ectx_store(struct ukarch_ectx *state);

/**
 * Restores a given extended context on the currently executing CPU.
 *
 * @param state
 *   Reference to extended context to restore
 */
void ukarch_ectx_load(struct ukarch_ectx *state);

#ifdef CONFIG_ARCH_X86_64
/**
 * Compare the given extended context with the state of the currently executing
 * CPU. If the state is different, crash the kernel.
 *
 * @param state
 *   Reference to extended context to compare to
 */
void ukarch_ectx_assert_equal(struct ukarch_ectx *state);
#endif

#endif /* !__ASSEMBLY__ */
#endif /* __UKARCH_CTX_H__ */

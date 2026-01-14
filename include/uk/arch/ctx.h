/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Sergiu Moga <sergiu@unikraft.io>
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH. All rights reserved.
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
#include <uk/asm/ctx.h>
#include <uk/essentials.h>

#include <uk/lcpu.h>

#ifndef __ASSEMBLY__
#include <uk/config.h>
#if CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#else /* !CONFIG_LIBUKDEBUG */
#define UK_ASSERT(...) do {} while (0)
#endif /* !CONFIG_LIBUKDEBUG */
#endif /*!__ASSEMBLY__*/

#define UKARCH_CTX_OFFSETOF_IP 0
#if (defined __PTR_IS_16)
#define UKARCH_CTX_OFFSETOF_SP 2
#elif (defined __PTR_IS_32)
#define UKARCH_CTX_OFFSETOF_SP 4
#elif (defined __PTR_IS_64)
#define UKARCH_CTX_OFFSETOF_SP 8
#endif

/* We must make sure that ECTX is aligned, so we make use of some padding,
 * whose size is equal to what we need to add to UK_LCPU_ECTX_SIZE
 * to make it aligned with UK_LCPU_ECTX_ALIGN
 */
#define UKARCH_EXECENV_PAD_SIZE						\
	(ALIGN_UP(UK_LCPU_ECTX_SIZE,				\
		  UK_LCPU_ECTX_ALIGN) -				\
	 UK_LCPU_ECTX_SIZE)

/* If we make sure that the in-memory structure's end address is aligned to
 * the ECTX alignment, then subtracting from that end address a value that is
 * also a multiple of that alignment, guarantees that the resulted address
 * is also ECTX aligned.
 */
#define UKARCH_EXECENV_END_ALIGN				\
	UK_LCPU_ECTX_ALIGN
#define UKARCH_EXECENV_SIZE					\
	(UKARCH_EXECENV_PAD_SIZE +				\
	 UK_LCPU_ECTX_SIZE +					\
	 UK_LCPU_SYSCTX_SIZE +					\
	 UK_LCPU_REGS_SIZE)

#define UKARCH_EXECENV_OFFSETOF_REGS				0x0
#define UKARCH_EXECENV_OFFSETOF_SYSCTX				\
	(UKARCH_EXECENV_OFFSETOF_REGS +	UK_LCPU_REGS_SIZE)
#define UKARCH_EXECENV_OFFSETOF_ECTX				\
	(UKARCH_EXECENV_OFFSETOF_SYSCTX + UK_LCPU_SYSCTX_SIZE)

/**
 * Size of the current frame pointer Auxiliary Stack Pointer Control Block:
 */
#if (defined __PTR_IS_16)
#define UKARCH_AUXSPCB_CURR_FP_SIZE				2
#elif (defined __PTR_IS_32)
#define UKARCH_AUXSPCB_CURR_FP_SIZE				4
#elif (defined __PTR_IS_64)
#define UKARCH_AUXSPCB_CURR_FP_SIZE				8
#endif

/**
 * Size of the Auxiliary Stack Pointer Control Block
 * - sizeof(__uptr) for the frame pointer field
 * - sizeof(struct uk_lcpu_sysctx) for the field representing the current
 * thread's Kernel system context
 */
#define UKARCH_AUXSPCB_SIZE					\
	(ALIGN_UP(UKARCH_AUXSPCB_CURR_FP_SIZE +			\
		  UK_LCPU_SYSCTX_SIZE, UKARCH_AUXSP_ALIGN))

/**
 * Size of the padding required to ensure the size of the Auxiliary Stack
 * Pointer Control Block is a multiple of the alignment required for the
 * auxiliary stack pointer.
 */
#define UKARCH_AUXSPCB_PAD					\
	(UKARCH_AUXSPCB_SIZE -					\
	 (UKARCH_AUXSPCB_CURR_FP_SIZE + UK_LCPU_SYSCTX_SIZE))

/**
 * Offset to current frame pointer field.
 */
#define UKARCH_AUXSPCB_OFFSETOF_CURR_FP				0x0

/**
 * Offset to current Unikraft system context field.
 */
#define UKARCH_AUXSPCB_OFFSETOF_UKSYSCTX			\
	(UKARCH_AUXSPCB_OFFSETOF_CURR_FP +			\
	 UKARCH_AUXSPCB_CURR_FP_SIZE)

#if !__ASSEMBLY__
struct ukarch_ctx {
	__uptr ip;	/**< instruction pointer */
	__uptr sp;	/**< stack pointer */
} __packed;

struct ukarch_execenv {
	/* General purpose/flags registers */
	__u8 regs[UK_LCPU_REGS_SIZE];
	/* System registers (e.g. TLS pointer) */
	__u8 sysctx[UK_LCPU_SYSCTX_SIZE];
	/* Extended context (e.g. SIMD etc.) */
	__u8 ectx[UK_LCPU_ECTX_SIZE];
	/* Padding for end alignment */
	__u8 pad[UKARCH_EXECENV_PAD_SIZE];
};

UK_CTASSERT(sizeof(struct ukarch_execenv) == UKARCH_EXECENV_SIZE);
UK_CTASSERT(IS_ALIGNED(UKARCH_EXECENV_PAD_SIZE + UK_LCPU_ECTX_SIZE,
		       UK_LCPU_ECTX_ALIGN));
UK_CTASSERT(__offsetof(struct ukarch_execenv, regs) ==
	    UKARCH_EXECENV_OFFSETOF_REGS);
UK_CTASSERT(__offsetof(struct ukarch_execenv, sysctx) ==
	    UKARCH_EXECENV_OFFSETOF_SYSCTX);
UK_CTASSERT(__offsetof(struct ukarch_execenv, ectx) ==
	    UKARCH_EXECENV_OFFSETOF_ECTX);

/**
 * Layout of the auxiliary stack and its embedded control block located at its
 * end (towards higher address space).
 *               ┌─────────────── auxsp
 *               │
 *    ┌──────────▼───────────┐  ▲ ▲                    │ auxsp
 *    │ struct ukarch_auxspcb│  │ │                    │
 *    │{                     │  │ │                    │
 *┌───┼─────curr_fp          │  │ │                    │
 *│   │     uksysctx         │  │ │UKARCH_AUXSPCB_SIZE │
 *│   │[pad till auxsp align]│  │ │                    │
 *│   │}                     │  │ │                    │
 *│ ┌►│◄─────────────────────►  │ ▼                    │
 *│ │ │   UKARCH_AUXSP_ALIGN │  │                      │
 *│ │ │                      │  │                      │ STACK GROWTH
 *│ │ │                      │  │ AUXSTACK_SIZE        │  DIRECTION
 *│ │ │ curr_fp points       │  │                      │
 *│ │ │ to a safe            │  │                      │
 *└►│ │ usable frame in      │  │                      │
 *  │ │ the auxstack         │  │                      │
 *  │ │ (the area below the  │  │                      │
 *  │ │  auxstack control    │  │                      │
 *  │ │       block)         │  │                      │
 *  │ │                      │  │                      │
 *  │ │                      │  │                      │
 *  │ │                      │  │                      │
 *  │ │                      │  │                      │
 *  └►└──────────────────────┘  ▼                      ▼ auxsp - AUXSTACK_SIZE
 *    ◄─────────────────────►
 *       UKARCH_AUXSP_ALIGN
 */

#define SP_IN_AUXSP(sp, auxsp)					\
	(IN_RANGE((sp), (auxsp) - AUXSTACK_SIZE, AUXSTACK_SIZE))

struct ukarch_auxspcb {
	/* Current safe frame pointer inside the auxiliary stack area */
	__uptr curr_fp;
	/* Unikraft system registers (e.g. TLS pointer) */
	__u8 uksysctx[UK_LCPU_SYSCTX_SIZE];
	/* Padding for end alignment, the auxiliary stack area begins after */
	__u8 pad[UKARCH_AUXSPCB_PAD];
};

UK_CTASSERT(sizeof(struct ukarch_auxspcb) == UKARCH_AUXSPCB_SIZE);
UK_CTASSERT(IS_ALIGNED(sizeof(struct ukarch_auxspcb), UKARCH_AUXSP_ALIGN));
UK_CTASSERT(__offsetof(struct ukarch_auxspcb, curr_fp) ==
	    UKARCH_AUXSPCB_OFFSETOF_CURR_FP);
UK_CTASSERT(__offsetof(struct ukarch_auxspcb, uksysctx) ==
	    UKARCH_AUXSPCB_OFFSETOF_UKSYSCTX);

/*
 * Context functions are not allowed to return
 */
typedef void (*ukarch_ctx_entry0)(void) __noreturn;
typedef void (*ukarch_ctx_entry1)(long) __noreturn;
typedef void (*ukarch_ctx_entry2)(long, long) __noreturn;
typedef void (*ukarch_ctx_entry3)(long, long, long) __noreturn;

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
 * Without saving or restoring anything, directly switch to stack pointer
 * and jump to instruction pointer stored in the context structure.
 *
 * @param ctx
 *   Reference to context that shall be executed
 */
void ukarch_ctx_jump(struct ukarch_ctx *ctx) __noreturn;

/**
 * Function that can be executed in a context where one can take
 * exceptions or yield the current thread.
 * After the function returns, execution will resume the state stored in
 * the execenv argument.
 *
 * @param execenv
 *   Pointer to the execution environment that the function will return to
 * @param arg
 *   Custom user-defined argument
 */
typedef void (*ukarch_ehtrampo_entry)(struct ukarch_execenv *execenv, long arg);

/**
 * Initializes a context that allows jumping from an exception handling
 * context to a caller-defined function in a context where one can take
 * exceptions or yield the current thread.
 * Returning from the caller-defined function resumes execution from
 * the place that the exception handler would resume from.
 *
 * After calling ukarch_ehtrampo_init() use ukarch_ctx_jmp() to jump to
 * the target context.
 *
 * NOTE: This function may return with a tainted extended context.
 *
 * @param ctx
 *   Reference to context to initialize
 * @param sp
 *   Stack pointer (required and must be aligned to `UKARCH_EXECENV_END_ALIGN`)
 *  The stack must have enough space to store both an entire execution
 *  environment context as well as run the user provided entry function
 * @param r
 *   Pointer to architecture specific general purpose registers saved on
 *  exception handling entry
 * @param entry
 *   Entry function to execute (required).
 * @param arg
 *   The argument `entry` callback will receive
 */
void ukarch_ctx_init_ehtrampo(struct ukarch_ctx *ctx,
			      struct uk_lcpu_regs *r,
			      __uptr sp,
			      ukarch_ehtrampo_entry entry, long arg);

/**
 * Initialize an auxiliary stack pointer. This must be always called the
 * first time you create an auxiliary stack pointer.
 *
 * @param auxsp
 *   The auxiliary stack pointer to initialize. Must point to the high end of
 *  the auxiliary stack.
 *
 * NOTE: Auxiliary stack pointer must have UKARCH_AUXSP_ALIGN alignment.
 *
 */
static inline void ukarch_auxsp_init(__uptr auxsp)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = (struct ukarch_auxspcb *)(auxsp - sizeof(*auxspcb_ptr));
	auxspcb_ptr->curr_fp = auxsp - sizeof(*auxspcb_ptr);
	UK_ASSERT(IS_ALIGNED(auxspcb_ptr->curr_fp, UKARCH_AUXSP_ALIGN));
}

/**
 * Get the control block of the auxiliary stack pointer.
 *
 * @param auxsp
 *   The auxiliary stack pointer whose control block to set.
 *  Must point to the high end of the auxiliary stack.
 * @return
 *   The control block of the auxiliary stack pointer
 *
 */
static inline struct ukarch_auxspcb *ukarch_auxsp_get_cb(__uptr auxsp)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = (struct ukarch_auxspcb *)(auxsp - sizeof(*auxspcb_ptr));
	UK_ASSERT(IS_ALIGNED(auxspcb_ptr->curr_fp, UKARCH_AUXSP_ALIGN));

	return auxspcb_ptr;
}

/**
 * Set the Unikraft TLS pointer of the control block of the auxiliary stack
 * pointer.
 *
 * @param auxspcb
 *   The auxiliary stack control block pointer whose Unikraft TLS pointer to
 *  set.
 * @param uktlsp
 *   The TLS pointer to set in the control block of the auxstack pointer.
 *
 */
static inline void ukarch_auxspcb_set_uktlsp(struct ukarch_auxspcb *auxspcb,
					     __uptr uktlsp)
{
	UK_ASSERT(auxspcb);
	UK_ASSERT(IS_ALIGNED((__uptr)auxspcb, UKARCH_AUXSP_ALIGN));
	uk_lcpu_sysctx_set(&auxspcb->uksysctx, TLSP, uktlsp);
}

/**
 * Get the Unikraft TLS pointer of the auxiliary stack pointer.
 *
 * @param auxspcb
 *   The auxiliary stack control block pointer whose Unikraft TLS pointer to
 *  get.
 * @return
 *   The Unikraft TLS pointer of the auxiliary stack pointer
 *
 */
static inline __uptr ukarch_auxspcb_get_uktlsp(struct ukarch_auxspcb *auxspcb)
{
	UK_ASSERT(auxspcb);
	UK_ASSERT(IS_ALIGNED((__uptr)auxspcb, UKARCH_AUXSP_ALIGN));
	return uk_lcpu_sysctx_get(&auxspcb->uksysctx, TLSP);
}

/**
 * Set the current frame pointer of the control block of the auxiliary stack
 * pointer.
 *
 * @param auxspcb
 *   The auxiliary stack control block pointer whose current frame pointer to
 *  set.
 * @param curr_fp
 *   The current frame pointer to set in the control block of the auxstack
 *  pointer.
 *
 */
static inline void ukarch_auxspcb_set_curr_fp(struct ukarch_auxspcb *auxspcb,
					      __uptr curr_fp)
{
	UK_ASSERT(auxspcb);
	UK_ASSERT(IS_ALIGNED((__uptr)auxspcb, UKARCH_AUXSP_ALIGN));
	UK_ASSERT(IS_ALIGNED(curr_fp, UKARCH_AUXSP_ALIGN));
	auxspcb->curr_fp = curr_fp;
}

/**
 * Get the current frame pointer of the control block of the auxiliary stack
 * pointer.
 *
 * @param auxspcb
 *   The auxiliary stack control block pointer whose current frame pointer to
 *  get.
 * @return
 *   The current frame pointer of the auxiliary stack pointer
 *
 */
static inline __uptr ukarch_auxspcb_get_curr_fp(struct ukarch_auxspcb *auxspcb)
{
	UK_ASSERT(auxspcb);
	UK_ASSERT(IS_ALIGNED((__uptr)auxspcb, UKARCH_AUXSP_ALIGN));
	return auxspcb->curr_fp;
}

/**
 * Loads a given execution environment on the currently executing CPU.
 *
 * NOTE: This function does not return, it overwrites the entire current
 *       context.
 *
 * @param state
 *   Reference to execution environment to load
 */
void ukarch_execenv_load(long state) __noreturn;

#endif /* !__ASSEMBLY__ */
#endif /* __UKARCH_CTX_H__ */

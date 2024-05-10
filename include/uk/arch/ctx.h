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
#include <uk/asm/sysctx.h>

#ifndef __ASSEMBLY__
#include <uk/config.h>
#if CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#else /* !CONFIG_LIBUKDEBUG */
#define UK_ASSERT(...) do {} while (0)
#endif /* !CONFIG_LIBUKDEBUG */
#include <uk/essentials.h>
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
 * whose size is equal to what we need to add to UKARCH_ECTX_SIZE
 * to make it aligned with UKARCH_ECTX_ALIGN
 */
#define UKARCH_EXECENV_PAD_SIZE			\
	(ALIGN_UP(UKARCH_ECTX_SIZE,		\
		 UKARCH_ECTX_ALIGN) -		\
	 UKARCH_ECTX_SIZE)

/* If we make sure that the in-memory structure's end address is aligned to
 * the ECTX alignment, then subtracting from that end address a value that is
 * also a multiple of that alignment, guarantees that the resulted address
 * is also ECTX aligned.
 */
#define UKARCH_EXECENV_END_ALIGN		\
	UKARCH_ECTX_ALIGN
#define UKARCH_EXECENV_SIZE			\
	(UKARCH_EXECENV_PAD_SIZE +		\
	 UKARCH_ECTX_SIZE +			\
	 UKARCH_SYSCTX_SIZE +			\
	 __REGS_SIZEOF)

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
 * - sizeof(struct ukarch_sysctx) for the field representing the current
 * thread's Kernel system context
 */
#define UKARCH_AUXSPCB_SIZE					\
	(ALIGN_UP(UKARCH_AUXSPCB_CURR_FP_SIZE +			\
		  UKARCH_SYSCTX_SIZE, UKARCH_AUXSP_ALIGN))

/**
 * Size of the padding required to ensure the size of the Auxiliary Stack
 * Pointer Control Block is a multiple of the alignment required for the
 * auxiliary stack pointer.
 */
#define UKARCH_AUXSPCB_PAD					\
	(UKARCH_AUXSPCB_SIZE -					\
	 (UKARCH_AUXSPCB_CURR_FP_SIZE + UKARCH_SYSCTX_SIZE))

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
	struct __regs regs;
	/* System registers (e.g. TLS pointer) */
	struct ukarch_sysctx sysctx;
	/* Extended context (e.g. SIMD etc.) */
	__u8 ectx[UKARCH_ECTX_SIZE];
	/* Padding for end alignment */
	__u8 pad[UKARCH_EXECENV_PAD_SIZE];
};

#define SP_IN_AUXSP(sp, auxsp)					\
	(IN_RANGE((sp), (auxsp) - AUXSTACK_SIZE, AUXSTACK_SIZE))

struct ukarch_auxspcb {
	/* Current safe frame pointer inside the auxiliary stack area */
	__uptr curr_fp;
	/* Unikraft system registers (e.g. TLS pointer) */
	struct ukarch_sysctx uksysctx;
	/* Padding for end alignment, the auxiliary stack area begins after */
	__u8 pad[UKARCH_AUXSPCB_PAD];
};

UK_CTASSERT(sizeof(struct ukarch_auxspcb) == UKARCH_AUXSPCB_SIZE);
UK_CTASSERT(IS_ALIGNED(sizeof(struct ukarch_auxspcb), UKARCH_AUXSP_ALIGN));
UK_CTASSERT(__offsetof(struct ukarch_auxspcb, curr_fp) ==
	    UKARCH_AUXSPCB_OFFSETOF_CURR_FP);
UK_CTASSERT(__offsetof(struct ukarch_auxspcb, uksysctx) ==
	    UKARCH_AUXSPCB_OFFSETOF_UKSYSCTX);

UK_CTASSERT(sizeof(struct ukarch_execenv) == UKARCH_EXECENV_SIZE);
UK_CTASSERT(IS_ALIGNED(UKARCH_EXECENV_PAD_SIZE + UKARCH_ECTX_SIZE,
		       UKARCH_ECTX_ALIGN));

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
 * Set the control block of the auxiliary stack pointer. At the moment,
 * this function only copies the current frame pointer inside the auxstack area
 * and the Unikraft system context.
 *
 * @param auxsp
 *   The auxiliary stack pointer whose control block to set.
 *  Must point to the high end of the auxiliary stack.
 * @param auxspcb
 *   The pointer to the control block to set for the auxstack pointer.
 *
 */
static inline void ukarch_auxsp_set_cb(__uptr auxsp,
				       struct ukarch_auxspcb *auxspcb)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = (struct ukarch_auxspcb *)(auxsp - sizeof(*auxspcb_ptr));
	auxspcb_ptr->curr_fp = auxspcb->curr_fp;
	UK_ASSERT(IS_ALIGNED(auxspcb_ptr->curr_fp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr->uksysctx = auxspcb->uksysctx;
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
 * @param auxsp
 *   The auxiliary stack pointer whose Unikraft TLS pointer to set.
 *  Must point to the high end of the auxiliary stack.
 * @param uktlsp
 *   The TLS pointer to set in the control block of the auxstack pointer.
 *
 */
static inline void ukarch_auxsp_set_uktlsp(__uptr auxsp, __uptr uktlsp)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = ukarch_auxsp_get_cb(auxsp);
	ukarch_sysctx_set_tlsp(&auxspcb_ptr->uksysctx, uktlsp);
}

/**
 * Get the Unikraft TLS pointer of the auxiliary stack pointer.
 *
 * @param auxsp
 *   The auxiliary stack pointer whose Unikraft TLS pointer to get.
 *  Must point to the high end of the auxiliary stack.
 * @return
 *   The Unikraft TLS pointer of the auxiliary stack pointer
 *
 */
static inline __uptr ukarch_auxsp_get_uktlsp(__uptr auxsp)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = ukarch_auxsp_get_cb(auxsp);
	return ukarch_sysctx_get_tlsp(&auxspcb_ptr->uksysctx);
}

/**
 * Set the current frame pointer of the control block of the auxiliary stack
 * pointer.
 *
 * @param auxsp
 *   The auxiliary stack pointer whose current frame pointer to set.
 *  Must point to the high end of the auxiliary stack.
 * @param curr_fp
 *   The current frame pointer to set in the control block of the auxstack
 *  pointer.
 *
 */
static inline void ukarch_auxsp_set_curr_fp(__uptr auxsp, __uptr curr_fp)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));
	UK_ASSERT(IS_ALIGNED(curr_fp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = ukarch_auxsp_get_cb(auxsp);
	auxspcb_ptr->curr_fp = curr_fp;
}

/**
 * Get the current frame pointer of the control block of the auxiliary stack
 * pointer.
 *
 * @param auxsp
 *   The auxiliary stack pointer whose current frame pointer to get.
 *  Must point to the high end of the auxiliary stack.
 * @return
 *   The current frame pointer of the auxiliary stack pointer
 *
 */
static inline __uptr ukarch_auxsp_get_curr_fp(__uptr auxsp)
{
	struct ukarch_auxspcb *auxspcb_ptr;

	UK_ASSERT(auxsp);
	UK_ASSERT(IS_ALIGNED(auxsp, UKARCH_AUXSP_ALIGN));

	auxspcb_ptr = ukarch_auxsp_get_cb(auxsp);
	return auxspcb_ptr->curr_fp;
}

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

/**
 * Loads a given execution environment on the currently executing CPU.
 *
 * NOTE: This function cannot be returned from, it overwrites the entire current
 *       context.
 *
 * @param state
 *   Reference to execution environment to load
 */
void ukarch_execenv_load(long state) __noreturn;

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

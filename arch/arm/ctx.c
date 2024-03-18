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
/* This file is derived from x86/ctx.c and adopted for Arm */

#include <uk/config.h>
#include <uk/arch/ctx.h>
#include <uk/plat/lcpu.h>
#if CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#include <uk/print.h>
#else /* !CONFIG_LIBUKDEBUG */
#define UK_ASSERT(..) do {} while (0)
#define uk_pr_debug(..) do {} while (0)
#endif /* !CONFIG_LIBUKDEBUG */

/* Assembler functions */
void _ctx_arm_clearregs(void);
void _ctx_arm_call0(void);
void _ctx_arm_call1(void);
void _ctx_arm_call2(void);
void _ctx_arm_call3(void);
void _ctx_arm_call4(void);
void _ctx_arm_call5(void);
void _ctx_arm_call6(void);

void ukarch_ctx_init(struct ukarch_ctx *ctx,
		     __uptr sp, int keep_regs,
		     __uptr ip)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(ip);			/* IP == NULL would cause a crash */
	UK_ASSERT(!keep_regs && sp);	/* stack is needed for clearing regs */

	_sp = ukarch_rstack_push(sp, (long)ip);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call0);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call0);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: start:%p sp:%p\n",
		    ctx, (void *)ip, (void *)sp);
}

void ukarch_ctx_init_entry0(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry0 entry)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* entry == NULL will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long) entry);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call0);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call0);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(), sp:%p\n",
		    ctx, entry, (void *)sp);
}

void ukarch_ctx_init_entry1(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry1 entry, long arg)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long)entry);
	_sp = ukarch_rstack_push(_sp, arg);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call1);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call1);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx), sp:%p\n",
		    ctx, entry, arg, (void *)sp);
}

void ukarch_ctx_init_entry2(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry2 entry, long arg0, long arg1)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long)entry);
	_sp = ukarch_rstack_push(_sp, arg0);
	_sp = ukarch_rstack_push(_sp, arg1);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call2);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call2);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx, %lx), sp:%p\n",
		    ctx, entry, arg0, arg1, (void *)sp);
}

void ukarch_ctx_init_entry3(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry3 entry,
			    long arg0, long arg1, long arg2)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long)entry);
	_sp = ukarch_rstack_push(_sp, arg0);
	_sp = ukarch_rstack_push(_sp, arg1);
	_sp = ukarch_rstack_push(_sp, arg2);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call3);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call3);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx, %lx, %lx), sp:%p\n",
		    ctx, entry, arg0, arg1, arg2, (void *)sp);
}

void ukarch_ctx_init_entry4(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry4 entry,
			    long arg0, long arg1, long arg2, long arg3)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long)entry);
	_sp = ukarch_rstack_push(_sp, arg0);
	_sp = ukarch_rstack_push(_sp, arg1);
	_sp = ukarch_rstack_push(_sp, arg2);
	_sp = ukarch_rstack_push(_sp, arg3);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call4);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call4);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx, %lx, %lx, %lx), sp:%p\n",
		    ctx, entry, arg0, arg1, arg2, arg3, (void *)sp);
}

void ukarch_ctx_init_entry5(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry5 entry,
			    long arg0, long arg1, long arg2, long arg3,
			    long arg4)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long)entry);
	_sp = ukarch_rstack_push(_sp, arg0);
	_sp = ukarch_rstack_push(_sp, arg1);
	_sp = ukarch_rstack_push(_sp, arg2);
	_sp = ukarch_rstack_push(_sp, arg3);
	_sp = ukarch_rstack_push(_sp, arg4);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call5);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call5);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx, %lx, %lx, %lx, %lx), sp:%p\n",
		    ctx, entry, arg0, arg1, arg2, arg3, arg4, (void *)sp);
}

void ukarch_ctx_init_entry6(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry6 entry,
			    long arg0, long arg1, long arg2, long arg3,
			    long arg4, long arg5)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	_sp = ukarch_rstack_push(sp, (long)entry);
	_sp = ukarch_rstack_push(_sp, arg0);
	_sp = ukarch_rstack_push(_sp, arg1);
	_sp = ukarch_rstack_push(_sp, arg2);
	_sp = ukarch_rstack_push(_sp, arg3);
	_sp = ukarch_rstack_push(_sp, arg4);
	_sp = ukarch_rstack_push(_sp, arg5);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_call6);
	} else {
		_sp = ukarch_rstack_push(_sp, (long)_ctx_arm_call6);
		ukarch_ctx_init_bare(ctx, _sp, (long)_ctx_arm_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx, %lx, %lx, %lx, %lx, %lx), "
		    "sp:%p\n",
		    ctx, entry, arg0, arg1, arg2, arg3, arg4, arg5,
		    (void *)sp);
}

static void ehtrampo_ectx_and_sysctx_store(struct ukarch_execenv *ee)
{
	UK_ASSERT(ee);

	ukplat_lcpu_enable_irq();

	/* Save extended register state */
	ukarch_ectx_sanitize((struct ukarch_ectx *)&ee->ectx);
	ukarch_ectx_store((struct ukarch_ectx *)&ee->ectx);

	ukarch_sysctx_store(&ee->sysctx);
}

void ukarch_ctx_init_ehtrampo0(struct ukarch_ctx *ctx,
			       struct __regs *r,
			       __uptr sp,
			       ukarch_execenv_entry0 entry)
{
	struct ukarch_execenv *ee;

	UK_ASSERT(ctx);
	UK_ASSERT(r);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* We re-align the stack anyway just to be sure */
	sp &= UKARCH_EXECENV_END_ALIGN - 1;
	sp -= ALIGN_UP(sizeof(*ee), UKARCH_EXECENV_END_ALIGN);
	ee = (struct ukarch_execenv *)sp;
	ee->regs = *r;

	sp = ukarch_rstack_push(sp, (long)ukarch_execenv_load);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp, (long)entry);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp,
				(long)ehtrampo_ectx_and_sysctx_store);
	sp = ukarch_rstack_push(sp, (long)ee);

	ukarch_ctx_init_bare(ctx, sp, (long)_ctx_arm_call1);
}

void ukarch_ctx_init_ehtrampo1(struct ukarch_ctx *ctx,
			       struct __regs *r,
			       __uptr sp,
			       ukarch_execenv_entry1 entry, long arg)
{
	struct ukarch_execenv *ee;

	UK_ASSERT(ctx);
	UK_ASSERT(r);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* We re-align the stack anyway just to be sure */
	sp &= UKARCH_EXECENV_END_ALIGN - 1;
	sp -= ALIGN_UP(sizeof(*ee), UKARCH_EXECENV_END_ALIGN);
	ee = (struct ukarch_execenv *)sp;
	ee->regs = *r;

	sp = ukarch_rstack_push(sp, (long)ukarch_execenv_load);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp, (long)entry);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)arg);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call2);

	sp = ukarch_rstack_push(sp,
				(long)ehtrampo_ectx_and_sysctx_store);
	sp = ukarch_rstack_push(sp, (long)ee);

	ukarch_ctx_init_bare(ctx, sp, (long)_ctx_arm_call1);
}
void ukarch_ctx_init_ehtrampo2(struct ukarch_ctx *ctx,
			       struct __regs *r,
			       __uptr sp,
			       ukarch_execenv_entry2 entry,
			       long arg0, long arg1)
{
	struct ukarch_execenv *ee;

	UK_ASSERT(ctx);
	UK_ASSERT(r);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* We re-align the stack anyway just to be sure */
	sp = ALIGN_DOWN(sp, UKARCH_EXECENV_END_ALIGN);
	sp -= sizeof(*ee);
	ee = (struct ukarch_execenv *)sp;
	ee->regs = *r;

	sp = ukarch_rstack_push(sp, (long)ukarch_execenv_load);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp, (long)entry);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)arg0);
	sp = ukarch_rstack_push(sp, (long)arg1);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call3);

	sp = ukarch_rstack_push(sp,
				(long)ehtrampo_ectx_and_sysctx_store);
	sp = ukarch_rstack_push(sp, (long)ee);

	ukarch_ctx_init_bare(ctx, sp, (long)_ctx_arm_call1);
}

void ukarch_ctx_init_ehtrampo3(struct ukarch_ctx *ctx,
			       struct __regs *r,
			       __uptr sp,
			       ukarch_execenv_entry3 entry,
			       long arg0, long arg1, long arg2)
{
	struct ukarch_execenv *ee;

	UK_ASSERT(ctx);
	UK_ASSERT(r);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* We re-align the stack anyway just to be sure */
	sp -= ALIGN_UP(sizeof(*ee), UKARCH_EXECENV_END_ALIGN);
	ee = (struct ukarch_execenv *)sp;
	ee->regs = *r;

	sp = ukarch_rstack_push(sp, (long)ukarch_execenv_load);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp, (long)entry);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)arg0);
	sp = ukarch_rstack_push(sp, (long)arg1);
	sp = ukarch_rstack_push(sp, (long)arg2);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call4);

	sp = ukarch_rstack_push(sp,
				(long)ehtrampo_ectx_and_sysctx_store);
	sp = ukarch_rstack_push(sp, (long)ee);

	ukarch_ctx_init_bare(ctx, sp, (long)_ctx_arm_call1);
}

void ukarch_ctx_init_ehtrampo4(struct ukarch_ctx *ctx,
			       struct __regs *r,
			       __uptr sp,
			       ukarch_execenv_entry4 entry,
			       long arg0, long arg1, long arg2, long arg3)
{
	struct ukarch_execenv *ee;

	UK_ASSERT(ctx);
	UK_ASSERT(r);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* We re-align the stack anyway just to be sure */
	sp -= ALIGN_UP(sizeof(*ee), UKARCH_EXECENV_END_ALIGN);
	ee = (struct ukarch_execenv *)sp;
	ee->regs = *r;

	sp = ukarch_rstack_push(sp, (long)ukarch_execenv_load);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp, (long)entry);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)arg0);
	sp = ukarch_rstack_push(sp, (long)arg1);
	sp = ukarch_rstack_push(sp, (long)arg2);
	sp = ukarch_rstack_push(sp, (long)arg3);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call5);

	sp = ukarch_rstack_push(sp,
				(long)ehtrampo_ectx_and_sysctx_store);
	sp = ukarch_rstack_push(sp, (long)ee);

	ukarch_ctx_init_bare(ctx, sp, (long)_ctx_arm_call1);
}

void ukarch_ctx_init_ehtrampo5(struct ukarch_ctx *ctx,
			       struct __regs *r,
			       __uptr sp,
			       ukarch_execenv_entry5 entry,
			       long arg0, long arg1, long arg2, long arg3,
			       long arg4)
{
	struct ukarch_execenv *ee;

	UK_ASSERT(ctx);
	UK_ASSERT(r);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* We re-align the stack anyway just to be sure */
	sp -= ALIGN_UP(sizeof(*ee), UKARCH_EXECENV_END_ALIGN);
	ee = (struct ukarch_execenv *)sp;
	ee->regs = *r;

	sp = ukarch_rstack_push(sp, (long)ukarch_execenv_load);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call1);

	sp = ukarch_rstack_push(sp, (long)entry);
	sp = ukarch_rstack_push(sp, (long)ee);
	sp = ukarch_rstack_push(sp, (long)arg0);
	sp = ukarch_rstack_push(sp, (long)arg1);
	sp = ukarch_rstack_push(sp, (long)arg2);
	sp = ukarch_rstack_push(sp, (long)arg3);
	sp = ukarch_rstack_push(sp, (long)arg4);
	sp = ukarch_rstack_push(sp, (long)_ctx_arm_call6);

	sp = ukarch_rstack_push(sp,
				(long)ehtrampo_ectx_and_sysctx_store);
	sp = ukarch_rstack_push(sp, (long)ee);

	ukarch_ctx_init_bare(ctx, sp, (long)_ctx_arm_call1);
}

void ukarch_ctx_jump(struct ukarch_ctx *ctx)
{
	UK_ASSERT(ctx);

	__asm__ __volatile__(
		"mov	sp, %0\n"
		"br	%1\n"
		:
		: "r" (ctx->sp), "r" (ctx->ip)
		:
	);

	__builtin_unreachable();
}

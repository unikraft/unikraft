/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009, Citrix Systems, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Some parts are derived from Mini-OS arch/x86/x86_64.S */

#include <uk/arch/ctx.h>

#define ENTRY(X) .globl X ; X :

ENTRY(_ctx_x86_clearregs)
	/* wipe argument and return registers */
	xorq %rdi, %rdi
	xorq %rsi, %rsi
	xorq %rdx, %rdx
	xorq %rcx, %rcx
	xorq %rax, %rax
	xorq %r8, %r8
	xorq %r9, %r9
	xorq %r10, %r10

	/* wipe callee registers (keep frame pointer, if used) */
#if __OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp
#endif /* __OMIT_FRAMEPOINTER__ */
	xorq %rbx, %rbx
	xorq %r12, %r12
	xorq %r13, %r13
	xorq %r14, %r14
	xorq %r15, %r15

	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call0)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call1)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	popq %rdi	/* load first argument from stack */
	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call2)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	popq %rsi	/* load second argument from stack */
	popq %rdi	/* load first argument from stack */
	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call3)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	popq %rdx	/* load third argument from stack */
	popq %rsi	/* load second argument from stack */
	popq %rdi	/* load first argument from stack */
	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call4)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	popq %rcx	/* load fourth argument from stack */
	popq %rdx	/* load third argument from stack */
	popq %rsi	/* load second argument from stack */
	popq %rdi	/* load first argument from stack */
	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call5)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	popq %r8	/* load fifth argument from stack */
	popq %rcx	/* load fourth argument from stack */
	popq %rdx	/* load third argument from stack */
	popq %rsi	/* load second argument from stack */
	popq %rdi	/* load first argument from stack */
	ret		/* jump to entrance function left on stack */

ENTRY(_ctx_x86_call6)
#if !__OMIT_FRAMEPOINTER__
	xorq %rbp, %rbp /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	popq %r9	/* load sixth argument from stack */
	popq %r8	/* load fifth argument from stack */
	popq %rcx	/* load fourth argument from stack */
	popq %rdx	/* load third argument from stack */
	popq %rsi	/* load second argument from stack */
	popq %rdi	/* load first argument from stack */
	ret		/* jump to entrance function left on stack */

/*
 * Do a local context switch
 */
ENTRY(ukarch_ctx_switch)
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	movq %rsp, UKARCH_CTX_OFFSETOF_SP(%rdi)       /* save ESP */
	movq UKARCH_CTX_OFFSETOF_SP(%rsi), %rsp       /* restore ESP */
	lea .Lresume(%rip), %rbx
	movq %rbx, UKARCH_CTX_OFFSETOF_IP(%rdi)       /* save EIP */
	pushq UKARCH_CTX_OFFSETOF_IP(%rsi)            /* restore EIP
						       * (to stack and return)
						       */
	ret

.Lresume:
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	ret

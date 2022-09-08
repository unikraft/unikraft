/*	$OpenBSD: asm.h,v 1.18 2019/04/02 03:35:08 mortimer Exp $	*/
/*	$NetBSD: asm.h,v 1.2 2003/05/02 18:05:47 yamt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _MACHINE_ASM_H_
#define _MACHINE_ASM_H_

# define _C_LABEL(x)	x
#define	_ASM_LABEL(x)	x

#define _ALIGN_TEXT	.align	16, 0x90
#define _ALIGN_TRAPS	.align	16, 0xcc

#define	_GENTRY(x)	.globl x; .type x,@function; x:
#define _ENTRY(x) \
	.text; _ALIGN_TRAPS; _GENTRY(x)
#define _NENTRY(x) \
	.text; _ALIGN_TEXT; _GENTRY(x)

# define RETGUARD_SETUP_OFF(x, reg, off)
# define RETGUARD_SETUP(x, reg)
# define RETGUARD_CHECK(x, reg)
# define RETGUARD_PUSH(reg)
# define RETGUARD_POP(reg)
# define RETGUARD_SYMBOL(x)

#define	ENTRY(y)	_ENTRY(_C_LABEL(y));
#define	NENTRY(y)	_NENTRY(_C_LABEL(y))
#define	ASENTRY(y)	_NENTRY(_ASM_LABEL(y));
#define	END(y)		.size y, . - y

#define	STRONG_ALIAS(alias,sym)						\
	.global alias;							\
	alias = sym
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym

#endif /* !_MACHINE_ASM_H_ */

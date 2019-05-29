/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
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
/*
 * Arch-specific events functions
 * Ported from Mini-OS
 */
#include <stdint.h>
#include <x86/cpu.h>
#include <uk/plat/config.h>
#include <uk/essentials.h>

#if defined(__x86_64__)
char irqstack[STACK_SIZE] __align(STACK_SIZE);

static struct pda {
	int irqcount;       /* offset 0 (used in x86_64.S) */
	char *irqstackptr;  /*        8 */
} cpu0_pda;
#endif

void arch_init_events(void)
{
#if defined(__x86_64__)
	asm volatile("movl %0,%%fs ; movl %0,%%gs" :: "r" (0));
	/* 0xc0000101 is MSR_GS_BASE */
	wrmsrl(0xc0000101, (uint64_t) &cpu0_pda);
	cpu0_pda.irqcount = -1;
	cpu0_pda.irqstackptr =
			(void *) ((unsigned long) irqstack + STACK_SIZE);
#endif
}

void arch_unbind_ports(void)
{
}

void arch_fini_events(void)
{
#if defined(__x86_64__)
	wrmsrl(0xc0000101, 0); /* 0xc0000101 is MSR_GS_BASE */
#endif
}

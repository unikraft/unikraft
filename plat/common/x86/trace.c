/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <trace.h>
#include <uk/print.h>

#define PAGE_SIZE 4096 /* TODO take this from another header */


void dump_regs(struct __regs *regs)
{
	uk_pr_info("RIP: %016lx CS: %04lx\n", regs->rip, regs->cs & 0xffff);
	uk_pr_info("RSP: %016lx SS: %04lx EFLAGS: %08lx\n",
			regs->rsp, regs->ss, regs->eflags);
	uk_pr_info("RAX: %016lx RBX: %016lx RCX: %016lx\n",
			regs->rax, regs->rbx, regs->rcx);
	uk_pr_info("RDX: %016lx RSI: %016lx RDI: %016lx\n",
			regs->rdx, regs->rsi, regs->rdi);
	uk_pr_info("RBP: %016lx R08: %016lx R09: %016lx\n",
			regs->rbp, regs->r8, regs->r9);
	uk_pr_info("R10: %016lx R11: %016lx R12: %016lx\n",
			regs->r10, regs->r11, regs->r12);
	uk_pr_info("R13: %016lx R14: %016lx R15: %016lx\n",
			regs->r13, regs->r14, regs->r15);
}

/* TODO to be removed; we should use uk_hexdump() instead */
void dump_mem(unsigned long addr)
{
	unsigned long i;

	if (addr < PAGE_SIZE)
		return;

	for (i = ((addr) - 16) & ~15; i < (((addr) + 48) & ~15); i++) {
		if (!(i % 16))
			uk_pr_info("\n%lx:", i);
		uk_pr_info(" %02x", *(unsigned char *) i);
	}
	uk_pr_info("\n");
}

void stack_walk(void)
{
	unsigned long bp;

	asm("movq %%rbp, %0" : "=r"(bp));

	stack_walk_for_frame(bp);
}

void stack_walk_for_frame(unsigned long frame_base)
{
	unsigned long *frame = (void *) frame_base;

	uk_pr_info("base is %#lx ", frame_base);
	uk_pr_info("caller is %#lx\n", frame[1]);
	if (frame[0])
		stack_walk_for_frame(frame[0]);
}

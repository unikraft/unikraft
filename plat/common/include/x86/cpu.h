/* SPDX-License-Identifier: BSD-2-Clause */
/*
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
/******************************************************************************
 * cpu.h
 *
 * CPU related macros and definitions copied from mini-os/os.h
 */

#ifndef __PLAT_COMMON_X86_CPU_H__
#define __PLAT_COMMON_X86_CPU_H__

#include <uk/arch.h>
#include <uk/arch/util.h>
#include <uk/arch/types.h>
#include <uk/arch/lcpu.h>
#include <stdint.h>
#include <uk/assert.h>
#include <uk/alloc.h>
#include <string.h>

void system_off(enum ukplat_gstate request);

#ifdef CONFIG_HAVE_SYSCALL
/* syscall entrance provided by platform library */
void _ukplat_syscall(void);

/* _init_syscall is derived from hermitux: `processor.c`:
 *
 * Copyright (c) 2010, Stefan Lankes, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static inline void _init_syscall(void)
{
	__u32 eax, ebx, ecx, edx;
	int have_syscall = 0;

	/* Check for availability of extended features */
	uk_arch_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x80000001) {
		uk_arch_cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
		have_syscall = (edx & UK_ARCH_CPUID3_SYSCALL);
	}

	if (!have_syscall)
		UK_CRASH("CPU does not support SYSCALL/SYSRET!\n");

	/* Enable and program syscall/sysret */
	uk_arch_wrmsrl(UK_ARCH_MSR_EFER,
		       uk_arch_rdmsrl(UK_ARCH_MSR_EFER) |
		       UK_ARCH_EFER_LMA | UK_ARCH_EFER_LME | UK_ARCH_EFER_SCE);
	uk_arch_wrmsrl(UK_ARCH_MSR_STAR, (0x08ULL << 48) | (0x08ULL << 32));
	uk_arch_wrmsrl(UK_ARCH_MSR_LSTAR, (__uptr)_ukplat_syscall);

	/* Clear IF flag during an interrupt */
	uk_arch_wrmsrl(UK_ARCH_MSR_SYSCALL_MASK,
		       UK_ARCH_EFLAGS_TF | UK_ARCH_EFLAGS_DF |
		       UK_ARCH_EFLAGS_IF | UK_ARCH_EFLAGS_AC |
		       UK_ARCH_EFLAGS_NT);

	uk_pr_info("SYSCALL entrance @ %p\n", _ukplat_syscall);
}
#endif /* CONFIG_HAVE_SYSCALL */

#if CONFIG_HAVE_X86PKU
static inline void _check_ospke(void)
{
	__u32 eax, ebx, ecx, edx;
	uk_arch_cpuid(0x7, 0, &eax, &ebx, &ecx, &edx);
	if (!(ecx & UK_ARCH_CPUID7_ECX_OSPKE)) {
		/* if PKU is not enabled, abort the boot process. Images
		 * compiled with HAVE_X86PKU are *specialized* to be executed on
		 * PKU-enabled hardware. This allows us to avoid checks later at
		 * runtime. */
		UK_CRASH("CPU does not support PKU!\n");
	}
}
#endif /* CONFIG_HAVE_X86PKU */

#endif /* __PLAT_COMMON_X86_CPU_H__ */

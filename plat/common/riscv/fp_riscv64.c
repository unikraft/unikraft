/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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
#include <uk/plat/common/cpu.h>

void fpsimd_save_state(uintptr_t ptr)
{
	__u32 fcsr;

	__asm__ __volatile__(
		"csrr %0, fcsr\n"
		"fsd f0, 0(%1)\n"
		"fsd f1, 8(%1)\n"
		"fsd f2, 16(%1)\n"
		"fsd f3, 24(%1)\n"
		"fsd f4, 32(%1)\n"
		"fsd f5, 40(%1)\n"
		"fsd f6, 48(%1)\n"
		"fsd f7, 56(%1)\n"
		"fsd f8, 64(%1)\n"
		"fsd f9, 72(%1)\n"
		"fsd f10, 80(%1)\n"
		"fsd f11, 88(%1)\n"
		"fsd f12, 96(%1)\n"
		"fsd f13, 104(%1)\n"
		"fsd f14, 112(%1)\n"
		"fsd f15, 120(%1)\n"
		"fsd f16, 128(%1)\n"
		"fsd f17, 136(%1)\n"
		"fsd f18, 144(%1)\n"
		"fsd f19, 152(%1)\n"
		"fsd f20, 160(%1)\n"
		"fsd f21, 168(%1)\n"
		"fsd f22, 176(%1)\n"
		"fsd f23, 184(%1)\n"
		"fsd f24, 192(%1)\n"
		"fsd f25, 200(%1)\n"
		"fsd f26, 208(%1)\n"
		"fsd f27, 216(%1)\n"
		"fsd f28, 224(%1)\n"
		"fsd f29, 232(%1)\n"
		"fsd f30, 240(%1)\n"
		"fsd f31, 248(%1)\n"
		: "=&r"(fcsr) : "r"(ptr));

	((struct fpsimd_state *)ptr)->fcsr = fcsr;
}

void fpsimd_restore_state(uintptr_t ptr)
{
	__u32 fcsr;

	fcsr = ((struct fpsimd_state *)ptr)->fcsr;

	__asm__ __volatile__(
		"csrw fcsr, %0\n"
		"fld f0, 0(%1)\n"
		"fld f1, 8(%1)\n"
		"fld f2, 16(%1)\n"
		"fld f3, 24(%1)\n"
		"fld f4, 32(%1)\n"
		"fld f5, 40(%1)\n"
		"fld f6, 48(%1)\n"
		"fld f7, 56(%1)\n"
		"fld f8, 64(%1)\n"
		"fld f9, 72(%1)\n"
		"fld f10, 80(%1)\n"
		"fld f11, 88(%1)\n"
		"fld f12, 96(%1)\n"
		"fld f13, 104(%1)\n"
		"fld f14, 112(%1)\n"
		"fld f15, 120(%1)\n"
		"fld f16, 128(%1)\n"
		"fld f17, 136(%1)\n"
		"fld f18, 144(%1)\n"
		"fld f19, 152(%1)\n"
		"fld f20, 160(%1)\n"
		"fld f21, 168(%1)\n"
		"fld f22, 176(%1)\n"
		"fld f23, 184(%1)\n"
		"fld f24, 192(%1)\n"
		"fld f25, 200(%1)\n"
		"fld f26, 208(%1)\n"
		"fld f27, 216(%1)\n"
		"fld f28, 224(%1)\n"
		"fld f29, 232(%1)\n"
		"fld f30, 240(%1)\n"
		"fld f31, 248(%1)\n"
		: : "r"(fcsr), "r"(ptr));
}

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Xiangyi Meng <xymeng16@gmail.com>
 *
 * Copyright (c) 2022. All rights reserved.
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

#ifndef _UK_SGX_ASM_H_
#define _UK_SGX_ASM_H_

/* __cpuid_count(unsinged int info[4], unsigned int leaf, unsigned int subleaf); */
#define __cpuid_count(x, y, z)                                                       \
	asm volatile("cpuid"                                                   \
		     : "=a"(x[0]), "=b"(x[1]), "=c"(x[2]), "=d"(x[3])          \
		     : "a"(y), "c"(z))

/* __cpuid(unsinged int info[4], unsigned int leaf); */
#define __cpuid(x, y)                                                    \
	asm volatile("cpuid"                                                   \
		     : "=a"(x[0]), "=b"(x[1]), "=c"(x[2]), "=d"(x[3])          \
		     : "a"(y))

#define Genu 0x756e6547
#define ineI 0x49656e69
#define ntel 0x6c65746e

#define SGX_CPUID 0x12

enum sgx_cpuid {
	SGX_CPUID_CAPABILITIES	= 0,
	SGX_CPUID_ATTRIBUTES	= 1,
	SGX_CPUID_EPC_BANKS	= 2,
};

enum sgx_commands {
	ECREATE	= 0x0,
	EADD	= 0x1,
	EINIT	= 0x2,
	EREMOVE	= 0x3,
	EDGBRD	= 0x4,
	EDGBWR	= 0x5,
	EEXTEND	= 0x6,
	ELDU	= 0x8,
	EBLOCK	= 0x9,
	EPA	= 0xA,
	EWB	= 0xB,
	ETRACK	= 0xC,
	EAUG	= 0xD,
	EMODPR	= 0xE,
	EMODT	= 0xF,
};

static inline unsigned long get_cpl(void)
{
	unsigned long cs;
	asm volatile("mov %%cs, %0" : "=r"(cs));
	return cs & 0x3;
}

#endif
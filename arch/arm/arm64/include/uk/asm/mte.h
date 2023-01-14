/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, Michalis Pappas <mpappas@fastmail.fm>.
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

#ifndef __UKARCH_MEMTAG_H__
#error Do not include this header directly
#endif

#define ARM64_FEAT_MTE		1
#define ARM64_FEAT_MTE2		2
#define ARM64_FEAT_MTE3		3

#define MTE_TAG_GRANULE		16
#define MTE_TAG_MASK		(0xFULL << 56)

#define MTE_TCF_IGNORE		0UL
#define MTE_TCF_SYNC		1UL
#define MTE_TCF_ASYNC		2UL
#define MTE_TCF_ASYMMETRIC	3UL

#define PSTATE_TCO_SET() ({					\
	__asm__ __volatile__ ("msr	tco, %0\n"		\
			      : : "r"(PSTATE_TCO_BIT));		\
})

#define PSTATE_TCO_CLEAR() ({					\
	__asm__ __volatile__ ("msr	tco, %0\n"		\
			      : : "r"(0));			\
})

/**
 * Tags address with a random tag.
 *
 * If the address is tagged, this function makes sure that the
 * generated tag does not match the current one.
 *
 * @param  addr input address
 * @return tagged address
 */
static inline __u64 mte_insert_random_tag(__u64 addr)
{
	__u64 reg;

	__asm__ __volatile__("gmi	%x0, %x1, xzr\n"
			     "irg	%x1, %x1, %x0\n"
			     : "=r"(reg) : "r" (addr));
	return addr;
}

/**
 * Reads tag from allocation memory
 *
 * @param  addr allocation address
 * @return allocation tag
 */
static inline __u64 mte_load_alloc(__u64 addr)
{
	__u64 tag;

	__asm__ __volatile__ ("ldg	%x0, [%x1]\n"
			      : "=&r" (tag) : "r"(addr));
	return tag;
}

/**
 * Writes a tag into tag allocation memory.
 * The tag is derived from the input address.
 *
 * @param addr tagged address
 */
static inline void mte_store_alloc(__u64 addr)
{
	__asm__ __volatile__ ("stg	%x0, [%x0]"
			      : : "r"(addr) : "memory");
}


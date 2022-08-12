/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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
#ifndef __CPU_ARM_64_DEFS_H__
#define __CPU_ARM_64_DEFS_H__

#include <uk/asm.h>
#include <uk/arch/lcpu.h>

/*
 * Definitions for Block and Page descriptor attributes
 */
/* Level 0 table, 512GiB per entry */
#define L0_SHIFT	39
#define L0_SIZE		(_AC(1, UL) << L0_SHIFT)
#define L0_OFFSET	(L0_SIZE - _AC(1, UL))
#define L0_INVAL	0x0 /* An invalid address */
	/* 0x1 Level 0 doesn't support block translation */
	/* 0x2 also marks an invalid address */
#define L0_TABLE	0x3 /* A next-level table */

/* Level 1 table, 1GiB per entry */
#define L1_SHIFT	30
#define L1_SIZE 	(1 << L1_SHIFT)
#define L1_OFFSET 	(L1_SIZE - 1)
#define L1_INVAL	L0_INVAL
#define L1_BLOCK	0x1
#define L1_TABLE	L0_TABLE

/* Level 2 table, 2MiB per entry */
#define L2_SHIFT	21
#define L2_SIZE 	(1 << L2_SHIFT)
#define L2_OFFSET 	(L2_SIZE - 1)
#define L2_INVAL	L1_INVAL
#define L2_BLOCK	L1_BLOCK
#define L2_TABLE	L1_TABLE

#define L2_BLOCK_MASK	_AC(0xffffffe00000, UL)

/* Level 3 table, 4KiB per entry */
#define L3_SHIFT	12
#define L3_SIZE 	(1 << L3_SHIFT)
#define L3_OFFSET 	(L3_SIZE - 1)
#define L3_SHIFT	12
#define L3_INVAL	0x0
	/* 0x1 is reserved */
	/* 0x2 also marks an invalid address */
#define L3_PAGE		0x3

#define L0_ENTRIES_SHIFT 9
#define L0_ENTRIES	(1 << L0_ENTRIES_SHIFT)
#define L0_ADDR_MASK	(L0_ENTRIES - 1)

#define Ln_ENTRIES_SHIFT 9
#define Ln_ENTRIES	(1 << Ln_ENTRIES_SHIFT)
#define Ln_ADDR_MASK	(Ln_ENTRIES - 1)
#define Ln_TABLE_MASK	((1 << 12) - 1)
#define Ln_TABLE	0x3
#define Ln_BLOCK	0x1

#endif /* __CPU_ARM_64_DEFS_H__ */

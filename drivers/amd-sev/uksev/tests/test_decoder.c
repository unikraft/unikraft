/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/paging.h>
#include <uk/sev.h>
#include <uk/test.h>
#include "../decoder.h"

static void u32_mov_r_rm_asm()
{
	// 89 07
	__asm__("movl %eax, (%rdi);");
}

static void u64_mov_r_rm_asm()
{
	// 48 89 07
	__asm__("movq %rax, (%rdi);");
}

static void u64_mov_r_m_asm()
{
	// 48 A3 EF BE AD DE 00 00 00 00
	__asm__("movq %rax, (0xdeadbeef);");
}

UK_TESTCASE(uksev_decoder, test_decode_inst)
{
	struct __regs regs = {0};
	struct uk_sev_decoded_inst instruction;
	unsigned long val;
	unsigned long* reg;
	int ret;

	regs.rdi = 0xcafebabe;
	regs.rax = 0xdeadbeef;

	ret = uk_sev_decode_inst((__vaddr_t) &u32_mov_r_rm_asm, &instruction);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(instruction.length, 2);
	UK_TEST_EXPECT_SNUM_EQ(instruction.opcode, 0x89);

	ret = uk_sev_inst_get_mem_reg_operand(&instruction, &regs, &reg);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(*reg, 0xcafebabe);

	ret = uk_sev_inst_get_reg_operand(&instruction, &regs, &reg);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(*reg, 0xdeadbeef);

	ret = uk_sev_decode_inst((__vaddr_t) &u64_mov_r_rm_asm, &instruction);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(instruction.length, 3);
	UK_TEST_EXPECT_SNUM_EQ(instruction.opcode, 0x89);

	ret = uk_sev_inst_get_mem_reg_operand(&instruction, &regs, &reg);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(*reg, 0xcafebabe);

	ret = uk_sev_inst_get_reg_operand(&instruction, &regs, &reg);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(*reg, 0xdeadbeef);

	ret = uk_sev_decode_inst((__vaddr_t) &u64_mov_r_m_asm, &instruction);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(instruction.length, 10);
	UK_TEST_EXPECT_SNUM_EQ(instruction.opcode, 0xA3);

	ret = uk_sev_inst_get_displacement(&instruction, &val);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(val, 0xdeadbeef);

	ret = uk_sev_inst_get_reg_operand(&instruction, &regs, &reg);
	UK_TEST_EXPECT_SNUM_EQ(ret, 0);
	UK_TEST_EXPECT_SNUM_EQ(*reg, 0xdeadbeef);
}

uk_testsuite_register(uksev_decoder, NULL);

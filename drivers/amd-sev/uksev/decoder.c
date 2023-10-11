/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <Zydis/Zydis.h>
#include <uk/arch/paging.h>
#include "Zydis/SharedTypes.h"
#include "Zydis/Utils.h"
#include "decoder.h"
#include "errno.h"

ZydisDecoder decoder;
ZydisFormatter formatter;


void uk_sev_pr_instruction(struct uk_sev_decoded_inst *instruction, char *buffer, __sz len)
{
	ZydisFormatterFormatInstruction(&formatter, &instruction->_zydis_inst,
					buffer, sizeof(buffer), 0);
	/* uk_pr_debug("%s\n", buffer); */
};

int uk_sev_decoder_init(void)
{
	ZyanStatus status = ZydisDecoderInit(
	    &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
	if (unlikely(!ZYAN_SUCCESS(status))) {
		return -1;
	}
	status = ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
	if (unlikely(!ZYAN_SUCCESS(status))) {
		return -1;
	}

	return 0;
}

int uk_sev_decode_inst(__vaddr_t addr, struct uk_sev_decoded_inst *instruction)
{

	ZydisDecodedInstruction *_inst = &instruction->_zydis_inst;
	ZyanStatus status;
	status = ZydisDecoderDecodeBuffer(&decoder, (void *)(addr),
					  ZYDIS_MAX_INSTRUCTION_LENGTH, _inst);
	if (unlikely(!ZYAN_SUCCESS(status))) {
		return -1;
	}

	instruction->runtime_addr = addr;
	instruction->opcode = _inst->opcode;
	instruction->length = _inst->length;
	instruction->immediate = (__u32)_inst->raw.imm[0].value.u;
	instruction->address_width = _inst->address_width;
	instruction->operand_width = _inst->operand_width;
	instruction->has_rep = (_inst->attributes & ZYDIS_ATTRIB_HAS_REP);

	return 0;
}

#define CASE_ZYDIS_REGISTER_ALL2(_reg, field)                                  \
	case ZYDIS_REGISTER_##_reg##L:                                         \
	case ZYDIS_REGISTER_##_reg:                                            \
	case ZYDIS_REGISTER_E##_reg:                                           \
	case ZYDIS_REGISTER_R##_reg:                                           \
		*reg = &regs->field;                                           \
		break;

#define CASE_ZYDIS_REGISTER_ALL(_reg, field)                                   \
	case ZYDIS_REGISTER_##_reg##H:                                         \
		*reg = &regs->field + 1;                                       \
		break;                                                         \
	case ZYDIS_REGISTER_##_reg##L:                                         \
	case ZYDIS_REGISTER_##_reg##X:                                         \
	case ZYDIS_REGISTER_E##_reg##X:                                        \
	case ZYDIS_REGISTER_R##_reg##X:                                        \
		*reg = &regs->field;                                           \
		break;

#define CASE_ZYDIS_EXT_REGISTER_ALL(_reg, field)                               \
	case ZYDIS_REGISTER_##_reg:                                            \
	case ZYDIS_REGISTER_##_reg##B:                                         \
	case ZYDIS_REGISTER_##_reg##W:                                         \
	case ZYDIS_REGISTER_##_reg##D:                                         \
		*reg = &regs->field;                                           \
		break;

int uk_sev_zydis_reg_to_reg_ptr(ZydisRegister zydis_reg, struct __regs *regs,
				unsigned long **reg)
{
	switch (zydis_reg) {
		CASE_ZYDIS_REGISTER_ALL(A, rax)
		CASE_ZYDIS_REGISTER_ALL(C, rcx)
		CASE_ZYDIS_REGISTER_ALL(D, rdx)
		CASE_ZYDIS_REGISTER_ALL(B, rbx)

		CASE_ZYDIS_EXT_REGISTER_ALL(R15, r15)
		CASE_ZYDIS_EXT_REGISTER_ALL(R14, r14)
		CASE_ZYDIS_EXT_REGISTER_ALL(R13, r13)
		CASE_ZYDIS_EXT_REGISTER_ALL(R12, r12)
		CASE_ZYDIS_EXT_REGISTER_ALL(R11, r11)
		CASE_ZYDIS_EXT_REGISTER_ALL(R10, r10)
		CASE_ZYDIS_EXT_REGISTER_ALL(R9, r9)
		CASE_ZYDIS_EXT_REGISTER_ALL(R8, r8)

		CASE_ZYDIS_REGISTER_ALL2(BP, rbp)
		CASE_ZYDIS_REGISTER_ALL2(SI, rsi)
		CASE_ZYDIS_REGISTER_ALL2(DI, rdi)
		CASE_ZYDIS_REGISTER_ALL2(SP, rsp)

	case ZYDIS_REGISTER_IP:
	case ZYDIS_REGISTER_EIP:
	case ZYDIS_REGISTER_RIP:
		*reg = &regs->rip;
		break;

	case ZYDIS_REGISTER_RFLAGS:
		*reg = &regs->eflags;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}
int uk_sev_inst_get_reg_operand(struct uk_sev_decoded_inst *instruction,
				struct __regs *regs, unsigned long **reg)
{
	int i;
	ZydisDecodedInstruction *_inst = &instruction->_zydis_inst;

	for (i = 0; i < _inst->operand_count; i++) {
		if (_inst->operands[i].type == ZYDIS_OPERAND_TYPE_REGISTER) {
			return uk_sev_zydis_reg_to_reg_ptr(_inst->operands[i].reg.value, regs, reg);
		}
	}

	/* Register operand not found */
	return -ENOTSUP;
}

int uk_sev_inst_get_mem_reg_operand(struct uk_sev_decoded_inst *instruction,
				struct __regs *regs, unsigned long **reg)
{

	int i;
	ZydisDecodedInstruction *_inst = &instruction->_zydis_inst;

	for (i = 0; i < _inst->operand_count; i++) {
		if (_inst->operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY) {
			return uk_sev_zydis_reg_to_reg_ptr(_inst->operands[i].mem.base, regs, reg);
		}
	}

	/* Valid memory operand not found */
	return -ENOTSUP;
}

int uk_sev_inst_get_displacement(struct uk_sev_decoded_inst *instruction, unsigned long *val)
{

	int i;
	ZydisDecodedInstruction *_inst = &instruction->_zydis_inst;

	for (i = 0; i < _inst->operand_count; i++) {
		if (_inst->operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY) {


			/* __u64 addr; */
			/* ZydisCalcAbsoluteAddress(_inst, &_inst->operands[i], */
			/* 			 instruction->runtime_addr, */
			/* 			 &addr); */

			/* uk_pr_info("ZydisCalcAbsoluteAddress: 0x%lx\n",
			 * addr); */
			if (_inst->operands[i].mem.disp.has_displacement) {
				*val = _inst->operands[i].mem.disp.value;
				return 0;
			}
		}
	}

	/* Valid memory operand not found */
	return -ENOTSUP;
}

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_SEV_DECODER_H__
#define __UK_SEV_DECODER_H__

#include <uk/essentials.h>
#include <x86/cpu.h>

#include <Zydis/Zydis.h>

struct uk_sev_decoded_inst {
	__u8 length;
	__u8 opcode;
	__u8 operand_width;
	__u8 address_width;
	__u32 immediate;
	__u8 has_rep; /* Has rep prefix */
	__u64 runtime_addr;

	/* Library-dependent structure, so that we can switch out other
	 * disassembler in the future */
	ZydisDecodedInstruction _zydis_inst;
};

/**
 * Decode the instruction at the given address
 *
 * @param addr The virtual address of the instruction
 * @param decoded_inst Pointer to the struct containing the decoded instruction
 * @return 0 on success, < 0 otherwise
 *
 */
int uk_sev_decode_inst(__vaddr_t addr,
		       struct uk_sev_decoded_inst *decoded_inst);
int uk_sev_decoder_init(void);
int uk_sev_inst_get_reg_operand(struct uk_sev_decoded_inst *instruction,
				struct __regs *regs, unsigned long **reg);

int uk_sev_inst_get_mem_reg_operand(struct uk_sev_decoded_inst *instruction,
				struct __regs *regs, unsigned long **reg);

int uk_sev_inst_get_displacement(struct uk_sev_decoded_inst *instruction,
				unsigned long *val);

void uk_sev_pr_instruction(struct uk_sev_decoded_inst *instruction, char* buffer, __sz len);

#endif /* __UK_SEV_DECODER_H__ */

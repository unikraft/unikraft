/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SEV_H__
#define __UK_SEV_H__

#include <uk/arch/lcpu.h>
#include <uk/asm/sev.h>

/* Macros for handling IOIO */
/* exitinfo1[31:16]: I/O port */
#define UK_SEV_IOIO_PORT(port)		(((port) & (UK_BIT(16) - 1)) << 16)
/* exitinfo1[12:10]: Segment selector */
#define UK_SEV_IOIO_SEG(seg)		(((seg) & (UK_BIT(3) - 1)) << 10)
#define UK_SEV_IOIO_SEG_ES		0
#define UK_SEV_IOIO_SEG_DS		3
#define UK_SEV_IOIO_A64			UK_BIT(9)
#define UK_SEV_IOIO_A32 		UK_BIT(8)
#define UK_SEV_IOIO_A16 		UK_BIT(7)
#define UK_SEV_IOIO_SZ32		UK_BIT(6)
#define UK_SEV_IOIO_SZ16 		UK_BIT(5)
#define UK_SEV_IOIO_SZ8			UK_BIT(4)
#define UK_SEV_IOIO_REP 		UK_BIT(3)
#define UK_SEV_IOIO_STR 		UK_BIT(2)
#define UK_SEV_IOIO_TYPE_OUT		0
#define UK_SEV_IOIO_TYPE_IN		UK_BIT(0)


int uk_sev_mem_encrypt_init(void);
int uk_sev_early_vc_handler_init(void);

int uk_sev_setup_ghcb(void);
void uk_sev_terminate(int set, int reason);
int uk_sev_ghcb_initialized(void);
int uk_sev_cpu_features_check(void);
int uk_sev_set_memory_private(__vaddr_t addr, unsigned long num_pages);
int uk_sev_set_memory_shared(__vaddr_t addr, unsigned long num_pages);

int uk_sev_set_pages_state(__vaddr_t vstart, __paddr_t pstart, unsigned long num_pages,
			   int page_state);

/* The #VC handler executed before a proper GHCB is set up. Only supports CPUID
 * #VC triggered by CPUID calls. */
int do_vmm_comm_exception_no_ghcb(struct __regs *regs,
				   unsigned long error_code);
void do_vmm_comm_exception(struct __regs *regs, unsigned long error_code);
void uk_sev_terminate(int set, int reason);

#endif /* __UK_SEV_H__ */

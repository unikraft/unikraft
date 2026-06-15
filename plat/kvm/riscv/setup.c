/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
 * Some parts from _init_paging() are taken from the x86 setup.c source code:
 *
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Felipe Huici <felipe.huici@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017 NEC Europe Ltd., NEC Corporation
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

#include <uk/config.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/plat/common/sections.h>
#include <uk/essentials.h>
#include <uk/boot.h>
#include <uk/plat/memory.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <libfdt.h>
#include <riscv/sbi.h>
#include <uk/plat/time.h>
#include <uk/intctlr.h>
#include <uk/lcpu.h>

void _ukplat_entry(void)
{

	struct ukplat_bootinfo *bi;
	void *bstack;
	int rc;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Could not retrieve bootinfo\n");

	uk_boot_early_init(bi);

	/* Allocate boot stack */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE);

	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize paging (%d)\n", rc);

	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");
	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	uk_pr_info("Entering from KVM (riscv64)...\n");

	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Interrupt controller init failed: %d\n", rc);

	rc = uk_lcpu_init(uk_lcpu_get_bsp());
	if (unlikely(rc))
		UK_CRASH("Bootstrap processor init failed: %d\n", rc);

	/**uk_pr_info("     heap start: %p\n", (void *)_libkvmplat_cfg.heap.start);*/
	uk_pr_info("Switch from bootstrap stack to stack @%p\n", bstack);

	uk_arch_riscv64_jump_to((__u64)bstack, (__u64)uk_boot_entry);
	uk_lcpu_halt();
}

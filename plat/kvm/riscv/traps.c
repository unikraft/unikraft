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

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk/event.h>
#include <uk/intctlr.h>
#include <uk/intctlr/plic.h>
#include <uk/pm.h>
#include <uk/plat/native/except.h>
#include <uk/print.h>
#include <stdint.h>

extern void __trap_handler(void);

static const char * const riscv_exception_table[] = {
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_INVALID_OP] = "invalid op",
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_DEBUG] = "debug",
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_PAGE_FAULT] = "page fault",
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_BUS_ERROR] = "bus error",
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_MATH] = "floating point",
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SECURITY] = "security",
	[UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SYSCALL] = "system call",
};

static int raise_trap_event(enum uk_plat_native_riscv64_except_id exception,
			    struct uk_plat_native_except_err_ctx *ctx)
{
	switch (exception) {
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_INVALID_OP:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_INVALID_OP,
				      ctx);
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_DEBUG:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_DEBUG, ctx);
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_PAGE_FAULT:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_PAGE_FAULT,
				      ctx);
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_BUS_ERROR:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_BUS_ERROR,
				      ctx);
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_MATH:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_MATH, ctx);
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SECURITY:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_ERR_SECURITY,
				      ctx);
	case UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SYSCALL:
		return uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_SYSCALL, ctx);
	default:
		return UK_EVENT_NOT_HANDLED;
	}
}

static enum uk_plat_native_riscv64_except_id
scause_to_exception(unsigned long scause)
{
	switch (scause) {
	case UK_ARCH_RISCV64_CAUSE_ILLEGAL_INSTRUCTION:
		return UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_INVALID_OP;
	case UK_ARCH_RISCV64_CAUSE_BREAKPOINT:
		return UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_DEBUG;
	case UK_ARCH_RISCV64_CAUSE_FETCH_PAGE_FAULT:
	case UK_ARCH_RISCV64_CAUSE_LOAD_PAGE_FAULT:
	case UK_ARCH_RISCV64_CAUSE_STORE_PAGE_FAULT:
		return UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_PAGE_FAULT;
	case UK_ARCH_RISCV64_CAUSE_MISALIGNED_FETCH:
	case UK_ARCH_RISCV64_CAUSE_FETCH_ACCESS:
	case UK_ARCH_RISCV64_CAUSE_MISALIGNED_LOAD:
	case UK_ARCH_RISCV64_CAUSE_LOAD_ACCESS:
	case UK_ARCH_RISCV64_CAUSE_MISALIGNED_STORE:
	case UK_ARCH_RISCV64_CAUSE_STORE_ACCESS:
		return UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_BUS_ERROR;
	case UK_ARCH_RISCV64_CAUSE_SUPERVISOR_ECALL:
		return UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_SYSCALL;
	default:
		return UK_PLAT_NATIVE_RISCV64_EXCEPT_ID_INVALID_OP;
	}
}

static void do_sync_exception(struct __regs *regs, unsigned long scause)
{
	struct uk_plat_native_except_err_ctx ctx;
	enum uk_plat_native_riscv64_except_id exception;
	int rc;

	exception = scause_to_exception(scause);

	ctx = (struct uk_plat_native_except_err_ctx) {
		.eid = exception,
		.str = riscv_exception_table[exception],
		.scause = scause,
		.stval = _csr_read(UK_ARCH_RISCV64_CSR_STVAL),
		.handler_err = 0,
		.regs = (struct uk_plat_native_regs *)regs,
	};

	rc = raise_trap_event(exception, &ctx);
	if (unlikely(rc < 0))
		uk_pr_crit("event handler returned error: %d\n", rc);
	else if (rc != UK_EVENT_NOT_HANDLED)
		return;

	rc = uk_raise_event(UK_PLAT_NATIVE_EXCEPT_EVENT_UNHANDLED, &ctx);
	if (unlikely(rc == UK_EVENT_NOT_HANDLED || rc < 0))
		uk_pr_crit("Unhandled Trap %d (%s), scause=0x%lx, stval=0x%lx\n",
			   exception, ctx.str, ctx.scause, ctx.stval);

	uk_pm_syscrash();
}

static void do_timer_irq(struct __regs *regs)
{
	struct uk_plat_native_except_irq_ctx ctx = {
		.regs = (struct uk_plat_native_regs *)regs,
		.irq = 0
	};

	uk_intctlr_irq_handle((struct uk_lcpu_except_irq_ctx *)&ctx);
}

void _trap_handler(struct __regs *regs)
{
	unsigned long scause = _csr_read(UK_ARCH_RISCV64_CSR_SCAUSE);

	switch (scause) {
	case UK_ARCH_RISCV64_CAUSE_SUPERVISOR_EXT:
		plic_handle_irq(regs);
		break;

	case UK_ARCH_RISCV64_CAUSE_SUPERVISOR_TIMER:
		/*
		 * Timer interrupts are not routed through the PLIC, so
		 * call _ukplat_irq_handle directly.
		 */
		do_timer_irq(regs);
		plic_ack_irq(0);
		break;

	default:
		do_sync_exception(regs, scause);
	}
}

__isr __uptr uk_plat_native_riscv64_traps_get_except_stack_base(void)
{
	return 0;
}

__isr void uk_plat_native_riscv64_traps_init(void)
{
	uintptr_t handler = (uintptr_t)&__trap_handler;

	_csr_write(UK_ARCH_RISCV64_CSR_STVEC,
		   handler | UK_ARCH_RISCV64_STVEC_MODE_DIRECT);

	uk_pr_debug("sscratch: 0x%lx\n",
		    _csr_read(UK_ARCH_RISCV64_CSR_SSCRATCH));
	uk_pr_debug("sip: 0x%lx\n", _csr_read(UK_ARCH_RISCV64_CSR_SIP));
	uk_pr_debug("sie: 0x%lx\n", _csr_read(UK_ARCH_RISCV64_CSR_SIE));
	uk_pr_debug("sstatus: 0x%lx\n",
		    _csr_read(UK_ARCH_RISCV64_CSR_SSTATUS));
	uk_pr_debug("stvec: 0x%lx\n", _csr_read(UK_ARCH_RISCV64_CSR_STVEC));
}

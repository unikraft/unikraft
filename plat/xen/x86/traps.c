/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Taken from Mini-OS */

#include <stddef.h>

#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/pm.h>
#include <uk/pcpuvar.h>
#include <uk/print.h>
#include <uk/plat/xen/auxsp.h>

#include <common/hypervisor.h>
#include <xen-x86/smp.h>
#include <xen-x86/traps.h>
#include <xen-x86/hypercall.h>

__uk_pcpuvar int xen_irqcount = -1;
__uk_pcpuvar __uptr uk_plat_xen_auxsp;

#ifdef XEN_PARAVIRT

/* Construct a trap table entry for trapname */
#define TRAP_TABLE_ENTRY(trapname, ring)				\
	{ UK_ARCH_X86_64_TRAPNUM_##trapname, (ring), __KERNEL_CS,	\
	  (unsigned long)ASM_TRAP_SYM(trapname) }

/*
 * Submit a virtual IDT to the hypervisor. This consists of tuples
 * (interrupt vector, privilege ring, CS:EIP of handler).
 * The 'privilege ring' field specifies the least-privileged ring that
 * can trap to that vector using a software-interrupt instruction (INT).
 */
static trap_info_t trap_table[] = {
	TRAP_TABLE_ENTRY(DIVIDE_ERROR, 0),
	TRAP_TABLE_ENTRY(DEBUG, 0),
	TRAP_TABLE_ENTRY(INT3, 3),
	TRAP_TABLE_ENTRY(OVERFLOW, 3),
	TRAP_TABLE_ENTRY(BOUNDS, 3),
	TRAP_TABLE_ENTRY(INVALID_OP, 0),
	TRAP_TABLE_ENTRY(NO_DEVICE, 0),
	TRAP_TABLE_ENTRY(COPROC_SEG_OVERRUN, 0),
	TRAP_TABLE_ENTRY(INVALID_TSS, 0),
	TRAP_TABLE_ENTRY(NO_SEGMENT, 0),
	TRAP_TABLE_ENTRY(STACK_ERROR, 0),
	TRAP_TABLE_ENTRY(GP_FAULT, 0),
	TRAP_TABLE_ENTRY(PAGE_FAULT, 0),
	TRAP_TABLE_ENTRY(SPURIOUS_INT, 0),
	TRAP_TABLE_ENTRY(COPROC_ERROR, 0),
	TRAP_TABLE_ENTRY(ALIGNMENT_CHECK, 0),
	TRAP_TABLE_ENTRY(SIMD_ERROR, 0),
	TRAP_TABLE_ENTRY(SECURITY_ERROR, 0),
	{ 0, 0, 0, 0 }
};

void xen_traps_init(void)
{
	int r;

	r = HYPERVISOR_set_trap_table(trap_table);
	if (unlikely(r))
		UK_CRASH("Error setting trap table: %d", r);

	r = HYPERVISOR_set_callbacks(
		(unsigned long)asm_trap_HYPERVISOR_CALLBACK,
		(unsigned long)asm_failsafe_callback,
		0);
	if (unlikely(r))
		UK_CRASH("Error setting callbacks: %d", r);
}

UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_DEBUG);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_ERR_X86_GP);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_SYSCALL);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_NMI);
UK_EVENT(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED);

static const char *x86_exception_table[] = {
	"divide error",           /* TRAPNUM_DIVIDE_ERROR (0) */
	"debug",                  /* TRAPNUM_DEBUG (1) */
	"NMI",                    /* TRAPNUM_NMI (2) */
	"int3",                   /* TRAPNUM_INT3 (3) */
	"overflow",               /* TRAPNUM_OVERFLOW (4) */
	"bounds",                 /* TRAPNUM_BOUNDS (5) */
	"invalid opcode",         /* TRAPNUM_INVALID_OP (6) */
	"device not available",   /* TRAPNUM_NO_DEVICE (7) */
	"double fault",           /* TRAPNUM_DOUBLE_FAULT (8) */
	__NULL,		      /* TRAPNUM_COPROC_SEG_OVERRUN (9) */
	"invalid TSS",           /* TRAPNUM_INVALID_TSS (10) */
	"segment not present",   /* TRAPNUM_NO_SEGMENT (11) */
	"stack segment",         /* TRAPNUM_STACK_ERROR (12) */
	"general protection",    /* TRAPNUM_GP_FAULT (13) */
	"page fault",            /* TRAPNUM_PAGE_FAULT (14) */
	__NULL,                  /* reserved (15) */
	"coprocessor",           /* TRAPNUM_COPROC_ERROR (16) */
	"alignment check",       /* TRAPNUM_ALIGNMENT_CHECK (17) */
	"machine check",         /* TRAPNUM_MACHINE_CHECK (18) */
	"SIMD coprocessor",      /* TRAPNUM_SIMD_ERROR (19) */
	"virtualization error",  /* TRAPNUM_VIRT_ERROR (20) */
	"control protection",    /* TRAPNUM_SECURITY_ERROR (21) */
};

static struct uk_event *trap_event_table[] = {
	[UK_ARCH_X86_64_TRAPNUM_DIVIDE_ERROR] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_X86_64_TRAPNUM_DEBUG] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_DEBUG),
	[UK_ARCH_X86_64_TRAPNUM_NMI] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_NMI),
	[UK_ARCH_X86_64_TRAPNUM_INT3] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_DEBUG),
	[UK_ARCH_X86_64_TRAPNUM_OVERFLOW] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_X86_64_TRAPNUM_BOUNDS] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_X86_64_TRAPNUM_INVALID_OP] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_INVALID_OP),
	[UK_ARCH_X86_64_TRAPNUM_NO_DEVICE] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_X86_64_TRAPNUM_DOUBLE_FAULT] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_X86_64_TRAPNUM_INVALID_TSS] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_X86_64_TRAPNUM_NO_SEGMENT] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_ARCH_X86_64_TRAPNUM_STACK_ERROR] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_ARCH_X86_64_TRAPNUM_GP_FAULT] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_X86_GP),
	[UK_ARCH_X86_64_TRAPNUM_PAGE_FAULT] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_PAGE_FAULT),
	[UK_ARCH_X86_64_TRAPNUM_COPROC_ERROR] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_X86_64_TRAPNUM_ALIGNMENT_CHECK] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_BUS_ERROR),
	[UK_ARCH_X86_64_TRAPNUM_MACHINE_CHECK] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_X86_64_TRAPNUM_SIMD_ERROR] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_MATH),
	[UK_ARCH_X86_64_TRAPNUM_VIRT_ERROR] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED),
	[UK_ARCH_X86_64_TRAPNUM_SECURITY_ERROR] =
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_ERR_SECURITY),
};

static unsigned long read_cr2(void)
{
	return HYPERVISOR_shared_info->vcpu_info[smp_processor_id()].arch.cr2;
}

void uk_plat_xen_except_err_handler(unsigned int trapnr,
				    struct uk_lcpu_regs *regs,
				    unsigned long error_code)
{
	struct uk_plat_xen_except_err_ctx ctx;
	int rc;

	ctx = (struct uk_plat_xen_except_err_ctx){
		.regs = (struct uk_plat_native_regs *)regs,
		.trapnr = trapnr,
		.str = x86_exception_table[trapnr],
		.error_code = error_code,
		.handler_err = 0,
		.cr2 = read_cr2(),
	};

	rc = uk_raise_event_ptr(trap_event_table[trapnr], &ctx);
	if (unlikely(rc < 0))
		uk_pr_crit("event handler returned error: %d\n", rc);
	else if (rc != UK_EVENT_NOT_HANDLED)
		return;

	if (trap_event_table[trapnr] ==
		UK_EVENT_PTR(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED))
		goto trap_halt;

	rc = uk_raise_event(UK_PLAT_XEN_EXCEPT_EVENT_UNHANDLED, &ctx);
	if (unlikely(rc == UK_EVENT_NOT_HANDLED || rc < 0))
		uk_pr_crit("Unhandled Trap %d (%s), error code=0x%lx\n",
			   trapnr, x86_exception_table[trapnr],
			   error_code);

trap_halt:
	/* We expect that if there is a handler it won't return,
	 * but as we can't control that, we place halt outside
	 * the above conditional.
	 */
	uk_pm_syscrash();
}

#endif

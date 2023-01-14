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
#include <xen-x86/traps.h>
#include <xen-x86/hypercall.h>
#include <uk/print.h>
#include <uk/plat/common/lcpu.h>

/* Traps used only on Xen */

DECLARE_TRAP_EC(coproc_seg_overrun, "coprocessor segment overrun", NULL)
DECLARE_TRAP   (spurious_int,       "spurious interrupt bug",      NULL)


#ifdef CONFIG_PARAVIRT

#define TRAP_TABLE_ENTRY(trapname, pl) \
	{ TRAP_##trapname, pl, __KERNEL_CS, (unsigned long) ASM_TRAP_SYM(trapname) }

/*
 * Submit a virtual IDT to teh hypervisor. This consists of tuples
 * (interrupt vector, privilege ring, CS:EIP of handler).
 * The 'privilege ring' field specifies the least-privileged ring that
 * can trap to that vector using a software-interrupt instruction (INT).
 */
static trap_info_t trap_table[] = {
	TRAP_TABLE_ENTRY(divide_error,        0),
	TRAP_TABLE_ENTRY(debug,               0),
	TRAP_TABLE_ENTRY(int3,                3),
	TRAP_TABLE_ENTRY(overflow,            3),
	TRAP_TABLE_ENTRY(bounds,              3),
	TRAP_TABLE_ENTRY(invalid_op,          0),
	TRAP_TABLE_ENTRY(no_device,           0),
	TRAP_TABLE_ENTRY(coproc_seg_overrun,  0),
	TRAP_TABLE_ENTRY(invalid_tss,         0),
	TRAP_TABLE_ENTRY(no_segment,          0),
	TRAP_TABLE_ENTRY(stack_error,         0),
	TRAP_TABLE_ENTRY(gp_fault,            0),
	TRAP_TABLE_ENTRY(page_fault,          0),
	TRAP_TABLE_ENTRY(spurious_int,        0),
	TRAP_TABLE_ENTRY(coproc_error,        0),
	TRAP_TABLE_ENTRY(alignment_check,     0),
	TRAP_TABLE_ENTRY(simd_error,          0),
	{ 0, 0, 0, 0 }
};

void traps_lcpu_init(struct lcpu *current __unused)
{
	HYPERVISOR_set_trap_table(trap_table);

	HYPERVISOR_set_callbacks((unsigned long) asm_trap_hypervisor_callback,
				 (unsigned long) asm_failsafe_callback, 0);
}

#endif

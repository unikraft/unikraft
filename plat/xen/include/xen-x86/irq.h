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
/******************************************************************************
 * irq.h
 *
 * IRQ related macros and definitions copied from os.h
 */

#ifndef PLAT_XEN_INCLUDE_XEN_X86_IRQ_H_
#define PLAT_XEN_INCLUDE_XEN_X86_IRQ_H_

#ifdef CONFIG_PARAVIRT

#include <common/hypervisor.h>
#include <xen-x86/smp.h>

/*
 * The use of 'barrier' in the following reflects their use as local-lock
 * operations. Reentrancy must be prevented (e.g., __cli()) /before/ following
 * critical operations are executed. All critical operations must complete
 * /before/ reentrancy is permitted (e.g., __sti()). Alpha architecture also
 * includes these barriers, for example.
 */

#define __cli()                                                                \
	do {                                                                   \
		vcpu_info_t *_vcpu;                                            \
		_vcpu =                                                        \
		    &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];    \
		_vcpu->evtchn_upcall_mask = 1;                                 \
		barrier();                                                     \
	} while (0)

#define __sti()                                                                \
	do {                                                                   \
		vcpu_info_t *_vcpu;                                            \
		barrier();                                                     \
		_vcpu =                                                        \
		    &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];    \
		_vcpu->evtchn_upcall_mask = 0;                                 \
		barrier(); /* unmask then check (avoid races) */               \
		if (unlikely(_vcpu->evtchn_upcall_pending))                    \
			ukplat_lcpu_irqs_handle_pending();		       \
	} while (0)

#define __save_flags(x)                                                        \
	do {                                                                   \
		vcpu_info_t *_vcpu;                                            \
		_vcpu =                                                        \
		    &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];    \
		(x) = _vcpu->evtchn_upcall_mask;                               \
	} while (0)

#define __restore_flags(x)                                                     \
	do {                                                                   \
		vcpu_info_t *_vcpu;                                            \
		barrier();                                                     \
		_vcpu =                                                        \
		    &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];    \
		if ((_vcpu->evtchn_upcall_mask = (x)) == 0) {                  \
			barrier(); /* unmask then check (avoid races) */       \
			if (unlikely(_vcpu->evtchn_upcall_pending))            \
				ukplat_lcpu_irqs_handle_pending();             \
		}                                                              \
	} while (0)

#define safe_halt() ((void)0)

#define __save_and_cli(x)                                                      \
	do {                                                                   \
		vcpu_info_t *_vcpu;                                            \
		_vcpu =                                                        \
		    &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];    \
		(x) = _vcpu->evtchn_upcall_mask;                               \
		_vcpu->evtchn_upcall_mask = 1;                                 \
		barrier();                                                     \
	} while (0)

#define irqs_disabled()                                                        \
	HYPERVISOR_shared_info->vcpu_info[smp_processor_id()].evtchn_upcall_mask

#define local_irq_save(x)        __save_and_cli(x)
#define local_irq_restore(x)     __restore_flags(x)
#define local_save_flags(x)      __save_flags(x)
#define local_irq_disable()      __cli()
#define local_irq_enable()       __sti()

#else
#include <x86/irq.h>
#endif

#endif /* PLAT_XEN_INCLUDE_XEN_X86_IRQ_H_ */

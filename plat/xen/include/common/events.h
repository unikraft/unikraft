/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Authors: Rolf Neugebauer <neugebar@dcs.gla.ac.uk>
 *          Grzegorz Milos <gm281@cam.ac.uk>
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2003-2005, Intel Research Cambridge
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation.
 *
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
/*
 * Deals with events received on event channels
 * Ported from Mini-OS
 */

#ifndef _EVENTS_H_
#define _EVENTS_H_

#if defined(__X86_32__) || defined(__x86_64__)
#include <xen-x86/hypercall.h>
#include <xen-x86/traps.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/hypercall.h>
#else
#error "Unsupported architecture"
#endif
#include <xen/event_channel.h>
#include <uk/arch/lcpu.h>


typedef void (*evtchn_handler_t)(evtchn_port_t, struct __regs *, void *);

/* prototypes */
void arch_init_events(void);

/* Called by fini_events to close any ports opened by arch-specific code. */
void arch_unbind_ports(void);

void arch_fini_events(void);

int do_event(evtchn_port_t port, struct __regs *regs);
evtchn_port_t bind_virq(uint32_t virq, evtchn_handler_t handler, void *data);
evtchn_port_t bind_pirq(uint32_t pirq, int will_share,
			evtchn_handler_t handler, void *data);
evtchn_port_t bind_evtchn(evtchn_port_t port, evtchn_handler_t handler,
			  void *data);
void unbind_evtchn(evtchn_port_t port);

/* Disable events for <port> by setting the masking bit */
void mask_evtchn(evtchn_port_t port);

/*
 * Enable events for <port> by unsetting the masking bit.
 * If pending events are present, call ukplat_lcpu_irqs_handle_pending
 */
void unmask_evtchn(evtchn_port_t port);

/*
 * Clear pending events from <port> by unsetting the pending
 * events bit
 */
void clear_evtchn(evtchn_port_t port);

void init_events(void);
int evtchn_alloc_unbound(domid_t pal, evtchn_handler_t handler,
			 void *data, evtchn_port_t *port);
int evtchn_bind_interdomain(domid_t pal, evtchn_port_t remote_port,
			    evtchn_handler_t handler, void *data,
			    evtchn_port_t *local_port);
int evtchn_get_peercontext(evtchn_port_t local_port, char *ctx, int size);
void unbind_all_ports(void);

static inline int notify_remote_via_evtchn(evtchn_port_t port)
{
	evtchn_send_t op;

	op.port = port;
	return HYPERVISOR_event_channel_op(EVTCHNOP_send, &op);
}

void fini_events(void);
#ifdef CONFIG_MIGRATION /* TODO wip */
void suspend_events(void);
#endif

#endif /* _EVENTS_H_ */

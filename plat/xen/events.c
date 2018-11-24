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
#include <stdlib.h>
#include <stdint.h>
#include <common/hypervisor.h>
#include <common/events.h>
#include <xen/xen.h>
#include <uk/print.h>
#include <uk/bitops.h>

#define NR_EVS 1024

/* this represents a event handler. Chaining or sharing is not allowed */
typedef struct _ev_action_t {
	evtchn_handler_t handler;
	void *data;
	uint32_t count;
} ev_action_t;

static ev_action_t ev_actions[NR_EVS];
static void default_handler(evtchn_port_t port, struct __regs *regs,
			    void *data);

static unsigned long bound_ports[NR_EVS/(8*sizeof(unsigned long))];

void unbind_all_ports(void)
{
	uint32_t i;
	int cpu = 0;
	shared_info_t *s = HYPERVISOR_shared_info;
	vcpu_info_t   *vcpu_info = &s->vcpu_info[cpu];

	for (i = 0; i < NR_EVS; i++) {
#if 0 /* TODO revisit after enabling console and xenbus */
		if (i == start_info.console.domU.evtchn ||
			i == start_info.store_evtchn)
			continue;
#endif

		if (__uk_test_and_clear_bit(i, bound_ports)) {
			uk_pr_warn("Port %d still bound!\n", i);
			unbind_evtchn(i);
		}
	}
	vcpu_info->evtchn_upcall_pending = 0;
	vcpu_info->evtchn_pending_sel = 0;
}

/*
 * Demux events to different handlers.
 */
int do_event(evtchn_port_t port, struct __regs *regs)
{
	ev_action_t *action;

	clear_evtchn(port);

	if (port >= NR_EVS) {
		uk_pr_err("%s: Port number too large: %d\n", __func__, port);
		return 1;
	}

	action = &ev_actions[port];
	action->count++;

	/* call the handler */
	action->handler(port, regs, action->data);

	return 1;

}

evtchn_port_t bind_evtchn(evtchn_port_t port, evtchn_handler_t handler,
			  void *data)
{
	if (ev_actions[port].handler != default_handler)
		uk_pr_warn("Handler for port %d already registered, replacing\n",
			   port);

	ev_actions[port].data = data;
	wmb();
	ev_actions[port].handler = handler;
	__uk_set_bit(port, bound_ports);

	return port;
}

void unbind_evtchn(evtchn_port_t port)
{
	struct evtchn_close close;
	int rc;

	if (ev_actions[port].handler == default_handler)
		uk_pr_warn("No handler for port %d when unbinding\n", port);
	mask_evtchn(port);
	clear_evtchn(port);

	ev_actions[port].handler = default_handler;
	wmb();
	ev_actions[port].data = NULL;
	__uk_clear_bit(port, bound_ports);

	close.port = port;
	rc = HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
	if (rc)
		uk_pr_warn("close_port %u failed rc=%d. ignored\n", port, rc);

}

evtchn_port_t bind_virq(uint32_t virq, evtchn_handler_t handler, void *data)
{
	evtchn_bind_virq_t op;
	int rc;

	/* Try to bind the virq to a port */
	op.virq = virq;
	op.vcpu = smp_processor_id();

	rc = HYPERVISOR_event_channel_op(EVTCHNOP_bind_virq, &op);
	if (rc != 0) {
		uk_pr_err("Failed to bind virtual IRQ %d with rc=%d\n",
			  virq, rc);
		return -1;
	}
	bind_evtchn(op.port, handler, data);
	return op.port;
}

evtchn_port_t bind_pirq(uint32_t pirq, int will_share,
			evtchn_handler_t handler, void *data)
{
	evtchn_bind_pirq_t op;
	int rc;

	/* Try to bind the pirq to a port */
	op.pirq = pirq;
	op.flags = will_share ? BIND_PIRQ__WILL_SHARE : 0;

	rc = HYPERVISOR_event_channel_op(EVTCHNOP_bind_pirq, &op);
	if (rc != 0) {
		uk_pr_err("Failed to bind physical IRQ %d with rc=%d\n",
			  pirq, rc);
		return -1;
	}
	bind_evtchn(op.port, handler, data);
	return op.port;
}

/*
 * Initially all events are without a handler and disabled
 */
void init_events(void)
{
	int i;

	/* initialize event handler */
	for (i = 0; i < NR_EVS; i++) {
		ev_actions[i].handler = default_handler;
		mask_evtchn(i);
	}

	arch_init_events();
}

void fini_events(void)
{
	/* Dealloc all events */
	arch_unbind_ports();
	unbind_all_ports();
	arch_fini_events();
}

#ifdef CONFIG_MIGRATION /* TODO */
void suspend_events(void)
{
	unbind_all_ports();
}
#endif

static void default_handler(evtchn_port_t port, struct __regs *regs __unused,
			    void *ignore __unused)
{
	uk_pr_info("[Port %d] - event received\n", port);
}

/* Create a port available to the pal for exchanging notifications.
 * Returns the result of the hypervisor call.
 */

/* Unfortunate confusion of terminology: the port is unbound as far
 * as Xen is concerned, but we automatically bind a handler to it
 * from inside mini-os.
 */

int evtchn_alloc_unbound(domid_t pal, evtchn_handler_t handler,
			 void *data, evtchn_port_t *port)
{
	int rc;
	evtchn_alloc_unbound_t op;

	op.dom = DOMID_SELF;
	op.remote_dom = pal;
	rc = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &op);
	if (rc) {
		uk_pr_err("alloc_unbound failed with rc=%d\n", rc);
		return rc;
	}

	*port = bind_evtchn(op.port, handler, data);

	return rc;
}

/* Connect to a port so as to allow the exchange of notifications with
 * the pal. Returns the result of the hypervisor call.
 */
int evtchn_bind_interdomain(domid_t pal, evtchn_port_t remote_port,
			    evtchn_handler_t handler, void *data,
			    evtchn_port_t *local_port)
{
	int rc;
	evtchn_port_t port;
	evtchn_bind_interdomain_t op;

	op.remote_dom = pal;
	op.remote_port = remote_port;
	rc = HYPERVISOR_event_channel_op(EVTCHNOP_bind_interdomain, &op);
	if (rc) {
		uk_pr_err("bind_interdomain failed with rc=%d\n", rc);
		return rc;
	}

	port = op.local_port;
	*local_port = bind_evtchn(port, handler, data);

	return rc;
}

int evtchn_get_peercontext(evtchn_port_t local_port, char *ctx, int size)
{
	int rc;
	uint32_t sid;
	struct xen_flask_op op;

	op.cmd = FLASK_GET_PEER_SID;
	op.interface_version = XEN_FLASK_INTERFACE_VERSION;
	op.u.peersid.evtchn = local_port;
	rc = HYPERVISOR_xsm_op(&op);
	if (rc)
		return rc;

	sid = op.u.peersid.sid;
	op.cmd = FLASK_SID_TO_CONTEXT;
	op.u.sid_context.sid = sid;
	op.u.sid_context.size = size;
	set_xen_guest_handle(op.u.sid_context.context, ctx);
	rc = HYPERVISOR_xsm_op(&op);

	return rc;
}

inline void mask_evtchn(evtchn_port_t port)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	uk_set_bit(port, &s->evtchn_mask[0]);
}

inline void unmask_evtchn(evtchn_port_t port)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	vcpu_info_t *vcpu_info = &s->vcpu_info[smp_processor_id()];

	uk_clear_bit(port, &s->evtchn_mask[0]);

	/*
	 * The following is basically the equivalent of 'hw_resend_irq'.
	 * Just like a real IO-APIC we 'lose the interrupt edge' if the
	 * channel is masked.
	 */
	if (uk_test_bit(port, &s->evtchn_pending[0]) &&
	    !uk_test_and_set_bit(port / (sizeof(unsigned long) * 8),
				    &vcpu_info->evtchn_pending_sel)) {
		vcpu_info->evtchn_upcall_pending = 1;
#ifdef XEN_HAVE_PV_UPCALL_MASK
		if (!vcpu_info->evtchn_upcall_mask)
#endif
			ukplat_lcpu_irqs_handle_pending();
	}
}

inline void clear_evtchn(evtchn_port_t port)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	uk_clear_bit(port, &s->evtchn_pending[0]);
}

struct uk_alloc;

int ukplat_irq_init(struct uk_alloc *a __unused)
{
	/* Nothing for now */
	return 0;
}

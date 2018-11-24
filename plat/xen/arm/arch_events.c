/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
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

#include <xen-arm/os.h>
#include <common/events.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/essentials.h>

static void virq_debug(evtchn_port_t port __unused,
		       struct __regs *regs __unused,
		       void *params __unused)
{
	uk_pr_debug("Received a virq_debug event\n");
}

static evtchn_port_t debug_port = -1;

void arch_init_events(void)
{
	debug_port = bind_virq(VIRQ_DEBUG, (evtchn_handler_t)virq_debug, 0);
	if ((int) debug_port == -1)
		UK_CRASH("Failed to initialize events\n");
	unmask_evtchn(debug_port);
}

void arch_unbind_ports(void)
{
	if ((int) debug_port != -1) {
		mask_evtchn(debug_port);
		unbind_evtchn(debug_port);
	}
}

void arch_fini_events(void)
{
}

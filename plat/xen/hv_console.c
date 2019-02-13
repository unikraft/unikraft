/* SPDX-License-Identifier: BSD-3-Clause and MIT */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

/*
 * Some of this code was ported from Mini-OS:
 *  console/xencons_ring.c and console/console.c
 */
/*
 ****************************************************************************
 * (C) 2006 - Grzegorz Milos - Cambridge University
 ****************************************************************************
 *
 *        File: console.h
 *      Author: Grzegorz Milos
 *     Changes:
 *
 *        Date: Mar 2006
 *
 * Environment: Xen Minimal OS
 * Description: Console interface.
 *
 * Handles console I/O. Defines printk.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <inttypes.h>
#include <string.h>
#include <uk/plat/console.h>
#include <uk/arch/lcpu.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/config.h>

#define __XEN_CONSOLE_IMPL__
#include "hv_console.h"

#include <common/events.h>
#include <common/hypervisor.h>
#include <xen/xen.h>

#if (defined __X86_32__) || (defined __X86_64__)
#include <xen-x86/setup.h>
#include <xen-x86/mm.h>
#if defined __X86_32__
#include <xen-x86/hypercall32.h>
#elif defined __X86_64__
#include <xen-x86/hypercall64.h>
#endif
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/mm.h>
#include <xen-arm/hypercall.h>
#endif
#include <xen/io/console.h>
#include <xen/io/protocols.h>
#include <xen/io/ring.h>
#ifndef CONFIG_PARAVIRT
#include <xen/hvm/params.h>
#endif

static struct xencons_interface *console_ring;
static uint32_t console_evtchn;
static int console_ready;

#ifdef CONFIG_PARAVIRT
void hv_console_prepare(void)
{
	console_ring = mfn_to_virt(HYPERVISOR_start_info->console.domU.mfn);
	console_evtchn = HYPERVISOR_start_info->console.domU.evtchn;
}
#else
void hv_console_prepare(void)
{
	/* NOT IMPLEMENTED YET */
}
#endif

/*
 * hv_console_output operates in two modes: buffered and initialized.
 * The buffered mode is automatically activated after
 * _libxenplat_prepare_console() was called and we know where the console ring
 * is. The output string is put to the console ring until the ring is full. Any
 * further characters are discarded. Since the event channel is not initialized
 * yet, the backend is not notified. This mode is introduced to support early
 * printing, even before events are not initialized.
 * _libxenplat_init_console() finalizes the initialization and enables
 * the event channel. From now on, the console backend is notified whenever
 * we put characters on the console ring. Whenever this ring is full and there
 * are still characters that should be printed, we are entering a busy loop and
 * wait for the backend to make us space again. Of course this is slow: do not
 * print so much! ;-)
 */
int hv_console_output(const char *str, unsigned int len)
{
	unsigned int sent = 0;
	XENCONS_RING_IDX cons, prod;

	if (unlikely(!console_ring))
		return sent;

retry:
	cons = console_ring->out_cons;
	prod = console_ring->out_prod;

	mb(); /* make sure we have cons & prod before touching the ring */
	UK_BUGON((prod - cons) > sizeof(console_ring->out));

	while ((sent < len) && ((prod - cons) < sizeof(console_ring->out))) {
		if (str[sent] == '\n') {
			/* prepend '\r' for converting '\n' to '\r''\n' */
			if ((prod + 1 - cons) >= sizeof(console_ring->out))
				break; /* not enough space for '\r' and '\n'! */

			console_ring->out[MASK_XENCONS_IDX(prod++,
							   console_ring->out)] =
				'\r';
		}

		console_ring->out[MASK_XENCONS_IDX(prod++, console_ring->out)] =
			str[sent];
		sent++;
	}
	wmb(); /* ensure characters are written before increasing out_prod */
	console_ring->out_prod = prod;

	/* Is the console fully initialized?
	 * Are we able to notify the backend?
	 */
	if (likely(console_ready && console_evtchn)) {
		notify_remote_via_evtchn(console_evtchn);

		/* There are still bytes left to send out? If yes, do not
		 * discard them, retry sending (enters busy waiting)
		 */
		if (sent < len)
			goto retry;
	}
	return sent;
}

void hv_console_flush(void)
{
	struct xencons_interface *intf;

	if (!console_ready)
		return;

	intf = console_ring;
	if (unlikely(!intf))
		return;

	while (intf->out_cons < intf->out_prod)
		barrier();
}

int hv_console_input(char *str, unsigned int maxlen)
{
	int read = 0;
	XENCONS_RING_IDX cons, prod;

	cons = console_ring->in_cons;
	prod = console_ring->in_prod;
	rmb(); /* make sure in_cons, in_prod are read before enqueuing */
	UK_BUGON((prod - cons) > sizeof(console_ring->in));

	while (cons != prod && maxlen > 0) {
		*(str + read) = *(console_ring->in+
				  MASK_XENCONS_IDX(cons, console_ring->in));
		read++;
		cons++;
		maxlen--;
	}

	wmb(); /* ensure finished operation before updating in_cons */
	console_ring->in_cons = cons;

	return read;
}

static void hv_console_event(evtchn_port_t port __unused,
			     struct __regs *regs __unused,
			     void *data __unused)
{
	/* NOT IMPLEMENTED YET */
}

void hv_console_init(void)
{
	int err;

	UK_ASSERT(console_ring != NULL);

	uk_pr_debug("hv_console @ %p (evtchn: %"PRIu32")\n",
		    console_ring, console_evtchn);

	err = bind_evtchn(console_evtchn, hv_console_event, NULL);
	if (err <= 0)
		UK_CRASH("Failed to bind event channel for hv_console: %i\n",
			 err);
	unmask_evtchn(console_evtchn);

	console_ready = 1; /* enable notification of backend */
	/* flush queued output */
	notify_remote_via_evtchn(console_evtchn);
}

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <inttypes.h>
#include <string.h>
#include <uk/arch/lcpu.h>
#include <uk/plat/bootstrap.h>
#include <errno.h>

#include <xen/xen.h>
#include <common/console.h>

#if defined __X86_32__
#include <xen-x86/hypercall32.h>
#elif defined __X86_64__
#include <xen-x86/hypercall64.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/hypercall.h>
#endif

void ukplat_terminate(enum ukplat_gstate request)
{
	int reason;

	switch (request) {
	case UKPLAT_HALT:
		reason = SHUTDOWN_poweroff;
		break;
	case UKPLAT_RESTART:
		reason = SHUTDOWN_reboot;
		break;
	default: /* UKPLAT_CRASH */
		reason = SHUTDOWN_crash;
		break;
	}

	flush_console();

	for (;;) {
		struct sched_shutdown sched_shutdown = { .reason = reason };

		HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	}
}

int ukplat_suspend(void)
{
	int ret;

	ret = EBUSY;
	/* ret = HYPERVISOR_suspend(virt_to_mfn(start_info_ptr)); */
	return -ret;
}

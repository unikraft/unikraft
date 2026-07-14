/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *           (c) 2026, Unikraft GmbH and The Unikraft Authors
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

#include <errno.h>
#include <stdint.h>

#include <uk/config.h>
#include <uk/pm.h>
#include <uk/boot/earlytab.h>
#include <uk/prio.h>

#include <xen/xen.h>
#if CONFIG_LIBXENCONS
#include <uk/xen/console.h>
#endif /* CONFIG_LIBXENCONS */

#if (defined __X86_32__) || (defined __X86_64__)
#include <xen-x86/hypercall.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/hypercall.h>
#endif

__isr static inline int xen_shutdown(int reason)
{
	struct sched_shutdown sched_shutdown = { .reason = reason };

#if CONFIG_LIBXENCONS
	xencons_flush();
#endif /* CONFIG_LIBXENCONS */

	HYPERVISOR_sched_op(SCHEDOP_shutdown, &sched_shutdown);
	/* Report error in case shutdown hypercall fails & returns */
	return -EIO;
}

__isr static int xen_halt(void)
{
	return xen_shutdown(SHUTDOWN_poweroff);
}

__isr static int xen_restart(void)
{
	return xen_shutdown(SHUTDOWN_reboot);
}

__isr static int xen_crash(void)
{
	return xen_shutdown(SHUTDOWN_crash);
}

static const struct uk_pm_ops xen_pm_ops = {
	.syshalt = xen_halt,
	.sysrestart = xen_restart,
	.syscrash = xen_crash
};

static int xen_register_pm_ops(struct ukplat_bootinfo *bi __unused)
{
	return uk_pm_ops_register(&xen_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(xen_register_pm_ops, UK_PRIO_EARLIEST);

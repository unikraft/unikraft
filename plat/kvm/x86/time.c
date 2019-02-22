/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2017-2018, NEC Europe Ltd., NEC Corporation
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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

#include <stdlib.h>
#include <uk/plat/time.h>
#include <uk/plat/irq.h>
#include <kvm/tscclock.h>
#include <uk/assert.h>

/* return ns since time_init() */
__nsec ukplat_monotonic_clock(void)
{
	return tscclock_monotonic();
}

/* return wall time in nsecs */
__nsec ukplat_wall_clock(void)
{
	return tscclock_monotonic() + tscclock_epochoffset();
}

/* NB: If this ever does more than an immediate return, it will need to be
 * compiled with NO_X86_EXTREGS_FLAGS to prevent potential clobbering of
 * registers that are not saved on interrupt handling.
 */
static int timer_handler(void *arg __unused)
{
	/* Yes, we handled the irq. */
	return 1;
}

/* must be called before interrupts are enabled */
void ukplat_time_init(void)
{
	int rc;

	rc = ukplat_irq_register(0, timer_handler, NULL);
	if (rc < 0)
		UK_CRASH("Failed to register timer interrupt handler\n");

	rc = tscclock_init();
	if (rc < 0)
		UK_CRASH("Failed to initialize TSCCLOCK\n");
}

void ukplat_time_fini(void)
{
}

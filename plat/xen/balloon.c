/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cason Schindler & Jack Raney <cason.j.schindler@gmail.com>
 *          Cezar Craciunoiu <cezar.craciunoiu@gmail.com>
 *
 * Copyright (c) 2019, The University of Texas at Austin. All rights reserved.
 *               2021, University Politehnica of Bucharest. All rights reserved.
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
#include <stddef.h>

#include <uk/plat/common/sections.h>

#include <common/gnttab.h>
#if (defined __X86_32__) || (defined __X86_64__)
#include <xen-x86/setup.h>
#include <xen-x86/mm_pv.h>
#include <xen-x86/mm.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/setup.h>
#include <xen-arm/mm.h>
#endif

#include <xen/memory.h>
#include <uk/plat/balloon.h>
#include <common/hypervisor.h>

/**
 * Set up and call Xen hypercall to ask for memory back from Xen.
 */
static int xenmem_reservation_increase(int count, xen_pfn_t *frames, int order)
{
	struct xen_memory_reservation res = {
#if __XEN_INTERFACE_VERSION__ >= 0x00030209
		.memflags = 0;
#else
		.address_bits = 0,
#endif
		.extent_order = order,
		.domid        = DOMID_SELF
	};

	set_xen_guest_handle(res.extent_start, frames);
	res.nr_extents = count;

	/* Needs physical frame number */
	return HYPERVISOR_memory_op(XENMEM_populate_physmap, &res);
}

/**
 * Set up and call Xen hypercall to give memory to Xen.
 */
static int xenmem_reservation_decrease(int count, xen_pfn_t *frames, int order)
{
	struct xen_memory_reservation res = {
#if __XEN_INTERFACE_VERSION__ >= 0x00030209
		.mem_flags = 0,
#else
		.address_bits = 0,
#endif
		.extent_order = order,
		.domid        = DOMID_SELF
	};

	set_xen_guest_handle(res.extent_start, frames);
	res.nr_extents = count;

	/* Needs guest frame number */
	return HYPERVISOR_memory_op(XENMEM_decrease_reservation, &res);
}

/**
 * When we inflate we will be decreasing the memory available to the VM
 * We will give the extent of extent order = order starting at va to the host.
 */
int ukplat_inflate(void *va, int order)
{
	xen_pfn_t pfn = virt_to_pfn(va);

	if (!va)
		return -EINVAL;

	return xenmem_reservation_decrease(1, &pfn, order);
}

/**
 * When we deflate we will be increasing the memory available to the VM
 * We will ask for 1 extent of extent order = order back from the host.
 * It will map the extent to the address va.
 */
int ukplat_deflate(void *va, int order)
{
	/* Make sure we are sending the correct frame number. Should be a GFN */
	xen_pfn_t pfn = virt_to_pfn(va);

	if (!va)
		return -EINVAL;

	return xenmem_reservation_increase(1, &pfn, order);
}

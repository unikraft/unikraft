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
 */

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/memory.h>

#include <common/gnttab.h>
#if (defined __X86_32__) || (defined __X86_64__)
#include <xen-x86/setup.h>
#include <xen-x86/mm_pv.h>
#elif (defined __ARM_32__) || (defined __ARM_64__)
#include <xen-arm/setup.h>
#include <xen-arm/mm.h>
#endif

#include <uk/assert.h>

/* TODO: Replace with proper initialization of bootinfo */

int ukplat_memregion_count(void)
{
	return (int) _libxenplat_mrd_num + 7;
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc **m)
{
	static struct ukplat_memregion_desc mrd[7];

	UK_ASSERT(m);

	switch (i) {
	case 0: /* text */
		mrd[i].pbase = __TEXT;
		mrd[i].vbase = __TEXT;
		mrd[i].len   = __ETEXT - __TEXT;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_EXECUTE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "text", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 1: /* eh_frame */
		mrd[i].pbase = __EH_FRAME_START;
		mrd[i].vbase = __EH_FRAME_START;
		mrd[i].len   = __EH_FRAME_END - __EH_FRAME_START;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "eh_frame", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 2: /* eh_frame_hdr */
		mrd[i].pbase = __EH_FRAME_HDR_START;
		mrd[i].vbase = __EH_FRAME_HDR_START;
		mrd[i].len   = __EH_FRAME_HDR_END - __EH_FRAME_HDR_START;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "eh_frame_hdr", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 3:	/* ro data */
		mrd[i].pbase = __RODATA;
		mrd[i].vbase = __RODATA;
		mrd[i].len   = __ERODATA - __RODATA;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "rodata", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 4: /* ctors */
		mrd[i].pbase = __CTORS;
		mrd[i].vbase = __CTORS;
		mrd[i].len   = __ECTORS - __CTORS;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "ctors", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 5: /* data */
		mrd[i].pbase = __DATA;
		mrd[i].vbase = __DATA;
		mrd[i].len   = __EDATA - __DATA;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "data", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	case 6: /* bss */
		mrd[i].pbase = __BSS_START;
		mrd[i].vbase = __BSS_START;
		mrd[i].len   = __END - __BSS_START;
		mrd[i].type  = UKPLAT_MEMRT_KERNEL;
		mrd[i].flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[i].name, "bss", sizeof(mrd[i].name) - 1);
#endif
		*m = &mrd[i];
		break;
	default:
		if (i < 0 || i >= ukplat_memregion_count())
			return -1;

		*m = &_libxenplat_mrd[i - 7];
		break;
	}

	return 0;
}

void mm_init(void)
{
	arch_mm_init(ukplat_memallocator_get());
}

int _ukplat_mem_mappings_init(void)
{
	mm_init();
#ifdef CONFIG_XEN_GNTTAB
	gnttab_init();
#endif
	return 0;
}

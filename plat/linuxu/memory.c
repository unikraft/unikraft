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

#include <errno.h>
#include <string.h>
#include <uk/arch/types.h>
#include <linuxu/setup.h>
#include <uk/errptr.h>
#include <uk/assert.h>
#include <uk/plat/memory.h>
#include <uk/libparam.h>

int ukplat_memregion_count(void)
{
	static int init;
	int rc;

	if (init)
		return init;

	rc = __linuxu_plat_heap_init();
	init += (rc == 0) ? 1 : 0;

	rc = __linuxu_plat_initrd_init();
	init += (rc == 0) ? 1 : 0;

	return init;
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc **m)
{
	static struct ukplat_memregion_desc mrd[2];
	int ret;

	UK_ASSERT(m);

	if (i >= ukplat_memregion_count())
		return -1;

	if (i == 0 && _liblinuxuplat_opts.heap.base) {
		mrd[0].pbase = (__paddr_t)_liblinuxuplat_opts.heap.base;
		mrd[0].vbase = mrd[0].pbase;
		mrd[0].len   = _liblinuxuplat_opts.heap.len;
		mrd[0].type  = UKPLAT_MEMRT_FREE;
		mrd[0].flags = 0;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[0].name, "heap", sizeof(mrd[0].name) - 1);
#endif
		*m = &mrd[0];
		ret = 0;
	} else if ((i == 0 && !_liblinuxuplat_opts.heap.base
				&& _liblinuxuplat_opts.initrd.base)
				|| (i == 1 && _liblinuxuplat_opts.heap.base
					&& _liblinuxuplat_opts.initrd.base)) {
		mrd[1].pbase = (__paddr_t)_liblinuxuplat_opts.initrd.base;
		mrd[1].vbase = mrd[1].pbase;
		mrd[1].len   = _liblinuxuplat_opts.initrd.len;
		mrd[1].type  = UKPLAT_MEMRT_INITRD;
		mrd[1].flags = UKPLAT_MEMRF_WRITE | UKPLAT_MEMRF_MAP;
#if CONFIG_UKPLAT_MEMRNAME
		strncpy(mrd[1].name, "initrd", sizeof(mrd[1].name) - 1);
#endif
		*m = &mrd[1];
		ret = 0;
	} else {
		ret = -1;
	}

	return ret;
}

int _ukplat_mem_mappings_init(void)
{
	return 0;
}

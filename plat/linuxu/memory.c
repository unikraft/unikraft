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

#include <errno.h>
#include <uk/arch/types.h>
#include <linuxu/setup.h>
#include <uk/errptr.h>
#include <uk/assert.h>
#include <linuxu/syscall.h>
#include <uk/plat/memory.h>
#include <uk/libparam.h>

#define MB2B		(1024 * 1024)

static __u32 heap_size = CONFIG_LINUXU_DEFAULT_HEAPMB;
UK_LIB_PARAM(heap_size, __u32);

static int __linuxu_plat_heap_init(void)
{
	void *pret;
	int rc = 0;

	_liblinuxuplat_opts.heap.len = heap_size * MB2B;
	uk_pr_info("Allocate memory for heap (%u MiB)\n", heap_size);

	/**
	 * Allocate heap memory
	 */
	if (_liblinuxuplat_opts.heap.len > 0) {
		pret = sys_mapmem(NULL, _liblinuxuplat_opts.heap.len);
		if (PTRISERR(pret)) {
			rc = PTR2ERR(pret);
			uk_pr_err("Failed to allocate memory for heap: %d\n",
				   rc);
		} else
			_liblinuxuplat_opts.heap.base = pret;
	}

	return rc;

}

int ukplat_memregion_count(void)
{
	static int have_heap = 0;
	int rc = 0;

	if (!have_heap) {
		/*
		 * NOTE: The heap size can be changed by a library parameter.
		 * We assume that those ones are processed by the boot library
		 * shortly before memory regions are scanned. This is why
		 * we initialize the heap here.
		 */
		rc = __linuxu_plat_heap_init();
		have_heap = (rc == 0) ? 1 : 0;
	}

	return (have_heap) ? 1 : 0;
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc *m)
{
	int ret;

	UK_ASSERT(m);

	if (i == 0 && _liblinuxuplat_opts.heap.base) {
		m->base  = _liblinuxuplat_opts.heap.base;
		m->len   = _liblinuxuplat_opts.heap.len;
		m->flags = UKPLAT_MEMRF_ALLOCATABLE;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = "heap";
#endif
		ret = 0;
	} else {
		/* invalid memory region index or no heap allocated */
		m->base  = __NULL;
		m->len   = 0;
		m->flags = 0x0;
#if CONFIG_UKPLAT_MEMRNAME
		m->name  = __NULL;
#endif
		ret = -1;
	}

	return ret;
}

int _ukplat_mem_mappings_init(void)
{
	return 0;
}

void ukplat_stack_set_current_thread(void *thread_addr __unused)
{
	/* For now, signals use the current process stack */
}

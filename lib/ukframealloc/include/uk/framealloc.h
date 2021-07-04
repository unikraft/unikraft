/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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

#ifndef __UK_FRAMEALLOC_H__
#define __UK_FRAMEALLOC_H__

#include <uk/asm/page.h>
#include <uk/plat/mm.h>
#include <uk/config.h>
#include <uk/print.h>

/* TODO: this will be fixed when moving mm_*.h to plat folders */
#ifdef CONFIG_PARAVIRT
#include <uk/asm/mm_pv.h>
#else
#include <uk/asm/mm_native.h>
#endif	/* CONFIG_PARAVIRT */

#ifndef CONFIG_PT_API
#error Using this header requires enabling the virtual memory management API
#endif /* CONFIG_PT_API */

struct uk_framealloc;

typedef __paddr_t (*uk_framealloc_palloc_func_t)
		(struct uk_framealloc *a, unsigned long num_pages);
typedef void  (*uk_framealloc_pfree_func_t)
		(struct uk_framealloc *a, __paddr_t obj, unsigned long num_pages);
typedef int   (*uk_framealloc_addmem_func_t)
		(struct uk_framealloc *a, __paddr_t base, size_t size, void *metadata_base);

struct uk_framealloc {
	/* page allocation interface */
	uk_framealloc_palloc_func_t palloc;
	uk_framealloc_pfree_func_t pfree;

	/* memory adding interface */
	uk_framealloc_addmem_func_t addmem;

	size_t available_memory;

	/* internal */
	char priv[];
};

/**
 * Get a free frame in the physical memory where a new mapping can be created.
 *
 * @param flags: specify any criteria that the frame has to meet (e.g. a 2MB
 * frame for a large page). These are constructed by or'ing PAGE_FLAG_* flags.
 *
 * @return: physical address of an unused frame or PAGE_INVALID on failure.
 */
__paddr_t uk_get_next_free_frame(struct uk_framealloc *fa, unsigned long flags);

#endif /* __UK_FRAMEALLOC_H__ */

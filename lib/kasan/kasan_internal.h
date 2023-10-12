/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Badoiu Vlad-Andrei <vlad_andrei.badoiu@upb.ro>
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

#ifndef _SYS_KASAN_INTERNAL_H_
#define _SYS_KASAN_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <uk/essentials.h>
#include <uk/kasan.h>

/* Part of internal compiler interface */
#define KASAN_SHADOW_SCALE_SHIFT 3
#define KASAN_ALLOCA_REDZONE_SIZE 32

#define KASAN_SHADOW_SCALE_SIZE (1 << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK (KASAN_SHADOW_SCALE_SIZE - 1)

#define is_aligned(addr, size)				\
	({						\
	 intptr_t _addr = (intptr_t)(addr);		\
	 intptr_t _size = (intptr_t)(size);		\
	 !(_addr & (_size - 1));			\
	 })

#define KASAN_SHADOW_SCALE_SHIFT 3

extern void *shadow_mem_base;

#define SUPERPAGESIZE (1 << 22) /* 4 MB */

/* We reserve a portion from the start of the mem for the shadow memory */
#define KASAN_MD_SHADOW_START ALIGN_UP((uintptr_t) shadow_mem_base, __PAGE_SIZE)

#define KASAN_MD_SHADOW_END (KASAN_MD_SHADOW_START + KASAN_MD_SHADOW_SIZE)

/* Sanitized memory (accesses within this range are checked) */
#define KASAN_MD_SANITIZED_START ALIGN_UP(KASAN_MD_SHADOW_START		\
		+ KASAN_MD_SHADOW_SIZE, __PAGE_SIZE)

#define KASAN_MD_SANITIZED_SIZE						\
	(KASAN_MD_SHADOW_SIZE << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_MD_SANITIZED_END						\
	(KASAN_MD_SANITIZED_START + KASAN_MD_SANITIZED_SIZE)

#define KASAN_MD_OFFSET							\
	(KASAN_MD_SHADOW_START -					\
	 (KASAN_MD_SANITIZED_START >> KASAN_SHADOW_SCALE_SHIFT))

struct __asan_global_source_location {
	const char *filename;
	int line_no;
	int column_no;
};

struct __asan_global {
	uintptr_t beg;			/* The address of the global */
	uintptr_t size;			/* The original size of the global */
	uintptr_t size_with_redzone;	/* The size with the redzone */
	const char *name;		/* Name as a C string */
	const char *module_name;	/* Module name as a C string */
	/* Does the global have dynamic initializer */
	uintptr_t has_dynamic_init;	/* Location of a global */
	struct __asan_global_source_location *location;
	uintptr_t odr_indicator; /* The address of the ODR indicator symbol */
};

static inline int8_t *kasan_md_addr_to_shad(uintptr_t addr)
{
	return (int8_t *)(KASAN_MD_OFFSET + (addr >> KASAN_SHADOW_SCALE_SHIFT));
}

bool kasan_md_addr_supported(uintptr_t addr)
{
	return addr >= KASAN_MD_SANITIZED_START
		&& addr < KASAN_MD_SANITIZED_END;
}

#endif /* !_SYS_KASAN_INTERNAL_H_ */

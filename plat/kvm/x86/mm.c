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

#include <uk/plat/mm.h>
#include <uk/plat/common/mem_layout.h>

/* TODO: add all relevant features here */
/* TODO: make static inline */
unsigned long ukplat_mm_supported_features(void)
{
	return UKPLAT_SUPPORT_LARGE_PAGES;
}

int ukplat_map_specific_areas(struct uk_pagetable *pt)
{
	unsigned long mbinfo_pages, vgabuffer_pages;

	mbinfo_pages = DIV_ROUND_UP(MBINFO_AREA_SIZE, PAGE_SIZE);
	vgabuffer_pages = DIV_ROUND_UP(VGABUFFER_AREA_SIZE, PAGE_SIZE);
	if (_initmem_page_map_many(pt, MBINFO_AREA_START, MBINFO_AREA_START,
			mbinfo_pages, PAGE_PROT_READ, 0))
		return -1;

	if (_initmem_page_map_many(pt, VGABUFFER_AREA_START,
			VGABUFFER_AREA_START, vgabuffer_pages,
			PAGE_PROT_READ | PAGE_PROT_WRITE, 0))
		return -1;

	return 0;
}

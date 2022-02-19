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

#include <inttypes.h>
#include <errno.h>
#include <stddef.h>

#include <balloon/balloon.h>
#include <uk/plat/balloon.h>
#include <uk/asm/limits.h>

/**
 * Fills addresses for page range starting at first_page
 *
 * @param pages_array the array to fill
 * @param first_page the page to start the fill from
 * @param num_pages number of pages in the array
 */
static inline void fill_page_array(uintptr_t *pages_array,
			void *first_page, int num_pages)
{
	uint64_t current_pg = (uint64_t) first_page;
	int i;

	for (i = 0; i < num_pages; i++) {
		pages_array[i] = current_pg;
		current_pg += __PAGE_SIZE;
	}
}

int ukplat_inflate(void *page, int order)
{
	int num_pages = 1 << order;
	uintptr_t pages_to_host[num_pages];

	if (!page)
		return -EINVAL;

	fill_page_array(pages_to_host, page, num_pages);
	return inflate_balloon(pages_to_host, num_pages);
}

int ukplat_deflate(void *page, int order)
{
	int num_pages = 1 << order;
	uintptr_t pages_to_guest[num_pages];

	if (!page)
		return -EINVAL;

	fill_page_array(pages_to_guest, page, num_pages);
	return deflate_balloon(pages_to_guest, num_pages);
}

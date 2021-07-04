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


#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <uk/bitmap.h>
#include <uk/print.h>
#include <uk/plat/mm.h>
#include <uk/syscall.h>
#include <uk/plat/common/mem_layout.h>

#include <sys/mman.h>

/*
 * XXX
 * This does for now a linear search, starting from |start|, looking for a
 * memory area with |length| bytes (aligned to page size)
 */
static unsigned long get_free_virtual_area(unsigned long start, size_t length)
{
	unsigned long page;

	if (!PAGE_ALIGNED(length))
		return -1;

	while (start <= MMAP_AREA_END - length) {
		for (page = start; page < start + length; page += PAGE_SIZE) {
			if (PAGE_PRESENT(uk_virt_to_pte(page)))
				break;
		}

		if (page == start + length)
			return start;

		start = page + PAGE_SIZE;
	}

	return -1;
}

static int libc_to_internal_prot(int prot)
{
	int page_prot = PAGE_PROT_NONE;

	if (prot & PROT_READ)
		page_prot |= PAGE_PROT_READ;
	if (prot & PROT_WRITE)
		page_prot |= PAGE_PROT_WRITE;
	if (prot & PROT_EXEC)
		page_prot |= PAGE_PROT_EXEC;

	return page_prot;
}

UK_SYSCALL_DEFINE(void*, mmap, void*, addr, size_t, len, int, prot,
		int, flags, int, fildes, off_t, off)
{
	unsigned long page_addr = (unsigned long) addr;
	unsigned long area_to_map, page_prot, page;
	size_t i;

	if (flags & MAP_ANONYMOUS) {
		if (fd != -1 || offset) {
			errno = EINVAL;
			return MAP_FAILED;
		}
	} else {
		/* TODO: We don't currently support mapping files */
		errno = ENOTSUP;
		return MAP_FAILED;
	}


	/* At least one of MAP_SHARED or MAP_PRIVATE has to be specified */
	if (!(flags & MAP_SHARED) && !(flags & MAP_PRIVATE)) {
		errno = EINVAL;
		return MAP_FAILED;
	}

	if (!length) {
		errno = EINVAL;
		return MAP_FAILED;
	}

	length = PAGE_ALIGN_UP(length);
	if (!length) {
		errno = ENOMEM;
		return MAP_FAILED;
	}

	if (flags & MAP_FIXED) {
		/* Discard any overlapping mappings */
		if (munmap(addr, length)) {
			errno = EINVAL;
			return MAP_FAILED;
		}
		page_addr = PAGE_ALIGN_UP(page_addr);
	} else {
		if ((void *) page_addr == NULL || page_addr < MMAP_AREA_START)
			page_addr = MMAP_AREA_START;
		else
			page_addr = PAGE_ALIGN_UP(page_addr);
	}

	area_to_map = get_free_virtual_area(page_addr, length);
	if (area_to_map == (unsigned long) -1) {
		errno = ENOMEM;
		return MAP_FAILED;
	}

	for (i = 0; i < length; i += PAGE_SIZE) {
		page = area_to_map + i;
		if (ukplat_page_map(ukplat_get_active_pt(), page,
					PAGE_PADDR_ANY,
					PAGE_PROT_READ | PAGE_PROT_WRITE, 0)) {
			munmap((void *) area_to_map, length);
			errno = ENOMEM;
			return MAP_FAILED;
		}
	}

	if (flags & MAP_ANONYMOUS) {
		/* MAP_ANONYMOUS pages are zeroed out */
		/*
		 * XXX: there is a bug when building with performance
		 * optimizations flag that make this memset loop infintely.
		 * Using for loop for now.
		 */
		/* memset((void *) area_to_map, 0, length); */
		for (i = 0; i < length / sizeof(unsigned long); i++)
			*((unsigned long *) area_to_map + i) = 0;
	} else {
		/* TODO: file mapping */
	}

	page_prot = libc_to_internal_prot(prot);
	for (page = area_to_map; page < area_to_map + length; page += PAGE_SIZE)
		ukplat_page_set_prot(ukplat_get_active_pt(), page, page_prot);

	return (void *) area_to_map;
}

UK_SYSCALL_DEFINE(int, munmap, void*, addr, size_t, len)
{
	unsigned long start = (unsigned long) addr;
	unsigned long page;

	if (!PAGE_ALIGNED(start)) {
		errno = EINVAL;
		return -1;
	}

	if (!length)
		return 0;

	length = PAGE_ALIGN_UP(length);
	for (page = start; page < start + length; page += PAGE_SIZE)
		ukplat_page_unmap(ukplat_get_active_pt(), page);

	return 0;
}

UK_SYSCALL_DEFINE(int, mprotect, void*, addr, size_t, len, int, prot)
{
	unsigned long start = (unsigned long) addr;
	unsigned long page_prot, page;

	if (PAGE_ALIGNED(start)) {
		errno = EINVAL;
		return -1;
	}

	if (!length)
		return 0;

	if ((prot & PROT_NONE) && (prot != PROT_NONE)) {
		errno = EINVAL;
		return -1;
	}

	page_prot = PAGE_PROT_NONE;
	if (prot & PROT_READ)
		page_prot |= PAGE_PROT_READ;
	if (prot & PROT_WRITE)
		page_prot |= PAGE_PROT_WRITE;
	if (prot & PROT_EXEC)
		page_prot |= PAGE_PROT_EXEC;

	length = PAGE_ALIGN_UP(length);
	for (page = start; page < start + length; page += PAGE_SIZE)
		ukplat_page_set_prot(ukplat_get_active_pt(), page, page_prot);

	return 0;
}

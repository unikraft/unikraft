/* SPDX-License-Identifier: BSD-3-Clause */
/*
 *
 * Authors: Charalampos Mainas <charalampos.mainas@neclab.eu>
 *
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <sys/mman.h>
#include <uk/alloc.h>
#include <string.h>

struct mmap_addr {
	void *begin;
	void *end;
	struct mmap_addr *next;
};

static struct mmap_addr *mmap_addr;

/**
 * This is not a correct implementation of mmap. It is just a trick that works
 * for Go but it needs to be revisited. Instead of mapping, it allocates len
 * bytes of memory and stores the beginninig and the end of that memory chunk
 * in struct mmap_addr. At first it checks if addr belongs to one of the memory
 * chunks that have been allocated in a previous call of mmap. If that is the
 * case addr is the return value. Otherwise a new memory block is allocated and
 * the return value is a pointer to the beginninig of that block.
 *
 * Go uses mmap always with:
 * @prot   =	either PROT_NONE or PROT_READ|PROT_WRITE,
 * @flags  =	as MAP_ANON|MAP_PRIVATE, or MAP_FIXED|MAP_ANON|MAP_PRIVATE
 *		or MAP_NORESERVE|MAP_ANON|MAP_PRIVATE
 * @fildes =	-1
 * @off    =	0
 *
 */

void *mmap(void *addr, size_t len, int prot,
		int flags, int fildes, off_t off)
{
	struct mmap_addr *tmp = mmap_addr, *last = NULL, *new = NULL;

	if (!len) {
		errno = EINVAL;
		return (void *) -1;
	}

	/* Check if parameters match the ones that go use
	 * Otherwise return 0 (unimplemented mmap)
	 */
	if (fildes != -1 || off)
		return 0;
	if (!(prot & (PROT_READ|PROT_WRITE)) && (prot != 0))
		return 0;
	if (!(flags & (MAP_ANON|MAP_PRIVATE)) &&
			!(flags & (MAP_FIXED|MAP_ANON|MAP_PRIVATE)) &&
			!(flags & (MAP_NORESERVE|MAP_ANON|MAP_PRIVATE)))
		return 0;

	while (tmp) {
		if (addr) {
			if (addr >= tmp->begin && addr < tmp->end)
				return addr;
		}
		last = tmp;
		tmp = tmp->next;
	}
	void *mem = uk_malloc(uk_alloc_get_default(), len);

	if (!mem) {
		errno = ENOMEM;
		return (void *) -1;
	}
	new = uk_malloc(uk_alloc_get_default(), sizeof(struct mmap_addr));
	new->begin = mem;
	new->end = mem + len;
	new->next = NULL;
	if (!mmap_addr)
		mmap_addr = new;
	else
		last->next = new;
	return mem;
}

/*
 * munmap frees len bytes os memory starting from addr.
 * addr needs to be a pointer to a memory block that has been allocated from
 * mmap. If len has the same value with the size of the memory block that has
 * been allocated from mmap the struct mmap_addr counterpart is destroyed.
 * Otherwise the initial memory block is replaced by a smaller one.
 */

int munmap(void *addr, size_t len)
{
	struct mmap_addr *tmp = mmap_addr, *prev = NULL;
	size_t remain_mem;

	if (!len) {
		errno = EINVAL;
		return -1;
	}
	if (!addr)
		return 0;
	while (tmp) {
		if (addr != tmp->begin) {
			if (tmp->end > addr + len) {
				errno = EINVAL;
				return -1;
			}
			remain_mem = tmp->end - addr - len;
			if (remain_mem) {

				void *mem = uk_malloc(uk_alloc_get_default(),
						remain_mem);
				if (!mem) {
					errno = ENOMEM;
					return -1;
				}
				memcpy(mem, addr+len, remain_mem);
				tmp->begin = mem;
			} else {

				if (!prev)
					mmap_addr = tmp->next;
				else
					prev->next = tmp->next;
				uk_free(uk_alloc_get_default(), tmp);
			}
			uk_free(uk_alloc_get_default(), addr);
			return 0;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	/* unimplemented munmap */
	return 0;
}

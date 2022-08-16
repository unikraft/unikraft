/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Xiangyi Meng <xymeng16@gmail.com>
 *
 * Copyright (c) 2022. All rights reserved.
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

#ifndef _UK_SGX_INTERNAL_H_
#define _UK_SGX_INTERNAL_H_

#include <uk/arch/spinlock.h>
#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/bitops.h>
#include <uk/mutex.h>
#include <uk/list.h>
#include <uk/refcount.h>
#include <devfs/device.h>
#include <errno.h>
#include <signal.h>

#include <uk/sgx_cpu.h>

#define SGX_VA_SLOT_COUNT 512
#define SGX_PAGE_SIZE 4096

extern __spinlock sgx_free_list_lock;

struct sgx_epc_bank {
	unsigned long pa;
	unsigned long va;
	unsigned long size;
	unsigned long npages;
};

struct sgx_epc_page {
	__paddr_t	pa;
	struct uk_list_head list;
	struct sgx_encl_page *encl_page;
};

struct sgx_va_page {
	struct sgx_epc_page *epc_page;
	unsigned long slots[UK_BITS_TO_LONGS(SGX_VA_SLOT_COUNT)];
	struct uk_list_head list;
};

static inline unsigned int sgx_alloc_va_slot(struct sgx_va_page *page)
{
	int slot = uk_find_first_zero_bit(page->slots, SGX_VA_SLOT_COUNT);

	if (slot < SGX_VA_SLOT_COUNT)
		set_bit(slot, page->slots);

	return slot << 3;
}

static inline void sgx_free_va_slot(struct sgx_va_page *page,
				    unsigned int offset)
{
	uk_clear_bit(offset >> 3, page->slots);
}


struct sgx_encl_page {
	unsigned long addr;
	unsigned int flags;
	struct sgx_epc_page *epc_page;
	struct sgx_va_page *va_page;
	unsigned int va_offset;
};

struct sgx_tgid_ctx {
	// struct pid *tgid;
	pid_t *tgid;
	__atomic refcount;
	struct uk_list_head encl_list;
	struct uk_list_head list;
};

enum sgx_encl_flags {
	SGX_ENCL_INITIALIZED	= BIT(0),
	SGX_ENCL_DEBUG		= BIT(1),
	SGX_ENCL_SECS_EVICTED	= BIT(2),
	SGX_ENCL_SUSPEND	= BIT(3),
	SGX_ENCL_DEAD		= BIT(4),
};

struct sgx_encl {
	unsigned int flags;
	uint64_t attributes;
	uint64_t xfrm;
	unsigned int secs_child_cnt;
	struct uk_mutex lock;
	// struct mm_struct *mm;
	// struct file *backing;
	// struct file *pcmd;
	struct uk_list_head load_list;
	__atomic refcount;
	unsigned long base;
	unsigned long size;
	unsigned long ssaframesize;
	struct uk_list_head va_pages;
	// struct radix_tree_root page_tree;
	struct uk_list_head add_page_reqs;
	// struct work_struct add_page_work;
	struct sgx_encl_page secs;
	struct sgx_tgid_ctx *tgid_ctx;
	struct uk_list_head encl_list;
	// struct mmu_notifier mmu_notifier;
	unsigned int shadow_epoch;
};


/*
 * Decrement the reference counter, if no one ref it, release it by calling release().
 * @param refs
 *	Reference to the atomic counter.
 * @return
 *	0: there are more active reference
 *	1: this was let reference to the counter.
 */
static inline void uk_refcount_put(__atomic *ref, void (*release)(__atomic *ref))
{
	if (release == NULL) {
		uk_pr_warn("release == NULL!");
		return 0;
	}

	if (uk_refcount_release(ref)) {
		release(ref);
		return 1;
	}
	
	return 0;
}

/* sgx_page_cache.c */
int sgx_add_epc_bank(__paddr_t start, unsigned long size);
int sgx_page_cache_init(void);
struct sgx_epc_page *sgx_alloc_page(unsigned int flags);
void sgx_free_page(struct sgx_epc_page *entry, struct sgx_encl *encl);
void *sgx_get_page(struct sgx_epc_page *entry);
void sgx_put_page(void *epc_page_vaddr);

/* sgx_ioctl.c */
int sgx_ioctl(struct device *filep, unsigned long cmd, void *arg);
int sgx_open(struct device *dev, int flags);
int sgx_close(struct device *dev);

/* sgx_encl.c */
int sgx_encl_create(struct sgx_secs *secs);
void sgx_encl_release(__atomic *ref);
int sgx_encl_add_page(struct sgx_encl *encl, unsigned long addr, void *data,
		      struct sgx_secinfo *secinfo, unsigned int mrmask);

/* sgx_util.c */
int cpl_switch_init();
int cpl_switch(__u8 rpl);

#endif
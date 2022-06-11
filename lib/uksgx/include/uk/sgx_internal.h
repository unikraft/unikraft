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

extern __spinlock sgx_free_list_lock;

struct sgx_epc_bank {
	unsigned long pa;
	unsigned long va;
	unsigned long size;
};

struct sgx_epc_page {
	__paddr_t	pa;
	struct uk_list_head list;
	struct sgx_encl_page *encl_page;
};

#define DECLARE_BITMAP(name,bits) \
	unsigned long name[UK_BITS_TO_LONGS(bits)]
struct sgx_va_page {
	struct sgx_epc_page *epc_page;
	DECLARE_BITMAP(slots, SGX_VA_SLOT_COUNT);
	struct uk_list_head list;
};

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

int sgx_add_epc_bank(__paddr_t start, unsigned long size, int bank);
int sgx_page_cache_init(void);
int sgx_ioctl(struct device *filep, unsigned long cmd, void *arg);
int sgx_open(struct device *dev, int flags);
int sgx_close(struct device *dev);
int cpl_switch_init();
int cpl_switch(__u8 rpl);
#endif
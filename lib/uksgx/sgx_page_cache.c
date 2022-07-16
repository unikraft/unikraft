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

#include <uk/sgx_internal.h>
#include <uk/list.h>
#include <uk/assert.h>
#include <uk/arch/paging.h>
#include <stdlib.h>

#define SGX_NR_LOW_EPC_PAGES_DEFAULT 32
#define SGX_NR_SWAP_CLUSTER_MAX 16

__spinlock sgx_free_list_lock;
UK_LIST_HEAD(sgx_free_list);

static unsigned int sgx_nr_total_epc_pages;
static unsigned int sgx_nr_free_pages;
static unsigned int sgx_nr_low_pages = SGX_NR_LOW_EPC_PAGES_DEFAULT;
static unsigned int sgx_nr_high_pages;

unsigned int sgx_va_pages_cnt;

int sgx_add_epc_bank(__paddr_t start, unsigned long size)
{
	unsigned long i;
	struct sgx_epc_page *new_epc_page, *entry;
	struct uk_list_head *parser, *temp;

	for (i = 0; i < size; i += PAGE_SIZE) {
		new_epc_page = calloc(sizeof(*new_epc_page), 1);
		if (!new_epc_page)
			goto err_freelist;
		new_epc_page->pa = start + i;

		ukarch_spin_lock(&sgx_free_list_lock);
		uk_list_add_tail(&new_epc_page->list, &sgx_free_list);
		sgx_nr_total_epc_pages++;
		sgx_nr_free_pages++;
		ukarch_spin_unlock(&sgx_free_list_lock);
	}

	return 0;
err_freelist:
	uk_list_for_each_safe(parser, temp, &sgx_free_list) {
		ukarch_spin_lock(&sgx_free_list_lock);
		entry = uk_list_entry(parser, struct sgx_epc_page, list);
		uk_list_del(&entry->list);
		ukarch_spin_unlock(&sgx_free_list_lock);
		free(entry);
	}
	return -ENOMEM;
}

int sgx_page_cache_init(void)
{
/*	struct task_struct *tmp;

	sgx_nr_high_pages = 2 * sgx_nr_low_pages;

	tmp = kthread_run(ksgxswapd, NULL, "ksgxswapd");
	if (!IS_ERR(tmp))
		ksgxswapd_tsk = tmp;
	return PTR_ERR_OR_ZERO(tmp);*/
	WARN_STUBBED();
	return 0;
}


/**
 * sgx_alloc_page - allocate an EPC page
 * @flags:	allocation flags
 *
 * Try to grab a page from the free EPC page list. If there is a free page
 * available, it is returned to the caller. If called with SGX_ALLOC_ATOMIC,
 * the function will return immediately if the list is empty. Otherwise, it
 * will swap pages up until there is a free page available. Before returning
 * the low watermark is checked and ksgxswapd is waken up if we are below it.
 *
 * Return: an EPC page or a system error code
 */
struct sgx_epc_page *sgx_alloc_page(unsigned int flags)
{
	struct sgx_epc_page *entry = NULL;

	ukarch_spin_lock(&sgx_free_list_lock);

	if(!uk_list_empty(&sgx_free_list)) {
		entry = uk_list_first_entry(&sgx_free_list, struct sgx_epc_page, list);
		uk_list_del(&entry->list);
		sgx_nr_free_pages--;
	}

	ukarch_spin_unlock(&sgx_free_list_lock);

	return entry;
}
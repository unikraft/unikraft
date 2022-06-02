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
#include <devfs/device.h>
#include <errno.h>

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

int sgx_add_epc_bank(__paddr_t start, unsigned long size, int bank);
int sgx_page_cache_init(void);
long sgx_ioctl(struct device *filep, unsigned int cmd, unsigned long arg);

#endif
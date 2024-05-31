/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UK_RANDOM_H__
#define __UK_RANDOM_H__

#include <sys/types.h>
#include <uk/arch/types.h>
#include <uk/plat/lcpu.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/plat/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_swrand;
extern struct uk_swrand uk_swrand_def;

__u32 uk_swrand_randr_r(struct uk_swrand *r);

/* Uses the pre-initialized default generator  */
/* TODO: Add assertion when we can test if we are in interrupt context */
/* TODO: Revisit with multi-CPU support */
static inline __u32 uk_swrand_randr(void)
{
	unsigned long iflags;
	__u32 ret;

	iflags = ukplat_lcpu_save_irqf();
	ret = uk_swrand_randr_r(&uk_swrand_def);
	ukplat_lcpu_restore_irqf(iflags);

	return ret;
}

__ssz uk_swrand_fill_buffer(void *buf, __sz buflen);

#ifdef __cplusplus
}
#endif

#endif /* __UK_RANDOM_H__ */

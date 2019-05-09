/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation,
 *                     All rights reserved.
 * Copyright (c) 2018, Arm Ltd., All rights reserved.
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

#ifndef __KVM_CONFIG_H__
#define __KVM_CONFIG_H__

#include <inttypes.h>
#include <sys/types.h>

struct kvmplat_config_memregion {
	uintptr_t start;
	uintptr_t end;
	size_t len;
};

struct kvmplat_config {
	struct kvmplat_config_memregion heap;
	struct kvmplat_config_memregion bstack;
	struct kvmplat_config_memregion initrd;
	/* `heap2` potentially exists only if `heap` exists */
	struct kvmplat_config_memregion heap2;

#ifdef CONFIG_ARCH_ARM_64
	struct kvmplat_config_memregion pagetable;
	void *dtb;
#endif
};

/* Initialized and defined in setup.c */
extern struct kvmplat_config _libkvmplat_cfg;

#endif /* __KVM_CONFIG_H__ */

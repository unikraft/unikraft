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

#ifndef __UKPLAT_BALLOON_H__
#define __UKPLAT_BALLOON_H__

/**
 * Inflates the memory balloon by 1 extent. After this point, the extent
 * starting at the specified virtual address will be unavailable to the guest.
 * The host will then be able to use the extent for its own purposes.
 *
 * @param va The starting virtual address to release
 * @param order The size of the extent to be released
 * @return >= 0 on success, < 0 otherwise
 */
int ukplat_inflate(void *va, int order);

/**
 * Deflates the memory balloon by 1 extent, and returns a frame to the
 * hypervisor starting at the specified virtual address.
 *
 * @param va The starting virtual address to recover
 * @param order The size of the extent to be recovered
 * @return num pages reclaimed on success, < 0 otherwise
 */
int ukplat_deflate(void *va, int order);

#if CONFIG_PLAT_KVM && !CONFIG_VIRTIO_BALLOON

#include <errno.h>

static inline int __balloon_not_enabled(void)
{
	return -ENOSYS;
}

#define ukplat_inflate(va, order)	\
	__balloon_not_enabled()

#define ukplat_inflate(va, order)	\
	__balloon_not_enabled()

#endif /* CONFIG_PLAT_KVM && !CONFIG_VIRTIO_BALLOON */

#endif /* __UKPLAT_BALLOON_H__ */

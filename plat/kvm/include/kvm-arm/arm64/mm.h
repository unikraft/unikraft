/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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

#ifndef __KVM_ARM_64_MM_H__
#define __KVM_ARM_64_MM_H__

/*
 * We will place the pagetable and boot stack after image area,
 * So we define the address offset of pagetable and boot stack
 * here.
 */

/*
 * Each entry in L0_TABLE can link to a L1_TABLE which supports 512GiB
 * memory mapping. One 4K page can provide 512 entries. In this case,
 * one page for L0_TABLE is enough for current stage.
 */
#define L0_TABLE_OFFSET 0
#define L0_TABLE_SIZE   __PAGE_SIZE

/*
 * Each entry in L1_TABLE can map to a 1GiB memory or link to a
 * L2_TABLE which supports 1GiB memory mapping. One 4K page can provide
 * 512 entries. We need at least 2 pages to support 1TB memory space
 * for platforms like KVM QEMU virtual machine.
 */
#define L1_TABLE_OFFSET (L0_TABLE_OFFSET + L0_TABLE_SIZE)
#define L1_TABLE_SIZE   (__PAGE_SIZE * 2)

/*
 * Each entry in L2_TABLE can map to a 2MiB block memory or link to a
 * L3_TABLE which supports 2MiB memory mapping. We need a L3_TABLE to
 * cover image area for us to manager different sections attributes.
 * So, we need one page for L2_TABLE to provide 511 enties for 2MiB
 * block mapping and 1 entry for L3_TABLE link.
 */
#define L2_TABLE_OFFSET (L1_TABLE_OFFSET + L1_TABLE_SIZE)
#define L2_TABLE_SIZE   __PAGE_SIZE

/*
 * As Unikraft image's size is very tiny, from tens to hundreds kilo
 * bytes. So one page for L3_TABLE is enough for us to manage section
 * attributes of image.
 */
#define L3_TABLE_OFFSET (L2_TABLE_OFFSET + L2_TABLE_SIZE)
#define L3_TABLE_SIZE   __PAGE_SIZE

/* Total memory size that will be used by pagetable */
#define PAGE_TABLE_SIZE (L0_TABLE_SIZE + L1_TABLE_SIZE + \
				L2_TABLE_SIZE + L3_TABLE_SIZE)

#endif /* __KVM_ARM_64_MM_H__ */

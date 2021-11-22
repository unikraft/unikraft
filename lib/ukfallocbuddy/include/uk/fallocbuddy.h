/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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

#ifndef __UK_BFALLOC_H__
#define __UK_BFALLOC_H__

#include <uk/falloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a buddy frame allocator
 *
 * @param fa pointer to an uninitialized buddy frame allocator. Use
 *    uk_fallocbuddy_size() when allocating the required memory
 *
 * @return 0 on success, a non-zero error value otherwise
 */
int uk_fallocbuddy_init(struct uk_falloc *fa);

/**
 * Returns the size of a buddy frame allocator
 *
 * @return size of allocator in bytes
 */
__sz uk_fallocbuddy_size(void);

/**
 * Returns the size in bytes required for the metadata when adding a range of
 * physical memory to be managed by the allocator
 *
 * @param frames number of frames (i.e., PAGE_SIZE) to be added to allocator
 *
 * @return size of metadata in bytes
 */
__sz uk_fallocbuddy_metadata_size(unsigned long frames);

#ifdef __cplusplus
}
#endif

#endif /* __UK_BFALLOC_H__ */

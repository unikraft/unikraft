/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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

#ifndef __UKARCH_CTX_H__
#define __UKARCH_CTX_H__

#include <uk/arch/types.h>

/**
 * State of extended context, like additional CPU registers and units
 * (e.g., floating point, vector registers)
 */
struct ukarch_ectx;

/**
 * Size needed to allocate memory to store an extended context state
 */
__sz ukarch_ectx_size(void);

/**
 * Alignment requirement for allocated memory to store an
 * extended context state
 */
__sz ukarch_ectx_align(void);

/**
 * Initializes an extended context so that it can be loaded
 * into a logical CPU with `ukarch_ectx_load()`.
 *
 * @param state
 *   Reference to extended context to initialize
 */
void ukarch_ectx_init(struct ukarch_ectx *state);

/**
 * Stores the extended context of the currently executing CPU to `state`.
 * Such an extended context can be restored with `ukarch_ectx_load()`.
 *
 * @param state
 *   Reference to extended context to save to
 */
void ukarch_ectx_store(struct ukarch_ectx *state);

/**
 * Restores a given extended context on the currently executing CPU.
 *
 * @param state
 *   Reference to extended context to restore
 */
void ukarch_ectx_load(struct ukarch_ectx *state);

#endif /* __UKARCH_CTX_H__ */

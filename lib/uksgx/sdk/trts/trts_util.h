/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef _TRTS_UTIL_H_
#define _TRTS_UTIL_H_

#include <stddef.h>    /* for size_t */
#include <stdbool.h>
#include "se_types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t get_enclave_size(void);
size_t get_enclave_end(void);
void * get_heap_base(void);
size_t get_heap_size(void);
size_t get_heap_min_size(void);
void * get_rsrv_base(void);
size_t get_rsrv_end(void);
size_t get_rsrv_size(void);
size_t get_rsrv_min_size(void);
int * get_errno_addr(void);
bool is_stack_addr(void *address, size_t size);
bool is_valid_sp(uintptr_t sp);

int heap_init(void *_heap_base, size_t _heap_size, size_t _heap_min_size, int _is_edmm_supported);
int feature_supported(const uint64_t *feature_set, uint32_t feature_shift);
bool is_utility_thread();
size_t get_max_tcs_num();
bool is_pkru_enabled();

bool is_tcs_binding_mode();

#ifdef __cplusplus
}
#endif

#endif


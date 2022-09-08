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

#ifndef _SL_ATOMIC_H_
#define _SL_ATOMIC_H_

#include <stdlib.h>
#include <sl_types.h>
#include "sgx_lfence.h"

// check 8-byte-aligned to the untrusted address.
//if the check fails, it should be hacked. call abort directly
#define CHECK_ALIGNMENT(x)             if (((uint64_t)x) % 8 != 0) abort();

static inline void lock_or64(volatile uint64_t* p, uint64_t v)
{
    CHECK_ALIGNMENT(p);
    __asm__( "lock ; orq %1, %0"
        : "=m"(*p) : "r"(v) : "memory" );
}

#define lock_or lock_or64

static inline void lock_and64(volatile uint64_t *p, uint64_t v)
{
    CHECK_ALIGNMENT(p);
    __asm__( "lock ; andq %1, %0"
             : "=m"(*p) : "r"(v) : "memory" );
}

#define lock_and lock_and64

static inline uint64_t lock_cmpxchg64(volatile uint64_t *p, uint64_t old_val, uint64_t new_val)
{
    CHECK_ALIGNMENT(p);
    __asm__( "lock ; cmpxchgq %3, %1"
        : "=a"(old_val), "=m"(*p) : "a"(old_val), "r"(new_val) : "memory" );
    return old_val;
}

#define lock_cmpxchg lock_cmpxchg64

static inline void* lock_cmpxchg_ptr(void * volatile *p, void* old_val, void* new_val)
{
    return (void*)lock_cmpxchg64((volatile uint64_t*)p, (uint64_t)old_val, (uint64_t)new_val);
}

static inline uint64_t xchg64(volatile uint64_t* x, uint64_t v)
{
    CHECK_ALIGNMENT(x);
    __asm__( "xchgq %0, %1" : "=r"(v), "=m"(*x) : "0"(v) : "memory" );
    return v;
}

#define xchg xchg64 

static inline void lock_inc64(volatile uint64_t *x)
{
    CHECK_ALIGNMENT(x);
    __asm__( "lock ; incq %0" : "=m"(*x) : "m"(*x) : "memory" );
}

#define lock_inc lock_inc64

static inline void lock_dec64(volatile uint64_t *x)
{
    CHECK_ALIGNMENT(x);
    __asm__( "lock ; decq %0" : "=m"(*x) : "m"(*x) : "memory" );
}

#define lock_dec lock_dec64


static inline void asm_pause(void)
{
    __asm__ __volatile__( "pause" : : : "memory" );
}


#define lock_xchg_add __sync_fetch_and_add


#ifdef __cplusplus
extern "C" {
#endif

    extern void __builtin_ia32_mfence(void);

#ifdef __cplusplus
}
#endif

#define sgx_mfence  __builtin_ia32_mfence

#endif

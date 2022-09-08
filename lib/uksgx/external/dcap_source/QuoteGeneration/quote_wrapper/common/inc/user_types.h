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
/**
 * File: user_types.h 
 *  
 * Description: General ECDSA
 * refercne code definitions.
 *
 */

/* User defined types */
#ifndef _USER_TYPES_H_
#define _USER_TYPES_H_
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sgx_key.h"
#include "se_trace.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define REF_ECP256_KEY_SIZE     32
#define REF_NISTP_ECP256_KEY_SIZE       (REF_ECP256_KEY_SIZE/sizeof(uint32_t))
#define REF_SHA256_HASH_SIZE            32
#define REF_RSA_OAEP_2048_MOD_SIZE      256 //hardcode n size to be 256
#define REF_RSA_OAEP_2048_EXP_SIZE      4   //hardcode e size to be 4
#define REF_RSA_OAEP_3072_MOD_SIZE      384 //hardcode n size to be 384
#define REF_RSA_OAEP_3072_EXP_SIZE      4   //hardcode e size to be 4

#pragma pack(push, 1)
typedef uint8_t ref_sha256_hash_t[REF_SHA256_HASH_SIZE];

typedef struct _ref_ec256_public_t {
    uint8_t gx[REF_ECP256_KEY_SIZE];
    uint8_t gy[REF_ECP256_KEY_SIZE];
} ref_ec256_public_t;

typedef struct _ref_ec256_signature_t {
    uint32_t x[REF_NISTP_ECP256_KEY_SIZE];
    uint32_t y[REF_NISTP_ECP256_KEY_SIZE];
} ref_ec256_signature_t;

/** Data structure without alignment required. */
typedef struct _sgx_psvn_t {
    sgx_cpu_svn_t    cpu_svn;  ///< CPUSVN of the platform
    sgx_isv_svn_t    isv_svn;  ///< ISV_SVN of the enclave
}sgx_psvn_t;

#pragma pack(pop)

#ifndef _ERRNO_T_DEFINED
    #define _ERRNO_T_DEFINED
typedef int errno_t;
#endif

#ifndef _MSC_VER
#define STATIC_ASSERT_UNUSED_ATTRIBUTE __attribute__((unused)) 
#else
#define STATIC_ASSERT_UNUSED_ATTRIBUTE
#endif
#define _ASSERT_CONCAT(a, b) a##b
#define ASSERT_CONCAT(a, b) _ASSERT_CONCAT(a, b)
#define ref_static_assert(e) typedef char ASSERT_CONCAT(assert_line, __LINE__)[(e)?1:-1] \
        STATIC_ASSERT_UNUSED_ATTRIBUTE
#define MIN(x, y) (((x)>(y))?(y):(x))
#define MAX(x, y) (((x)>(y))?(x):(y))
#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x[0]))
#define    ROUND_TO(x, align)  (((x) + ((align)-1)) & ~((align)-1))
#if !defined(SWAP_ENDIAN_DW)
    #define SWAP_ENDIAN_DW(dw)    ((((dw) & 0x000000ff) << 24)              \
    | (((dw) & 0x0000ff00) << 8)                                            \
    | (((dw) & 0x00ff0000) >> 8)                                            \
    | (((dw) & 0xff000000) >> 24))
#endif

#if !defined(SWAP_ENDIAN_32B)
    #define SWAP_ENDIAN_32B(ptr)                                            \
{                                                                           \
    unsigned int temp = 0;                                                  \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[0]);                       \
    ((unsigned int*)(ptr))[0] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[7]);  \
    ((unsigned int*)(ptr))[7] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[1]);                       \
    ((unsigned int*)(ptr))[1] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[6]);  \
    ((unsigned int*)(ptr))[6] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[2]);                       \
    ((unsigned int*)(ptr))[2] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[5]);  \
    ((unsigned int*)(ptr))[5] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[3]);                       \
    ((unsigned int*)(ptr))[3] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[4]);  \
    ((unsigned int*)(ptr))[4] = temp;                                       \
}
#endif

// Some utility functions to output some of the data structures.
#ifdef DISABLE_TRACE
#define PRINT_BYTE_ARRAY(...)
#else
#define PRINT_BYTE_ARRAY(level, mem, len)               \
{                                                       \
    if (!mem || !len)                                   \
    {                                                   \
        SE_TRACE(level, "\n( null )\n");                \
    }                                                   \
    else                                                \
    {                                                   \
        uint8_t *array = (uint8_t *)mem;                \
        uint32_t pba_i = 0;                             \
        for (pba_i = 0; pba_i < len - 1; pba_i++)       \
        {                                               \
            SE_TRACE(level, "%02x", array[pba_i]);      \
            if (pba_i % 32 == 31) SE_TRACE(level, "\n");\
        }                                               \
        SE_TRACE(level, "%02x", array[pba_i]);          \
    }                                                   \
}
#endif

#endif  // _USER_TYPES_H_



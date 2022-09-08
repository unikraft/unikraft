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
#pragma once

#ifndef _METADATA_H_
#define _METADATA_H_
#include "arch.h"
#include "se_macro.h"

#pragma pack(1)

 //version of metadata
#define MAJOR_VERSION 1         //MAJOR_VERSION should not larger than 0ffffffff

#ifdef SGX_GPR_PRE_ECO
#define MINOR_VERSION_1_0 0         //MINOR_VERSION should not larger than 0ffffffff
#else
#define MINOR_VERSION_1_0 1         //MINOR_VERSION should not larger than 0ffffffff
#endif

#define MINOR_VERSION 2

#define META_DATA_MAKE_VERSION(major, minor) (((uint64_t)major)<<32 | minor)
#define MAJOR_VERSION_OF_METADATA(version) (((uint64_t)version) >> 32)
#define MINOR_VERSION_OF_METADATA(version) (((uint64_t)version) & 0xFFFFFFFF)

#define METADATA_MAGIC 0x86A80294635D0E4CULL
#define METADATA_SIZE 0x754

// TCS Policy bit masks
#define TCS_POLICY_BIND     0x00000000  // If set, the TCS is bound to the application thread
#define TCS_POLICY_UNBIND   0x00000001

#define MAX_SAVE_BUF_SIZE 2632    

#define TCS_NUM_MIN 1
#define SSA_NUM_MIN 2
#define SSA_FRAME_SIZE_MIN 1
#define SSA_FRAME_SIZE_MAX 2
#define STACK_SIZE_MIN 0x1000
#define HEAP_SIZE_MIN 0
#define DEFAULT_MISC_SELECT 0
#define DEFAULT_MISC_MASK 0xFFFFFFFF
#define ISVFAMILYID_MAX   0xFFFFFFFFFFFFFFFFULL
#define ISVEXTPRODID_MAX  0xFFFFFFFFFFFFFFFFULL

typedef struct _metadata_t 
{
    uint64_t            magic_num;             //(0x00)  The magic number identifying the file as a signed enclave image
    uint64_t            version;               //(0x08)  The metadata version
    uint32_t            size;                  //(0x10)  The size of this structure
    uint32_t            tcs_num;               //(0x14)   Then number of TCS
    uint32_t            tcs_policy;            //(0x18)   TCS management policy
    uint32_t            ssa_num;               //(0x1C)   The number of SSA frames per TCS 
    uint32_t            ssa_frame_size;        //(0x20)   The size of SSA frame in page
    uint32_t            stack_max_size;        //(0x24)   The maximum stack size per thread
    uint32_t            heap_max_size;         //(0x28)   The maximum heap size
    uint32_t            max_save_buffer_size;  //(0x2C)   Max buffer size  is 2632
    uint32_t            desired_misc_select;   //(0x30)   Desired MISCSELECT for the enclave
    sgx_attributes_t    attributes;            //(0x34)   enclave attributes to be set in SECS.
    enclave_css_t       enclave_css;           //(0x44)   The enclave signature
}metadata_t;

se_static_assert(sizeof(metadata_t) == METADATA_SIZE);

#pragma pack()

#endif

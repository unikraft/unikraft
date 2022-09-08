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
#include <InitGuid.h>

#include "sgx_lc_msr_public.h"

#include "arch.h"
#include "sgx_attributes.h"

DEFINE_GUID(GUID_DEVINTERFACE_SGX_LAUNCH_TOKEN_INTERFACE,
            0x17eaf82e, 0xe167, 0x4763, 0xb5, 0x69, 0x5b, 0x82, 0x73, 0xce, 0xf6, 0xe1);

#define IOCTL_SGX_GETTOKEN \
    CTL_CODE( SGXIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define SGX_LAUNCH_TOKEN_INTERFACE_NAME                  L"SGX Launch Token Interface"

#pragma pack(push)
#pragma pack(1)
typedef struct sgx_launch_token_request
{
    UINT32 version;
    enclave_css_t css;
    sgx_attributes_t secs_attr;
} sgx_launch_token_request_t;

typedef struct sgx_launch_token_output
{
    token_t token;
    int result;
} sgx_launch_token_output_t;
#pragma pack(pop)

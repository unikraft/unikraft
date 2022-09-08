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

DEFINE_GUID (GUID_DEVINTERFACE_SGX_LAUNCH_CONTROL_MSR_INTERFACE,
             0x1260c6ce,0x7964,0x4b0e,0xb9,0x48,0x90,0x47,0x90,0xe0,0xe5,0x4d);

#define SGXIOCTL_TYPE 90001
#define IOCTL_SGX_GETLAUNCHSUPPORT \
    CTL_CODE( SGXIOCTL_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS  )


// The platform has opt'ed into PLE
#define SGX_PLE_REGISTRY_OPT_IN   0x000000001

// The platform supports Launch Control Policy
#define SGX_LCP_PLATFORM_SUPPORT  0x000000002

// The OS has granted FLCMSR the option to set LCP
//   (pubKeyHash is valid)
#define SGX_LCP_OS_PERMISSION     0x000000004


#pragma pack(push)
#pragma pack(1)
typedef struct _SGX_PUBKEYHASH
{
    ULONGLONG    pubKeyHash_Value_0;
    ULONGLONG    pubKeyHash_Value_1;
    ULONGLONG    pubKeyHash_Value_2;
    ULONGLONG    pubKeyHash_Value_3;
} SGX_PUBKEYHASH;

typedef struct sgx_get_launch_support_input
{
    UINT32          version;
} sgx_get_launch_support_input_t;

typedef struct sgx_get_launch_support_output
{
    UINT32          configurationFlags;
    SGX_PUBKEYHASH  pubKeyHash;
} sgx_get_launch_support_output_t;
#pragma pack(pop)
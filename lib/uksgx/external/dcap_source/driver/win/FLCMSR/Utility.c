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

#include "Utility.h"
#include "Utility.tmh"
#include "Key.h"


BOOLEAN is_HW_support_FLC()
{
    int cpuInfo[4] = { 0 };

    __cpuidex(cpuInfo, 0x7, 0);

    if (!(cpuInfo[2] & ((int)1 << 30)))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN is_OS_support_FLC()
{
    NTSTATUS status;
    SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION FLC_info = { 0 };

    FLC_info.key0 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_0;
    FLC_info.key1 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_1;
    FLC_info.key2 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_2;
    FLC_info.key3 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_3;

    status = ZwSetSystemInformation(
                 SystemLEnclaveLaunchControlInfomation,
                 &FLC_info,
                 sizeof(SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UTILITY, "ZwQuerySystemInformation %!STATUS!", status);

    //Windows owns the launch control
    if (status == STATUS_ACCESS_DENIED)
    {
        return FALSE;
    }

    return TRUE;
}




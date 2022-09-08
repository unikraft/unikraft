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
#include "Wdm.h"
#include "trace.h"

#define MSR_IA32_FEATURE_CONTROL                                      0x0000003A

#define MSR_IA32_SGX_LE_PUBKEYHASH_0                                  0x0000008C
#define MSR_IA32_SGX_LE_PUBKEYHASH_1                                  0x0000008D
#define MSR_IA32_SGX_LE_PUBKEYHASH_2                                  0x0000008E
#define MSR_IA32_SGX_LE_PUBKEYHASH_3                                  0x0000008F

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemLEnclaveLaunchControlInfomation = 203
} SYSTEM_INFORMATION_CLASS;

NTSTATUS ZwQuerySystemInformation(
    _In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Inout_   PVOID                    SystemInformation,
    _In_      ULONG                    SystemInformationLength,
    _Out_opt_ PULONG                   ReturnLength
);

NTSTATUS ZwSetSystemInformation(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength);

typedef struct _SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION
{
    UINT64 key0;
    UINT64 key1;
    UINT64 key2;
    UINT64 key3;
} SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION, *PSYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION;

BOOLEAN is_HW_support_FLC();
BOOLEAN is_OS_support_FLC();

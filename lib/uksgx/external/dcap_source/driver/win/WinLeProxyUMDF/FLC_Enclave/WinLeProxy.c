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

#include <stdio.h>
#include <windows.h>
#include <WinBase.h>
#include <Enclaveapi.h>

#include "sgx_arch.h"

#ifndef _CONSOLE
#include <wdf.h>
#include "Trace.h"
#include "winleproxy.tmh"
#else
#define TraceEvents(...)
#endif

// Include enclave and sigstruct
#include "sgx_le_blob.h"
#include "sgx_le_ss.h"

void *start_launch_enclave(void)
{
    struct sgx_secs secs;
    UINT64 offset;
    BOOL bSuccess;
    ENCLAVE_INIT_INFO_SGX initInfo;
    HANDLE hProcess = GetCurrentProcess();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_PROXY, "%!FUNC! Entry");

    memset(&secs, 0, sizeof(secs));
    memset(&initInfo, 0, sizeof(initInfo));

    secs.ssaframesize = 1;
    secs.attributes = SGX_ATTR_MODE64BIT | SGX_ATTR_EINITTOKENKEY;
    secs.xfrm = 3;

    for (secs.size = 0x1000; secs.size < sgx_le_blob_length; )
        secs.size <<= 1;

    DWORD enclaveError = 0;
    PVOID lpBase = CreateEnclave(
                       hProcess,
                       NULL,
                       secs.size,
                       sgx_le_blob_length,
                       ENCLAVE_TYPE_SGX,
                       (LPCVOID)&secs,
                       sizeof(secs),
                       &enclaveError);

    if (!lpBase)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PROXY, "CreateEnclave failed - error: %d lasterror: %d\n", enclaveError, GetLastError());
        goto out;
    }

    for (offset = 0; offset < sgx_le_blob_length; offset += 0x1000)
    {
        SIZE_T numberOfBytesWritten;
        LPVOID lpAddress = (LPVOID)((UINT64)lpBase + offset);
        LPVOID lpBuffer = (LPVOID)((UINT64)sgx_le_blob + offset);
        DWORD flProtect = PAGE_EXECUTE_READWRITE;
        if (!offset)
            flProtect = PAGE_ENCLAVE_THREAD_CONTROL | PAGE_READWRITE;

        bSuccess = LoadEnclaveData(hProcess, lpAddress, lpBuffer, 0x1000, flProtect, NULL, 0,
                                   &numberOfBytesWritten, &enclaveError);
        if (!bSuccess)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_PROXY, "LoadEnclaveData failed - offset: 0x%I64X error: %d lasterror: %d\n", offset, enclaveError, GetLastError());
            goto out;
        }
    }

    memcpy_s(&initInfo, sizeof(initInfo), sgx_le_ss, sgx_le_ss_length);
    bSuccess = InitializeEnclave(hProcess, lpBase, &initInfo, sizeof(initInfo), &enclaveError);
    if (!bSuccess)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PROXY, "InitializeEnclave failed - error: %d lasterror: %d\n", enclaveError, GetLastError());
        goto out;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_PROXY, "%!FUNC! Exit. InitializeEnclave success - 0x%p\n", lpBase);
    return (void *)lpBase;
out:
    TraceEvents(TRACE_LEVEL_ERROR, TRACE_PROXY, "%!FUNC! Exit. PLE load failed");
    if (lpBase)
    {
        VirtualFree(lpBase, 0, MEM_RELEASE);
        lpBase = NULL;
    }
    return NULL;
}


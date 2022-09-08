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
 * File: qve_parser.cpp
 *
 * Description: Wrapper functions for the
 * reference implementing the QvE
 * function defined in sgx_qve.h. This
 * would be replaced or used to wrap the
 * PSW defined interfaces to the QvE.
 *
 */

#include <tchar.h>
#include <windows.h>

#define QvE_ENCLAVE_NAME _T("qve.signed.dll")

bool get_qve_path(
    TCHAR *p_file_path,
    size_t buf_size)
{
    if(!p_file_path)
        return false;

    HMODULE hModule;
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, _T(__FUNCTION__), &hModule))
        return false;
    DWORD path_length = GetModuleFileName(hModule, p_file_path, static_cast<DWORD>(buf_size));
    if (path_length == 0)
        return false;
    if (path_length == buf_size)
        return false;

    TCHAR *p_last_slash = _tcsrchr(p_file_path, _T('\\'));
    if (p_last_slash != NULL)
    {
        p_last_slash++;
        *p_last_slash = _T('\0');
    }
    else
        return false;

    if (_tcsnlen(QvE_ENCLAVE_NAME, MAX_PATH) + _tcsnlen(p_file_path, MAX_PATH) + sizeof(TCHAR) > buf_size)
        return false;
    if (_tcscat_s(p_file_path, buf_size, QvE_ENCLAVE_NAME))
        return false;

    return true;
}

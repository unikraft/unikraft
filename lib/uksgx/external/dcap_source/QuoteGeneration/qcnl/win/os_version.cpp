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
  * File: os_version.cpp
  *
  * Description: Detect Windows OS version
  *
  */
#include <Windows.h>

// Get Windows Version
typedef LONG NTSTATUS, *PNTSTATUS;
typedef NTSTATUS(WINAPI *FpRtlGetVersion)(PRTL_OSVERSIONINFOW);

bool isWin81OrLater() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        FpRtlGetVersion rtlGetVersion = (FpRtlGetVersion)::GetProcAddress(hMod, "RtlGetVersion");
        if (rtlGetVersion != nullptr) {
            RTL_OSVERSIONINFOW versionInfo = {0};
            versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
            if (0 == rtlGetVersion(&versionInfo)) {
                // For Windows 10, MajorVersion is 10
                // For Windows 8.1, MajorVersion=6 and MinorVersion=3
                // For Windows 8, MajorVersion=6 and MinorVersion=2
                // For Windows 7, MajorVersion=6 and MinorVersion=1
                if (versionInfo.dwMajorVersion < 6 ||
                    (versionInfo.dwMajorVersion == 6 && versionInfo.dwMinorVersion <= 2))
                    return false;
            }
        }
    }
    // default to true if we failed to get the info
    return true;
}

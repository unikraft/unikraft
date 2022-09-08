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
 * File: defs.h 
 *   
 * Description: Definitions of the high-level data types used in the 
 * tool binaries. 
 */
#ifndef __MP_TOOL_DEFS_H
#define __MP_TOOL_DEFS_H

char const *MpSgxStatusValues[] =
{
    "MP_SGX_ENABLED",
    "MP_SGX_DISABLED_REBOOT_REQUIRED",
    "MP_SGX_DISABLED_LEGACY_OS",
    "MP_SGX_DISABLED",
    "MP_SGX_DISABLED_SCI_AVAILABLE",   
    "MP_SGX_DISABLED_MANUAL_ENABLE",  
    "MP_SGX_DISABLED_HYPERV_ENABLED",     
    "MP_SGX_DISABLED_UNSUPPORTED_CPU"    
};

char const *MpSgxStatusStr[] =
{
    "SGX is enabled",
    "A reboot is required to finish enabling SGX",
    "SGX is disabled and a Software Control Interface is not available to enable it",
    "SGX is not enabled on this platform. More details are unavailable",
    "SGX is disabled, but a Software Control Interface is available to enable it",   
    "SGX is disabled, but can be enabled manually in the BIOS setup",  
    "Detected an unsupported version of Windows* 10 with Hyper-V enabled",     
    "SGX is not supported by this CPU"    
};

#endif // #ifndef __MP_TOOL_DEFS_H

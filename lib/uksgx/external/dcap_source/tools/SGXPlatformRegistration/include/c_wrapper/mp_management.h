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
 * File: mp_mangement.h 
 *   
 * Description: C definition of function wrappers for C++ methods in 
 * the MPManagement class implementation.
 */
#ifndef __MP_MANAGEMENT_H
#define __MP_MANAGEMENT_H

#include "MultiPackageDefs.h"

extern "C" 
{
	void     mp_management_init(const char* path);

	MpResult mp_management_get_package_info_key_blobs(uint8_t *buffer, uint16_t *size);
    MpResult mp_management_get_platform_manifest(uint8_t *buffer, uint16_t *size);
    MpResult mp_management_get_registration_error_code(RegistrationErrorCode *error_code);
    MpResult mp_management_get_registration_status(MpTaskStatus *status);

    MpResult mp_management_get_sgx_status(MpSgxStatus *status);

    MpResult mp_management_set_registration_server_info(uint16_t flags, string url, const uint8_t *serverId, uint16_t serverIdSize);
    
	void     mp_management_terminate();
};
#endif // __#ifndef MP_MANAGEMENT_H
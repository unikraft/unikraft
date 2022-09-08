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
 * File: mp_uefi.cpp
 *   
 * Description: Implemenation of the C funcitons that wrap the class methods 
 * defined in the FSUefi class.
 */
#include "MPUefi.h"
#include "mp_uefi.h"
#include <string>

using std::string;

MPUefi* g_mpUefi = NULL;

MpResult mp_uefi_init(const char* path, const LogLevel logLevel)
{
    MpResult res = MP_UNEXPECTED_ERROR;
    if (g_mpUefi) {
        res = MP_REDUNDANT_OPERATION;
        goto out;
    }

	g_mpUefi = new MPUefi(path ? string(path) : "", logLevel);
    if (!g_mpUefi) {
        res = MP_MEM_ERROR;
        goto out;
    }
    res = MP_SUCCESS;
out:
    return res;
}

MpResult mp_uefi_get_request_type(MpRequestType* type)
{
    if (!type) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }
    
	return g_mpUefi->getRequestType(*type);
}

MpResult mp_uefi_get_request(uint8_t *request, uint16_t *request_size) {
    if (!request_size) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }

	return g_mpUefi->getRequest(request, *request_size);
}

MpResult mp_uefi_set_server_response(const uint8_t *response, uint16_t *response_size) {
    if (!response || !response_size) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }

	return g_mpUefi->setServerResponse(response, *response_size);
}

MpResult mp_uefi_get_key_blobs(uint8_t *blobs, uint16_t *blobs_size)
{
    if (!blobs_size) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }

	return g_mpUefi->getKeyBlobs(blobs, *blobs_size);
}

MpResult mp_uefi_get_registration_status(MpRegistrationStatus* status)
{
    if (!status) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }

	return g_mpUefi->getRegistrationStatus(*status);
}

MpResult mp_uefi_set_registration_status(const MpRegistrationStatus* status)
{
    if (!status) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }

	return g_mpUefi->setRegistrationStatus(*status);
}

MpResult mp_uefi_get_registration_server_info(uint16_t *flags, string *serverAddress, uint8_t *serverId, uint16_t *serverIdSize)
{
    uint16_t *usedServerIdSize = serverIdSize;
    uint16_t dummySize = 0;
    if (!flags || !serverAddress) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }
    
    if (!serverIdSize) {
        usedServerIdSize = &dummySize;
    }

	return g_mpUefi->getRegistrationServerInfo(*flags, *serverAddress, serverId, *usedServerIdSize);
}

MpResult mp_uefi_set_registration_server_info(const uint16_t flags, const string serverAddress ,const uint8_t *serverId, const uint16_t serverIdSize)
{
    if (!serverId) {
        return MP_INVALID_PARAMETER;
    }

    if (!g_mpUefi) {
        return MP_NOT_INITIALIZED;
    }

	return g_mpUefi->setRegistrationServerInfo(flags, serverAddress, serverId, serverIdSize);
}


MpResult mp_uefi_terminate() 
{
    MpResult res = MP_UNEXPECTED_ERROR;
    if (g_mpUefi) {
        delete g_mpUefi;
        g_mpUefi = NULL;
        res = MP_SUCCESS;
    } else {
        res = MP_REDUNDANT_OPERATION;
    }
    return res;
}

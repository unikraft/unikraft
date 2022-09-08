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
 * File: mp_network.cpp
 *   
 * Description: Implemenation of the C funcitons that wrap the class methods 
 * defined in the MPNetwork class.
 */
#include "MPNetwork.h"
#include "mp_network.h"

MPNetwork* g_mpNetwork = NULL;

MpResult mp_network_init(const char *server_address, const char *subscription_key, const ProxyConf *proxy, const LogLevel logLevel) {
    MpResult res = MP_UNEXPECTED_ERROR;
    string url_str;
    string subscription_key_str;

    if (!server_address || !subscription_key || !proxy) {
        res = MP_INVALID_PARAMETER;
        goto out;
    }

    if (g_mpNetwork) {
        res = MP_REDUNDANT_OPERATION;
        goto out;
    }

    url_str = string(server_address);
    subscription_key_str = string(subscription_key);
    g_mpNetwork = new MPNetwork(url_str, subscription_key_str, *proxy, logLevel);
    if (!g_mpNetwork) {
        res = MP_MEM_ERROR;
        goto out;
    }

    res = MP_SUCCESS;
out:
    return res;
}

MpResult mp_send_binary_request(const MpRequestType request_type, const uint8_t *request, const uint16_t request_size, uint8_t *response,
        uint16_t *response_size, HttpStatusCode *status_code, RegistrationErrorCode *error_code) {
    MpResult res = MP_UNEXPECTED_ERROR;

    if (!request || !response_size || !status_code || !error_code) {
        res = MP_INVALID_PARAMETER;
        goto out;
    }

    if (!g_mpNetwork) {
        res = MP_NOT_INITIALIZED;
        goto out;
    }

    res = g_mpNetwork->sendBinaryRequest(request_type, request, request_size, response, 
        *response_size, *status_code, *error_code);
out:
    return res;
}

MpResult mp_network_terminate()
{
    MpResult res = MP_UNEXPECTED_ERROR;

    if (g_mpNetwork) {
        delete g_mpNetwork;
        g_mpNetwork = NULL;
        res = MP_SUCCESS;
    } else {
        res = MP_REDUNDANT_OPERATION;
    }

    return res;
}

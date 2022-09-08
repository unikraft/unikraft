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
 * File: mp_network mangement.h 
 *   
 * Description: C definition of function wrappers for C++ methods in 
 * the MPNetwork class implementation.
 */
#ifndef __MP_NETWORK_H
#define __MP_NETWORK_H

#include "MPNetworkDefs.h"

/**
 * This is the main entry point for the SGX Multi-Package Network C interface.
 * Used to encode and send requests to the registration server, also decodes server responses.
 */

extern "C"
{
    /**
     * Multi-Package network interface initiation.
     *
     * @param server_address    - input parameter, server address.
     * @param subscription_key  - input parameter, server subscription key.
     * @param proxy             - input parameter, desired proxy configurations.
     * @param logLevel          - input parameter, desired logging level.
     *
     * @return status code, one of:
     *      - MP_SUCCESS
     *      - MP_INVALID_PARAMETER
     *      - MP_REDUNDANT_OPERATION
     *      - MP_MEM_ERROR
     */
	MPNetworkDllExport MpResult mp_network_init(const char *server_address, const char *subscription_key,
        const ProxyConf *proxy, const LogLevel logLevel = MP_REG_LOG_LEVEL_ERROR);

    /**
     * Sends request to the registration server.
     * 
     * @param request_type      - input parameter, request type.
     * @param request           - input parameter, binary request buffer to be sent.
     * @param request_size      - input parameter, size of request buffer in bytes.
     * @param response          - optional input parameter, binary response buffer to be populated.
     * @param response_size     - input parameter, size of response buffer in bytes.
     *                          - output paramerter, holds the actual size written to response buffer.
     *                            if result equals to MP_USER_INSUFFICIENT_MEM, holds the pending response size.
     * @param status_code       - output parameter, holds the received HTTP status code.
     * @param error_code        - output parameter, if statusCode equals to MPA_RS_BAD_REQUEST, holds the response error code.
     *
     * @return status code, one of:
     *      - MP_SUCCESS
     *      - MP_INVALID_PARAMETER
     *      - MP_USER_INSUFFICIENT_MEM
     *      - MP_NETWORK_ERROR
     *      - MP_UNEXPECTED_ERROR
     */
    MPNetworkDllExport MpResult mp_send_binary_request(const MpRequestType request_type, const uint8_t *request,
        const uint16_t request_size, uint8_t *response, uint16_t *response_size, HttpStatusCode *status_code, RegistrationErrorCode *error_code);

    /**
     * Multi-Package network interface termination.
     *
     * @return status code, one of:
     *      - MP_SUCCESS
     *      - MP_REDUNDANT_OPERATION
     */
	MPNetworkDllExport MpResult mp_network_terminate();
};

#endif // #ifndef __MP_NETWORK_H

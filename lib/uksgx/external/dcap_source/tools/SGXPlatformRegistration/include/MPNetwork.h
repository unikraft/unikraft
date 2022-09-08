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
 * File: MPNetwork.h 
 *   
 * Description: Class definition for the network functionality to 
 * communicate with the Registration Sever. 
 */
#ifndef __MPNETWORK_H
#define __MPNETWORK_H

#include "MPNetworkDefs.h"
#include <string>
using std::string;

class IMPSynchronicSender;

#ifdef _WIN32
#define MPNetworkDllExport  __declspec(dllexport)
#else
#define MPNetworkDllExport
#endif

/**
 * This is the main entry point for the SGX Multi-Package Network CPP interface.
 * Used to send requests to the SGX Registration Server.
 */

class MPNetwork {
public:
    /**
     * MPNetwork class constructor
     *
     * @param serverAddress     - input parameter, server address.
     * @param subscriptionKey   - input parameter, server subscription key.
     * @param proxy             - input parameter, desired proxy configurations.
     * @param logLevel          - input parameter, desired logging level.
     */
    MPNetworkDllExport MPNetwork(const string serverAddress, const string subscriptionKey, const ProxyConf proxy, const LogLevel logLevel = MP_REG_LOG_LEVEL_ERROR);

    /**
     * Sends request to the registration server.
     * 
     * @param requestType       - input parameter, request type.
     * @param request           - input parameter, binary request buffer to be sent.
     * @param requestSize       - input parameter, size of request buffer in bytes.
     * @param response          - optional input parameter, binary response buffer to be populated.
     * @param responseSize      - input parameter, size of response buffer in bytes.
     *                          - output paramerter, holds the actual size written to response buffer.
     *                            if result equals to MP_USER_INSUFFICIENT_MEM, holds the pending response size.
     * @param statusCode        - output parameter, holds the received HTTP status code.
     * @param errorCode         - output parameter, if statusCode equals to MPA_RS_BAD_REQUEST, holds the response error code.
     *
     * @return status code, one of:
     *      - MP_SUCCESS
     *      - MP_INVALID_PARAMETER
     *      - MP_USER_INSUFFICIENT_MEM
     *      - MP_NETWORK_ERROR
     *      - MP_UNEXPECTED_ERROR
     */
    MPNetworkDllExport MpResult sendBinaryRequest(const MpRequestType &requestType, const uint8_t *request, const uint16_t &requestSize,
        uint8_t *response, uint16_t &responseSize, HttpStatusCode &statusCode, RegistrationErrorCode &errorCode);

    /**
     * MPNetwork class destructor
     */
    MPNetworkDllExport virtual ~MPNetwork();
private:
    IMPSynchronicSender* m_sender;
    string m_serverAddress;
    string m_subscriptionKey;
    LogLevel m_logLevel;
    MPNetwork& operator=(const MPNetwork&) {return *this;}
    MPNetwork(const MPNetwork& src) {(void) src; }

};

#endif // #ifndef __MPNETWORK_H

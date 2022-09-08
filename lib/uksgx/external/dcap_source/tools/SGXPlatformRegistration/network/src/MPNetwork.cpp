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
 * File: MPNetwork.cpp
 *   
 * Description: Implementation for the high-level network functionality to 
 * communicate with the Registration Sever. 
 */
#include <map>
#include "network_logger.h"
#include "MPSynchronicSender.h"
#include "MPNetwork.h"

/* Registration Server ErrorCode Strings */
#define RS_ERR_INVALID_REQUEST_SYNTAX       "InvalidRequestSyntax"
#define RS_ERR_INVALID_REGISTRATION_SERVER  "InvalidRegistrationServer"
#define RS_ERR_INVALID_OR_REVOKED_PACKAGE   "InvalidOrRevokedPackage"
#define RS_ERR_PACKAGE_NOT_FOUND            "PackageNotFound" 
#define RS_ERR_INCOMPATIBLE_PACKAGE         "IncompatiblePackage"
#define RS_ERR_INVALID_PLATFORM_MANIFEST    "InvalidPlatformManifest"
#define RS_ERR_PLATFORM_NOT_FOUND           "PlatformNotFound" 
#define RS_ERR_INVALID_ADD_REQUEST          "InvalidAddRequest"

/* Registration Server URL path */
#define RS_PLATFORM_REGISTRATION_URL_PATH   "/sgx/registration/v1/platform"
#define RS_PACKAGE_ADD_URL_PATH             "/sgx/registration/v1/package"

MPNetwork::MPNetwork(const string serverAddress, const string subscriptionKey, const ProxyConf proxy, const LogLevel logLevel) {
    m_sender = new MPSynchronicSender(proxy, logLevel);
    m_logLevel = logLevel;
    m_serverAddress = serverAddress;
    m_subscriptionKey = subscriptionKey;
}

MpResult MPNetwork::sendBinaryRequest(const MpRequestType &requestType, const uint8_t *request, const uint16_t &requestSize, uint8_t *response,
    uint16_t &responseSize, HttpStatusCode &responseCode, RegistrationErrorCode &errorCode) {
    MpResult res = MP_UNEXPECTED_ERROR;
    string serverError;
    string path;
    string subscriptionKey = "";
    map<const string, RegistrationErrorCode> errorMap;
    map<const string, RegistrationErrorCode>::iterator errorMapItr;

    if ((NULL == request) || (0 == requestSize)) {
        res = MP_INVALID_PARAMETER;
        goto out;
    }

    if ((NULL != response) && (0 == responseSize)) {
        res = MP_INVALID_PARAMETER;
        goto out;
    }

    switch (requestType)
    {
    case MP_REQ_REGISTRATION:
        path = RS_PLATFORM_REGISTRATION_URL_PATH;
        break;
    case MP_REQ_ADD_PACKAGE:
        path = RS_PACKAGE_ADD_URL_PATH;
        subscriptionKey = m_subscriptionKey;

        if (subscriptionKey.length() != SUBSCRIPTION_KEY_SIZE) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Subscription key is invalid, please follow the instructions to configure a valid subscription key.\n");
            res = MP_INVALID_PARAMETER;
            goto out;
        }
        break;
    default:
        res = MP_INVALID_PARAMETER;
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "Unsupported request type.\n");
        goto out;
    }

    res = m_sender->sendBinaryRequest(m_serverAddress + path, subscriptionKey,
        request, requestSize, response, responseSize, responseCode, serverError);
    if (MP_SUCCESS != res) {
        goto out;
    }

    errorMap[RS_ERR_INVALID_REQUEST_SYNTAX] = MPA_RS_INVALID_REQUEST_SYNTAX;
    errorMap[RS_ERR_INVALID_REGISTRATION_SERVER] = MPA_RS_PM_INVALID_REGISTRATION_SERVER;
    errorMap[RS_ERR_INVALID_OR_REVOKED_PACKAGE] = MPA_RS_INVALID_OR_REVOKED_PACKAGE;
    errorMap[RS_ERR_PACKAGE_NOT_FOUND] = MPA_RS_PACKAGE_NOT_FOUND;
    errorMap[RS_ERR_INCOMPATIBLE_PACKAGE] = MPA_RS_PM_INCOMPATIBLE_PACKAGE;
    errorMap[RS_ERR_INVALID_PLATFORM_MANIFEST] = MPA_RS_PM_INVALID_PLATFORM_MANIFEST;
    errorMap[RS_ERR_PLATFORM_NOT_FOUND] = MPA_RS_AD_PLATFORM_NOT_FOUND;
    errorMap[RS_ERR_INVALID_ADD_REQUEST] = MPA_RS_AD_INVALID_ADD_REQUEST;

    errorMapItr = errorMap.find(serverError);
    if (errorMapItr != errorMap.end())
    {
        errorCode = errorMapItr->second;
    }
    else {
        errorCode = MPA_RS_UNKOWN_ERROR;
    }
    res = MP_SUCCESS;
out:
    return res;
}


MPNetwork::~MPNetwork()
{
    if (NULL != m_sender) {
        delete m_sender;
        m_sender = NULL;
    }
}

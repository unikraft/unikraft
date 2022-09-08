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

#include "EnclaveAdapter.h"

#include "Enclave_u.h"
#include "sgx_capable.h"

#include <iostream>
#include <stdexcept>
#include <memory>

#define ENCLAVE_FILE "../lib/libQuoteVerificationEnclave.signed.so"

EnclaveAdapter::EnclaveAdapter()
{
    sgx_device_status_t status;
    sgx_status_t ret;
    ret = sgx_cap_get_status(&status);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("SGX capability check failed: ret(" + std::to_string(ret) +
        ") status(" + std::to_string(status) + ")");
    }

    ret = sgx_cap_enable_device(&status);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed to enable SGX: ret(" + std::to_string(ret) +
        ") status(" + std::to_string(status) + ")");
    }

    sgx_launch_token_t token = { 0 };
    int updated = 0;
    ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed to create enclave: " + std::to_string(ret));
    }
}

EnclaveAdapter::~EnclaveAdapter()
{
    sgx_destroy_enclave(eid);
}

std::string EnclaveAdapter::getVersion() const
{
    size_t len = 10;
    std::unique_ptr<char[]> version(new char[len]);
    sgx_status_t ret = sgxEnclaveAttestationGetVersion(eid, version.get(), len);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed ecall to enclave: " + std::to_string(ret));
    }
    return std::string(version.get());
}


int EnclaveAdapter::verifyQuote(const uint8_t* rawQuote, uint32_t quoteSize, const char *pemPckCertificate, const char* pckCrl,
                                   const char* tcbInfoJson, const char* qeIdentityJson) const
{
    int status;
    sgx_status_t ret = sgxAttestationVerifyQuote(eid, &status, rawQuote, quoteSize, pemPckCertificate, pckCrl, tcbInfoJson, qeIdentityJson);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed verifyQuote ecall to enclave: " + std::to_string(ret));
    }
    return status;
}

int EnclaveAdapter::verifyPCKCertificate(const char *pemCertChain, const char *const crls[], const char *pemRootCaCertificate, const time_t* expirationDate) const
{
    int status;
    sgx_status_t ret = sgxAttestationVerifyPCKCertificate(eid, &status, pemCertChain, const_cast<char **>(crls), pemRootCaCertificate, expirationDate);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed verifyPCKCertificate ecall to enclave: " + std::to_string(ret));
    }
    return status;
}

int EnclaveAdapter::verifyTCBInfo(const char *tcbInfo, const char *pemCertChain, const char *pemRootCaCrl, const char *pemRootCaCertificate, const time_t* expirationDate) const
{
    int status;
    sgx_status_t ret = sgxAttestationVerifyTCBInfo(eid, &status, tcbInfo, pemCertChain, pemRootCaCrl, pemRootCaCertificate, expirationDate);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed verifyTCBInfo ecall to enclave: " + std::to_string(ret));
    }
    return status;
}

int EnclaveAdapter::verifyQEIdentity(const char *qeIdentity, const char *pemCertChain, const char *pemRootCaCrl, const char *pemRootCaCertificate, const time_t* expirationDate) const
{
    int status;
    sgx_status_t ret = sgxAttestationVerifyEnclaveIdentity(eid, &status, qeIdentity, pemCertChain, pemRootCaCrl, pemRootCaCertificate, expirationDate);
    if (ret != SGX_SUCCESS)
    {
        throw std::runtime_error("Failed verifyEnclaveIdentity ecall to enclave: " + std::to_string(ret));
    }
    return status;
}

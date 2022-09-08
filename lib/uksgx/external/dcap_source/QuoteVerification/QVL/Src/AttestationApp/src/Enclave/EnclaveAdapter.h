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

#ifndef SGXECDSAATTESTATION_ENCLAVEADAPTER_H
#define SGXECDSAATTESTATION_ENCLAVEADAPTER_H

#include <sgx_urts.h>
#include <string>
#include <vector>

class EnclaveAdapter
{
public:
    EnclaveAdapter();
    virtual ~EnclaveAdapter();

    std::string getVersion() const;

    int verifyQuote(const uint8_t* rawQuote, uint32_t quoteSize, const char *pemPckCertificate, const char* pckCrl,
                    const char* tcbInfoJson, const char* qeIdentityJson) const;

    int verifyPCKCertificate(const char *pemCertChain, const char *const crls[], const char *pemRootCaCertificate, const time_t* expirationDate) const;
    int verifyTCBInfo(const char *tcbInfo, const char *pemCertChain, const char *pemRootCaCrl, const char *pemRootCaCertificate, const time_t* expirationDate) const;
    int verifyQEIdentity(const char *qeIdentity, const char *pemCertChain, const char *pemRootCaCrl, const char *pemRootCaCertificate, const time_t* expirationDate) const;

private:
    sgx_enclave_id_t eid = 0;
};


#endif //SGXECDSAATTESTATION_ENCLAVEADAPTER_H

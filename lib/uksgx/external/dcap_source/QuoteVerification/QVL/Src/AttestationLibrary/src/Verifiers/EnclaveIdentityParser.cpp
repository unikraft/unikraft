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

#include <QuoteVerification/QuoteConstants.h>
#include <OpensslHelpers/Bytes.h>
#include "EnclaveIdentityParser.h"
#include "EnclaveIdentityV2.h"

#include <tuple>
#include <memory>

namespace intel { namespace sgx { namespace dcap {

    std::unique_ptr<dcap::EnclaveIdentityV2> EnclaveIdentityParser::parse(const std::string &input)
    {
        if (!jsonParser.parse(input))
        {
            throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT);
        }

        if (!jsonParser.getRoot()->IsObject())
        {
            throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
        }

        const auto* signature = jsonParser.getField("signature");

        if (signature == nullptr)
        {
            throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT);
        }

        if(!signature->IsString() || signature->GetStringLength() != constants::ECDSA_P256_SIGNATURE_BYTE_LEN * 2)
        {
            throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
        }

        auto signatureBytes = hexStringToBytes(signature->GetString());

        uint32_t version = 0;
        auto status = JsonParser::ParseStatus::Missing;

        auto identityField = jsonParser.getField("enclaveIdentity");

        if (identityField == nullptr || !identityField->IsObject())
        {
            throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
        }

        std::tie(version, status) = jsonParser.getIntFieldOf(*identityField, "version");
        if (status != JsonParser::OK)
        {
            throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
        }

        /// 4.1.2.9.4
        switch(version)
        {
            case EnclaveIdentityV2::V2:
            {
                std::unique_ptr<dcap::EnclaveIdentityV2> identity = std::unique_ptr<dcap::EnclaveIdentityV2>(new EnclaveIdentityV2(*identityField)); // TODO make std::make_unique work in SGX enclave
                if (identity->getStatus() != STATUS_OK)
                {
                    throw ParserException(identity->getStatus());
                }
                identity->setSignature(signatureBytes);
                return identity;
            }
            default:
                throw ParserException(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_VERSION);
        }
    }

    Status ParserException::getStatus() const
    {
        return status;
    }
}}}
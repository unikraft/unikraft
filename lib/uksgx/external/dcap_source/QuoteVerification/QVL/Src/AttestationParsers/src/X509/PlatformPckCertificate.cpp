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

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "OpensslHelpers/OidUtils.h"
#include "X509Constants.h"
#include "ParserUtils.h"

#include <algorithm> // find_if

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

PlatformPckCertificate::PlatformPckCertificate(const Certificate& certificate): PckCertificate(certificate)
{
    auto sgxExtensions = crypto::make_unique(getSgxExtensions());
    setMembers(sgxExtensions.get());
}

bool PlatformPckCertificate::operator==(const PlatformPckCertificate& other) const
{
    return PckCertificate::operator==(other) &&
           _platformInstanceId == other._platformInstanceId &&
           _configuration == other._configuration;
}

const std::vector<uint8_t>& PlatformPckCertificate::getPlatformInstanceId() const
{
    return _platformInstanceId;
}

const Configuration& PlatformPckCertificate::getConfiguration() const
{
    return _configuration;
}

PlatformPckCertificate PlatformPckCertificate::parse(const std::string& pem)
{
    return PlatformPckCertificate(pem);
}

// Private

PlatformPckCertificate::PlatformPckCertificate(const std::string& pem): PckCertificate(pem)
{
    auto sgxExtensions = crypto::make_unique(getSgxExtensions());
    setMembers(sgxExtensions.get());
}


void PlatformPckCertificate::setMembers(stack_st_ASN1_TYPE *sgxExtensions)
{
    PckCertificate::setMembers(sgxExtensions);

    const auto stackEntries = sk_ASN1_TYPE_num(sgxExtensions);
    if(stackEntries != PLATFORM_CA_EXTENSION_COUNT)
    {
        std::string err = "OID [" + oids::SGX_EXTENSION + "] expected to contain [" + std::to_string(PLATFORM_CA_EXTENSION_COUNT) +
                          "] elements when given [" + std::to_string(stackEntries) + "]";
        throw InvalidExtensionException(err);
    }

    std::vector<Extension::Type> expectedExtensions = constants::PLATFORM_PCK_REQUIRED_SGX_EXTENSIONS;

    // Iterate through SGX Extensions stored as sequence(tuple) of OIDName and OIDValue
    for (int i=0; i < stackEntries; i++) {
        const auto oidTupleWrapper = sk_ASN1_TYPE_value(sgxExtensions, i);
        crypto::validateOid(oids::SGX_EXTENSION, oidTupleWrapper, V_ASN1_SEQUENCE);

        const auto oidTuple = crypto::oidToStack(oidTupleWrapper);
        const auto oidTupleEntries = sk_ASN1_TYPE_num(oidTuple.get());
        if (oidTupleEntries != 2) {
            std::string err = "OID tuple [" + oids::SGX_EXTENSION + "] expected number of elements is [2] given [" +
                              std::to_string(oidTupleEntries) + "]";
            throw InvalidExtensionException(err);
        }

        const auto oidName = sk_ASN1_TYPE_value(oidTuple.get(), 0);
        const auto oidValue = sk_ASN1_TYPE_value(oidTuple.get(), 1);
        const auto oidNameStr = obj2Str(oidName->value.object);

        if (oidNameStr == oids::PLATFORM_INSTANCE_ID) {
            crypto::validateOid(oids::PLATFORM_INSTANCE_ID, oidValue, V_ASN1_OCTET_STRING,
                                constants::PLATFORM_INSTANCE_ID_LEN);
            _platformInstanceId = crypto::oidToBytes(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(),
                                                 Extension::Type::PLATFORM_INSTANCE_ID), expectedExtensions.end());
        } else if (oidNameStr == oids::CONFIGURATION) {
            _configuration = Configuration(oidValue);
            expectedExtensions.erase(
                    std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::CONFIGURATION),
                    expectedExtensions.end());
        }
    }

    if (!expectedExtensions.empty())
    {
        std::string err = "Required PCK SGX extensions not found. Missing [";

        // Convert all but the last element to avoid a trailing ","
        std::for_each(expectedExtensions.begin(), expectedExtensions.end() - 1, [&err](const auto& extension) {
            err += oids::type2Description(extension) + ", ";
        });

        // Now add the last element with no delimiter
        err += oids::type2Description(expectedExtensions.back()) + "]";

        throw InvalidExtensionException(err);
    }
}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

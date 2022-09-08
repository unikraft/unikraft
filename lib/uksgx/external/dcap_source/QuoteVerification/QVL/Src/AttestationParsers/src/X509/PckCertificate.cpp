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

#include "ParserUtils.h"
#include "X509Constants.h"
#include "OpensslHelpers/OidUtils.h"

#include <algorithm> // find_if

#include <openssl/asn1.h>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

PckCertificate::PckCertificate(const Certificate& certificate): Certificate(certificate)
{
    auto sgxExtensions = crypto::make_unique(getSgxExtensions());
    setMembers(sgxExtensions.get());
}

const std::vector<uint8_t>& PckCertificate::getPpid() const
{
    return _ppid;
}

const std::vector<uint8_t>& PckCertificate::getPceId() const
{
    return _pceId;
}

const std::vector<uint8_t>& PckCertificate::getFmspc() const
{
    return _fmspc;
}

SgxType PckCertificate::getSgxType() const
{
    return _sgxType;
}

const Tcb& PckCertificate::getTcb() const
{
    return _tcb;
}

bool PckCertificate::operator==(const PckCertificate& other) const
{
    return Certificate::operator==(other) &&
            _ppid == other._ppid &&
           _pceId == other._pceId &&
           _fmspc == other._fmspc &&
           _sgxType == other._sgxType &&
           _tcb == other._tcb;
}

PckCertificate PckCertificate::parse(const std::string& pem)
{
    return PckCertificate(pem);
}

// Private

PckCertificate::PckCertificate(const std::string& pem): Certificate(pem)
{
    auto sgxExtensions = crypto::make_unique(getSgxExtensions());
    setMembers(sgxExtensions.get());
}

stack_st_ASN1_TYPE* PckCertificate::getSgxExtensions()
{
    const auto sgxExtension = std::find_if(_extensions.begin(), _extensions.end(),
                                           [](const Extension &ext) { return ext.getNid() == NID_undef && ext.getName() == oids::SGX_EXTENSION; });

    if(sgxExtension == _extensions.end())
    {
        // Certificate has no SGX extensions, probably Root CA or Intermediate CA
        throw InvalidExtensionException("Certificate is missing SGX Extensions OID[" + oids::SGX_EXTENSION + "]");
    }

    // WARNING! Using this temporary pointer is mandatory!
    const auto *dtPtr = sgxExtension->getValue().data();
    const auto topSequence = crypto::make_unique(
            d2i_ASN1_TYPE(nullptr, &dtPtr, static_cast<long>(sgxExtension->getValue().size())));

    if(!topSequence)
    {
        throw InvalidExtensionException("d2i_ASN1_TYPE cannot parse data to correct type " + getLastError());
    }

    crypto::validateOid(oids::SGX_EXTENSION, topSequence.get(), V_ASN1_SEQUENCE);

    return crypto::oidToStack(topSequence.get()).release();
}

void PckCertificate::setMembers(stack_st_ASN1_TYPE *sgxExtensions)
{
    const auto stackEntries = sk_ASN1_TYPE_num(sgxExtensions);
    if(stackEntries != PROCESSOR_CA_EXTENSION_COUNT && stackEntries != PLATFORM_CA_EXTENSION_COUNT)
    {
        std::string err = "OID [" + oids::SGX_EXTENSION + "] expected to contain [" +
                std::to_string(PROCESSOR_CA_EXTENSION_COUNT) + "] or [" + std::to_string(PLATFORM_CA_EXTENSION_COUNT) +
                "] elements when given [" + std::to_string(stackEntries) + "]";
        throw InvalidExtensionException(err);
    }

    std::vector<Extension::Type> expectedExtensions = constants::PCK_REQUIRED_SGX_EXTENSIONS;

    // Iterate through SGX Extensions stored as sequence(tuple) of OIDName and OIDValue
    for (int i=0; i < stackEntries; i++)
    {
        const auto oidTupleWrapper = sk_ASN1_TYPE_value(sgxExtensions, i);
        crypto::validateOid(oids::SGX_EXTENSION, oidTupleWrapper, V_ASN1_SEQUENCE);

        const auto oidTuple = crypto::oidToStack(oidTupleWrapper);
        const auto oidTupleEntries = sk_ASN1_TYPE_num(oidTuple.get());
        if (oidTupleEntries != 2)
        {
            std::string err = "OID tuple [" + oids::SGX_EXTENSION + "] expected number of elements is [2] given [" +
                               std::to_string(oidTupleEntries) + "]";
            throw InvalidExtensionException(err);
        }

        const auto oidName = sk_ASN1_TYPE_value(oidTuple.get(), 0);
        const auto oidValue = sk_ASN1_TYPE_value(oidTuple.get(), 1);
        const auto oidNameStr = obj2Str(oidName->value.object);

        if (oidNameStr == oids::PPID) // TODO DRY
        {
            crypto::validateOid(oids::PPID, oidValue, V_ASN1_OCTET_STRING, constants::PPID_BYTE_LEN);
            _ppid = crypto::oidToBytes(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::PPID), expectedExtensions.end());
        }
        else if (oidNameStr == oids::TCB)
        {
            _tcb = Tcb(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::TCB), expectedExtensions.end());
        }
        else if (oidNameStr == oids::PCEID)
        {
            crypto::validateOid(oids::PCEID, oidValue, V_ASN1_OCTET_STRING, constants::PCEID_BYTE_LEN);
            _pceId = crypto::oidToBytes(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::PCEID), expectedExtensions.end());
        }
        else if (oidNameStr == oids::FMSPC)
        {
            crypto::validateOid(oids::FMSPC, oidValue, V_ASN1_OCTET_STRING, constants::FMSPC_BYTE_LEN);
            _fmspc = crypto::oidToBytes(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::FMSPC), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TYPE)
        {
            crypto::validateOid(oids::SGX_TYPE, oidValue, V_ASN1_ENUMERATED);
            _sgxType = static_cast<SgxType>(crypto::oidToEnum(oidValue));
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TYPE), expectedExtensions.end());
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

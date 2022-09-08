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

#include "X509Constants.h"
#include "ParserUtils.h"
#include "OpensslHelpers/OpensslTypes.h"
#include "OpensslHelpers/OidUtils.h"

#include <openssl/asn1.h>

#include <algorithm>


namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

Tcb::Tcb(const std::vector<uint8_t>& cpusvn,
         const std::vector<uint8_t>& cpusvnComponents,
         uint32_t pcesvn):
                              _cpuSvn(cpusvn),
                              _cpuSvnComponents(cpusvnComponents),
                              _pceSvn(pcesvn)
{}

uint32_t Tcb::getSgxTcbComponentSvn(uint32_t componentNumber) const
{
    return _cpuSvnComponents.at(componentNumber);
}

const std::vector<uint8_t>& Tcb::getSgxTcbComponents() const
{
    return _cpuSvnComponents;
}

const std::vector<uint8_t> &Tcb::getCpuSvn() const
{
    return _cpuSvn;
}

uint32_t Tcb::getPceSvn() const
{
    return _pceSvn;
}

bool Tcb::operator ==(const Tcb& other) const
{
    return _cpuSvnComponents == other._cpuSvnComponents &&
           _cpuSvn == other._cpuSvn &&
           _pceSvn == other._pceSvn;
}

// Private

Tcb::Tcb(const ASN1_TYPE *tcbSeq)
{
    _pceSvn = 0;
    crypto::validateOid(oids::TCB, tcbSeq, V_ASN1_SEQUENCE);

    const auto stack = crypto::oidToStack(tcbSeq);
    const auto stackEntries = sk_ASN1_TYPE_num(stack.get());
    if(stackEntries != constants::TCB_SEQUENCE_LEN)
    {
        std::string err = "TCB length expected [" + std::to_string(constants::TCB_SEQUENCE_LEN) + "] given [" +
                          std::to_string(stackEntries) + "]";
        throw InvalidExtensionException(err);
    }

    std::vector<Extension::Type> expectedExtensions = constants::TCB_REQUIRED_SGX_EXTENSIONS;
    _cpuSvnComponents = std::vector<uint8_t>(constants::CPUSVN_BYTE_LEN);

    // Iterate through SGX TCB Extensions stored as sequence(tuple) of OIDName and OIDValue
    for (int i=0; i < stackEntries; i++)
    {
        const auto oidTupleWrapper = sk_ASN1_TYPE_value(stack.get(), i);
        crypto::validateOid(oids::TCB, oidTupleWrapper, V_ASN1_SEQUENCE);

        const auto oidTuple = crypto::oidToStack(oidTupleWrapper);
        const auto oidTupleEntries = sk_ASN1_TYPE_num(oidTuple.get());
        if (oidTupleEntries != 2)
        {
            std::string err = "OID tuple [" + oids::TCB + "] expected number of elements is [2] given [" +
                              std::to_string(oidTupleEntries) + "]";
            throw InvalidExtensionException(err);
        }

        const auto oidName = sk_ASN1_TYPE_value(oidTuple.get(), 0);
        const auto oidValue = sk_ASN1_TYPE_value(oidTuple.get(), 1);
        const auto oidNameStr = obj2Str(oidName->value.object);

        if (oidNameStr == oids::SGX_TCB_COMP01_SVN) // TODO DRY
        {
            crypto::validateOid(oids::SGX_TCB_COMP01_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[0] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP01_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP02_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP02_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[1] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP02_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP03_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP03_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[2] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP03_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP04_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP04_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[3] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP04_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP05_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP05_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[4] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP05_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP06_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP06_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[5] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP06_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP07_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP07_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[6] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP07_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP08_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP08_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[7] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP08_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP09_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP09_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[8] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP09_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP10_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP10_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[9] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP10_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP11_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP11_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[10] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP11_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP12_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP12_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[11] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP12_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP13_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP13_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[12] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP13_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP14_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP14_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[13] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP14_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP15_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP15_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[14] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP15_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SGX_TCB_COMP16_SVN)
        {
            crypto::validateOid(oids::SGX_TCB_COMP16_SVN, oidValue, V_ASN1_INTEGER);
            _cpuSvnComponents[15] = crypto::oidToByte(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SGX_TCB_COMP16_SVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::PCESVN)
        {
            crypto::validateOid(oids::PCESVN, oidValue, V_ASN1_INTEGER);
            _pceSvn = crypto::oidToUInt(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::PCESVN), expectedExtensions.end());
        }
        else if (oidNameStr == oids::CPUSVN)
        {
            crypto::validateOid(oids::CPUSVN, oidValue, V_ASN1_OCTET_STRING, constants::CPUSVN_BYTE_LEN);
            _cpuSvn = crypto::oidToBytes(oidValue);
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::CPUSVN), expectedExtensions.end());
        }
    }

    if (!expectedExtensions.empty())
    {
        std::string err = "Required TCB SGX extensions not found. Missing [";

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

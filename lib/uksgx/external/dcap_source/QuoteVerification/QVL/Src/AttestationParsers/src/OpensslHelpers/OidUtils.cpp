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

#include "OidUtils.h"

#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "ParserUtils.h"

#include <openssl/asn1.h>

#include <vector>
#include <algorithm>

namespace intel { namespace sgx { namespace dcap { namespace crypto {

void validateOid(const std::string& oidName, const ASN1_TYPE *oidValue, int expectedType)
{
    if (oidValue->type != expectedType)
    {
        std::string err = "OID [" + oidName + "] type expected [" + std::to_string(expectedType) +
                          "] given [" + std::to_string(oidValue->type) + "]";
        throw parser::FormatException(err);
    }
}

void validateOid(const std::string& oidName, const ASN1_TYPE *oidValue, int expectedType, int expectedLength)
{
    validateOid(oidName, oidValue, expectedType);

    int oidValueLen;
    switch (oidValue->type)
    {
        case V_ASN1_INTEGER:
            oidValueLen = oidValue->value.integer->length;
            break;
        case V_ASN1_OCTET_STRING:
            oidValueLen = oidValue->value.octet_string->length;
            break;
        case V_ASN1_ENUMERATED:
            oidValueLen = oidValue->value.enumerated->length;
            break;
        case V_ASN1_SEQUENCE:
            oidValueLen = oidValue->value.sequence->length;
            break;
        default:
        {
            std::string err = "Unsupported OID [" + oidName + "] type [" + std::to_string(oidValue->type) + "]";
            throw parser::FormatException(err);
        }
    }
    if (oidValueLen != expectedLength)
    {
        std::string err = "OID [" + oidName + "] length expected [" + std::to_string(expectedLength) + "] given [" +
                          std::to_string(oidValueLen) + "]";
        throw parser::FormatException(err);
    }
}

std::vector<uint8_t> oidToBytes(const ASN1_TYPE *oidValue)
{
    const auto oidValueLen = oidValue->value.octet_string->length;
    auto bytes = std::vector<uint8_t>(static_cast<size_t>(oidValueLen));
    std::copy_n(oidValue->value.octet_string->data,
                oidValueLen,
                bytes.begin());
    return bytes;
}

uint8_t oidToByte(const ASN1_TYPE *oidValue)
{
    long ret = ASN1_INTEGER_get(oidValue->value.integer);
    return static_cast<uint8_t>(ret);
}

uint32_t oidToUInt(const ASN1_TYPE *oidValue)
{
    long ret = ASN1_INTEGER_get(oidValue->value.integer);
    return static_cast<uint32_t>(ret);
}

int oidToEnum(const ASN1_TYPE *oidValue)
{
    long ret = ASN1_ENUMERATED_get(oidValue->value.enumerated);
    return static_cast<int>(ret);
}

STACK_OF_ASN1TYPE_uptr oidToStack(const ASN1_TYPE *oidValue)
{
    const unsigned char *data = oidValue->value.sequence->data;
    auto stack = crypto::make_unique(d2i_ASN1_SEQUENCE_ANY(nullptr, &data, oidValue->value.sequence->length));

    if(!stack)
    {
        throw parser::FormatException("d2i_ASN1_SEQUENCE_ANY failed " + parser::getLastError());
    }

    return stack;
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {

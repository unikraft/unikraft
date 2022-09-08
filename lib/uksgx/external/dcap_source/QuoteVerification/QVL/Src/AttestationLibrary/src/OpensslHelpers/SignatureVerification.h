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

#ifndef SGXECDSAATTESTATION_SIGNATUREVERIFICATION_H_
#define SGXECDSAATTESTATION_SIGNATUREVERIFICATION_H_

#include <SgxEcdsaAttestation/AttestationParsers.h>

#include "OpensslHelpers/Bytes.h"
#include "OpensslHelpers/OpensslTypes.h"

#include <PckParser/CrlStore.h>
#include <QuoteVerification/QuoteConstants.h>

namespace intel { namespace sgx { namespace dcap { namespace crypto {

std::vector<uint8_t> rawEcdsaSignatureToDER(const std::array<uint8_t,constants::ECDSA_P256_SIGNATURE_BYTE_LEN>& sig);

bool verifySignature(const pckparser::CrlStore& crl, const std::vector<uint8_t>& publicKey);

bool verifySha256Signature(const Bytes& signature, const Bytes& message, const EC_KEY& publicKey);
bool verifySha256Signature(const Bytes& signature, const Bytes& message, const EVP_PKEY& publicKey);

template<size_t N>
bool verifySha256Signature(const Bytes& signature, const std::array<uint8_t,N>& message, const EC_KEY& publicKey)
{
    const std::vector<uint8_t> msg(std::begin(message), std::end(message));
    return verifySha256Signature(signature, msg, publicKey);
}

template<size_t N>
bool verifySha256EcdsaSignature(const std::array<uint8_t, constants::ECDSA_P256_SIGNATURE_BYTE_LEN> &signature,
                                const std::array<uint8_t, N> &message, const EC_KEY &publicKey)
{
    return verifySha256Signature(rawEcdsaSignatureToDER(signature), message, publicKey);
}

bool verifySha256EcdsaSignature(const std::array<uint8_t, constants::ECDSA_P256_SIGNATURE_BYTE_LEN> &signature,
                                const std::vector<uint8_t> &message, const EC_KEY &publicKey);

bool verifySha256EcdsaSignature(const Bytes &signature, const std::vector<uint8_t> &message, const EC_KEY &publicKey);

bool verifySha256EcdsaSignature(const dcap::parser::x509::Signature &signature, const std::vector<uint8_t> &message, const std::vector<uint8_t> &publicKey);

}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {


#endif //SGXECDSAATTESTATION_SIGNATUREVERIFICATION_H_

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

#include "EcdsaSignatureGenerator.h"

#include <algorithm>
#include <array>

static const unsigned ECDSA_SIG_R_LENGTH_BYTES = 32;
static const unsigned ECDSA_SIG_S_LENGTH_BYTES = 32;
static const unsigned RAW_ECDSA_SIG_LENGTH_BYTES = ECDSA_SIG_R_LENGTH_BYTES + ECDSA_SIG_S_LENGTH_BYTES;

static const int MAX_COMPONENT_LENGTH_BYTES = 33;     // maxLength of ASN1 ECDSA signature R and S components     (when there is a leading 0x00 byte)
static const int MIN_COMPONENT_LENGTH_BYTES = 1;      // minLength of ASN1 ECDSA signature R and S components     (when value is equal to 0)
static const int MAX_SEQUENCE_LENGTH_BYTES = 70;      // maxLength of ASN1 ECDSA signature sequence (1 + 1 + 33 + 2 + 33 = 70 calculated from: 0x02 b2 max(vr) 0x02 b3 max(vs))
static const int MIN_SEQUENCE_LENGTH_BYTES = 6;       // minLength of ASN1 ECDSA signature sequence = 4 (1 + 1 + 1 + 2 + 1 = 6 calculated from: 0x02 b2 min(vr) 0x02 b3 min(vs)

static const uint8_t ASN1_TAG_SEQUENCE = 0x30;
static const uint8_t ASN1_TAG_INTEGER = 0x02;

Bytes EcdsaSignatureGenerator::signECDSA_SHA256(const Bytes& message, EVP_PKEY* prvKey)
{
    auto signature = Bytes{};
    const int pkeyType = EVP_PKEY_base_id(prvKey);
    if (pkeyType != EVP_PKEY_EC)
    {
        return signature;
    }

    crypto::EVP_PKEY_CTX_uptr ctx = crypto::make_unique(EVP_PKEY_CTX_new(prvKey, nullptr));
    if (!ctx)
    {
        return signature;
    }

    if (EVP_PKEY_sign_init(ctx.get()) != 1)
    {
        return signature;
    }

    Bytes digest(SHA256_DIGEST_LENGTH, 0x0);
    if (nullptr == SHA256(message.data(), message.size(), digest.data()))
    {
        return signature;
    }

    size_t maxSigLen = 0;
    if (EVP_PKEY_sign(ctx.get(), nullptr, &maxSigLen, digest.data(), digest.size()) != 1)
    {
        return signature;
    }

    Bytes finalSignature(maxSigLen, 0);
    size_t sigLen = maxSigLen;
    if (EVP_PKEY_sign(ctx.get(), finalSignature.data(), &sigLen, digest.data(), digest.size()) != 1)
    {
        return signature;
    }
    // ECDSA signature returned is ANS.1 encoded
    // To be consistent with IPP we simply decode it to RAW format
    return convertECDSASignatureToRaw(finalSignature);
}

std::string EcdsaSignatureGenerator::signatureToHexString(const Bytes &signature)
{
    std::string result;
    result.reserve(signature.size() * 2);   // two digits per character

    static constexpr char hex[] = "0123456789ABCDEF";

    for (uint8_t c : signature)
    {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

namespace iter
{
    uint8_t read(std::vector<uint8_t>::iterator &it)
    {
        uint8_t value = (*it);
        it++;
        return value;
    }
    void skip(std::vector<uint8_t>::iterator &it, int toSkip)
    {
        for(auto i = 0; i < toSkip; ++i)
        {
            read(it);
        }
    }
}
Bytes EcdsaSignatureGenerator::convertECDSASignatureToRaw(Bytes &ecdsaSig)
{
    /*
     When ecdsaSig is encoded in DER, this becomes the following sequence of bytes:
     0x30 b1 0x02 b2 (vr) 0x02 b3 (vs)
     where:
        b1 is a single byte value, equal to the length, in bytes, of the remaining list of bytes (from the first 0x02 to the end of the encoding);
        b2 is a single byte value, equal to the length, in bytes, of (vr);
        b3 is a single byte value, equal to the length, in bytes, of (vs);
        (vr) is the signed big-endian encoding of the value "r", of minimal length;
        (vs) is the signed big-endian encoding of the value "s", of minimal length.
    */
    Bytes invalidSignature{};

    auto sigIter = ecdsaSig.begin();
    if (iter::read(sigIter) != ASN1_TAG_SEQUENCE)
    {
        return invalidSignature;
    }

    const uint8_t sequenceSize = iter::read(sigIter); /* get b1 */
    if (sequenceSize < MIN_SEQUENCE_LENGTH_BYTES || sequenceSize > MAX_SEQUENCE_LENGTH_BYTES)
    {
        return invalidSignature;
    }

    if (iter::read(sigIter) != ASN1_TAG_INTEGER)
    {
        return invalidSignature;
    }

    /* reading r component */
    Bytes r{};
    r.reserve(ECDSA_SIG_R_LENGTH_BYTES);
    const uint8_t rSize = iter::read(sigIter); /* get b2 */
    if (rSize < MIN_COMPONENT_LENGTH_BYTES || rSize > MAX_COMPONENT_LENGTH_BYTES)
    {
        return invalidSignature;
    }

    int offset = ECDSA_SIG_R_LENGTH_BYTES - rSize; /* offset is the number of zeroes which will be prepended to r buffer in case when r component is shorter than 32 bytes*/
    uint32_t bytesToRead; /* number of component bytes which will be read ( =< 32)*/

    if (offset > 0)
    {
        bytesToRead = rSize;
    }
    else
    {
        bytesToRead = ECDSA_SIG_R_LENGTH_BYTES;
        offset = 0; /* bytesToRead == 32, so there is no need to prepend zeroes */

        iter::skip(sigIter, rSize - ECDSA_SIG_R_LENGTH_BYTES); /* skipping the leading zero */
    }
    for (auto i = 0; i < offset; ++i)
    {
        r.push_back(0);
    }
    for (uint32_t i = 0; i < bytesToRead; ++i)
    {
        r.push_back(iter::read(sigIter));
    }

    /* reading s component */
    Bytes s{};
    s.reserve(ECDSA_SIG_S_LENGTH_BYTES);
    if (iter::read(sigIter) != ASN1_TAG_INTEGER)
    {
        return invalidSignature;
    }
    const uint8_t sSize = iter::read(sigIter); /* get b3 */
    if (sSize < MIN_COMPONENT_LENGTH_BYTES || sSize > MAX_COMPONENT_LENGTH_BYTES)
    {
        return invalidSignature;
    }

    offset = ECDSA_SIG_S_LENGTH_BYTES - sSize; /* offset is the number of zeroes which will be prepended to s buffer in case when s component is shorter than 32 bytes*/
    if (offset > 0)
    {
        bytesToRead = sSize;
    }
    else
    {
        bytesToRead = ECDSA_SIG_S_LENGTH_BYTES;
        iter::skip(sigIter, sSize - ECDSA_SIG_S_LENGTH_BYTES); /* skipping the leading zero */
        offset = 0; /* bytesToRead == 32, so there is no need to prepend zeroes*/
    }
    for (auto i = 0; i < offset; ++i)
    {
        s.push_back(0);
    }
    for (uint32_t i = 0; i < bytesToRead; ++i)
    {
        s.push_back(iter::read(sigIter));
    }

    /* writing r and s components to output buffer */
    Bytes rawSignature{};
    rawSignature.reserve(RAW_ECDSA_SIG_LENGTH_BYTES);

    rawSignature.insert(rawSignature.end(), r.begin(), r.end());
    rawSignature.insert(rawSignature.end(), s.begin(), s.end());

    if (rawSignature.size() != RAW_ECDSA_SIG_LENGTH_BYTES)
    {
        return invalidSignature;
    }

    return rawSignature;
}

std::array<uint8_t, 64> EcdsaSignatureGenerator::convertECDSASignatureToRawArray(Bytes& ecdsaSig)
{
    auto rawSig = EcdsaSignatureGenerator::convertECDSASignatureToRaw(ecdsaSig);
    std::array<uint8_t, 64> signatureArr{};
    std::copy_n(rawSig.begin(), signatureArr.size(), signatureArr.begin());
    return signatureArr;
}

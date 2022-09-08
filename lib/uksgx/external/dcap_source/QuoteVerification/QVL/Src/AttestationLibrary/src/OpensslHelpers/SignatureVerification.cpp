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

#include <algorithm>
#include "SignatureVerification.h"
#include "KeyUtils.h"

namespace intel { namespace sgx { namespace dcap { namespace crypto {

bool verifySignature(const pckparser::CrlStore& crl, const std::vector<uint8_t>& pubKey)
{
    auto publicKey = crypto::rawToP256PubKey(pubKey);
    if (publicKey == nullptr)
    {
        return false;
    }
    const auto evp = crypto::toEvp(*publicKey);
    if(!evp)
    {
        return false;
    }
    return 1 == X509_CRL_verify(&const_cast<X509_CRL&>(crl.getCrl()), evp.get());
}

bool verifySha256Signature(const Bytes& signature, const Bytes& msg, const EC_KEY& pubKey)
{
    const auto evp = crypto::toEvp(pubKey);
    if(!evp)
    {
        return false;
    }
    return verifySha256Signature(signature, msg, *evp);
}

bool verifySha256Signature(const Bytes& signature, const Bytes& message, const EVP_PKEY& pubKey)
{
    auto ctx = crypto::make_unique(EVP_MD_CTX_new());
    if (!ctx)
    {
        return false;
    }

    return (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, &const_cast<EVP_PKEY&>(pubKey)) == 1)
        && (EVP_DigestVerifyUpdate(ctx.get(), message.data(), message.size()) == 1)
        && (EVP_DigestVerifyFinal(ctx.get(), signature.data(), signature.size()) == 1);
}

std::vector<uint8_t> rawEcdsaSignatureToDER(const std::array<uint8_t,constants::ECDSA_P256_SIGNATURE_BYTE_LEN>& sig)
{ 
    auto bnR = crypto::make_unique(BN_new());
    auto bnS = crypto::make_unique(BN_new());

    BN_bin2bn(sig.data(), 32, bnR.get());
    BN_bin2bn(sig.data() + 32, 32, bnS.get());

    auto ecdsaSig = crypto::make_unique(ECDSA_SIG_new());
    if(1 != ECDSA_SIG_set0(ecdsaSig.get(), bnR.release(), bnS.release()))
    {
        return {};
    }

    const auto expectedSize = i2d_ECDSA_SIG(ecdsaSig.get(), nullptr);
    if(0 >= expectedSize)
    {
        return {};
    }

    std::vector<uint8_t> derSig(static_cast<size_t>(expectedSize));
    auto it = derSig.data();
    if(0 == i2d_ECDSA_SIG(ecdsaSig.get(), &it))
    {
        return {};
    }

    return derSig;
}

bool verifySha256EcdsaSignature(const std::array<uint8_t, constants::ECDSA_P256_SIGNATURE_BYTE_LEN> &signature,
                                const std::vector<uint8_t> &message, const EC_KEY &publicKey)
{
    const std::vector<uint8_t> sig = rawEcdsaSignatureToDER(signature);
    return verifySha256Signature(sig, message, publicKey);
}

bool verifySha256EcdsaSignature(const Bytes &signature, const std::vector<uint8_t> &message, const EC_KEY &publicKey)
{
    if(signature.size() != constants::ECDSA_P256_SIGNATURE_BYTE_LEN)
    {
        return false;
    }
    std::array<uint8_t, constants::ECDSA_P256_SIGNATURE_BYTE_LEN> signatureArr{};
    std::copy_n(signature.begin(), constants::ECDSA_P256_SIGNATURE_BYTE_LEN, signatureArr.begin());
    return verifySha256EcdsaSignature(signatureArr, message, publicKey);
}

bool verifySha256EcdsaSignature(const dcap::parser::x509::Signature &signature, const std::vector<uint8_t> &message, const std::vector<uint8_t> &publicKey)
{
    auto pubKey = rawToP256PubKey(publicKey);
    if (pubKey == nullptr)
    {
        return false;
    }
    return verifySha256Signature(signature.getRawDer(), message, *pubKey);
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {

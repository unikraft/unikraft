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

#include "DigestUtils.h"

#include <OpensslHelpers/OpensslTypes.h>

#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include <cstring>
#include <algorithm>

namespace intel{
namespace sgx{
namespace dcap{
namespace DigestUtils{


std::array<uint8_t,32> sha256DigestArray(const Bytes& data)
{
    const auto vec = sha256Digest(data);
    std::array<uint8_t,32> ret;
    std::copy_n(vec.begin(), 32, ret.begin());
    return ret;
}

Bytes sha256Digest(const Bytes& data)
{
    Bytes hash(SHA256_DIGEST_LENGTH);
    SHA256_CTX ctx;
    if(SHA256_Init(&ctx) == 1 &&
        SHA256_Update(&ctx, data.data(), data.size()) == 1 &&
        SHA256_Final(hash.data(), &ctx) == 1)
    {
        return hash;
    }
    else
    {
        return Bytes{};
    }
}

Bytes sha256Digest(const std::string& data)
{
    Bytes input{data.begin(), data.end()};
    return sha256Digest(input);
}

Bytes sha384Digest(const Bytes& data)
{
    Bytes hash(SHA384_DIGEST_LENGTH);
    SHA512_CTX ctx;
    if(SHA384_Init(&ctx) == 1 &&
        SHA384_Update(&ctx, data.data(), data.size()) == 1 &&
        SHA384_Final(hash.data(), &ctx) == 1)
    {
        return hash;
    }
    else
    {
        return Bytes{};
    }
}

Bytes sha384Digest(const std::string& data)
{
    Bytes input{data.begin(), data.end()};
    return sha384Digest(input);
}

Bytes signMessageSha256(const Bytes& message, EVP_PKEY& privateKey)
{
    auto mdCtx = crypto::make_unique(EVP_MD_CTX_new());
    if(!mdCtx)
    {
        return Bytes{};
    }
    auto initStatus = EVP_DigestSignInit(mdCtx.get(), nullptr, EVP_sha256(), nullptr, &privateKey);
    if(initStatus != 1)
    {
        return Bytes{};
    }
    auto updateStatus = EVP_DigestSignUpdate(mdCtx.get(), message.data(), message.size());
    if(updateStatus != 1)
    {
        return Bytes{};
    }
    size_t sigLen = 0;
    auto signStatus = EVP_DigestSignFinal(mdCtx.get(), nullptr, &sigLen);
    
    Bytes signature;
    if(signStatus == 1)
    {
        Bytes tmp(sigLen);
        signStatus = EVP_DigestSignFinal(mdCtx.get(), tmp.data(), &sigLen);
        if(1 == signStatus)
        {
            // siglen maybe shorter at this point !!!
            signature.reserve(sigLen);
            std::copy_n(tmp.begin(), sigLen, std::back_inserter(signature));
        }
    }
    return signStatus == 1 ? signature : Bytes{};
}

bool verifySig(const Bytes& signature, const Bytes& message, EC_KEY& pubKey)
{
    const auto digest = dcap::DigestUtils::sha256Digest(message);
    return 1 == ECDSA_verify(0, digest.data(), static_cast<int>(digest.size()), signature.data(), static_cast<int>(signature.size()), &pubKey);
}

}}}}

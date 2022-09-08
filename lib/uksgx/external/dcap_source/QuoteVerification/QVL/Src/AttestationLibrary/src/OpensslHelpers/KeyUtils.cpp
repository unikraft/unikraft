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

#include "KeyUtils.h"

#include <algorithm>
#include <array>

namespace intel { namespace sgx { namespace dcap { namespace crypto {

crypto::EVP_PKEY_uptr toEvp(const EC_KEY &ecKey)
{
    auto empty = crypto::make_unique<EVP_PKEY>(nullptr);

    auto copy = crypto::make_unique(EC_KEY_dup(&ecKey));
    if(!copy)
    {
        return empty;
    }

    crypto::EVP_PKEY_uptr evpKey = crypto::make_unique<EVP_PKEY>(EVP_PKEY_new());
    if (!evpKey)
    {
        return empty;
    }

    if (1 != EVP_PKEY_set1_EC_KEY(evpKey.get(), copy.get()))
    {
        return empty;
    }

    return evpKey;
}

crypto::EC_KEY_uptr rawToP256PubKey(const std::array<uint8_t, 64>& rawKey)
{
    const auto group = crypto::make_unique(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    auto bnX = crypto::make_unique(BN_new());
    auto bnY = crypto::make_unique(BN_new());
    BN_bin2bn(rawKey.data(), 32, bnX.get());
    BN_bin2bn((rawKey.data() + 32), 32, bnY.get());

    auto empty = crypto::make_unique<EC_KEY>(nullptr);
    auto point = crypto::make_unique(EC_POINT_new(group.get()));
    if(1 != EC_POINT_set_affine_coordinates_GFp(group.get(), point.get(), bnX.get(), bnY.get(), nullptr))
    {
        return empty;
    }
   
    auto ret = crypto::make_unique(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    if(1 != EC_KEY_set_public_key(ret.get(), point.get()))
    {
        return empty;
    }

    return ret;
}

crypto::EC_KEY_uptr rawToP256PubKey(const std::vector<uint8_t>& rawKey)
{
    std::array<uint8_t, 64> raw{};
    std::copy_n(rawKey.begin() + 1, raw.size(), raw.begin()); // skip header byte

    return rawToP256PubKey(raw);
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {

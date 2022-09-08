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

#ifndef SGX_DCAP_COMMONS_BYTES_H
#define SGX_DCAP_COMMONS_BYTES_H

#include <vector>
#include <stdexcept>
#include <ctype.h>
#include <string>
#include <algorithm>
#include <iterator>

namespace intel { namespace sgx { namespace dcap {

using Bytes = std::vector<uint8_t>;

namespace detail{
inline uint8_t asciiToValue(const char in)
{
    if (::isxdigit(static_cast<unsigned char>(in)))
    {
        if(in >= '0' && in <= '9')
        {
            return static_cast<uint8_t>(in - '0');
        }
        if(in >= 'A' && in <= 'F')
        {
            return static_cast<uint8_t>(in - 'A' + 10);
        }
        if(in >= 'a' && in <= 'f')
        {
             return static_cast<uint8_t>(in - 'a' + 10);
        }
    }
    throw std::invalid_argument("Invalid hex character");
}
}//namespace detail

inline Bytes operator+(const Bytes& lhs, const Bytes& rhs)
{
    Bytes retVal{lhs};
    retVal.insert(retVal.end(), rhs.cbegin(), rhs.cend());
    return retVal;
}

inline Bytes hexStringToBytes(const std::string& hexEncoded)
{
    try{
        if (hexEncoded.length() % 2 == 1) {
            return {};
        }

        auto pos = hexEncoded.cbegin();
        Bytes outBuffer;
        outBuffer.reserve(hexEncoded.length() / 2);

        while (pos < hexEncoded.cend())
        {
            outBuffer.push_back(static_cast<uint8_t>(detail::asciiToValue(*(pos + 1)) + (detail::asciiToValue(*pos) << 4)));
            pos = std::next(pos, 2);
        }
        return outBuffer;
    }
    catch(const std::invalid_argument&)
    {
        return {};
    }
}

inline std::string bytesToHexString(const Bytes &vector)
{
    std::string result;
    result.reserve(vector.size() * 2);   // two digits per character

    static constexpr char hex[] = "0123456789ABCDEF";

    for (const uint8_t c : vector)
    {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

inline std::vector<uint8_t> applyMask(const std::vector<uint8_t>& base, const std::vector<uint8_t>& mask)
{
    std::vector<uint8_t> result;

    if(base.size() != mask.size())
    {
        return result;
    }

    auto mask_it = mask.cbegin();
    std::transform(base.cbegin(), base.cend(), std::back_inserter(result), [&mask_it](auto &base_it) {
        return (uint8_t)((base_it) & (*(mask_it++)));
    });

    return result;
}

}}}

#endif //SGX_DCAP_COMMONS_BYTES_H

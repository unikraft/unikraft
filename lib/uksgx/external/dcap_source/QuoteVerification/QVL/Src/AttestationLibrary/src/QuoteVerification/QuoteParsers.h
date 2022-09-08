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

#ifndef INTEL_SGX_QVL_QUOTEPARSERS_H_
#define INTEL_SGX_QVL_QUOTEPARSERS_H_

#include <vector>
#include "QuoteConstants.h"
#include "ByteOperands.h"

namespace intel { namespace sgx { namespace dcap { namespace quote {

template<typename T>
inline bool copyAndAdvance(T &val, std::vector<uint8_t>::const_iterator &from, size_t amount,
                           const std::vector<uint8_t>::const_iterator &totalEnd) {
    const auto available = std::distance(from, totalEnd);
    if (available < 0 || (unsigned) available < amount) {
        return false;
    }
    const auto end = std::next(from, static_cast<long>(amount));
    return val.insert(from, end);
}

template<size_t N>
inline bool copyAndAdvance(std::array <uint8_t, N> &arr, std::vector<uint8_t>::const_iterator &from,
                           const std::vector<uint8_t>::const_iterator &totalEnd) {
    const auto capacity = std::distance(arr.cbegin(), arr.cend());
    if (std::distance(from, totalEnd) < capacity) {
        return false;
    }
    const auto end = std::next(from, capacity);
    std::copy(from, end, arr.begin());
    std::advance(from, capacity);
    return true;
}

inline bool copyAndAdvance(uint16_t &val, std::vector<uint8_t>::const_iterator &from,
                           const std::vector<uint8_t>::const_iterator &totalEnd) {
    const auto available = std::distance(from, totalEnd);
    const auto capacity = sizeof(uint16_t);
    if (available < 0 || (unsigned) available < capacity) {
        return false;
    }

    val = swapBytes(toUint16(*from, *(std::next(from))));
    std::advance(from, capacity);
    return true;
}


inline bool copyAndAdvance(uint32_t &val, std::vector<uint8_t>::const_iterator &position,
                           const std::vector<uint8_t>::const_iterator &totalEnd) {
    const auto available = std::distance(position, totalEnd);
    const auto capacity = sizeof(uint32_t);
    if (available < 0 || (unsigned) available < capacity) {
        return false;
    }

    val = swapBytes(toUint32(*position, *(std::next(position)), *(std::next(position, 2)),
                             *(std::next(position, 3))));
    std::advance(position, capacity);
    return true;
}

}}}}

#endif //INTEL_SGX_QVL_QUOTEPARSERS_H_

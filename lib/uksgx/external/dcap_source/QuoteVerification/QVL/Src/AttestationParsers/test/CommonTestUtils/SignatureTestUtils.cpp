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

#include "SignatureTestUtils.h"

namespace intel { namespace sgx { namespace dcap { namespace parser {

const std::vector<uint8_t> RAW_DER = { 0x30, 0x46, 0x02, 0x21, 0x00, 0xB7, 0x4B, 0xC2,
                                       0xB2, 0xA4, 0xB6, 0x5D, 0xC7, 0x31, 0x6B, 0x75,
                                       0x5C, 0x4C, 0x59, 0x9D, 0xB1, 0x80, 0xC2, 0x52,
                                       0x0F, 0x1A, 0xB1, 0x2E, 0xDD, 0x22, 0xE8, 0xF5,
                                       0x37, 0x2B, 0xFC, 0x55, 0xA7, 0x02, 0x21, 0x00,
                                       0xF6, 0x63, 0x98, 0x70, 0xBF, 0xC2, 0x5D, 0x1F,
                                       0x8A, 0xE9, 0x1F, 0x40, 0x76, 0x71, 0x28, 0x1C,
                                       0xF1, 0x9D, 0x80, 0x91, 0xDB, 0x82, 0x90, 0xD0,
                                       0x9A, 0x01, 0xE3, 0x40, 0x8B, 0x37, 0x85, 0x8C };
const std::vector<uint8_t> R = { 0xB7, 0x4B, 0xC2, 0xB2, 0xA4, 0xB6, 0x5D, 0xC7,
                                 0x31, 0x6B, 0x75, 0x5C, 0x4C, 0x59, 0x9D, 0xB1,
                                 0x80, 0xC2, 0x52, 0x0F, 0x1A, 0xB1, 0x2E, 0xDD,
                                 0x22, 0xE8, 0xF5, 0x37, 0x2B, 0xFC, 0x55, 0xA7 };
const std::vector<uint8_t> S = { 0xF6, 0x63, 0x98, 0x70, 0xBF, 0xC2, 0x5D, 0x1F,
                                 0x8A, 0xE9, 0x1F, 0x40, 0x76, 0x71, 0x28, 0x1C,
                                 0xF1, 0x9D, 0x80, 0x91, 0xDB, 0x82, 0x90, 0xD0,
                                 0x9A, 0x01, 0xE3, 0x40, 0x8B, 0x37, 0x85, 0x8C };

x509::Signature createSignature(const std::vector<uint8_t>& rawDer,
                                const std::vector<uint8_t>& r,
                                const std::vector<uint8_t>& s)
{
    return x509::Signature(rawDer, r, s);
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser {

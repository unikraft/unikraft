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

#include <QuoteVerification/ByteOperands.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace intel::sgx;
using namespace testing;

TEST(byteOperands, swapBytesShouldSuccessUint16)
{
    const std::array<uint16_t, 5> in        {{0x0011, 0xaaff, 0x1100, 0x0001, 0x1000}};
    const std::array<uint16_t, 5> expected  {{0x1100, 0xffaa, 0x0011, 0x0100, 0x0010}};

    for(size_t i=0; i<in.size(); ++i)
    {
        EXPECT_EQ(expected[i], dcap::swapBytes(in[i]));
    }
}

TEST(byteOperands, swapBytesShouldSuccessUint32)
{
    const std::array<uint32_t, 5> in        {{0x00110000, 0xaaffaaff, 0x11000000, 0x00000001, 0x10001100}};
    const std::array<uint32_t, 5> expected  {{0x00001100, 0xffaaffaa, 0x00000011, 0x01000000, 0x00110010}}; 

    for(size_t i=0; i<in.size(); ++i)
    {
        EXPECT_EQ(expected[i], dcap::swapBytes(in[i]));
    }
}

TEST(byteOperands, toUint16ShouldSuccess)
{
    using InputType = std::array<std::pair<uint8_t,uint8_t>, 3>;

    const InputType in                  {{ {0x00, 0xaa}, {0x11, 0x00}, {0xfa, 0xaf} }};
    const std::array<uint16_t, 3> expected {{ 0x00aa,       0x1100,       0xfaaf    }};

    for(size_t i=0; i<in.size(); ++i)
    {
        EXPECT_EQ(expected[i], dcap::toUint16(in[i].first, in[i].second));
    }
}

TEST(byteOperands, toUint32TwoArgShouldSuccess)
{
    using InputType = std::array<std::pair<uint16_t,uint16_t>, 3>;
    
    const InputType in                  {{ {0x0110, 0xffaa}, {0x0000, 0x1100}, {0x0011, 0x1100} }};
    const std::array<uint32_t, 3> expected {{ 0x0110ffaa,       0x00001100,       0x00111100    }};

    for(size_t i=0; i<in.size(); ++i)
    {
        EXPECT_EQ(expected[i], dcap::toUint32(in[i].first, in[i].second));
    }
}

TEST(byteOperands, toUint32FourArgShouldSuccess)
{ 
    using InputType = std::array<std::tuple<uint8_t,uint8_t,uint8_t,uint8_t>, 3>;

    const InputType in {{ std::make_tuple(0x01, 0xff, 0xaa, 0xbb), std::make_tuple(0x10, 0xff, 0xaa, 0x00), std::make_tuple(0x01, 0x00, 0x00, 0x10) }};
    const std::array<uint32_t, 3> expected {{   0x01ffaabb,                             0x10ffaa00,                                 0x01000010    }};

    for(size_t i=0; i<in.size(); ++i)
    {
        EXPECT_EQ(expected[i], dcap::toUint32(std::get<0>(in[i]), std::get<1>(in[i]), std::get<2>(in[i]), std::get<3>(in[i])));
    }
}

TEST(byteOperands, toArrayUint16)
{
    using InputType = std::array<uint16_t, 4>;
    using OutType = std::array<std::array<uint8_t,2>, 4>;

    const InputType in     {{      0xffaa,         0x0001,         0x1001,         0x1100    }};
    const OutType expected {{ {{0xff, 0xaa}}, {{0x00, 0x01}}, {{0x10, 0x01}}, {{0x11, 0x00}} }};
    
    for(size_t i=0; i<in.size(); ++i)
    {
        const auto actual = dcap::toArray(in[i]);
        const auto exp = expected[i];
        EXPECT_TRUE(std::equal(actual.begin(), actual.end(), exp.begin()));
    }
}

TEST(byteOperands, toArrayUint32)
{
    using InputType = std::array<uint32_t, 4>;
    using OutType = std::array<std::array<uint8_t,4>, 4>;

    const InputType in     {{         0xffaaffaa,              0x00000001,                    0x10000001,                  0x11000000    }};
    const OutType expected {{ {{0xff, 0xaa, 0xff, 0xaa}}, {{0x00, 0x00, 0x00, 0x01}}, {{0x10, 0x00, 0x00, 0x01}}, {{0x11, 0x00, 0x00, 0x00}} }};
    
    for(size_t i=0; i<in.size(); ++i)
    {
        const auto actual = dcap::toArray(in[i]);
        const auto exp = expected[i];
        EXPECT_TRUE(std::equal(actual.begin(), actual.end(), exp.begin()));
    }
}


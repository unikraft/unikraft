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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DigestUtils.h"

using namespace testing;
using namespace ::intel::sgx::dcap;

struct DigestUtilsTests : public Test
{
};

TEST_F(DigestUtilsTests, shouldCalculateSha256)
{
    Bytes input = {'S', 'o', 'm', 'e', ' ', 'd', 'a', 't', 'a', ' ', 't', 'o', ' ', '#'};
    Bytes expectedHash = hexStringToBytes("a01957460f7adb337a87fb070b5b1a8dce957082c3823c3510a30893e3e84094");

    EXPECT_EQ(DigestUtils::sha256Digest(input), expectedHash);
}

TEST_F(DigestUtilsTests, shouldCalculateSha256FromString)
{
    std::string input = "Some data to #";
    Bytes expectedHash = hexStringToBytes("a01957460f7adb337a87fb070b5b1a8dce957082c3823c3510a30893e3e84094");

    EXPECT_EQ(DigestUtils::sha256Digest(input), expectedHash);
}

TEST_F(DigestUtilsTests, shouldCalculateSha384)
{
    Bytes input = {'S', 'o', 'm', 'e', ' ', 'd', 'a', 't', 'a', ' ', 't', 'o', ' ', '#'};
    Bytes expectedHash = hexStringToBytes("cb826e9e9c491ddc144bd27c2f254865dfe150e08c1a118d89589e6fb597519a71c79652c3db1351d8338ded36fc7e38");

    EXPECT_EQ(DigestUtils::sha384Digest(input), expectedHash);
}

TEST_F(DigestUtilsTests, shouldCalculateSha384FromString)
{
    std::string input = "Some data to #";
    Bytes expectedHash = hexStringToBytes("cb826e9e9c491ddc144bd27c2f254865dfe150e08c1a118d89589e6fb597519a71c79652c3db1351d8338ded36fc7e38");

    EXPECT_EQ(DigestUtils::sha384Digest(input), expectedHash);
}

TEST_F(DigestUtilsTests, shouldSHA256VectorAndArrayVersionReturnSameResult)
{
    const std::vector<uint8_t> input(100, 0xff);

    const auto array = DigestUtils::sha256DigestArray(input);
    const auto vector = DigestUtils::sha256Digest(input);

    ASSERT_EQ(32, vector.size());
    const bool areEqual = std::equal(array.begin(), array.end(), vector.begin());

    EXPECT_TRUE(areEqual);
}

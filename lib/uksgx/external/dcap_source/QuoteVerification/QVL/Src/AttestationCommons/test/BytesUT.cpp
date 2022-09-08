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

#include "OpensslHelpers/Bytes.h"

#include <gtest/gtest.h>

using namespace intel::sgx::dcap;
using namespace ::testing;

struct BytesUT: public testing::Test {
    Bytes val1 = { 0x00, 0x01, 0x09, 0x0A, 0x0D, 0x0F, 0x14, 0x9F, 0xAF, 0xFF };
    Bytes val2 = { 0x00, 0x01, 0x09, 0x0A, 0x0D, 0x0F, 0x14, 0x9F, 0xAF, 0xFF,
                   0x00, 0x01, 0x09, 0x0A, 0x0D, 0x0F, 0x14, 0x9F, 0xAF, 0xFF };
};

TEST_F(BytesUT, concatenation)
{
    ASSERT_NE(val1, val2);
    ASSERT_EQ(val1 + val1, val2);
}

TEST_F(BytesUT, hexStringToBytes)
{
    ASSERT_EQ(intel::sgx::dcap::hexStringToBytes("0001090A0D0f149FAFFf"), val1);
    ASSERT_NE(intel::sgx::dcap::hexStringToBytes("0001090A0D0f149FAFFf"), val2);
}


TEST_F(BytesUT, bytesToHexString)
{
    ASSERT_EQ(intel::sgx::dcap::bytesToHexString(val1), "0001090A0D0F149FAFFF");
    ASSERT_NE(intel::sgx::dcap::bytesToHexString(val2), "0001090A0D0F149FAFFF");
}

TEST_F(BytesUT, OddLengthHexStringToBytes)
{
    ASSERT_EQ(intel::sgx::dcap::hexStringToBytes("0001090A0D0f149FAFF"), Bytes {});
}

TEST_F(BytesUT, NotValidHexStringToBytes)
{
    ASSERT_EQ(intel::sgx::dcap::hexStringToBytes("0001090X0D0f149FAFFF"), Bytes {});
}
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
#include <PckParser/PckParser.h>

using namespace testing;
using namespace intel::sgx::dcap::pckparser;

time_t toTimestamp(const std::string& timeString)
{
    struct std::tm tm{};
    std::istringstream(timeString) >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return mktime(&tm);
}

struct PckParserUT : public Test
{
    Validity validity = {
            toTimestamp("2022-01-14 06:00:00"),
            toTimestamp("2022-05-14 06:00:00"),
    };
};


TEST_F(PckParserUT, validityIsValidTrueWhenDateInsidePeriod)
{
    auto result = validity.isValid(toTimestamp("2022-04-14 05:00:00"));

    EXPECT_TRUE(result);
}

TEST_F(PckParserUT, validityIsValidTrueWhenDateAtEnd)
{
    auto result = validity.isValid(toTimestamp("2022-05-14 06:00:00"));

    EXPECT_TRUE(result);
}

TEST_F(PckParserUT, validityIsValidTrueWhenDateAtBeginning)
{
    auto result = validity.isValid(toTimestamp("2022-01-14 06:00:00"));

    EXPECT_TRUE(result);
}

TEST_F(PckParserUT, validityIsValidFalseWhenDateAfter)
{
    auto result = validity.isValid(toTimestamp("2022-10-14 05:00:00"));

    EXPECT_FALSE(result);
}

TEST_F(PckParserUT, validityIsValidFalseWhenDateBefore)
{
    auto result = validity.isValid(toTimestamp("2021-10-14 05:00:00"));

    EXPECT_FALSE(result);
}

TEST_F(PckParserUT, validityIsValidFalseWhenValidityInversed)
{
    Validity validityInversed = {
            toTimestamp("2022-05-14 06:00:00"),
            toTimestamp("2022-01-14 06:00:00"),
    };
    auto result = validityInversed.isValid(toTimestamp("2022-04-14 05:00:00"));

    EXPECT_FALSE(result);
}
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

#include <Utils/TimeUtils.h>

#include <gtest/gtest.h>

#include <chrono>

using namespace intel::sgx::dcap;
using namespace ::testing;

struct TimeUtilsUT: public testing::TestWithParam<time_t>
{
    static void assertEqualTM(const tm *val1, const tm *val2)
    {
        ASSERT_EQ(val1->tm_sec, val2->tm_sec);
        ASSERT_EQ(val1->tm_min, val2->tm_min);
        ASSERT_EQ(val1->tm_hour, val2->tm_hour);
        ASSERT_EQ(val1->tm_mday, val2->tm_mday);
        ASSERT_EQ(val1->tm_mon, val2->tm_mon);
        ASSERT_EQ(val1->tm_year, val2->tm_year);
        ASSERT_EQ(val1->tm_isdst, val2->tm_isdst);
    }

    time_t now = time(0);
#if defined(_MSC_VER)
#pragma warning(disable:4996)
#endif
    struct tm *tm_now = std::gmtime(&now);
#if defined(_MSC_VER)
#pragma warning(default:4996)
#endif
    time_t tmp = std::mktime(tm_now);
};

TEST_P(TimeUtilsUT, gmtime)
{
    std::cout << "gmtime input value: " << GetParam() << std::endl;
    assertEqualTM(standard::gmtime(&GetParam()), enclave::gmtime(&GetParam()));
}

const time_t positiveInput[] = {
        0,
        rand(),
        rand(),
};

INSTANTIATE_TEST_SUITE_P(TestsWithParameters, TimeUtilsUT, ::testing::ValuesIn(positiveInput));

TEST_P(TimeUtilsUT, mktime)
{
    std::cout << "mktime input value: " << GetParam() << std::endl;
#if defined(_MSC_VER)
#pragma warning(disable:4996)
#endif
    auto val = std::gmtime(&GetParam());
#if defined(_MSC_VER)
#pragma warning(default:4996)
#endif
    assert(val != nullptr);
    ASSERT_EQ(standard::mktime(val), enclave::mktime(val));
}

TEST_F(TimeUtilsUT, mktimeNullAsParam)
{
    EXPECT_THROW(standard::gmtime(nullptr), std::runtime_error);
    EXPECT_THROW(enclave::gmtime(nullptr), std::runtime_error);
}

TEST_F(TimeUtilsUT, getTimeFromString)
{
    auto date = std::string("2017-10-04T11:10:45Z");
    const auto standard = standard::getTimeFromString(date);
    const auto enclave = enclave::getTimeFromString(date);
    assertEqualTM(&standard, &enclave);
}

TEST_F(TimeUtilsUT, getTimeFromString_empty)
{
    auto date = std::string("");
    const auto standard = standard::getTimeFromString(date);
    const auto enclave = enclave::getTimeFromString(date);
    assertEqualTM(&standard, &enclave);
}

/*
 * Asserts both standard and enclave implementation of IsValidTimeString
 */
void assertIsValidTimeString(const std::string& date, bool expected)
{
    auto standardResult = standard::isValidTimeString(date);
    auto enclaveResult = enclave::isValidTimeString(date);
    ASSERT_EQ(standardResult, expected);
    ASSERT_EQ(enclaveResult, expected);
}

TEST_F(TimeUtilsUT, isValidTimeString)
{
    auto date = std::string("2017-10-04T11:10:45Z");
    assertIsValidTimeString(date, true);
}

TEST_F(TimeUtilsUT, isValidTimeString_empty)
{
    auto date = std::string("");
    assertIsValidTimeString(date, false);
}

TEST_F(TimeUtilsUT, isValidTimeStringIncorrectDate)
{
    auto date = std::string("2017-06-31T11:10:45Z");
    assertIsValidTimeString(date, false);
}
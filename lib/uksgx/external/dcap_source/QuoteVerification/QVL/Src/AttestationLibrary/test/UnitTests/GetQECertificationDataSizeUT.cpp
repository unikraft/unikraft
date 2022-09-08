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

#include <SgxEcdsaAttestation/QuoteVerification.h>
#include "QuoteV3Generator.h"
#include <numeric>

using namespace testing;
using namespace ::intel::sgx::dcap;

struct GetQECertificationDataSizeTests : public Test
{
    std::vector<uint8_t> rawQuote;
    uint32_t qeCertificationDataSize = 0;
};

TEST_F(GetQECertificationDataSizeTests, nullptrArgumentsShouldReturnMissingParametersStatus)
{
    ASSERT_EQ(STATUS_MISSING_PARAMETERS, sgxAttestationGetQECertificationDataSize(nullptr, 0, nullptr));
}

TEST_F(GetQECertificationDataSizeTests, nullptrQuoteShouldReturnMissingParametersStatus)
{
    EXPECT_EQ(STATUS_MISSING_PARAMETERS, sgxAttestationGetQECertificationDataSize(nullptr, 0, &qeCertificationDataSize));
    EXPECT_EQ(0, qeCertificationDataSize);
}

TEST_F(GetQECertificationDataSizeTests, nullptrQECertificationDataSizeShouldReturnMissingParametersStatus)
{
    ASSERT_EQ(STATUS_MISSING_PARAMETERS, sgxAttestationGetQECertificationDataSize(rawQuote.data(), 0, nullptr));
}

TEST_F(GetQECertificationDataSizeTests, invalidQuoteFormatShouldReturnUnsupportedQuoteFormat)
{
    // GIVEN
    rawQuote = std::vector<uint8_t>(256, 0x00);
    std::iota(std::begin(rawQuote), std::end(rawQuote), 1);

    // WHEN/THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_QUOTE_FORMAT, sgxAttestationGetQECertificationDataSize(rawQuote.data(), (uint32_t) rawQuote.size(), &qeCertificationDataSize));
    EXPECT_EQ(0, qeCertificationDataSize);
}

TEST_F(GetQECertificationDataSizeTests, positiveValidParametersShouldReturnOkStatusAndUpdateQeCertificationDataSize)
{
    // GIVEN
    test::QuoteV3Generator generator;
    const Bytes pckData{'p', 'c', 'k', 'd', 'a', 't', 'a'};
    generator.withCertificationData(2, pckData)
             .withAuthDataSize((uint32_t) (generator.getAuthSize() + pckData.size()));

    auto quote = generator.buildQuote();

    // WHEN
    const auto status = sgxAttestationGetQECertificationDataSize(quote.data(), (uint32_t) quote.size(), &qeCertificationDataSize);

    // THEN
    EXPECT_EQ(STATUS_OK, status);
    EXPECT_EQ(pckData.size(), qeCertificationDataSize) << "'qeCertificationDataSize' extracted from quote differs from expected";
}

TEST_F(GetQECertificationDataSizeTests, positiveEmptycertificationDataShouldReturnOkStatusAndUpdateQeCertificationDataSize)
{
    // GIVEN
    test::QuoteV3Generator generator;
    const Bytes pckData; // empty
    generator.withCertificationData(2, pckData)
             .withAuthDataSize((uint32_t) (generator.getAuthSize() + pckData.size()));
    const auto quote = generator.buildQuote();

    // WHEN
    const auto status = sgxAttestationGetQECertificationDataSize(quote.data(), (uint32_t) quote.size(), &qeCertificationDataSize);

    // THEN
    EXPECT_EQ(STATUS_OK, status);
    EXPECT_EQ(pckData.size(), qeCertificationDataSize) << "'qeCertificationDataSize' extracted from quote differs from expected";
}

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
#include "Constants/QuoteTestConstants.h"
#include <numeric>

using namespace testing;
using namespace ::intel::sgx::dcap;

struct GetQECertificationDataTests : public Test
{
    uint8_t placeholder[12] = {'p', 'l', 'a', 'c', 'e', '_', 'h', 'o', 'l', 'd', 'e', 'r'};
    uint32_t placeholderSize = 0;
    uint16_t placeholderQeCertificationDataType = test::constants::PCK_ID_PLAIN_PPID;

    Bytes prepareQuoteWithCertData(const uint16_t type, const Bytes &data) const
    {
        test::QuoteV3Generator generator;
        const Bytes pckData{'p', 'c', 'k', 'd', 'a', 't', 'a'};
        generator.withCertificationData(type, data)
                 .withAuthDataSize((uint32_t)(generator.getAuthSize() + data.size()));
        return generator.buildQuote();
    }
};

TEST_F(GetQECertificationDataTests, nullptrArgumentsShouldReturnMissingParametersStatus)
{
    ASSERT_EQ(STATUS_MISSING_PARAMETERS,
              sgxAttestationGetQECertificationData(
                      nullptr, 0, 0, nullptr, nullptr));
}

TEST_F(GetQECertificationDataTests, nullptrQuoteShouldReturnMissingParametersStatus)
{
    ASSERT_EQ(STATUS_MISSING_PARAMETERS,
              sgxAttestationGetQECertificationData(
                      nullptr, placeholderSize, placeholderSize, placeholder, &placeholderQeCertificationDataType));
}

TEST_F(GetQECertificationDataTests, nullptrQECertificationDataShouldReturnMissingParametersStatus)
{
    ASSERT_EQ(STATUS_MISSING_PARAMETERS,
              sgxAttestationGetQECertificationData(
                      placeholder, placeholderSize, placeholderSize, nullptr, &placeholderQeCertificationDataType));
}

TEST_F(GetQECertificationDataTests, nullptrQECertificationDataTypeShouldReturnMissingParametersStatus)
{
    ASSERT_EQ(STATUS_MISSING_PARAMETERS,
              sgxAttestationGetQECertificationData(
                      placeholder, placeholderSize, placeholderSize, placeholder, nullptr));
}

TEST_F(GetQECertificationDataTests, invalidQuoteFormatShouldReturnUnsupportedQuoteFormat)
{
    // GIVEN
    std::vector<uint8_t> rawQuote = std::vector<uint8_t>(256, 0x00);
    std::iota(std::begin(rawQuote), std::end(rawQuote), 1);

    // WHEN/THEN
    ASSERT_EQ(STATUS_UNSUPPORTED_QUOTE_FORMAT,
              sgxAttestationGetQECertificationData(
                      rawQuote.data(), (uint32_t) rawQuote.size(), placeholderSize, placeholder, &placeholderQeCertificationDataType));
}

TEST_F(GetQECertificationDataTests, qeCertificationDataSizeAsParameterDiffersFromQuoteDataShouldReturnInvalidQeCertificationDataSizeStatus)
{
    // GIVEN
    auto quote = prepareQuoteWithCertData(test::constants::PCK_ID_PLAIN_PPID, {'p', 'c', 'k', 'd', 'a', 't', 'a'});

    // WHEN/THEN
    ASSERT_EQ(STATUS_INVALID_QE_CERTIFICATION_DATA_SIZE,
              sgxAttestationGetQECertificationData(
                      quote.data(), (uint32_t) quote.size(), placeholderSize, placeholder, &placeholderQeCertificationDataType));
}

TEST_F(GetQECertificationDataTests, positiveEmptycertificationDataShouldReturnOkStatusAndUpdateQeCertificationDataAndType)
{
    // GIVEN
    const auto expectedQeCertType = test::constants::PCK_ID_PLAIN_PPID;
    const auto quote = prepareQuoteWithCertData(expectedQeCertType, {});
    uint32_t qeCertificationDataSize = 0;

    const uint32_t CERT_DATA_BUFFER_SIZE = 1024;
    std::vector<uint8_t> qeCertificationDataBuffer(CERT_DATA_BUFFER_SIZE, 0x00);  // allocate an empty vector, it should be unchanged
    uint16_t qeCertificationDataType = 0;

    // WHEN
    const auto status = sgxAttestationGetQECertificationData(
            quote.data(),
            (uint32_t) quote.size(),
            qeCertificationDataSize,
            qeCertificationDataBuffer.data(),
            &qeCertificationDataType);

    // THEN
    EXPECT_EQ(STATUS_OK, status);
    EXPECT_EQ(expectedQeCertType, qeCertificationDataType) << "certificationDataType extracted from quote does not match expected";
    EXPECT_EQ(std::vector<uint8_t>(CERT_DATA_BUFFER_SIZE), qeCertificationDataBuffer) << "CertificationData extracted from quote does not match expected";
}

struct GetQECertificationDataPositiveTests : public GetQECertificationDataTests,
    public WithParamInterface<uint16_t>
{};

TEST_P(GetQECertificationDataPositiveTests, validcertificationDataShouldReturnOkStatusAndUpdateQeCertificationDataAndType)
{
    // GIVEN
    const auto expectedQeCertType = GetParam();
    const auto expectedcertificationData = Bytes{'p', 'c', 'k', 'd', 'a', 't', 'a'};
    const auto quote = prepareQuoteWithCertData(expectedQeCertType, expectedcertificationData);

    std::vector<uint8_t> qeCertificationDataBuffer(expectedcertificationData.size(), 0x00);  // allocate an empty vector, it should be unchanged
    uint16_t qeCertificationDataType = 0;

    // WHEN
    const auto status = sgxAttestationGetQECertificationData(
            quote.data(),
            (uint32_t) quote.size(),
            (uint32_t) qeCertificationDataBuffer.size(),
            qeCertificationDataBuffer.data(),
            &qeCertificationDataType);

    // THEN
    EXPECT_EQ(STATUS_OK, status);
    EXPECT_EQ(expectedQeCertType, qeCertificationDataType) << "certificationDataType extracted from quote does not match expected";
    EXPECT_EQ(expectedcertificationData, qeCertificationDataBuffer) << "CertificationData extracted from quote does not match expected";
}

INSTANTIATE_TEST_SUITE_P(AllSupportedPckIdTypes,
                        GetQECertificationDataPositiveTests,
                        Values(
                          test::constants::PCK_ID_PLAIN_PPID,
                          test::constants::PCK_ID_ENCRYPTED_PPID_2048,
                          test::constants::PCK_ID_ENCRYPTED_PPID_3072,
                          test::constants::PCK_ID_PCK_CERTIFICATE,
                          test::constants::PCK_ID_PCK_CERT_CHAIN));

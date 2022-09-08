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

#include <QuoteVerification/Quote.h>
#include <SgxEcdsaAttestation/QuoteVerification.h>
#include "QuoteV3Generator.h"
#include "QuoteUtils.h"

using namespace testing;
using namespace ::intel::sgx::dcap;

struct QuoteV3GeneratorTests : public Test
{
};

namespace {

bool areBytesAtPosition(Bytes container, size_t position, const Bytes& bytes)
{
    if (bytes.size() + position > container.size())
    {
        return false;
    }
    auto sliceBegin = std::next(container.cbegin(), (long) position);
    auto sliceEnd = std::next(sliceBegin, (long) bytes.size());
    Bytes slice(sliceBegin, sliceEnd);
    return bytes == slice;
}

MATCHER_P2(DataAtPositionEq, position, data,
    std::string("Data at position " + PrintToString(position) + (negation ? " isn't" : " is") + " equal to " + PrintToString(data)))
{
    auto bytes = test::toBytes(data);
    return areBytesAtPosition(arg, (size_t) position, bytes);
}

MATCHER_P2(BytesAtPositionEq, position, bytes,
    std::string("Data at position " + PrintToString(position) + (negation ? " isn't" : " is") + " equal to " + PrintToString(bytes)))
{
    return areBytesAtPosition(arg, position, bytes);
}


constexpr size_t QE_SVN_POSITION_IN_HEADER = 8;
constexpr size_t PCE_SVN_POSITION_IN_HEADER = 10;
constexpr size_t QUOTE_AUTH_DATA_POSITION = 
    test::QUOTE_HEADER_SIZE +
    test::BODY_SIZE;

constexpr size_t BODY_POSITION = test::QUOTE_HEADER_SIZE;

constexpr size_t QE_REPORT_DATA_POSITION = 
    QUOTE_AUTH_DATA_POSITION + 
    test::QUOTE_AUTH_DATA_SIZE_FIELD_SIZE +
    test::ENCLAVE_REPORT_SIGNATURE_SIZE +
    test::ECDSA_PUBLIC_KEY_SIZE;

constexpr size_t QE_REPORT_SIGNATURE_POSITION = 
    QE_REPORT_DATA_POSITION +
    test::ENCLAVE_REPORT_SIZE; 

constexpr size_t QE_AUTH_DATA_POSITION = 
    QE_REPORT_SIGNATURE_POSITION +
    test::ENCLAVE_REPORT_SIGNATURE_SIZE;

constexpr size_t ECDSA_ATTESTATION_KEY_POSITION = 
    QUOTE_AUTH_DATA_POSITION +
    test::QUOTE_AUTH_DATA_SIZE_FIELD_SIZE + 
    test::ENCLAVE_REPORT_SIGNATURE_SIZE;
} //anonymous namespace

TEST_F(QuoteV3GeneratorTests, shouldProvideGeneratedBinaryQuote)
{
    test::QuoteV3Generator generator;
    auto quote = generator.buildQuote();
    EXPECT_THAT(quote, SizeIs(test::QUOTE_V3_MINIMAL_SIZE));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingQeSvn)
{
    test::QuoteV3Generator generator;
    uint16_t qeSvn = 55;

    generator.withQeSvn(qeSvn);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(QE_SVN_POSITION_IN_HEADER, qeSvn));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingPceSvn)
{
    test::QuoteV3Generator generator;
    uint16_t pceSvn = 256;

    generator.withPceSvn(pceSvn);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(PCE_SVN_POSITION_IN_HEADER, pceSvn));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowChainingMethods)
{
    test::QuoteV3Generator generator;
    uint16_t pceSvn = 5;
    uint16_t qeSvn = 88;

    generator.withQeSvn(qeSvn).withPceSvn(pceSvn);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, AllOf(DataAtPositionEq(QE_SVN_POSITION_IN_HEADER, qeSvn), DataAtPositionEq(PCE_SVN_POSITION_IN_HEADER, pceSvn)));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingHeader)
{
    test::QuoteV3Generator generator;
    test::QuoteV3Generator::QuoteHeader header = {5, 1, 229, 0, 0, 823, {{0, 1, 4}}, {{20, 50, 88, 153}}};

    generator.withHeader(header);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(0, header));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingBody)
{
    test::QuoteV3Generator generator;
    test::QuoteV3Generator::EnclaveReport er = {{{45, 88, 62}}, 2222, {{}}, {{32}}, {{'m', 'r', 'e'}}, {{}}, {{'m', 'r', 's'}}, {{}}, 4, 35, {{}}, {{99, 194, 78}}};

    generator.withEnclaveReport(er);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(BODY_POSITION, er));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingQeReport)
{
    test::QuoteV3Generator generator;
    test::QuoteV3Generator::EnclaveReport er = {{{45, 88, 62}}, 2222, {{}}, {{32}}, {{'m', 'r', 'e'}}, {{}}, {{'m', 'r', 's'}}, {{}}, 4, 35, {{}}, {{99, 194, 78}}};

    generator.withQeReport(er);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(QE_REPORT_DATA_POSITION, er));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingQeReportSignature)
{
    test::QuoteV3Generator generator;
    test::QuoteV3Generator::EcdsaSignature sign = {{"signature"}};

    generator.withQeReportSignature(sign);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(QE_REPORT_SIGNATURE_POSITION, sign));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingEcdsaAttestationKey)
{
    test::QuoteV3Generator generator;
    test::QuoteV3Generator::EcdsaPublicKey key = {{"public key"}};

    generator.withAttestationKey(key);
    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, DataAtPositionEq(ECDSA_ATTESTATION_KEY_POSITION, key));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingEmptyPCKData)
{
    test::QuoteV3Generator generator;
    Bytes pckData{};
    generator.withCertificationData(1, pckData);

    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, SizeIs(test::QUOTE_V3_MINIMAL_SIZE));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingArbitraryPCKData)
{
    test::QuoteV3Generator generator;
    Bytes pckData{'p', 'c', 'k', 'd', 'a', 't', 'a'};
    generator.withCertificationData(2, pckData);

    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, SizeIs(test::QUOTE_V3_MINIMAL_SIZE + pckData.size()));
    EXPECT_THAT(quote,
            BytesAtPositionEq(QE_AUTH_DATA_POSITION +
                test::QE_AUTH_DATA_MIN_SIZE + 6, pckData));

}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingEmptyAuthData)
{
    test::QuoteV3Generator generator;
    Bytes authData{};
    generator.withQeAuthData(authData);

    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, SizeIs(test::QUOTE_V3_MINIMAL_SIZE));
}

TEST_F(QuoteV3GeneratorTests, shouldAllowSettingArbitraryAuthData)
{
    test::QuoteV3Generator generator;
    Bytes authData{'a', 'u', 't', 'h'};
    generator.withQeAuthData(authData);

    auto quote = generator.buildQuote();

    EXPECT_THAT(quote, SizeIs(test::QUOTE_V3_MINIMAL_SIZE + authData.size()));
    EXPECT_THAT(quote,
            BytesAtPositionEq(QE_AUTH_DATA_POSITION + test::QE_AUTH_SIZE_BYTE_LEN, authData));
}

TEST_F(QuoteV3GeneratorTests, withArbitraryPckDataShouldBeParsable)
{
    // GIVEN
    test::QuoteV3Generator generator;
    const Bytes pckData{'p', 'c', 'k', 'd', 'a', 't', 'a'};
    generator.withCertificationData(2, pckData)
             .withAuthDataSize((uint32_t) (generator.getAuthSize() + pckData.size()));
    const auto generatedQuote = generator.buildQuote();

    intel::sgx::dcap::Quote quote;

    // WHEN/THEN
    ASSERT_TRUE(quote.parse(generatedQuote));
}

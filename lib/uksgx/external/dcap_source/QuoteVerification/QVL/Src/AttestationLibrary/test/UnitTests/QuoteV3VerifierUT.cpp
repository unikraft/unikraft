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


#include <Verifiers/QuoteVerifier.h>
#include <PckParser/FormatException.h>

#include <utility>

#include "Mocks/CertCrlStoresMocks.h"
#include "Mocks/EnclaveIdentityMock.h"
#include "Mocks/EnclaveReportVerifierMock.h"
#include "Mocks/TcbInfoMock.h"
#include "DigestUtils.h"
#include "QuoteV3Generator.h"
#include "KeyHelpers.h"
#include "Constants/QuoteTestConstants.h"
#include "EcdsaSignatureGenerator.h"
#include "CertVerification/X509Constants.h"

using namespace intel::sgx;
using namespace ::testing;

namespace {

    uint16_t toUint16(uint8_t leftMostByte, uint8_t rightMostByte)
    {
        int32_t ret = 0;
        ret |= static_cast<int32_t>(rightMostByte);
        ret |= (static_cast<int32_t>(leftMostByte) << 8) & 0xff00;
        return (uint16_t) ret;
    }

    std::array<uint8_t,64> signAndGetRaw(const std::vector<uint8_t>& data, EVP_PKEY& key)
    {
        auto signature = dcap::DigestUtils::signMessageSha256(data, key);
        return EcdsaSignatureGenerator::convertECDSASignatureToRawArray(signature);
    }

    std::vector<uint8_t> concat(const std::vector<uint8_t>& rhs, const std::vector<uint8_t>& lhs)
    {
        std::vector<uint8_t> ret = rhs;
        std::copy(lhs.begin(), lhs.end(), std::back_inserter(ret));
        return ret;
    }

    template<size_t N>
    std::vector<uint8_t> concat(const std::array<uint8_t,N>& rhs, const std::vector<uint8_t>& lhs)
    {
        std::vector<uint8_t> ret(std::begin(rhs), std::end(rhs));
        std::copy(lhs.begin(), lhs.end(), std::back_inserter(ret));
        return ret;
    }

    std::array<uint8_t,64> assingFirst32(const std::array<uint8_t,32>& in)
    {
        std::array<uint8_t,64> ret{};
        std::copy_n(in.begin(), 32, ret.begin());
        return ret;
    }

    std::array<uint8_t,64> signEnclaveReport(const dcap::test::QuoteV3Generator::EnclaveReport& report, EVP_PKEY& key)
    {
        return signAndGetRaw(report.bytes(), key);
    }
}//anonymous namespace

struct QuoteV3VerifierUT: public testing::Test
{
    QuoteV3VerifierUT()
            : ppid(16, 0xaa), cpusvn(16, 0x40), fmspc(6, 0xde), pcesvn{0xaa, 0xbb}
    {

    }
    const dcap::crypto::EVP_PKEY_uptr privKey = dcap::test::toEvp(*dcap::test::priv(dcap::test::PEM_PRV));
    const dcap::crypto::EC_KEY_uptr pubKey = dcap::test::pub(dcap::test::PEM_PUB);
    const std::vector<uint8_t> pckPubKey = dcap::test::getVectorPub(*pubKey);
    const std::vector<uint8_t> ppid{ 1 , 2 };
    const std::vector<uint8_t> cpusvn{ 3, 4 };
    const std::vector<uint8_t> fmspc{ 5 , 6 };
    const std::vector<uint8_t> pcesvn{ 7, 8 };
    const std::vector<uint8_t> pceId{ 0, 1};
    const std::vector<dcap::pckparser::Revoked> emptyRevoked{};
    std::set<dcap::parser::json::TcbLevel, std::greater<dcap::parser::json::TcbLevel>> tcbs{};

    NiceMock<test::ValidityMock> validityMock;
    NiceMock<test::TcbMock> tcbMock;
    NiceMock<dcap::test::PckCertificateMock> pck;
    NiceMock<dcap::test::CrlStoreMock> crl;
    NiceMock<dcap::test::TcbInfoMock> tcbInfoJson;
    NiceMock<dcap::test::EnclaveIdentityMock> enclaveIdentityV2;
    NiceMock<dcap::test::EnclaveReportVerifierMock> enclaveReportVerifier;
    dcap::test::QuoteV3Generator gen;
    const time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    /*
     * SetUp represent minimal, positive quote verification data preparation
     */
    void SetUp() override
    {
        ON_CALL(validityMock, getNotAfterTime()).WillByDefault(Return(currentTime));
        ON_CALL(tcbMock, getPceSvn()).WillByDefault(Return(48042));
        ON_CALL(tcbMock, getCpuSvn()).WillByDefault(ReturnRef(cpusvn));
        ON_CALL(tcbMock, getSgxTcbComponentSvn(_)).WillByDefault(Return(0x40));

        ON_CALL(pck, getValidity()).WillByDefault(ReturnRef(validityMock));
        ON_CALL(pck, getSubject()).WillByDefault(testing::ReturnRef(dcap::constants::PCK_SUBJECT));
        ON_CALL(pck, getIssuer()).WillByDefault(testing::ReturnRef(dcap::constants::PLATFORM_CA_SUBJECT));
        ON_CALL(pck, getTcb()).WillByDefault(testing::ReturnRef(tcbMock));
        ON_CALL(pck, getPubKey()).WillByDefault(testing::ReturnRef(pckPubKey));
        ON_CALL(pck, getFmspc()).WillByDefault(testing::ReturnRef(fmspc));
        ON_CALL(pck, getPceId()).WillByDefault(testing::ReturnRef(pceId));
        ON_CALL(pck, getPpid()).WillByDefault(testing::ReturnRef(ppid));

        ON_CALL(crl, expired(currentTime)).WillByDefault(testing::Return(false));
        ON_CALL(crl, getIssuer()).WillByDefault(testing::ReturnRef(dcap::constants::PCK_PLATFORM_CRL_ISSUER));
        ON_CALL(crl, getRevoked()).WillByDefault(testing::ReturnRef(emptyRevoked));

        ON_CALL(tcbInfoJson, getId()).WillByDefault(testing::Return("SGX"));
        ON_CALL(tcbInfoJson, getVersion()).WillByDefault(testing::Return(2));
        ON_CALL(tcbInfoJson, getPceId()).WillByDefault(testing::ReturnRef(pceId));
        ON_CALL(tcbInfoJson, getFmspc()).WillByDefault(testing::ReturnRef(fmspc));
        ON_CALL(tcbInfoJson, getTcbLevels()).WillByDefault(testing::ReturnRef(tcbs));
        ON_CALL(tcbInfoJson, getNextUpdate()).WillByDefault(testing::Return(currentTime));

        ON_CALL(enclaveIdentityV2, getVersion()).WillByDefault(Return(2));
        ON_CALL(enclaveIdentityV2, getStatus()).WillByDefault(Return(STATUS_OK));
        ON_CALL(enclaveReportVerifier, verify(_, _, _)).WillByDefault(Return(STATUS_OK));
        ON_CALL(enclaveReportVerifier, verify(_, _)).WillByDefault(Return(STATUS_OK));

        dcap::test::QuoteV3Generator::CertificationData certificationData;
        certificationData.keyDataType = dcap::test::constants::PCK_ID_PLAIN_PPID;
        certificationData.keyData = concat(ppid, concat(cpusvn, pcesvn));
        certificationData.size = static_cast<uint16_t>(certificationData.keyData.size());
        gen.withcertificationData(certificationData);
        gen.getAuthSize() += (uint32_t) certificationData.keyData.size();
        gen.getAuthData().ecdsaAttestationKey.publicKey = dcap::test::getRawPub(*pubKey);

        gen.getAuthData().qeReport.reportData = assingFirst32(
                dcap::DigestUtils::sha256DigestArray(concat(gen.getAuthData().ecdsaAttestationKey.publicKey, gen.getAuthData().qeAuthData.data)));


        gen.getAuthData().qeReportSignature.signature = signEnclaveReport(gen.getAuthData().qeReport, *privKey);


        gen.getAuthData().ecdsaSignature.signature =
                signAndGetRaw(concat(gen.getHeader().bytes(), gen.getEnclaveReport().bytes()), *privKey);
    }
};

TEST_F(QuoteV3VerifierUT, shouldReturnStatusTcbInfoMismatchWhenFmspcDoesNotMatch)
{
    dcap::Quote quote;
    std::vector<uint8_t> emptyVector{};

    EXPECT_CALL(tcbInfoJson, getFmspc()).WillRepeatedly(testing::ReturnRef(emptyVector));

    EXPECT_EQ(STATUS_TCB_INFO_MISMATCH, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnStatusTcbInfoMismatchWhenPceIdDoesNotMatch)
{
    dcap::Quote quote;
    std::vector<uint8_t> emptyVector{};

    EXPECT_CALL(tcbInfoJson, getPceId()).WillRepeatedly(testing::ReturnRef(emptyVector));

    EXPECT_EQ(STATUS_TCB_INFO_MISMATCH, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnStatusInvalidPckCrlWhenPeriodAndIssuerIsInvalid)
{
    const auto quoteBin = gen.buildQuote();
    dcap::Quote quote;

    EXPECT_CALL(crl, getIssuer()).WillRepeatedly(testing::ReturnRef(dcap::constants::ROOT_CA_CRL_ISSUER));


    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_INVALID_PCK_CRL, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnStatusInvalidPckCrlWhenCrlIssuerIsDifferentThanPck)
{
    const auto quoteBin = gen.buildQuote();
    dcap::Quote quote;

    EXPECT_CALL(crl, getIssuer()).WillRepeatedly(testing::ReturnRef(dcap::constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(pck, getIssuer()).WillRepeatedly(testing::ReturnRef(dcap::constants::PROCESSOR_CA_SUBJECT));

    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_INVALID_PCK_CRL, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnStatusPckRevoked)
{
    const auto quoteBin = gen.buildQuote();
    dcap::Quote quote;

    EXPECT_CALL(crl, isRevoked(testing::_)).WillOnce(testing::Return(true));

    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_PCK_REVOKED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnStatusInvalidQeFormat)
{
    dcap::Quote quote;
    gen.getAuthData().ecdsaAttestationKey.publicKey = std::array<uint8_t, 64>{};
    const auto quoteBin = gen.buildQuote();

    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_INVALID_QE_REPORT_DATA, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldVerifySgxCorrectly)
{
    const auto quoteBin = gen.buildQuote();

    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{cpusvn, toUint16(pcesvn[1], pcesvn[0]), "UpToDate"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_OK, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnInvalidPCKCert)
{
    const auto emptySubject = dcap::parser::x509::DistinguishedName("", "", "", "", "", "");
    ON_CALL(pck, getSubject()).WillByDefault(testing::ReturnRef(emptySubject));
    const auto quoteBin = gen.buildQuote();

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_INVALID_PCK_CERT, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

struct QuoteV3VerifierUTPckTypesParametrized : public QuoteV3VerifierUT,
                                             public testing::WithParamInterface<uint16_t>
{};

TEST_P(QuoteV3VerifierUTPckTypesParametrized, shouldReturnStatusOkEvenWhenPpidIsNotMatching)
{
    const std::vector<uint8_t> notMatchingPpid(16, 0x00);
    ON_CALL(pck, getPpid()).WillByDefault(testing::ReturnRef(notMatchingPpid));

    dcap::test::QuoteV3Generator::CertificationData certificationData;
    certificationData.keyDataType = GetParam();
    certificationData.keyData = concat(ppid, concat(cpusvn, pcesvn));
    certificationData.size = static_cast<uint16_t>(certificationData.keyData.size());

    const auto quoteBin = gen.withcertificationData(certificationData).buildQuote();

    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{cpusvn, toUint16(pcesvn[1], pcesvn[0]), "UpToDate"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_OK, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

INSTANTIATE_TEST_SUITE_P(PckIdTypesThatDoNotValidatecertificationData,
                        QuoteV3VerifierUTPckTypesParametrized,
                        testing::Values(
                                dcap::test::constants::PCK_ID_ENCRYPTED_PPID_2048,
                                dcap::test::constants::PCK_ID_ENCRYPTED_PPID_3072,
                                dcap::test::constants::PCK_ID_PCK_CERTIFICATE,
                                dcap::test::constants::PCK_ID_PCK_CERT_CHAIN));

TEST_F(QuoteV3VerifierUT, shouldReturnQuoteInvalidSignature)
{
    gen.getAuthData().ecdsaSignature.signature[0] = (unsigned char) ~gen.getAuthData().ecdsaSignature.signature[0];
    const auto quoteBin = gen.buildQuote();

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_INVALID_QUOTE_SIGNATURE, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnInvalidQeReportSignature)
{
    gen.getAuthData().qeReportSignature.signature[0] = (unsigned char) ~gen.getAuthData().qeReportSignature.signature[0];
    const auto quoteBin = gen.buildQuote();

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_INVALID_QE_REPORT_SIGNATURE, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnTcbRevokedOnLatestRevokedEqualPckTCB)
{
    const auto quoteBin = gen.buildQuote();

    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{cpusvn, toUint16(pcesvn[1], pcesvn[0]), "Revoked"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_REVOKED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldMatchToLowerTCBWhenBothSVNsAreLowerAndReturnConfigurationNeeded)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> higherCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x41, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    std::vector<uint8_t> lowerCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

    const std::vector<uint8_t> higherPcesvn = {0xff, 0xff};
    const std::vector<uint8_t> lowerPcesvn = {0x00, 0x00};

    tcbs.insert(dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(lowerPcesvn[1], lowerPcesvn[0]), "ConfigurationNeeded"});
    tcbs.insert(dcap::parser::json::TcbLevel{higherCpusvn, toUint16(higherPcesvn[1], higherPcesvn[0]), "Revoked"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_CONFIGURATION_NEEDED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldMatchToLowerTCBWhenBothSVNsAreLowerAndReturnConfigurationNeededForTcbInfoV2)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> higherCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x41, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    std::vector<uint8_t> lowerCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

    const std::vector<uint8_t> higherPcesvn = {0xff, 0xff};
    const std::vector<uint8_t> lowerPcesvn = {0x00, 0x00};

    tcbs.insert(dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(lowerPcesvn[1], lowerPcesvn[0]), "ConfigurationNeeded"});
    tcbs.insert(dcap::parser::json::TcbLevel{higherCpusvn, toUint16(higherPcesvn[1], higherPcesvn[0]), "Revoked"});
    ON_CALL(tcbInfoJson, getVersion()).WillByDefault(testing::Return(2));
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_CONFIGURATION_NEEDED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldMatchToLowerTCBWhenBothSVNsAreLowerAndReturnOutOfDateConfigurationNeeded)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> higherCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x41, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    std::vector<uint8_t> lowerCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

    const std::vector<uint8_t> higherPcesvn = {0xff, 0xff};
    const std::vector<uint8_t> lowerPcesvn = {0x00, 0x00};

    tcbs.insert(dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(lowerPcesvn[1], lowerPcesvn[0]), "OutOfDateConfigurationNeeded"});
    tcbs.insert(dcap::parser::json::TcbLevel{higherCpusvn, toUint16(higherPcesvn[1], higherPcesvn[0]), "Revoked"});
    ON_CALL(tcbInfoJson, getVersion()).WillByDefault(testing::Return(2));
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldMatchToLowerTCBAndReturnConfigurationNeeded)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> higherCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x41, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    std::vector<uint8_t> lowerCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

    auto higherTcb = dcap::parser::json::TcbLevel{higherCpusvn, toUint16(pcesvn[1], pcesvn[0]), "Revoked"};
    auto lowerTcb = dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(pcesvn[1], pcesvn[0]), "ConfigurationNeeded"};
    tcbs.insert(lowerTcb);
    tcbs.insert(higherTcb);
    EXPECT_EQ(tcbs.size(), 2);
    for (uint32_t i=0; i< constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(tcbs.begin()->getSgxTcbComponentSvn(i), higherTcb.getSgxTcbComponentSvn(i)); // make sure that TCB levels has been inserted and sorted correctly
    }
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_CONFIGURATION_NEEDED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}



TEST_F(QuoteV3VerifierUT, shouldMatchToLowerTCBAndReturnConfigurationAndSwHardeningNeeded)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> higherCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x41, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    std::vector<uint8_t> lowerCpusvn = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

    auto higherTcb = dcap::parser::json::TcbLevel{higherCpusvn, toUint16(pcesvn[1], pcesvn[0]), "Revoked"};
    auto lowerTcb = dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(pcesvn[1], pcesvn[0]), "ConfigurationAndSWHardeningNeeded"};
    tcbs.insert(lowerTcb);
    tcbs.insert(higherTcb);
    EXPECT_EQ(tcbs.size(), 2);
    for (uint32_t i=0; i< constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(tcbs.begin()->getSgxTcbComponentSvn(i), higherTcb.getSgxTcbComponentSvn(i)); // make sure that TCB levels has been inserted and sorted correctly
    }
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnTcbNotSupportedWhenOnlyPceSvnIsHigher)
{
    const auto quoteBin = gen.buildQuote();

    const std::vector<uint8_t> higherPcesvn = {0xff, 0xff};

    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{cpusvn, toUint16(higherPcesvn[1], higherPcesvn[0]), "Revoked"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_NOT_SUPPORTED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnTcbRevokedWhenOnlyCpuSvnIsLower)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> lowerCpusvn = cpusvn;
    lowerCpusvn[8]--;

    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(pcesvn[1], pcesvn[0]), "Revoked"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_REVOKED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnTcbRevokedWhenOnlyPcesvnIsLower)
{
    const auto quoteBin = gen.buildQuote();

    const std::vector<uint8_t> lowerPcesvn = {0x21, 0x12};

    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{cpusvn, toUint16(lowerPcesvn[1], lowerPcesvn[0]), "Revoked"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_REVOKED, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldNOTReturnTcbRevokedWhenRevokedPcesvnAndCpusvnAreLower)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> lowerCpusvn = cpusvn;
    lowerCpusvn[8]--;
    std::vector<uint8_t> lowerPcesvn = pcesvn;
    lowerPcesvn[0]--;

    tcbs.insert(dcap::parser::json::TcbLevel{cpusvn, toUint16(pcesvn[1], pcesvn[0]), "UpToDate"});
    tcbs.insert(dcap::parser::json::TcbLevel{lowerCpusvn, toUint16(lowerPcesvn[1], lowerPcesvn[0]), "Revoked"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_OK, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

TEST_F(QuoteV3VerifierUT, shouldReturnSwHardeningNeeded)
{
    const auto quoteBin = gen.buildQuote();

    std::vector<uint8_t> lowerCpusvn = cpusvn;
    lowerCpusvn[8]--;
    std::vector<uint8_t> lowerPcesvn = pcesvn;
    lowerPcesvn[0]--;

    tcbs.insert(dcap::parser::json::TcbLevel{cpusvn, toUint16(pcesvn[1], pcesvn[0]), "SWHardeningNeeded"});
    EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));

    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    EXPECT_EQ(STATUS_TCB_SW_HARDENING_NEEDED , dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

struct QeIdentityStatuses {
    Status enclaveVerifierStatus;
    Status expectedStatus;
};

struct QuoteV3VerifierUTQeIdentityStatusParametrized : public QuoteV3VerifierUT,
                                                     public testing::WithParamInterface<QeIdentityStatuses>
{};

TEST_P(QuoteV3VerifierUTQeIdentityStatusParametrized, testAllStatuses)
{
    auto params = GetParam();
    const auto quoteBin = gen.buildQuote();
    dcap::Quote quote;
    ASSERT_TRUE(quote.parse(quoteBin));
    tcbs.insert(tcbs.begin(), dcap::parser::json::TcbLevel{cpusvn, toUint16(pcesvn[1], pcesvn[0]), "UpToDate"});
    if (params.enclaveVerifierStatus == STATUS_OK)
    {
        EXPECT_CALL(tcbInfoJson, getTcbLevels()).WillOnce(testing::ReturnRef(tcbs));
    }

    EXPECT_CALL(enclaveReportVerifier, verify(_, _)).WillOnce(Return(params.enclaveVerifierStatus));
    EXPECT_CALL(enclaveIdentityV2, getStatus()).WillRepeatedly(Return(STATUS_OK));
    EXPECT_EQ(params.expectedStatus, dcap::QuoteVerifier{}.verify(quote, pck, crl, tcbInfoJson, &enclaveIdentityV2, enclaveReportVerifier));
}

INSTANTIATE_TEST_SUITE_P(AllStatutes,
                        QuoteV3VerifierUTQeIdentityStatusParametrized,
                        testing::Values(
                                QeIdentityStatuses{STATUS_OK, STATUS_OK},
                                QeIdentityStatuses{STATUS_SGX_ENCLAVE_REPORT_MISCSELECT_MISMATCH, STATUS_QE_IDENTITY_MISMATCH},
                                QeIdentityStatuses{STATUS_SGX_ENCLAVE_REPORT_ATTRIBUTES_MISMATCH, STATUS_QE_IDENTITY_MISMATCH},
                                QeIdentityStatuses{STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH, STATUS_QE_IDENTITY_MISMATCH},
                                QeIdentityStatuses{STATUS_SGX_ENCLAVE_REPORT_ISVPRODID_MISMATCH, STATUS_QE_IDENTITY_MISMATCH},
                                QeIdentityStatuses{STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE, STATUS_TCB_OUT_OF_DATE}
                        ));

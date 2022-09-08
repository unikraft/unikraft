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
#include <SgxEcdsaAttestation/AttestationParsers.h>
#include <Verifiers/TCBInfoVerifier.h>
#include <Mocks/CertCrlStoresMocks.h>
#include <Mocks/TcbSigningChainMock.h>
#include <Mocks/CommonVerifierMock.h>
#include <Mocks/PckCrlVerifierMock.h>
#include <TcbInfoGenerator.h>
#include <PckParser/PckParser.h>
#include "KeyHelpers.h"

using namespace testing;
using namespace intel::sgx::dcap;

struct TCBInfoVerifierUT : public Test
{
    std::vector<pckparser::Extension> extensions;
    pckparser::Signature signature;
    intel::sgx::dcap::parser::json::TcbInfo tcbInfo = intel::sgx::dcap::parser::json::TcbInfo::parse(TcbInfoGenerator::generateTcbInfo());
    std::vector<uint8_t> pubKey = {};
    const time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
};

TEST_F(TCBInfoVerifierUT, shouldReturnStatusOkWhenVerifyPassPositive)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    StrictMock<test::ValidityMock> validityMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillRepeatedly(Return(currentTime));
    EXPECT_CALL(crlStoreMock, expired(_)).WillOnce(Return(false));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, 1529580962);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(TCBInfoVerifierUT, shouldReturnRootCaMissingWhenTcbSigningChainVerifyFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_SGX_ROOT_CA_MISSING));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_MISSING, result);
}

TEST_F(TCBInfoVerifierUT, shouldReturnInfoInvalidSignatureWhenCheckSha256EcdsaSignatureFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(false));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_TCB_INFO_INVALID_SIGNATURE, result);
}

TEST_F(TCBInfoVerifierUT, shouldReturnStatusSigningCertChainExpiredWhenRootCaExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    StrictMock<test::ValidityMock> validityMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillOnce(Return(currentTime - 1));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED, result);
}

TEST_F(TCBInfoVerifierUT, shouldReturnStatusSigningCertChainExpiredWhenSigningCertExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    StrictMock<test::ValidityMock> validityMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillOnce(Return(currentTime)).WillOnce(Return(currentTime - 1));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED, result);
}

TEST_F(TCBInfoVerifierUT, shouldReturnStatusCrlExpiredWhenRootCaCrlExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    StrictMock<test::ValidityMock> validityMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillRepeatedly(Return(currentTime));
    EXPECT_CALL(crlStoreMock, expired(_)).WillOnce(Return(true));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_EXPIRED, result);
}

TEST_F(TCBInfoVerifierUT, shouldReturnStatusTcbInfoExpiredWhenTcbInfoExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    StrictMock<test::ValidityMock> validityMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillRepeatedly(Return(currentTime));
    EXPECT_CALL(crlStoreMock, expired(_)).WillOnce(Return(false));

    TCBInfoVerifier tcbInfoVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = tcbInfoVerifier.verify(tcbInfo, certificateChainMock, crlStoreMock, certStoreMock, 1529584563);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_INFO_EXPIRED, result);
}
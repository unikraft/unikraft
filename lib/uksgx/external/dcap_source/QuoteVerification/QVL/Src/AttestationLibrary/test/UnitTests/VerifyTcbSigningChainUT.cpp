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
#include <Verifiers/TCBSigningChain.h>
#include <Mocks/CertCrlStoresMocks.h>
#include <Mocks/CommonVerifierMock.h>
#include <Mocks/PckCrlVerifierMock.h>
#include <CertVerification/X509Constants.h>

using namespace testing;
using namespace intel::sgx::dcap;
using namespace intel::sgx;

struct VerifyTcbSigningChainUT : public Test
{
    std::vector<dcap::parser::x509::Extension> extensions;
    std::vector<uint8_t> emptySignature = {};
    StrictMock<test::SignatureMock> signatureMock;
    const time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
};

TEST_F(VerifyTcbSigningChainUT, shouldReturnedStatusOkWhenVerifyPassPositive)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const dcap::parser::x509::Certificate&>(), An<const dcap::parser::x509::Certificate&>()))
            .WillOnce(Return(true));

    EXPECT_CALL(*pckCrlVerifierMock, verify(_, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(crlStoreMock, isRevoked(_)).WillOnce(Return(false));

    EXPECT_CALL(signatureMock, getRawDer()).WillRepeatedly(ReturnRef(emptySignature));

    EXPECT_CALL(trustedRootMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(trustedRootMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(trustedRootMock, getSignature()).WillOnce(ReturnRef(signatureMock));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(*rootCertMock, getSignature()).WillOnce(ReturnRef(signatureMock));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));
    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTcbSigningCertChainUntrustedWhenSignatureIsWrong)
{
    // GIVEN
    dcap::parser::x509::Signature wrongSignature({1}, {}, {});
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const dcap::parser::x509::Certificate&>(), An<const dcap::parser::x509::Certificate&>()))
            .WillOnce(Return(true));

    EXPECT_CALL(*pckCrlVerifierMock, verify(_, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(crlStoreMock, isRevoked(_)).WillOnce(Return(false));

    EXPECT_CALL(signatureMock, getRawDer()).WillRepeatedly(ReturnRef(emptySignature));

    EXPECT_CALL(trustedRootMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(trustedRootMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(trustedRootMock, getSignature()).WillOnce(ReturnRef(signatureMock));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(*rootCertMock, getSignature()).WillOnce(ReturnRef(wrongSignature));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));
    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_SIGNING_CERT_CHAIN_UNTRUSTED, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTrustedRootCaInvalidWhenTrustedRootIsNotSelfSigned) {
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const dcap::parser::x509::Certificate&>(), An<const dcap::parser::x509::Certificate&>()))
            .WillOnce(Return(true));

    EXPECT_CALL(*pckCrlVerifierMock, verify(_, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(crlStoreMock, isRevoked(_)).WillOnce(Return(false));

    EXPECT_CALL(trustedRootMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(trustedRootMock, getIssuer()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));
    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_TRUSTED_ROOT_CA_INVALID, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTcbSigningCertRevokedWhenRootCertIsRevoked) {
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const dcap::parser::x509::Certificate&>(), An<const dcap::parser::x509::Certificate&>()))
            .WillOnce(Return(true));

    EXPECT_CALL(*pckCrlVerifierMock, verify(_, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(crlStoreMock, isRevoked(_)).WillOnce(Return(true));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_SIGNING_CERT_REVOKED, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTcbSigningCertMissingWhenCrlVerifyFail) {
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const dcap::parser::x509::Certificate&>(), An<const dcap::parser::x509::Certificate&>()))
            .WillOnce(Return(true));

    EXPECT_CALL(*pckCrlVerifierMock, verify(_, _)).WillOnce(Return(STATUS_SGX_TCB_SIGNING_CERT_MISSING));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_SIGNING_CERT_MISSING, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTcbSigningCertInvalidIssuerWhenIssuerIsWrong) {
// GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_SIGNING_CERT_INVALID_ISSUER, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTcbSigningCertInvalidIssuerWhenSignatureIsWrong) {
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const dcap::parser::x509::Certificate&>(), An<const dcap::parser::x509::Certificate&>()))
            .WillOnce(Return(false));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(tcbSigningCertMock));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::TCB_SUBJECT));

    EXPECT_CALL(*tcbSigningCertMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_SIGNING_CERT_INVALID_ISSUER, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedTcbSigningCertMissingWhenTcbCertIsNotPresent) {
// GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillRepeatedly(Return(nullptr));

    EXPECT_CALL(*rootCertMock, getSubject())
            .WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_TCB_SIGNING_CERT_MISSING, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedRootCaMissingWhenVerifyRootCACertFail) {
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_SGX_ROOT_CA_MISSING));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(rootCertMock));

    EXPECT_CALL(*rootCertMock, getSubject()).WillRepeatedly(ReturnRef(constants::ROOT_CA_SUBJECT));

    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_MISSING, result);
}

TEST_F(VerifyTcbSigningChainUT, shouldReturnedRootCaMissingWhenRootCertIsNotPresent) {
// GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> trustedRootMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto rootCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();
    auto tcbSigningCertMock = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, getRootCert()).WillRepeatedly(Return(nullptr));
    TCBSigningChain tcbSigningChain(std::move(commonVerifierMock), std::move(pckCrlVerifierMock));

    // WHEN
    auto result = tcbSigningChain.verify(certificateChainMock, crlStoreMock, trustedRootMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_MISSING, result);
}
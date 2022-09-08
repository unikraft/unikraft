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
#include <Verifiers/PckCrlVerifier.h>
#include <Mocks/CertCrlStoresMocks.h>
#include <Mocks/CommonVerifierMock.h>

#include <CertVerification/X509Constants.h>
#include <PckParser/PckParser.h>

using namespace testing;
using namespace intel::sgx::dcap;
using namespace intel::sgx;


struct VerifyPckCrlUT : public Test
{
    std::vector<pckparser::Extension> extensions;
    dcap::parser::x509::Signature signature;
};

TEST_F(VerifyPckCrlUT, shouldReturnedStatusOkWhenVerifyWithoutCertChainPassPositive)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::PckCertificateMock> certMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(certMock, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(true));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const pckparser::CrlStore&>(), An<const dcap::parser::x509::Certificate&>())).WillOnce(Return(true));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certMock);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCrlInvalidSignatureWhenSignatureIsInvalid)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(true));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const pckparser::CrlStore&>(), An<const dcap::parser::x509::Certificate&>())).WillOnce(Return(false));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_INVALID_SIGNATURE, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCrlInvalidExtensionWhenExtensionIsInvalid)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(false));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_INVALID_EXTENSIONS, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCrlUnkownIssuerWhenIssuerIsWrong)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER));

    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNKNOWN_ISSUER, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedStatusOkWhenVerifyWithCertChainPassPositive)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(crlStoreMock, getIssuer())
             .WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER))
             .WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));
    EXPECT_CALL(*certStoreMockPtr, getSignature()).WillOnce(ReturnRef(signature));

    EXPECT_CALL(certStoreMock, getSignature()).WillOnce(ReturnRef(signature));
    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillRepeatedly(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(true));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const pckparser::CrlStore&>(), An<const dcap::parser::x509::Certificate&>())).WillOnce(Return(true));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedRootCaUntrustedWhenSignatureIsWrong)
{
    // GIVEN
    dcap::parser::x509::Signature wrongSignature({1}, {}, {});
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));
    EXPECT_CALL(crlStoreMock, getIssuer())
            .WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER))
            .WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));
    EXPECT_CALL(*certStoreMockPtr, getSignature()).WillOnce(ReturnRef(signature));

    EXPECT_CALL(certStoreMock, getSignature()).WillOnce(ReturnRef(wrongSignature));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillRepeatedly(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(true));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const pckparser::CrlStore&>(), An<const dcap::parser::x509::Certificate&>())).WillOnce(Return(true));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_UNTRUSTED, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedRootCaMissingWhenRootCertIsNotPresent)
{
    // GIVEN

    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();
    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(ReturnNull());

    EXPECT_CALL(crlStoreMock, getIssuer())
            .WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER))
            .WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillRepeatedly(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(true));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const pckparser::CrlStore&>(), An<const dcap::parser::x509::Certificate&>())).WillOnce(Return(true));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_MISSING, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedTrustedRootCaInvalidWhenVerifyRootCACertFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certStoreMock, getSubject()).WillOnce(ReturnRef(constants::ROOT_CA_SUBJECT));

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(crlStoreMock, getIssuer())
            .WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER))
            .WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));
    EXPECT_CALL(crlStoreMock, getExtensions()).WillOnce(ReturnRef(extensions));

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_))
           .WillOnce(Return(STATUS_OK))
           .WillOnce(Return(STATUS_TRUSTED_ROOT_CA_INVALID));
    EXPECT_CALL(*commonVerifierMock, checkStandardExtensions(std::vector<pckparser::Extension>(), _)).WillOnce(Return(true));
    EXPECT_CALL(*commonVerifierMock, checkSignature(
            An<const pckparser::CrlStore&>(), An<const dcap::parser::x509::Certificate&>())).WillOnce(Return(true));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_TRUSTED_ROOT_CA_INVALID, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCrlUnkonwIssuerWhenVerifyWithoutChainFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(crlStoreMock, getIssuer())
            .WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER))
            .WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER));

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillOnce(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNKNOWN_ISSUER, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCrlUnkonwIssuerWhenTomposCertIsNotPresent)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(ReturnNull());

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNKNOWN_ISSUER, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedRootCaInvalidWhenVerifyCRLIssuerCertChainFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(ReturnNull());

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verify(crlStoreMock, certificateChainMock, certStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_INVALID, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedStatusOkWhenVerifyCRLIssuerCertChainPassWithoutError)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(1));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedStatusOkWhenVerifyIntermediateSuccess)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillRepeatedly(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(2));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillRepeatedly(ReturnRef(constants::PCK_PROCESSOR_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, verifyIntermediate(_, _)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedRootCaInvalidWhenRootCertChainIsNotPresent)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(ReturnNull());

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_INVALID, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCaCertInvalidWhenVerifyRootCACertFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_SGX_CA_CERT_INVALID));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CA_CERT_INVALID, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedUnsuportedFormatWhenRootCaHasIncorrectSize)
{
    // GIVEN
    auto chainSizeTooLarge = 2;
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(chainSizeTooLarge));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::ROOT_CA_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CA_CERT_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedUnsuportedFormatWhenPckPlatformHasIncorrectSize)
{
    // GIVEN
    auto chainSizeTooLarge = 3;
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(chainSizeTooLarge));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillRepeatedly(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CA_CERT_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedUnsuportedFormatWhenPckProcesorHasIncorrectSize)
{
    // GIVEN
    auto chainSizeTooLarge = 3;
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(chainSizeTooLarge));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillRepeatedly(ReturnRef(constants::PCK_PROCESSOR_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CA_CERT_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedIntermediateCaMissingWhenPckProcesorHasIncorrectSize)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(2));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(ReturnNull());

    EXPECT_CALL(crlStoreMock, getIssuer()).WillRepeatedly(ReturnRef(constants::PCK_PROCESSOR_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_INTERMEDIATE_CA_MISSING, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCaCertInvalidWhenVerifyIntermediateFail)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(*certStoreMockPtr, getSubject()).WillRepeatedly(ReturnRef(constants::PLATFORM_CA_SUBJECT));

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, length()).WillOnce(Return(2));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillRepeatedly(ReturnRef(constants::PCK_PROCESSOR_CRL_ISSUER));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*commonVerifierMock, verifyIntermediate(_, _)).WillOnce(Return(STATUS_SGX_CA_CERT_INVALID));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CA_CERT_INVALID, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnedCrlUnknownIssuerWhenIssuerIsUnknown)
{
    // GIVEN
    const pckparser::Issuer unknownIssuer;
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(certificateChainMock, get(_)).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(crlStoreMock, getIssuer()).WillRepeatedly(ReturnRef(unknownIssuer));

    EXPECT_CALL(*commonVerifierMock, verifyRootCACert(_)).WillOnce(Return(STATUS_OK));

    PckCrlVerifier pckCrlVerifier(std::move(commonVerifierMock));

    // WHEN
    auto result = pckCrlVerifier.verifyCRLIssuerCertChain(certificateChainMock, crlStoreMock);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNKNOWN_ISSUER, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnTrueWhenPckPlatformIsPresent)
{
    // GIVEN
    const pckparser::Issuer unknownIssuer;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::PCK_PLATFORM_CRL_ISSUER));

    PckCrlVerifier pckCrlVerifier;

    // WHEN
    auto result = pckCrlVerifier.checkIssuer(crlStoreMock);

    // THEN
    EXPECT_EQ(true, result);
}

TEST_F(VerifyPckCrlUT, shouldReturnTrueWhenPckProcesorIsPresent)
{
    // GIVEN
    const pckparser::Issuer unknownIssuer;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();

    EXPECT_CALL(crlStoreMock, getIssuer()).WillOnce(ReturnRef(constants::PCK_PROCESSOR_CRL_ISSUER));

    PckCrlVerifier pckCrlVerifier;

    // WHEN
    auto result = pckCrlVerifier.checkIssuer(crlStoreMock);

    // THEN
    EXPECT_EQ(true, result);
}
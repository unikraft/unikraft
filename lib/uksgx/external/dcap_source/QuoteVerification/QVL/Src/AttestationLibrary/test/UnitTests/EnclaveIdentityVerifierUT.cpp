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
#include <Verifiers/EnclaveIdentityVerifier.h>
#include <Mocks/CertCrlStoresMocks.h>
#include <Mocks/TcbSigningChainMock.h>
#include <Mocks/CommonVerifierMock.h>
#include <Mocks/PckCrlVerifierMock.h>
#include <Mocks/EnclaveIdentityMock.h>
#include <CertVerification/X509Constants.h>
#include <Verifiers/EnclaveIdentityV2.h>
#include "KeyHelpers.h"

using namespace testing;
using namespace intel::sgx::dcap;

struct EnclaveIdentityVerifierUT : public Test
{
    NiceMock<test::EnclaveIdentityMock> enclaveIdentity;

    std::vector<uint8_t> pubKey = {};
    const time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
};

TEST_F(EnclaveIdentityVerifierUT, shouldReturnStatusOkWhenVerifyPassPositive)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();
    StrictMock<test::ValidityMock> validityMock;

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillRepeatedly(Return(currentTime));
    EXPECT_CALL(crlStoreMock, expired(currentTime - 86400)).WillOnce(Return(false));

    EXPECT_CALL(enclaveIdentity, getNextUpdate()).WillOnce(Return(currentTime));

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime - 86400);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(EnclaveIdentityVerifierUT, shouldReturnRootCaMissingWhenTcbSigningChainVerifyFail)
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

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_MISSING, result);
}

TEST_F(EnclaveIdentityVerifierUT, shouldReturnQeidentityInvalidSignatureWhenCheckSha256EcdsaSignatureFail)
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

    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID_SIGNATURE, result);
}

TEST_F(EnclaveIdentityVerifierUT, shouldReturnStatusCertChainExpiredWhenRootCertExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();
    StrictMock<test::ValidityMock> validityMock;

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillOnce(Return(currentTime - 1));

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED, result);
}

TEST_F(EnclaveIdentityVerifierUT, shouldReturnStatusCertChainExpiredWhenTcbSiningCertExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();
    StrictMock<test::ValidityMock> validityMock;

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillOnce(Return(currentTime)).WillOnce(Return(currentTime - 1));

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED, result);
}

TEST_F(EnclaveIdentityVerifierUT, shouldReturnStatusCrlExpiredWhenRootCrlExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();
    StrictMock<test::ValidityMock> validityMock;

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillRepeatedly(Return(currentTime));
    EXPECT_CALL(crlStoreMock, expired(currentTime)).WillOnce(Return(true));

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_EXPIRED, result);
}

TEST_F(EnclaveIdentityVerifierUT, shouldReturnStatusEnclaveIdentityExpiredWhenNextUpdateExpired)
{
    // GIVEN
    auto commonVerifierMock = std::make_unique<StrictMock<test::CommonVerifierMock>>();
    auto pckCrlVerifierMock = std::make_unique<StrictMock<test::PckCrlVerifierMock>>();
    auto tcbSigningChainMock = std::make_unique<StrictMock<test::TcbSigningChainMock>>();
    StrictMock<test::CertificateChainMock> certificateChainMock;
    StrictMock<test::PckCertificateMock> certStoreMock;
    StrictMock<test::CrlStoreMock> crlStoreMock;
    auto certStoreMockPtr = std::make_shared<StrictMock<test::PckCertificateMock>>();
    StrictMock<test::ValidityMock> validityMock;

    EXPECT_CALL(*commonVerifierMock, checkSha256EcdsaSignature(_, _, _)).WillOnce(Return(true));

    EXPECT_CALL(*tcbSigningChainMock, verify(_, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_CALL(certificateChainMock, getRootCert()).WillOnce(Return(certStoreMockPtr));
    EXPECT_CALL(certificateChainMock, getTopmostCert()).WillOnce(Return(certStoreMockPtr));

    EXPECT_CALL(*certStoreMockPtr, getPubKey()).WillOnce(ReturnRef(pubKey));
    EXPECT_CALL(*certStoreMockPtr, getValidity()).WillRepeatedly(ReturnRef(validityMock));

    EXPECT_CALL(validityMock, getNotAfterTime()).WillRepeatedly(Return(currentTime + 1));
    EXPECT_CALL(crlStoreMock, expired(currentTime + 1)).WillOnce(Return(false));

    EXPECT_CALL(enclaveIdentity, getNextUpdate()).WillOnce(Return(currentTime));

    EnclaveIdentityVerifier enclaveIdentityVerifier(std::move(commonVerifierMock), std::move(tcbSigningChainMock));

    // WHEN
    auto result = enclaveIdentityVerifier.verify(enclaveIdentity, certificateChainMock, crlStoreMock, certStoreMock, currentTime + 1);

    // THEN
    EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_EXPIRED, result);
}
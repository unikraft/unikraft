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
#include <CertVerification/X509Constants.h>
#include "X509CertGenerator.h"
#include "X509CrlGenerator.h"

using namespace testing;
using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::test;
using namespace intel::sgx::dcap::parser::test;
using namespace std;

struct VerifyPCKRevocationListIT : public Test
{
    X509CertGenerator certGenerator{};
    X509CrlGenerator crlGenerator{};

    Bytes rootSerial {0x00, 0x45};
    Bytes intermediateSerial {0x01, 0x01};

    crypto::X509_uptr rootCaCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr intermediateCaCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_CRL_uptr rootCaCrl = crypto::make_unique<X509_CRL>(nullptr);
    crypto::X509_CRL_uptr intermediateCaCrl = crypto::make_unique<X509_CRL>(nullptr);

    crypto::EVP_PKEY_uptr rootKeys = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr intKeys = crypto::make_unique<EVP_PKEY>(nullptr);

    int timeNow = 0;
    int timeOneHour = 3600;

    VerifyPCKRevocationListIT()
    {
        rootKeys = certGenerator.generateEcKeypair();
        intKeys = certGenerator.generateEcKeypair();

        rootCaCert = certGenerator.generateCaCert(2, rootSerial, timeNow, timeOneHour, rootKeys.get(), rootKeys.get(),
                                                       constants::ROOT_CA_SUBJECT, constants::ROOT_CA_SUBJECT);
        intermediateCaCert = certGenerator.generateCaCert(2, intermediateSerial, timeNow, timeOneHour, intKeys.get(), rootKeys.get(),
                                                               constants::PROCESSOR_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        rootCaCrl = crlGenerator.generateCRL(CRLVersion::CRL_VERSION_2, timeNow, timeOneHour, rootCaCert, std::vector<Bytes>{});
        intermediateCaCrl = crlGenerator.generateCRL(CRLVersion::CRL_VERSION_2, timeNow, timeOneHour, intermediateCaCert, std::vector<Bytes>{});
    }
};

TEST_F(VerifyPCKRevocationListIT, shouldVerifyRootCaCrlPositiveWhenCrlIsPem)
{
    const auto rootCaCertPEM = certGenerator.x509ToString(rootCaCert.get());
    const auto rootCaCrlPEM = X509CrlGenerator::x509CrlToPEMString(rootCaCrl.get());

    EXPECT_EQ(STATUS_OK,
            sgxAttestationVerifyPCKRevocationList(rootCaCrlPEM.c_str(), rootCaCertPEM.c_str(), rootCaCertPEM.c_str()));
}

TEST_F(VerifyPCKRevocationListIT, shouldVerifyRootCaCrlPositiveWhenCrlIsDer)
{
    const auto rootCaCertPEM = certGenerator.x509ToString(rootCaCert.get());
    const auto rootCaCrlDER = X509CrlGenerator::x509CrlToDERString(rootCaCrl.get());

    EXPECT_EQ(STATUS_OK,
              sgxAttestationVerifyPCKRevocationList(rootCaCrlDER.c_str(), rootCaCertPEM.c_str(), rootCaCertPEM.c_str()));
}

TEST_F(VerifyPCKRevocationListIT, shouldVerifyIntermediateCaCrlPositiveWhenCrlIsPem)
{
    const auto rootCaCertPEM = certGenerator.x509ToString(rootCaCert.get());
    const auto intermediateCaCertPEM = certGenerator.x509ToString(intermediateCaCert.get());
    const auto intermediateCaCrlPEM = X509CrlGenerator::x509CrlToPEMString(intermediateCaCrl.get());
    const auto certChain = rootCaCertPEM + intermediateCaCertPEM;

    EXPECT_EQ(STATUS_OK,
              sgxAttestationVerifyPCKRevocationList(intermediateCaCrlPEM.c_str(), certChain.c_str(), rootCaCertPEM.c_str()));
}

TEST_F(VerifyPCKRevocationListIT, shouldVerifyIntermediateCaCrlPositiveWhenCrlIsDer)
{
    const auto rootCaCertPEM = certGenerator.x509ToString(rootCaCert.get());
    const auto intermediateCaCertPEM = certGenerator.x509ToString(intermediateCaCert.get());
    const auto intermediateCaCrlDER = X509CrlGenerator::x509CrlToDERString(intermediateCaCrl.get());
    const auto certChain = rootCaCertPEM + intermediateCaCertPEM;

    EXPECT_EQ(STATUS_OK,
              sgxAttestationVerifyPCKRevocationList(intermediateCaCrlDER.c_str(), certChain.c_str(), rootCaCertPEM.c_str()));
}

TEST_F(VerifyPCKRevocationListIT, shouldReturnCrlUnsupportedFormatWhenInvalidInput)
{
    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT, sgxAttestationVerifyPCKRevocationList(nullptr, "str", "str"));
    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT, sgxAttestationVerifyPCKRevocationList("str", nullptr, "str"));
    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT, sgxAttestationVerifyPCKRevocationList("str", "str", nullptr));
}

TEST_F(VerifyPCKRevocationListIT, shouldReturnCrlUnsupportedFormatWhenInvalidCrl)
{
    const auto rootCaCertPEM = certGenerator.x509ToString(rootCaCert.get());
    const auto invalidRootCaCrlPEM = "Not a valid X509 CRL PEM";

    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT,
              sgxAttestationVerifyPCKRevocationList(invalidRootCaCrlPEM, rootCaCertPEM.c_str(), rootCaCertPEM.c_str()));
}

TEST_F(VerifyPCKRevocationListIT, shouldReturnCaCertUnsupportedFormatWhenInvalidCertChain)
{
    const auto trustedRootCaCert = certGenerator.x509ToString(rootCaCert.get());
    const auto invalidCertChainPEM = "No a valid X509 CERT CHAIN PEM";
    const auto rootCaCrlPEM = X509CrlGenerator::x509CrlToPEMString(rootCaCrl.get());

    EXPECT_EQ(STATUS_SGX_CA_CERT_UNSUPPORTED_FORMAT,
              sgxAttestationVerifyPCKRevocationList(rootCaCrlPEM.c_str(), invalidCertChainPEM, trustedRootCaCert.c_str()));
}

TEST_F(VerifyPCKRevocationListIT, shouldReturnTrustedRootCaCertUnsupportedFormatWhenInvalidTrustedRootCaCert)
{
    const auto certChain = certGenerator.x509ToString(rootCaCert.get());
    const auto invalidTrustedRootCaCert = "No a valid X509 CERT PEM";
    const auto rootCaCrlPEM = X509CrlGenerator::x509CrlToPEMString(rootCaCrl.get());

    EXPECT_EQ(STATUS_TRUSTED_ROOT_CA_UNSUPPORTED_FORMAT,
              sgxAttestationVerifyPCKRevocationList(rootCaCrlPEM.c_str(), certChain.c_str(), invalidTrustedRootCaCert));
}
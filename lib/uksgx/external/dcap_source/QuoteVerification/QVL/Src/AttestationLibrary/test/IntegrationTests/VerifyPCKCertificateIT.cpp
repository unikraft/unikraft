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

#include <SgxEcdsaAttestation/QuoteVerification.h>
#include <CertVerification/X509Constants.h>
#include <X509CertGenerator.h>
#include <X509CrlGenerator.h>
#include <array>

using namespace testing;
using namespace ::intel::sgx::dcap;
using namespace ::intel::sgx::dcap::test;
using namespace intel::sgx::dcap::parser::test;

struct VerifyPCKCertificateIT : public Test
{
    int timeNow = 0;
    int timeOneHour = 3600;

    X509CrlGenerator crlGenerator;
    Bytes sn {0x23, 0x45};
    Bytes ppid = Bytes(16, 0xaa);
    Bytes cpusvn = Bytes(16, 0x09);
    Bytes pcesvn = {0x03, 0xf2};
    Bytes pceId = {0x04, 0xf3};
    Bytes fmspc = {0x05, 0xf4, 0x44, 0x45, 0xaa, 0x00};
    X509CertGenerator certGenerator{};

    crypto::EVP_PKEY_uptr keyRoot = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr keyInt = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr key = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::X509_uptr rootCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr intCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr cert = crypto::make_unique<X509>(nullptr);

    VerifyPCKCertificateIT()
    {
        keyRoot = certGenerator.generateEcKeypair();
        keyInt = certGenerator.generateEcKeypair();
        key = certGenerator.generateEcKeypair();
        rootCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, keyRoot.get(), keyRoot.get(),
                                                     constants::ROOT_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        intCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, keyInt.get(), keyRoot.get(),
                                                    constants::PLATFORM_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        cert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                                  constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                                  ppid, cpusvn, pcesvn, pceId, fmspc, 0);
    }

    std::string getValidPemCrl(const crypto::X509_uptr &ucert)
    {
        auto revokedList = std::vector<Bytes>{{0x12, 0x10, 0x13, 0x11}, {0x11, 0x33, 0xff, 0x56}};
        auto rootCaCRL = crlGenerator.generateCRL(CRLVersion::CRL_VERSION_2, 0, 3600, ucert, revokedList);

        return X509CrlGenerator::x509CrlToPEMString(rootCaCRL.get());
    }

    std::string getValidDerCrl(const crypto::X509_uptr &ucert)
    {
        auto revokedList = std::vector<Bytes>{{0x12, 0x10, 0x13, 0x11}, {0x11, 0x33, 0xff, 0x56}};
        auto rootCaCRL = crlGenerator.generateCRL(CRLVersion::CRL_VERSION_2, 0, 3600, ucert, revokedList);

        return X509CrlGenerator::x509CrlToDERString(rootCaCRL.get());
    }
};

TEST_F(VerifyPCKCertificateIT, shouldReturnedStatusOkWhenPassingArgumnetsAreValidCrlAsPem)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    auto rootCaCrl = getValidPemCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedStatusOkWhenPassingArgumnetsAreValidCrlAsDer)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    auto rootCaCrl = getValidDerCrl(rootCert);
    auto intermediateCaCrl = getValidDerCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedStatusOkWhenPassingArgumnetsAreValidCrlFormatsMixed)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    auto rootCaCrl = getValidDerCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedTrustedrootCaUnsuportedFormatWhenRootCaCertDerIsWrong)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;
    auto rootCertPemWrong = "rootCertPemWrong";

    auto rootCaCrl = getValidPemCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPemWrong, nullptr);

    // THEN
    EXPECT_EQ(STATUS_TRUSTED_ROOT_CA_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedCrlUnsuportedFormatWhenRootCaCrlIsWrong)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;
    auto rotCaCrlWrong = "rootCertPemWrong";

    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rotCaCrlWrong, intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedCrlUnsuportedFormatWhenIntermediateCrlIsWrong)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;
    auto intermediateWrong = "intermediateWrong";

    auto rootCaCrl = getValidPemCrl(rootCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateWrong}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedCrlUnsuportedFormatWhenIntermediateCrlPEMIsWrong)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;
    auto intermediateWrong = "-----BEGIN X509 CRL-----\n intermediateWrong";

    auto rootCaCrl = getValidPemCrl(rootCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateWrong}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_SGX_CRL_UNSUPPORTED_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUnsuportedCertFormatWhenCertChainParseFail)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto certChain = "wrongCertChain";

    auto rootCaCrl = getValidPemCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain, crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUnsuportedCertFormatWhenCertChainLengthIsIncorrect)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto certChainWithWrongSize = rootCertPem  + intPem ;

    auto rootCaCrl = getValidPemCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChainWithWrongSize.c_str(), crls.data(), rootCertPem.c_str(),
                                                     nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUnsuportedCertFormatWhenCertChainIsNull)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChainWithWrongSize = rootCertPem  + intPem ;

    auto rootCaCrl = getValidPemCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(nullptr, crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUsuportedCertFormatWhenRootCaCrlIsNull)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{nullptr, intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUsuportedCertFormatWhenIntermediateIsNull)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    auto rootCaCrl = getValidPemCrl(rootCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), nullptr}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUsuportedCertFormatWhenCrlsIsNull)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), nullptr, rootCertPem.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}

TEST_F(VerifyPCKCertificateIT, shouldReturnedUsuportedCertFormatWhenRootCertIsNull)
{
    // GIVEN
    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(intCert.get());
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto certChain = rootCertPem  + intPem + pckPem;

    auto rootCaCrl = getValidPemCrl(rootCert);
    auto intermediateCaCrl = getValidPemCrl(intCert);

    const std::array<const char*, 2> crls{{rootCaCrl.data(), intermediateCaCrl.data()}};

    // WHEN
    auto result = sgxAttestationVerifyPCKCertificate(certChain.c_str(), crls.data(), nullptr, nullptr);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, result);
}
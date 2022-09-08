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

#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "X509TestConstants.h"
#include "X509CertGenerator.h"

#include <gtest/gtest.h>

#include <gmock/gmock-matchers.h>

using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::parser;
using namespace ::testing;


struct CertificateUT: public testing::Test {
    int timeNow = 0;
    int timeOneHour = 3600;

    Bytes sn { 0x40, 0x66, 0xB0, 0x01, 0x4B, 0x71, 0x7C, 0xF7, 0x01, 0xD5,
               0xB7, 0xD8, 0xF1, 0x36, 0xB1, 0x99, 0xE9, 0x73, 0x96, 0xC8 };
    Bytes ppid = Bytes(16, 0xaa);
    Bytes cpusvn = Bytes(16, 0x09);
    Bytes pcesvn = {0x03, 0xf2};
    Bytes pceId = {0x04, 0xf3};
    Bytes fmspc = {0x05, 0xf4, 0x44, 0x45, 0xaa, 0x00};
    test::X509CertGenerator certGenerator;

    crypto::EVP_PKEY_uptr keyRoot = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr keyInt = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr key = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::X509_uptr rootCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr intCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr cert = crypto::make_unique<X509>(nullptr);

    std::string pemPckCert, pemIntCert, pemRootCert;

    CertificateUT()
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

        pemPckCert = certGenerator.x509ToString(cert.get());
        pemIntCert = certGenerator.x509ToString(intCert.get());
        pemRootCert = certGenerator.x509ToString(rootCert.get());
    }
};

TEST_F(CertificateUT, certificateParse)
{
    ASSERT_NO_THROW(x509::Certificate::parse(pemPckCert));
    ASSERT_NO_THROW(x509::Certificate::parse(pemIntCert));
    ASSERT_NO_THROW(x509::Certificate::parse(pemRootCert));
}

TEST_F(CertificateUT, certificateConstructors)
{
    const auto& certificate = x509::Certificate::parse(pemPckCert);
    const auto& copyCertificate = certificate;

    ASSERT_EQ(copyCertificate.getVersion(), certificate.getVersion());
    ASSERT_EQ(copyCertificate.getSerialNumber(), certificate.getSerialNumber());
    ASSERT_EQ(copyCertificate.getSubject(), certificate.getSubject());
    ASSERT_EQ(copyCertificate.getIssuer(), certificate.getIssuer());
    ASSERT_EQ(copyCertificate.getValidity(), certificate.getValidity());
    ASSERT_EQ(copyCertificate.getExtensions(), certificate.getExtensions());
    ASSERT_EQ(copyCertificate.getSignature(), certificate.getSignature());
    ASSERT_EQ(copyCertificate.getPubKey(), certificate.getPubKey());
    ASSERT_EQ(copyCertificate.getInfo(), certificate.getInfo());
    ASSERT_EQ(copyCertificate.getPem(), certificate.getPem());
}

TEST_F(CertificateUT, certificateGetters)
{
    const auto& certificate = x509::Certificate::parse(pemIntCert);

    ASSERT_EQ(certificate.getVersion(), 3);
    ASSERT_THAT(certificate.getSerialNumber(), ElementsAreArray(sn));

    auto ecKey = crypto::make_unique(EVP_PKEY_get1_EC_KEY(keyInt.get()));
    uint8_t *pubKey = nullptr;
    auto pKeyLen = EC_KEY_key2buf(ecKey.get(), EC_KEY_get_conv_form(ecKey.get()), &pubKey, NULL);
    std::vector<uint8_t> expectedPublicKey { pubKey, pubKey + pKeyLen };

    ASSERT_THAT(certificate.getPubKey(), ElementsAreArray(expectedPublicKey));
    ASSERT_EQ(certificate.getIssuer(), constants::ROOT_CA_SUBJECT);
    ASSERT_EQ(certificate.getSubject(), constants::PLATFORM_CA_SUBJECT);
    ASSERT_NE(certificate.getIssuer(), certificate.getSubject());
    ASSERT_EQ(certificate.getPem(), pemIntCert);

    ASSERT_LT(certificate.getValidity().getNotBeforeTime(), certificate.getValidity().getNotAfterTime());

    const std::vector<x509::Extension> expectedExtensions = constants::REQUIRED_X509_EXTENSIONS;
    ASSERT_THAT(certificate.getExtensions().size(), expectedExtensions.size());

    free(pubKey);
}

TEST_F(CertificateUT, certificateOperators)
{
    const auto& certificate1 = x509::Certificate::parse(pemIntCert);
    const auto& certificate2 = x509::Certificate::parse(pemIntCert);
    const auto& certificate3 = x509::Certificate::parse(pemPckCert);
    const auto& certificate4 = x509::Certificate::parse(pemRootCert);

    ASSERT_EQ(certificate1, certificate2);
    ASSERT_FALSE(certificate1 == certificate3);
    ASSERT_FALSE(certificate1 == certificate4);
    ASSERT_FALSE(certificate3 == certificate4);
}


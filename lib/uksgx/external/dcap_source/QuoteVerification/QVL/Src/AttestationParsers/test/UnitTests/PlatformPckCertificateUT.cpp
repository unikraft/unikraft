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
#include <gmock/gmock-matchers.h>

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "X509TestConstants.h"
#include "X509CertGenerator.h"

using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::parser;
using namespace ::testing;


struct PlatformPckCertificateUT: public testing::Test {
    int timeNow = 0;
    int timeOneHour = 3600;

    Bytes sn { 0x40, 0x66, 0xB0, 0x01, 0x4B, 0x71, 0x7C, 0xF7, 0x01, 0xD5,
               0xB7, 0xD8, 0xF1, 0x36, 0xB1, 0x99, 0xE9, 0x73, 0x96, 0xC8 };
    Bytes ppid = Bytes(16, 0xaa);
    Bytes cpusvn = Bytes(16, 0x09);
    Bytes pcesvn = {0x03, 0xf2};
    Bytes pceId = {0x04, 0xf3};
    Bytes fmspc = {0x05, 0xf4, 0x44, 0x45, 0xaa, 0x00};
    Bytes platformInstanceId = {0x0A, 0xBB, 0xFF, 0x05, 0xf4, 0x44, 0xB0, 0x01,
                                0x4B, 0x71, 0xB1, 0x99, 0xE9, 0xE9, 0x73, 0x96};
    test::X509CertGenerator certGenerator;

    crypto::EVP_PKEY_uptr keyRoot = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr keyInt = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr key = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::X509_uptr rootCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr intCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr processorCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr platformCert = crypto::make_unique<X509>(nullptr);

    std::string pemProcessorPckCert, pemPlatformPckCert, pemIntCert, pemRootCert;

    PlatformPckCertificateUT()
    {
        keyRoot = certGenerator.generateEcKeypair();
        keyInt = certGenerator.generateEcKeypair();
        key = certGenerator.generateEcKeypair();
        rootCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, keyRoot.get(), keyRoot.get(),
                                                constants::ROOT_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        intCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, keyInt.get(), keyRoot.get(),
                                               constants::PLATFORM_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        processorCert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                                      constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                                      ppid, cpusvn, pcesvn, pceId, fmspc, 0);
        platformCert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                                      constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                                      ppid, cpusvn, pcesvn, pceId, fmspc, 1, platformInstanceId,
                                                      true, true, true);

        pemProcessorPckCert = certGenerator.x509ToString(processorCert.get());
        pemPlatformPckCert = certGenerator.x509ToString(platformCert.get());
        pemIntCert = certGenerator.x509ToString(intCert.get());
        pemRootCert = certGenerator.x509ToString(rootCert.get());
    }
};

TEST_F(PlatformPckCertificateUT, pckCertificateParse)
{
    ASSERT_NO_THROW(x509::PlatformPckCertificate::parse(pemPlatformPckCert));
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::PlatformPckCertificate::parse(pemProcessorPckCert), InvalidExtensionException);
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::PlatformPckCertificate::parse(pemIntCert), InvalidExtensionException);
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::PlatformPckCertificate::parse(pemRootCert), InvalidExtensionException);
}

TEST_F(PlatformPckCertificateUT, pckCertificateConstructors)
{
    const auto& certificate = x509::Certificate::parse(pemPlatformPckCert);
    const auto& pckCertificate = x509::PckCertificate::parse(pemPlatformPckCert);
    const auto& platformPckCertificateFromCert = x509::PlatformPckCertificate(certificate);
    const auto& platformPckCertificateFromPckCert = x509::PlatformPckCertificate(pckCertificate);
    const auto& platformPckCertificateFromPEM = x509::PlatformPckCertificate::parse(pemPlatformPckCert);

    ASSERT_EQ(platformPckCertificateFromCert.getVersion(), platformPckCertificateFromPEM.getVersion());
    ASSERT_EQ(platformPckCertificateFromCert.getSerialNumber(), platformPckCertificateFromPEM.getSerialNumber());
    ASSERT_EQ(platformPckCertificateFromCert.getSubject(), platformPckCertificateFromPEM.getSubject());
    ASSERT_EQ(platformPckCertificateFromCert.getIssuer(), platformPckCertificateFromPEM.getIssuer());
    ASSERT_EQ(platformPckCertificateFromCert.getValidity(), platformPckCertificateFromPEM.getValidity());
    ASSERT_EQ(platformPckCertificateFromCert.getExtensions(), platformPckCertificateFromPEM.getExtensions());
    ASSERT_EQ(platformPckCertificateFromCert.getSignature(), platformPckCertificateFromPEM.getSignature());
    ASSERT_EQ(platformPckCertificateFromCert.getPubKey(), platformPckCertificateFromPEM.getPubKey());

    ASSERT_EQ(platformPckCertificateFromCert.getTcb(), platformPckCertificateFromPEM.getTcb());
    ASSERT_EQ(platformPckCertificateFromCert.getPpid(), platformPckCertificateFromPEM.getPpid());
    ASSERT_EQ(platformPckCertificateFromCert.getPceId(), platformPckCertificateFromPEM.getPceId());
    ASSERT_EQ(platformPckCertificateFromCert.getSgxType(), platformPckCertificateFromPEM.getSgxType());
    
    ASSERT_EQ(platformPckCertificateFromCert.getPlatformInstanceId(), platformPckCertificateFromPEM.getPlatformInstanceId());
    ASSERT_EQ(platformPckCertificateFromCert.getConfiguration(), platformPckCertificateFromPEM.getConfiguration());

    ASSERT_EQ(platformPckCertificateFromPckCert.getVersion(), platformPckCertificateFromPEM.getVersion());
    ASSERT_EQ(platformPckCertificateFromPckCert.getSerialNumber(), platformPckCertificateFromPEM.getSerialNumber());
    ASSERT_EQ(platformPckCertificateFromPckCert.getSubject(), platformPckCertificateFromPEM.getSubject());
    ASSERT_EQ(platformPckCertificateFromPckCert.getIssuer(), platformPckCertificateFromPEM.getIssuer());
    ASSERT_EQ(platformPckCertificateFromPckCert.getValidity(), platformPckCertificateFromPEM.getValidity());
    ASSERT_EQ(platformPckCertificateFromPckCert.getExtensions(), platformPckCertificateFromPEM.getExtensions());
    ASSERT_EQ(platformPckCertificateFromPckCert.getSignature(), platformPckCertificateFromPEM.getSignature());
    ASSERT_EQ(platformPckCertificateFromPckCert.getPubKey(), platformPckCertificateFromPEM.getPubKey());

    ASSERT_EQ(platformPckCertificateFromPckCert.getTcb(), platformPckCertificateFromPEM.getTcb());
    ASSERT_EQ(platformPckCertificateFromPckCert.getPpid(), platformPckCertificateFromPEM.getPpid());
    ASSERT_EQ(platformPckCertificateFromPckCert.getPceId(), platformPckCertificateFromPEM.getPceId());
    ASSERT_EQ(platformPckCertificateFromPckCert.getSgxType(), platformPckCertificateFromPEM.getSgxType());

    ASSERT_EQ(platformPckCertificateFromPckCert.getPlatformInstanceId(), platformPckCertificateFromPEM.getPlatformInstanceId());
    ASSERT_EQ(platformPckCertificateFromPckCert.getConfiguration(), platformPckCertificateFromPEM.getConfiguration());
}

TEST_F(PlatformPckCertificateUT, processorPckCertificateGetters)
{
    const auto& pckCertificate = x509::PlatformPckCertificate::parse(pemPlatformPckCert);

    ASSERT_EQ(pckCertificate.getVersion(), 3);
    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(sn));

    auto ecKey = crypto::make_unique(EVP_PKEY_get1_EC_KEY(key.get()));
    uint8_t *pubKey = nullptr;
    auto pKeyLen = EC_KEY_key2buf(ecKey.get(), EC_KEY_get_conv_form(ecKey.get()), &pubKey, NULL);
    std::vector<uint8_t> expectedPublicKey { pubKey, pubKey + pKeyLen };

    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expectedPublicKey));
    ASSERT_EQ(pckCertificate.getIssuer(), constants::PLATFORM_CA_SUBJECT);
    ASSERT_EQ(pckCertificate.getSubject(), constants::PCK_SUBJECT);
    ASSERT_NE(pckCertificate.getIssuer(), pckCertificate.getSubject()); // PCK certificate should not be self-signed

    ASSERT_LT(pckCertificate.getValidity().getNotBeforeTime(), pckCertificate.getValidity().getNotAfterTime());

    const std::vector<x509::Extension> expectedExtensions = constants::PCK_X509_EXTENSIONS;
    ASSERT_THAT(pckCertificate.getExtensions().size(), expectedExtensions.size());

    ASSERT_THAT(pckCertificate.getPpid(), ElementsAreArray(ppid));
    ASSERT_THAT(pckCertificate.getPceId(), ElementsAreArray(pceId));
    ASSERT_THAT(pckCertificate.getFmspc(), ElementsAreArray(fmspc));
    ASSERT_EQ(pckCertificate.getSgxType(), x509::SgxType::Scalable);

    const auto &tcb = x509::Tcb(cpusvn, cpusvn, 1010);
    ASSERT_THAT(pckCertificate.getTcb().getCpuSvn(), ElementsAreArray(tcb.getCpuSvn()));
    ASSERT_THAT(pckCertificate.getTcb().getSgxTcbComponents(), ElementsAreArray(tcb.getSgxTcbComponents()));
    ASSERT_EQ(pckCertificate.getTcb().getPceSvn(), tcb.getPceSvn());
    ASSERT_EQ(pckCertificate.getTcb(), tcb);

    ASSERT_THAT(pckCertificate.getPlatformInstanceId(), ElementsAreArray(platformInstanceId));
    const auto &configuration = x509::Configuration(true, true, true);
    ASSERT_EQ(pckCertificate.getConfiguration().isDynamicPlatform(), configuration.isDynamicPlatform());
    ASSERT_EQ(pckCertificate.getConfiguration().isCachedKeys(), configuration.isCachedKeys());
    ASSERT_EQ(pckCertificate.getConfiguration().isSmtEnabled(), configuration.isSmtEnabled());
    ASSERT_EQ(pckCertificate.getConfiguration(), configuration);

    free(pubKey);
}

TEST_F(PlatformPckCertificateUT, platformPckCertificateGetters)
{
    const auto& pckCertificate = x509::PlatformPckCertificate::parse(pemPlatformPckCert);

    ASSERT_EQ(pckCertificate.getVersion(), 3);
    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(sn));

    auto ecKey = crypto::make_unique(EVP_PKEY_get1_EC_KEY(key.get()));
    uint8_t *pubKey = nullptr;
    auto pKeyLen = EC_KEY_key2buf(ecKey.get(), EC_KEY_get_conv_form(ecKey.get()), &pubKey, NULL);
    std::vector<uint8_t> expectedPublicKey { pubKey, pubKey + pKeyLen };

    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expectedPublicKey));
    ASSERT_EQ(pckCertificate.getIssuer(), constants::PLATFORM_CA_SUBJECT);
    ASSERT_EQ(pckCertificate.getSubject(), constants::PCK_SUBJECT);
    ASSERT_NE(pckCertificate.getIssuer(), pckCertificate.getSubject()); // PCK certificate should not be self-signed

    ASSERT_LT(pckCertificate.getValidity().getNotBeforeTime(), pckCertificate.getValidity().getNotAfterTime());

    const std::vector<x509::Extension> expectedExtensions = constants::PCK_X509_EXTENSIONS;
    ASSERT_THAT(pckCertificate.getExtensions().size(), expectedExtensions.size());

    ASSERT_THAT(pckCertificate.getPpid(), ElementsAreArray(ppid));
    ASSERT_THAT(pckCertificate.getPceId(), ElementsAreArray(pceId));
    ASSERT_THAT(pckCertificate.getFmspc(), ElementsAreArray(fmspc));
    ASSERT_EQ(pckCertificate.getSgxType(), x509::SgxType::Scalable);

    const auto &tcb = x509::Tcb(cpusvn, cpusvn, 1010);
    ASSERT_THAT(pckCertificate.getTcb().getCpuSvn(), ElementsAreArray(tcb.getCpuSvn()));
    ASSERT_THAT(pckCertificate.getTcb().getSgxTcbComponents(), ElementsAreArray(tcb.getSgxTcbComponents()));
    ASSERT_EQ(pckCertificate.getTcb().getPceSvn(), tcb.getPceSvn());
    ASSERT_EQ(pckCertificate.getTcb(), tcb);

    ASSERT_THAT(pckCertificate.getPlatformInstanceId(), ElementsAreArray(platformInstanceId));

    const auto &configuration = x509::Configuration(true, true, true);
    ASSERT_EQ(pckCertificate.getConfiguration().isDynamicPlatform(), configuration.isDynamicPlatform());
    ASSERT_EQ(pckCertificate.getConfiguration().isCachedKeys(), configuration.isCachedKeys());
    ASSERT_EQ(pckCertificate.getConfiguration().isSmtEnabled(), configuration.isSmtEnabled());
    ASSERT_EQ(pckCertificate.getConfiguration(), configuration);

    free(pubKey);
}

TEST_F(PlatformPckCertificateUT, certificateOperators)
{
    const auto& certificate1 = x509::PlatformPckCertificate::parse(pemPlatformPckCert);
    const auto& certificate2 = x509::PlatformPckCertificate::parse(pemPlatformPckCert);
    const auto ucert = certGenerator.generatePCKCert(3, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                                     constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                                     ppid, cpusvn, pcesvn, pceId, fmspc, 1, platformInstanceId, true);
    const auto pemCert = certGenerator.x509ToString(ucert.get());
    const auto& certificate3 = x509::PlatformPckCertificate::parse(pemCert);

    ASSERT_EQ(certificate1, certificate2);
    ASSERT_FALSE(certificate1 == certificate3);
    ASSERT_FALSE(certificate2 == certificate3);
}

TEST_F(PlatformPckCertificateUT, pckCertificateParseWithWrongAmountOfExtensions)
{
    const auto& brokenCert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                  constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                  ppid, cpusvn, pcesvn, pceId, fmspc, 1, platformInstanceId, false, false, true, true);
    pemPlatformPckCert = certGenerator.x509ToString(brokenCert.get());
    // Exception thrown because of SGX TCB extension is not equal 7
    ASSERT_THROW(x509::PlatformPckCertificate::parse(pemPlatformPckCert), InvalidExtensionException);
}
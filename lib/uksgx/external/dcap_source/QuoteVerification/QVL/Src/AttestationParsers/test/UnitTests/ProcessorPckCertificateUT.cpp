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


struct ProcessorPckCertificateUT: public testing::Test {
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

    ProcessorPckCertificateUT()
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

TEST_F(ProcessorPckCertificateUT, pckCertificateParse)
{
    ASSERT_NO_THROW(x509::ProcessorPckCertificate::parse(pemProcessorPckCert));
    // Exception thrown because of excess SGX TCB extensions
    ASSERT_THROW(x509::ProcessorPckCertificate::parse(pemPlatformPckCert), InvalidExtensionException);
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::ProcessorPckCertificate::parse(pemIntCert), InvalidExtensionException);
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::ProcessorPckCertificate::parse(pemRootCert), InvalidExtensionException);
}

TEST_F(ProcessorPckCertificateUT, pckCertificateConstructors)
{
    const auto& certificate = x509::Certificate::parse(pemProcessorPckCert);
    const auto& pckCertificate = x509::PckCertificate::parse(pemProcessorPckCert);
    const auto& processorPckCertificateFromCert = x509::ProcessorPckCertificate(certificate);
    const auto& processorPckCertificateFromPckCert = x509::ProcessorPckCertificate(pckCertificate);
    const auto& processorPckCertificateFromPEM = x509::ProcessorPckCertificate::parse(pemProcessorPckCert);

    ASSERT_EQ(processorPckCertificateFromCert.getVersion(), processorPckCertificateFromPEM.getVersion());
    ASSERT_EQ(processorPckCertificateFromCert.getSerialNumber(), processorPckCertificateFromPEM.getSerialNumber());
    ASSERT_EQ(processorPckCertificateFromCert.getSubject(), processorPckCertificateFromPEM.getSubject());
    ASSERT_EQ(processorPckCertificateFromCert.getIssuer(), processorPckCertificateFromPEM.getIssuer());
    ASSERT_EQ(processorPckCertificateFromCert.getValidity(), processorPckCertificateFromPEM.getValidity());
    ASSERT_EQ(processorPckCertificateFromCert.getExtensions(), processorPckCertificateFromPEM.getExtensions());
    ASSERT_EQ(processorPckCertificateFromCert.getSignature(), processorPckCertificateFromPEM.getSignature());
    ASSERT_EQ(processorPckCertificateFromCert.getPubKey(), processorPckCertificateFromPEM.getPubKey());

    ASSERT_EQ(processorPckCertificateFromCert.getTcb(), processorPckCertificateFromPEM.getTcb());
    ASSERT_EQ(processorPckCertificateFromCert.getPpid(), processorPckCertificateFromPEM.getPpid());
    ASSERT_EQ(processorPckCertificateFromCert.getPceId(), processorPckCertificateFromPEM.getPceId());
    ASSERT_EQ(processorPckCertificateFromCert.getSgxType(), processorPckCertificateFromPEM.getSgxType());

    ASSERT_EQ(processorPckCertificateFromPckCert.getVersion(), processorPckCertificateFromPEM.getVersion());
    ASSERT_EQ(processorPckCertificateFromPckCert.getSerialNumber(), processorPckCertificateFromPEM.getSerialNumber());
    ASSERT_EQ(processorPckCertificateFromPckCert.getSubject(), processorPckCertificateFromPEM.getSubject());
    ASSERT_EQ(processorPckCertificateFromPckCert.getIssuer(), processorPckCertificateFromPEM.getIssuer());
    ASSERT_EQ(processorPckCertificateFromPckCert.getValidity(), processorPckCertificateFromPEM.getValidity());
    ASSERT_EQ(processorPckCertificateFromPckCert.getExtensions(), processorPckCertificateFromPEM.getExtensions());
    ASSERT_EQ(processorPckCertificateFromPckCert.getSignature(), processorPckCertificateFromPEM.getSignature());
    ASSERT_EQ(processorPckCertificateFromPckCert.getPubKey(), processorPckCertificateFromPEM.getPubKey());

    ASSERT_EQ(processorPckCertificateFromPckCert.getTcb(), processorPckCertificateFromPEM.getTcb());
    ASSERT_EQ(processorPckCertificateFromPckCert.getPpid(), processorPckCertificateFromPEM.getPpid());
    ASSERT_EQ(processorPckCertificateFromPckCert.getPceId(), processorPckCertificateFromPEM.getPceId());
    ASSERT_EQ(processorPckCertificateFromPckCert.getSgxType(), processorPckCertificateFromPEM.getSgxType());
}

TEST_F(ProcessorPckCertificateUT, processorPckCertificateGetters)
{
    const auto& pckCertificate = x509::ProcessorPckCertificate::parse(pemProcessorPckCert);

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
    ASSERT_EQ(pckCertificate.getSgxType(), x509::SgxType::Standard);

    const auto &tcb = x509::Tcb(cpusvn, cpusvn, 1010);
    ASSERT_THAT(pckCertificate.getTcb().getCpuSvn(), ElementsAreArray(tcb.getCpuSvn()));
    ASSERT_THAT(pckCertificate.getTcb().getSgxTcbComponents(), ElementsAreArray(tcb.getSgxTcbComponents()));
    ASSERT_EQ(pckCertificate.getTcb().getPceSvn(), tcb.getPceSvn());
    ASSERT_EQ(pckCertificate.getTcb(), tcb);

    free(pubKey);
}

TEST_F(ProcessorPckCertificateUT, platformPckCertificateGetters)
{
    const auto& pckCertificate = x509::ProcessorPckCertificate::parse(pemProcessorPckCert);

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
    ASSERT_EQ(pckCertificate.getSgxType(), x509::SgxType::Standard);

    const auto &tcb = x509::Tcb(cpusvn, cpusvn, 1010);
    ASSERT_THAT(pckCertificate.getTcb().getCpuSvn(), ElementsAreArray(tcb.getCpuSvn()));
    ASSERT_THAT(pckCertificate.getTcb().getSgxTcbComponents(), ElementsAreArray(tcb.getSgxTcbComponents()));
    ASSERT_EQ(pckCertificate.getTcb().getPceSvn(), tcb.getPceSvn());
    ASSERT_EQ(pckCertificate.getTcb(), tcb);

    free(pubKey);
}

TEST_F(ProcessorPckCertificateUT, certificateOperators)
{
    const auto& certificate1 = x509::ProcessorPckCertificate::parse(pemProcessorPckCert);
    const auto& certificate2 = x509::ProcessorPckCertificate::parse(pemProcessorPckCert);
    const auto ucert = certGenerator.generatePCKCert(3, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                                     constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                                     ppid, cpusvn, pcesvn, pceId, fmspc, 0);
    const auto pemCert = certGenerator.x509ToString(ucert.get());
    const auto& certificate3 = x509::ProcessorPckCertificate::parse(pemCert);

    ASSERT_EQ(certificate1, certificate2);
    ASSERT_FALSE(certificate1 == certificate3);
    ASSERT_FALSE(certificate2 == certificate3);
}

TEST_F(ProcessorPckCertificateUT, pckCertificateParseWithWrongAmountOfExtensions)
{
    const auto& brokenCert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                  constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                  ppid, cpusvn, pcesvn, pceId, fmspc, 0, {}, false, false, false, true);
    pemProcessorPckCert = certGenerator.x509ToString(brokenCert.get());
    // Exception thrown because of SGX TCB extension is not equal 5
    ASSERT_THROW(x509::ProcessorPckCertificate::parse(pemProcessorPckCert), InvalidExtensionException);
}
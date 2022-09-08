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
#include <EnclaveIdentityGenerator.h>
#include <EcdsaSignatureGenerator.h>
#include "X509CertGenerator.h"
#include "X509CrlGenerator.h"

using namespace testing;
using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::test;
using namespace intel::sgx::dcap::parser::test;

struct VerifyEnclaveIdentityIT : public Test
{
    const char placeholder[8] = "1234567";

    X509CertGenerator certGenerator = X509CertGenerator{};
    X509CrlGenerator crlGenerator = X509CrlGenerator{};

    const int timeNow = 0;
    const int timeOneHour = 3600;
    const Bytes serialNumber {0x23, 0x45};
};

TEST_F(VerifyEnclaveIdentityIT, nullptrArgumentsShouldReturnUnsupportedCertStatus)
{
    ASSERT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, sgxAttestationVerifyEnclaveIdentity(nullptr, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(VerifyEnclaveIdentityIT, nullptrEnclaveIdentityShoudReturnUnsupportedCertStatus)
{
    ASSERT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, sgxAttestationVerifyEnclaveIdentity(nullptr, placeholder, placeholder, placeholder, nullptr));
}

TEST_F(VerifyEnclaveIdentityIT, nullptrCertChainShoudReturnUnsupportedCertStatus)
{
    ASSERT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, sgxAttestationVerifyEnclaveIdentity(placeholder, nullptr, placeholder, placeholder, nullptr));
}

TEST_F(VerifyEnclaveIdentityIT, nullptrRootCrlShoudReturnUnsupportedCertStatus)
{
    ASSERT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, sgxAttestationVerifyEnclaveIdentity(placeholder, placeholder, nullptr, placeholder, nullptr));
}

TEST_F(VerifyEnclaveIdentityIT, nullptrRootCertShoudReturnUnsupportedCertStatus)
{
    ASSERT_EQ(STATUS_UNSUPPORTED_CERT_FORMAT, sgxAttestationVerifyEnclaveIdentity(placeholder, placeholder, placeholder, nullptr,
                                                                             nullptr));
}

TEST_F(VerifyEnclaveIdentityIT, verifyEnclaveIdentityShouldReturnStatusOkWhenCrlAsDER)
{
    auto keyRoot = certGenerator.generateEcKeypair();
    auto tcbSigningKey = certGenerator.generateEcKeypair();

    auto rootCert = certGenerator.generateCaCert(2, serialNumber, timeNow, timeOneHour, keyRoot.get(), keyRoot.get(),
                                            constants::ROOT_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

    auto tcbSigningCert = certGenerator.generateCaCert(2, serialNumber, timeNow, timeOneHour, tcbSigningKey.get(), keyRoot.get(),
                                           constants::TCB_SUBJECT, constants::ROOT_CA_SUBJECT);

    auto rootCaCrl = crlGenerator.generateCRL(CRL_VERSION_2, timeNow, timeOneHour, rootCert, std::vector<Bytes>{});

    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(tcbSigningCert.get());
    auto certChain = rootCertPem  + intPem;
    auto rootCaCrlDer = X509CrlGenerator::x509CrlToDERString(rootCaCrl.get());

    auto qeIdentityBody = EnclaveIdentityVectorModel{}.toV2JSON();
    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), qeIdentityBody.begin(), qeIdentityBody.end());
    auto signature = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, tcbSigningKey.get());

    auto qeIdentityJson = enclaveIdentityJsonWithSignature(qeIdentityBody,
                                                      EcdsaSignatureGenerator::signatureToHexString(signature));

    ASSERT_EQ(STATUS_OK, sgxAttestationVerifyEnclaveIdentity(qeIdentityJson.c_str(), certChain.c_str(), rootCaCrlDer.c_str(), rootCertPem.c_str(), nullptr));
}

TEST_F(VerifyEnclaveIdentityIT, verifyEnclaveIdentityShouldReturnStatusOkWhenCrlAsPEM)
{
    auto keyRoot = certGenerator.generateEcKeypair();
    auto tcbSigningKey = certGenerator.generateEcKeypair();

    auto rootCert = certGenerator.generateCaCert(2, serialNumber, timeNow, timeOneHour, keyRoot.get(), keyRoot.get(),
                                                 constants::ROOT_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

    auto tcbSigningCert = certGenerator.generateCaCert(2, serialNumber, timeNow, timeOneHour, tcbSigningKey.get(), keyRoot.get(),
                                                       constants::TCB_SUBJECT, constants::ROOT_CA_SUBJECT);

    auto rootCaCrl = crlGenerator.generateCRL(CRL_VERSION_2, timeNow, timeOneHour, rootCert, std::vector<Bytes>{});

    auto rootCertPem = certGenerator.x509ToString(rootCert.get());
    auto intPem = certGenerator.x509ToString(tcbSigningCert.get());
    auto certChain = rootCertPem  + intPem;
    auto rootCaCrlPem = X509CrlGenerator::x509CrlToPEMString(rootCaCrl.get());

    auto qeIdentityBody = EnclaveIdentityVectorModel{}.toV2JSON();
    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), qeIdentityBody.begin(), qeIdentityBody.end());
    auto signature = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, tcbSigningKey.get());

    auto qeIdentityJson = enclaveIdentityJsonWithSignature(qeIdentityBody,
                                                      EcdsaSignatureGenerator::signatureToHexString(signature));

    ASSERT_EQ(STATUS_OK, sgxAttestationVerifyEnclaveIdentity(qeIdentityJson.c_str(), certChain.c_str(), rootCaCrlPem.c_str(), rootCertPem.c_str(), nullptr));
}
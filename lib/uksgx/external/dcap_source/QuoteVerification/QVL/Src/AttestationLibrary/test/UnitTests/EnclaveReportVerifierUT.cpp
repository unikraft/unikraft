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

#include "QuoteV3Generator.h"
#include "EnclaveIdentityGenerator.h"

#include <SgxEcdsaAttestation/QuoteVerification.h>
#include <QuoteVerification/Quote.h>
#include <Verifiers/EnclaveReportVerifier.h>
#include <Verifiers/EnclaveIdentityParser.h>
#include "EcdsaSignatureGenerator.h"
#include "X509CertGenerator.h"

#include <gtest/gtest.h>

#include <array>

using namespace testing;
using namespace ::intel::sgx::dcap;
using namespace ::intel::sgx::dcap::test;
using namespace ::intel::sgx::dcap::parser::test;
using namespace std;

struct EnclaveReportVerifierUT : public Test
{
    EnclaveReportVerifier enclaveReportVerifier;
    test::QuoteV3Generator quoteGenerator;
    test::QuoteV3Generator::EnclaveReport enclaveReport;
    EnclaveIdentityParser parser;

    X509CertGenerator certGenerator = X509CertGenerator{};
    crypto::EVP_PKEY_uptr tcbSigningKey;
    EnclaveReportVerifierUT(): tcbSigningKey(certGenerator.generateEcKeypair())
    {
    }

    EnclaveReport getEnclaveReport()
    {
        quoteGenerator.withEnclaveReport(enclaveReport);
        const auto enclaveReportBody = quoteGenerator.getEnclaveReport();
        EnclaveReport eReport{};
        eReport.cpuSvn = enclaveReportBody.cpuSvn;
        eReport.miscSelect = enclaveReportBody.miscSelect;
        eReport.reserved1 = enclaveReportBody.reserved1;
        eReport.attributes = enclaveReportBody.attributes;
        eReport.mrEnclave = enclaveReportBody.mrEnclave;
        eReport.reserved2 = enclaveReportBody.reserved2;
        eReport.mrSigner = enclaveReportBody.mrSigner;
        eReport.reserved3 = enclaveReportBody.reserved3;
        eReport.isvProdID = enclaveReportBody.isvProdID;
        eReport.isvSvn = enclaveReportBody.isvSvn;
        eReport.reserved4 = enclaveReportBody.reserved4;
        eReport.reportData = enclaveReportBody.reportData;

        return eReport;
    }

    std::string generateEnclaveIdentity(std::string bodyJson);
};

std::string EnclaveReportVerifierUT::generateEnclaveIdentity(std::string bodyJson)
{
    std::vector<uint8_t> enclaveIdentityBodyBytes(bodyJson.begin(), bodyJson.end());
    auto signature = EcdsaSignatureGenerator::signECDSA_SHA256(enclaveIdentityBodyBytes, tcbSigningKey.get());

    return enclaveIdentityJsonWithSignature(bodyJson, EcdsaSignatureGenerator::signatureToHexString(signature));
}


TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportMiscselectMismatchWhenMiscselectIsDifferent)
{
    EnclaveIdentityVectorModel model;
    model.miscselect = {{1, 1, 1, 1}};
    model.applyTo(enclaveReport);
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_MISCSELECT_MISMATCH, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportAttributestMismatchWhenAttributesIsDifferent)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    model.attributes = {{9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_ATTRIBUTES_MISMATCH, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportAttributestMismatchWhenIdentityAttributesHasIncorrectSize)
{
    EnclaveIdentityVectorModel model;
    model.attributesMask = {{9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9}};
    model.applyTo(enclaveReport);
    string json = model.toV2JSON();

    ASSERT_THROW({
                     try
                     {
                         parser.parse(generateEnclaveIdentity(json));
                     }
                     catch (const ParserException &ex)
                     {
                         EXPECT_EQ(ex.getStatus(), STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
                         throw;
                     }
                 }, ParserException);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnStausStausSgxEnclaveIndentityWhenMrsignerIsNotPresent)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    string json = model.toV2JSON();

    removeWordFromString("mrsigner", json);

    ASSERT_THROW({
                     try
                     {
                         parser.parse(generateEnclaveIdentity(json));
                     }
                     catch (const ParserException &ex)
                     {
                         EXPECT_EQ(ex.getStatus(), STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
                         throw;
                     }
                 }, ParserException);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnStausStausSgxEnclaveIndentityWhenIsvprodidIsNotPresent)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    string json = model.toV2JSON();

    removeWordFromString("isvprodid", json);

    ASSERT_THROW({
                     try
                     {
                         parser.parse(generateEnclaveIdentity(json));
                     }
                     catch (const ParserException &ex)
                     {
                         EXPECT_EQ(ex.getStatus(), STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
                         throw;
                     }
                 }, ParserException);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnStausStausSgxEnclaveIndentityWhenIsvsvnIsNotPresent)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    string json = model.toV2JSON();

    removeWordFromString("isvsvn", json);

    ASSERT_THROW({
                     try
                     {
                         parser.parse(generateEnclaveIdentity(json));
                     }
                     catch (const ParserException &ex)
                     {
                         EXPECT_EQ(ex.getStatus(), STATUS_SGX_ENCLAVE_IDENTITY_INVALID);
                         throw;
                     }
                 }, ParserException);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportMrsignerMismatchWhenMrsignerIsDifferent)
{
    EnclaveIdentityVectorModel model{};
    model.applyTo(enclaveReport);
    model.mrsigner = {{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportIsvprodidMismatchWhenIsvprodidIsDifferent)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    model.isvprodid = 11;
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_ISVPRODID_MISMATCH, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportRevokedWhenIsvsvnIsBelowAllLevels)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    enclaveReport.isvSvn = 2;
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportRevokedWhenIsvsvnIsOnRevokedLevel)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    enclaveReport.isvSvn = 4;
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnEnclaveReportOutOfDateWhenIsvsvnIsOnOutOfDateLevel)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    enclaveReport.isvSvn = 5;
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnStatusOkWhenJsonIsOk)
{
    EnclaveIdentityVectorModel model;
    model.applyTo(enclaveReport);
    string json = model.toV2JSON();

    auto enclaveIdentity = parser.parse(generateEnclaveIdentity(json));

    auto result = enclaveReportVerifier.verify(enclaveIdentity.get(), getEnclaveReport());

    ASSERT_EQ(STATUS_OK, result);
}

TEST_F(EnclaveReportVerifierUT, shouldReturnOutOfDateWhenIsvsvnIsOutOfDate)
{

}
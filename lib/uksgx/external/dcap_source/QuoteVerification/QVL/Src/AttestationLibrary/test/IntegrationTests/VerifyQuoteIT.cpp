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
#include <SgxEcdsaAttestation/AttestationParsers.h>
#include <CertVerification/X509Constants.h>
#include <QuoteV3Generator.h>
#include <QuoteV4Generator.h>
#include <EnclaveIdentityGenerator.h>
#include <EcdsaSignatureGenerator.h>
#include <QuoteVerification/QuoteConstants.h>
#include <TcbInfoJsonGenerator.h>
#include <X509CertGenerator.h>
#include <X509CrlGenerator.h>
#include <DigestUtils.h>
#include <KeyHelpers.h>

using namespace std;
using namespace testing;
using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::test;
using namespace intel::sgx::dcap::parser::test;

struct VerifyQuoteIT : public Test
{
    ~VerifyQuoteIT() override
    {
        delete quotePlaceHolder;
    };

    const char* placeHolder = "placeHolder";
    uint8_t* quotePlaceHolder = new uint8_t;

    int timeNow = 0;
    int timeOneHour = 3600;

    X509CertGenerator certGenerator;
    X509CrlGenerator crlGenerator;
    Bytes sn {0x23, 0x45};
    Bytes ppid = Bytes(16, 0xaa);
    Bytes cpusvn = Bytes(16, 0xff);
    Bytes pceId = {0x04, 0xf3};
    Bytes fmspc = {0x04, 0xf3, 0x44, 0x45, 0xaa, 0x00};
    Bytes pcesvnLE = {0x01, 0x02};
    Bytes pcesvnBE = {0x02, 0x01};
    /*std::string mrSignerSeamString = "9469467a6643a4fcc8c7b7601eab9da2baf89108879f2620146cae385c191f854d4c736cabdf09c5dd6824fe5ef57aed";
    array<uint8_t, 48> mrSignerSeamBytes{};*/
    TdxModule tdxModule = { std::vector<uint8_t>(48, 0x00), std::vector<uint8_t>(8, 0x00), std::vector<uint8_t>(8, 0xFF) };

    crypto::EVP_PKEY_uptr keyInt = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr key = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::X509_uptr cert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr interCert = crypto::make_unique<X509>(nullptr);

    QuoteV3Generator quoteV3Generator;
    QuoteV4Generator quoteV4Generator;
    uint32_t version = 2;
    uint16_t pcesvn = 1;
    uint16_t isvprodid = 1;
    uint16_t isvsvn = 1;
    string issueDate = "2018-08-22T10:09:10Z";
    string nextUpdate = "2118-08-23T10:09:10Z";
    string fmspcStr = "04F34445AA00";
    string pceIdStr = "04F3";
    string status = "UpToDate";
    uint32_t tcbType = 1;
    uint32_t tcbEvaluationDataNumber = 1;
    std::string tcbDate = "2018-08-01T10:00:00Z";
    string miscselect = "";
    string miscselectMask = "";
    string attributes = "";
    string attributesMask = "";
    string mrsigner = "";
    string positiveTcbInfoV2JsonBody;
    string positiveTdxTcbInfoV3JsonBody;
    string positiveSgxTcbInfoV3JsonBody;
    string positiveQEIdentityV2JsonBody;
    std::vector<TcbLevelV3> tdxTcbLevels;

    test::QuoteV3Generator::EnclaveReport enclaveReport;

    VerifyQuoteIT()
    {
        keyInt = certGenerator.generateEcKeypair();
        key = certGenerator.generateEcKeypair();

        cert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                             constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                             ppid, cpusvn, pcesvnBE, pceId, fmspc, 0);

        intel::sgx::dcap::parser::x509::DistinguishedName subject =
                {"", "Intel SGX PCK Platform CA", "US", "Intel Corporation", "Santa Clara", "CA"};
        intel::sgx::dcap::parser::x509::DistinguishedName issuer =
                {"", "Intel SGX PCK Platform CA", "US", "Intel Corporation", "Santa Clara", "CA"};

        interCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(), subject, issuer);

        positiveTcbInfoV2JsonBody = tcbInfoJsonV2Body(version, issueDate, nextUpdate, fmspcStr, pceIdStr,
                                                      getRandomTcb(), pcesvn, status, tcbType, tcbEvaluationDataNumber,
                                                      tcbDate);

        std::vector<TcbLevelV3> sgxTcbLevels;
        sgxTcbLevels.push_back(TcbLevelV3{
                getRandomTcbComponent(),
                {},
                5,
                "UpToDate",
                "2058-08-23T10:09:10Z"
        });

        positiveSgxTcbInfoV3JsonBody = tcbInfoJsonV3Body("SGX", 3, issueDate, nextUpdate, fmspcStr, pceIdStr,
                                                         1, 1, sgxTcbLevels, false, tdxModule);


        tdxTcbLevels.push_back(TcbLevelV3{
                getRandomTcbComponent(),
                getRandomTcbComponent(),
                5,
                "UpToDate",
                "2058-08-23T10:09:10Z"
        });
        positiveTdxTcbInfoV3JsonBody = tcbInfoJsonV3Body("TDX", 3, issueDate, nextUpdate, fmspcStr, pceIdStr,
                                                         1, 1, tdxTcbLevels, true, tdxModule);
        EnclaveIdentityVectorModel model;
        positiveQEIdentityV2JsonBody = model.toV2JSON();
        model.applyTo(enclaveReport);
    }

    std::string getValidCrl(const crypto::X509_uptr &ucert)
    {
        auto revokedList = std::vector<Bytes>{{0x12, 0x10, 0x13, 0x11}, {0x11, 0x33, 0xff, 0x56}};
        auto rootCaCRL = crlGenerator.generateCRL(CRLVersion::CRL_VERSION_2, 0, 3600, ucert, revokedList);

        return X509CrlGenerator::x509CrlToDERString(rootCaCRL.get());
    }

    std::vector<uint8_t> concat(const std::vector<uint8_t>& rhs, const std::vector<uint8_t>& lhs)
    {
        std::vector<uint8_t> ret = rhs;
        std::copy(lhs.begin(), lhs.end(), std::back_inserter(ret));
        return ret;
    }

    template<size_t N>
    std::vector<uint8_t> concat(const std::array<uint8_t,N>& rhs, const std::vector<uint8_t>& lhs)
    {
        std::vector<uint8_t> ret(std::begin(rhs), std::end(rhs));
        std::copy(lhs.begin(), lhs.end(), std::back_inserter(ret));
        return ret;
    }

    std::array<uint8_t,64> assingFirst32(const std::array<uint8_t,32>& in)
    {
        std::array<uint8_t,64> ret{};
        std::copy_n(in.begin(), 32, ret.begin());
        return ret;
    }

    std::array<uint8_t,64> signEnclaveReport(const test::QuoteV3Generator::EnclaveReport& report, EVP_PKEY& ukey)
    {
        return signAndGetRaw(report.bytes(), ukey);
    }

    std::array<uint8_t,64> signEnclaveReport(const test::QuoteV4Generator::EnclaveReport& report, EVP_PKEY& ukey)
    {
        return signAndGetRaw(report.bytes(), ukey);
    }

    std::array<uint8_t,64> signAndGetRaw(const std::vector<uint8_t>& data, EVP_PKEY& ukey)
    {
        auto usignature = EcdsaSignatureGenerator::signECDSA_SHA256(data, &ukey);
        std::array<uint8_t, 64> signatureArr{};
        std::copy_n(usignature.begin(), signatureArr.size(), signatureArr.begin());
        return signatureArr;
    }

};

TEST_F(VerifyQuoteIT, shouldReturnedMissingParmatersWhenQuoteIsNull)
{
    // GIVEN / WHEN
    auto result = sgxAttestationVerifyQuote(nullptr, 0, placeHolder, placeHolder, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_MISSING_PARAMETERS, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedMissingParmatersWhenPckCertificateIsNull)
{
    // GIVEN / WHEN
    auto result = sgxAttestationVerifyQuote(quotePlaceHolder, 0, nullptr, placeHolder, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_MISSING_PARAMETERS, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedMissingParmatersWhenPckCrlIsNull)
{
    // GIVEN / WHEN
    auto result = sgxAttestationVerifyQuote(quotePlaceHolder, 0, placeHolder, nullptr, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_MISSING_PARAMETERS, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedMissingParmatersWhenTcbInfoJsonIsNull)
{
    // GIVEN / WHEN
    auto result = sgxAttestationVerifyQuote(quotePlaceHolder, 0, placeHolder, placeHolder, nullptr, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_MISSING_PARAMETERS, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedQuoteFormatWhenQuoteParseFail)
{
    // GIVEN / WHEN
    auto result = sgxAttestationVerifyQuote(quotePlaceHolder, 0, placeHolder, placeHolder, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_QUOTE_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedQuoteFormatWhenQuoteSizeIsIncorrect)
{
    // GIVEN
    auto incorrectQouteSize = 0;
    auto quote = quoteV3Generator.buildQuote();

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (unsigned) incorrectQouteSize, placeHolder, placeHolder, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_QUOTE_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedQuoteFormatWhenQuoteHeaderVersionIsWrong)
{
    // GIVEN
    QuoteV3Generator::QuoteHeader quoteHeader{};
    quoteHeader.version = 999;
    quoteV3Generator.withHeader(quoteHeader);
    auto quote = quoteV3Generator.buildQuote();


    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), placeHolder, placeHolder, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_QUOTE_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedPckCertFormatWhenVerifyPckCertFail)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    test::QuoteV3Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PLAIN_PPID;
    certificationData.keyData = concat(ppid, concat(cpusvn, pcesvnLE));
    certificationData.size = static_cast<uint16_t>(certificationData.keyData.size());

    quoteV3Generator.withcertificationData(certificationData);
    quoteV3Generator.getAuthSize() += (uint32_t) certificationData.keyData.size();
    quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);

    enclaveReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey,
                                                                                   quoteV3Generator.getAuthData().qeAuthData.data)));

    quoteV3Generator.getAuthData().qeReport = enclaveReport;
    quoteV3Generator.getAuthData().qeReportSignature.signature =
            signEnclaveReport(quoteV3Generator.getAuthData().qeReport, *pckCertKeyPtr);
    quoteV3Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV3Generator.getHeader().bytes(), quoteV3Generator.getEnclaveReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV3Generator.buildQuote();
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));
    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), positiveQEIdentityV2JsonBody.begin(), positiveQEIdentityV2JsonBody.end());
    auto signatureQE = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, key.get());
    auto qeIdentityJsonWithSignature = ::enclaveIdentityJsonWithSignature(positiveQEIdentityV2JsonBody,
                                                                     EcdsaSignatureGenerator::signatureToHexString(
                                                                           signatureQE));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), placeHolder, pckCrl.c_str(), tcbInfoJsonWithSignature.c_str(),
                                            qeIdentityJsonWithSignature.c_str());

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_PCK_CERT_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedPckCrlFormatWhenVerifyPckCrlFail)
{
    // GIVEN
    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), placeHolder, placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_PCK_RL_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedTcbInfoFormatWhenVerifyTcbInfoFail)
{
    // GIVEN
    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(cert);

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(), placeHolder, placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_TCB_INFO_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedQeIdentityFormatWhenVerifyQEidentityFail)
{
    // GIVEN
    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(cert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signature = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signature));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_QE_IDENTITY_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedUnsuportedQeIdentityFormatWhenQEIdentityIsWrong)
{
    // GIVEN
    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(cert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signature = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signature));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), placeHolder);

    // THEN
    EXPECT_EQ(STATUS_UNSUPPORTED_QE_IDENTITY_FORMAT, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifyQuoteV3Successffuly)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    test::QuoteV3Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PLAIN_PPID;
    certificationData.keyData = concat(ppid, concat(cpusvn, pcesvnLE));
    certificationData.size = static_cast<uint16_t>(certificationData.keyData.size());

    quoteV3Generator.withcertificationData(certificationData);
    quoteV3Generator.getAuthSize() += (uint32_t) certificationData.keyData.size();
    quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);

    enclaveReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey,
                                                                                   quoteV3Generator.getAuthData().qeAuthData.data)));

    quoteV3Generator.getAuthData().qeReport = enclaveReport;
    quoteV3Generator.getAuthData().qeReportSignature.signature =
            signEnclaveReport(quoteV3Generator.getAuthData().qeReport, *pckCertKeyPtr);
    quoteV3Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV3Generator.getHeader().bytes(), quoteV3Generator.getEnclaveReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), positiveQEIdentityV2JsonBody.begin(), positiveQEIdentityV2JsonBody.end());
    auto signatureQE = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, key.get());
    auto qeIdentityJsonWithSignature = ::enclaveIdentityJsonWithSignature(positiveQEIdentityV2JsonBody,
                                                                     EcdsaSignatureGenerator::signatureToHexString(
                                                                           signatureQE));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), qeIdentityJsonWithSignature.c_str());

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifyQuoteV3SuccessffulyWithNoQeIdentityJson)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    test::QuoteV3Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PLAIN_PPID;
    certificationData.keyData = concat(ppid, concat(cpusvn, pcesvnLE));
    certificationData.size = static_cast<uint16_t>(certificationData.keyData.size());

    quoteV3Generator.withcertificationData(certificationData);
    quoteV3Generator.getAuthSize() += (uint32_t) certificationData.keyData.size();
    quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);

    enclaveReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey,
                                                                                   quoteV3Generator.getAuthData().qeAuthData.data)));

    quoteV3Generator.getAuthData().qeReport = enclaveReport;
    quoteV3Generator.getAuthData().qeReportSignature.signature =
            signEnclaveReport(quoteV3Generator.getAuthData().qeReport, *pckCertKeyPtr);
    quoteV3Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV3Generator.getHeader().bytes(), quoteV3Generator.getEnclaveReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifySgxQuoteV4Successffuly)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    test::QuoteV4Generator::EnclaveReport qeReport{};

    EnclaveIdentityVectorModel model;
    positiveQEIdentityV2JsonBody = model.toV2JSON();
    model.applyTo(qeReport);

    test::QuoteV4Generator::QeAuthData qeAuthData;
    qeAuthData.data = {};
    qeAuthData.size = 0;

    test::QuoteV4Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PCK_CERT_CHAIN;
    certificationData.keyData = {};
    certificationData.size = 0;

    test::QuoteV4Generator::QEReportCertificationData qeReportCertificationData;
    qeReportCertificationData.qeAuthData = qeAuthData;
    qeReportCertificationData.qeReport = qeReport;
    qeReportCertificationData.certificationData = certificationData;
    qeReportCertificationData.qeReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(test::getRawPub(*pckCertPubKeyPtr), qeAuthData.data)));
    qeReportCertificationData.qeReportSignature.signature = signEnclaveReport(qeReportCertificationData.qeReport, *pckCertKeyPtr);

    test::QuoteV4Generator::CertificationData qeCertificationData;
    qeCertificationData.keyDataType = constants::PCK_ID_QE_REPORT_CERTIFICATION_DATA;
    qeCertificationData.keyData = qeReportCertificationData.bytes();
    qeCertificationData.size = static_cast<uint16_t>(qeCertificationData.keyData.size());

    quoteV4Generator.withCertificationData(qeCertificationData);
    quoteV4Generator.getAuthSize() = 134 + (uint32_t) qeCertificationData.keyData.size();
    quoteV4Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);
    quoteV4Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV4Generator.getHeader().bytes(), quoteV4Generator.getEnclaveReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV4Generator.buildSgxQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), positiveQEIdentityV2JsonBody.begin(), positiveQEIdentityV2JsonBody.end());
    auto signatureQE = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, key.get());
    auto qeIdentityJsonWithSignature = ::enclaveIdentityJsonWithSignature(positiveQEIdentityV2JsonBody,
                                                                     EcdsaSignatureGenerator::signatureToHexString(
                                                                             signatureQE));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), qeIdentityJsonWithSignature.c_str());

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifySgxQuoteV4WithoutQeIdentitySuccessffuly)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    test::QuoteV4Generator::EnclaveReport qeReport{};

    test::QuoteV4Generator::QeAuthData qeAuthData;
    qeAuthData.data = {};
    qeAuthData.size = 0;

    test::QuoteV4Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PCK_CERT_CHAIN;
    certificationData.keyData = {};
    certificationData.size = 0;

    test::QuoteV4Generator::QEReportCertificationData qeReportCertificationData;
    qeReportCertificationData.qeAuthData = qeAuthData;
    qeReportCertificationData.qeReport = qeReport;
    qeReportCertificationData.certificationData = certificationData;
    qeReportCertificationData.qeReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(test::getRawPub(*pckCertPubKeyPtr), qeAuthData.data)));
    qeReportCertificationData.qeReportSignature.signature = signEnclaveReport(qeReportCertificationData.qeReport, *pckCertKeyPtr);

    test::QuoteV4Generator::CertificationData qeCertificationData;
    qeCertificationData.keyDataType = constants::PCK_ID_QE_REPORT_CERTIFICATION_DATA;
    qeCertificationData.keyData = qeReportCertificationData.bytes();
    qeCertificationData.size = static_cast<uint16_t>(qeCertificationData.keyData.size());

    quoteV4Generator.withCertificationData(qeCertificationData);
    quoteV4Generator.getAuthSize() = 134 + (uint32_t) qeCertificationData.keyData.size();
    quoteV4Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);
    quoteV4Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV4Generator.getHeader().bytes(), quoteV4Generator.getEnclaveReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV4Generator.buildSgxQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTcbInfoV2JsonBody.begin(), positiveTcbInfoV2JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTcbInfoV2JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifyTdxQuoteV4Successffuly)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    quoteV4Generator.getHeader().teeType = constants::TEE_TYPE_TDX;
    std::copy_n(tdxModule.mrsigner.begin(), tdxModule.mrsigner.size(), quoteV4Generator.getTdReport().mrSignerSeam.begin());
    quoteV4Generator.getTdReport().seamAttributes.fill(0x00);
    quoteV4Generator.getTdReport().teeTcbSvn = {0xFF, tdxTcbLevels[0].tdxTcbComponents[1].svn, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,};

    test::QuoteV4Generator::EnclaveReport qeReport{};

    EnclaveIdentityVectorModel model;
    model.version = 2;
    model.id = "TD_QE";
    
    positiveQEIdentityV2JsonBody = model.toV2JSON();
    model.applyTo(qeReport);

    test::QuoteV4Generator::QeAuthData qeAuthData;
    qeAuthData.data = {};
    qeAuthData.size = 0;

    test::QuoteV4Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PCK_CERT_CHAIN;
    certificationData.keyData = {};
    certificationData.size = 0;

    test::QuoteV4Generator::QEReportCertificationData qeReportCertificationData;
    qeReportCertificationData.qeAuthData = qeAuthData;
    qeReportCertificationData.qeReport = qeReport;
    qeReportCertificationData.certificationData = certificationData;
    qeReportCertificationData.qeReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(test::getRawPub(*pckCertPubKeyPtr), qeAuthData.data)));
    qeReportCertificationData.qeReportSignature.signature = signEnclaveReport(qeReportCertificationData.qeReport, *pckCertKeyPtr);

    test::QuoteV4Generator::CertificationData qeCertificationData;
    qeCertificationData.keyDataType = constants::PCK_ID_QE_REPORT_CERTIFICATION_DATA;
    qeCertificationData.keyData = qeReportCertificationData.bytes();
    qeCertificationData.size = static_cast<uint16_t>(qeCertificationData.keyData.size());

    quoteV4Generator.withCertificationData(qeCertificationData);
    quoteV4Generator.getAuthSize() = 134 + (uint32_t) qeCertificationData.keyData.size();
    quoteV4Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);
    quoteV4Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV4Generator.getHeader().bytes(), quoteV4Generator.getTdReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV4Generator.buildTdxQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTdxTcbInfoV3JsonBody.begin(), positiveTdxTcbInfoV3JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTdxTcbInfoV3JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));
    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), positiveQEIdentityV2JsonBody.begin(), positiveQEIdentityV2JsonBody.end());
    auto signatureQE = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, key.get());
    auto qeIdentityJsonWithSignature = ::enclaveIdentityJsonWithSignature(positiveQEIdentityV2JsonBody,
                                                                     EcdsaSignatureGenerator::signatureToHexString(
                                                                             signatureQE));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), qeIdentityJsonWithSignature.c_str());

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusTdxModuleMismatchWhenVerifyTdxQuoteV4WithDifferentMrsignerSeam)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    quoteV4Generator.getHeader().teeType = constants::TEE_TYPE_TDX;
    quoteV4Generator.getTdReport().mrSignerSeam.fill(0x01);
    quoteV4Generator.getTdReport().seamAttributes.fill(0x00);
    quoteV4Generator.getTdReport().teeTcbSvn = {0xFF, tdxTcbLevels[0].tdxTcbComponents[1].svn, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,};

    test::QuoteV4Generator::EnclaveReport qeReport{};

    EnclaveIdentityVectorModel model;
    model.version = 2;
    model.id = "TD_QE";

    positiveQEIdentityV2JsonBody = model.toV2JSON();
    model.applyTo(qeReport);

    test::QuoteV4Generator::QeAuthData qeAuthData;
    qeAuthData.data = {};
    qeAuthData.size = 0;

    test::QuoteV4Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PCK_CERT_CHAIN;
    certificationData.keyData = {};
    certificationData.size = 0;

    test::QuoteV4Generator::QEReportCertificationData qeReportCertificationData;
    qeReportCertificationData.qeAuthData = qeAuthData;
    qeReportCertificationData.qeReport = qeReport;
    qeReportCertificationData.certificationData = certificationData;
    qeReportCertificationData.qeReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(test::getRawPub(*pckCertPubKeyPtr), qeAuthData.data)));
    qeReportCertificationData.qeReportSignature.signature = signEnclaveReport(qeReportCertificationData.qeReport, *pckCertKeyPtr);

    test::QuoteV4Generator::CertificationData qeCertificationData;
    qeCertificationData.keyDataType = constants::PCK_ID_QE_REPORT_CERTIFICATION_DATA;
    qeCertificationData.keyData = qeReportCertificationData.bytes();
    qeCertificationData.size = static_cast<uint16_t>(qeCertificationData.keyData.size());

    quoteV4Generator.withCertificationData(qeCertificationData);
    quoteV4Generator.getAuthSize() = 134 + (uint32_t) qeCertificationData.keyData.size();
    quoteV4Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);
    quoteV4Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV4Generator.getHeader().bytes(), quoteV4Generator.getTdReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV4Generator.buildTdxQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTdxTcbInfoV3JsonBody.begin(), positiveTdxTcbInfoV3JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTdxTcbInfoV3JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_TDX_MODULE_MISMATCH, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusTdxModuleMismatchWhenVerifyTdxQuoteV4WithDifferentSeamAttributes)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    quoteV4Generator.getHeader().teeType = constants::TEE_TYPE_TDX;
    quoteV4Generator.getTdReport().mrSignerSeam.fill(0x00);
    quoteV4Generator.getTdReport().seamAttributes.fill(0x01);
    quoteV4Generator.getTdReport().teeTcbSvn = {0xFF, tdxTcbLevels[0].tdxTcbComponents[1].svn, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,};

    test::QuoteV4Generator::EnclaveReport qeReport{};

    EnclaveIdentityVectorModel model;
    model.version = 2;
    model.id = "TD_QE";

    positiveQEIdentityV2JsonBody = model.toV2JSON();
    model.applyTo(qeReport);

    test::QuoteV4Generator::QeAuthData qeAuthData;
    qeAuthData.data = {};
    qeAuthData.size = 0;

    test::QuoteV4Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PCK_CERT_CHAIN;
    certificationData.keyData = {};
    certificationData.size = 0;

    test::QuoteV4Generator::QEReportCertificationData qeReportCertificationData;
    qeReportCertificationData.qeAuthData = qeAuthData;
    qeReportCertificationData.qeReport = qeReport;
    qeReportCertificationData.certificationData = certificationData;
    qeReportCertificationData.qeReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(test::getRawPub(*pckCertPubKeyPtr), qeAuthData.data)));
    qeReportCertificationData.qeReportSignature.signature = signEnclaveReport(qeReportCertificationData.qeReport, *pckCertKeyPtr);

    test::QuoteV4Generator::CertificationData qeCertificationData;
    qeCertificationData.keyDataType = constants::PCK_ID_QE_REPORT_CERTIFICATION_DATA;
    qeCertificationData.keyData = qeReportCertificationData.bytes();
    qeCertificationData.size = static_cast<uint16_t>(qeCertificationData.keyData.size());

    quoteV4Generator.withCertificationData(qeCertificationData);
    quoteV4Generator.getAuthSize() = 134 + (uint32_t) qeCertificationData.keyData.size();
    quoteV4Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);
    quoteV4Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV4Generator.getHeader().bytes(), quoteV4Generator.getTdReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV4Generator.buildTdxQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTdxTcbInfoV3JsonBody.begin(), positiveTdxTcbInfoV3JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTdxTcbInfoV3JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_TDX_MODULE_MISMATCH, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifyTdxQuoteV4WithoutQeIdentitySuccessffuly)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    quoteV4Generator.getHeader().teeType = constants::TEE_TYPE_TDX;
    std::copy_n(tdxModule.mrsigner.begin(), tdxModule.mrsigner.size(), quoteV4Generator.getTdReport().mrSignerSeam.begin());
    quoteV4Generator.getTdReport().seamAttributes.fill(0x00);
    quoteV4Generator.getTdReport().teeTcbSvn = {0xFF, tdxTcbLevels[0].tdxTcbComponents[1].svn, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,};

    test::QuoteV4Generator::EnclaveReport qeReport{};

    test::QuoteV4Generator::QeAuthData qeAuthData;
    qeAuthData.data = {};
    qeAuthData.size = 0;

    test::QuoteV4Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PCK_CERT_CHAIN;
    certificationData.keyData = {};
    certificationData.size = 0;

    test::QuoteV4Generator::QEReportCertificationData qeReportCertificationData;
    qeReportCertificationData.qeAuthData = qeAuthData;
    qeReportCertificationData.qeReport = qeReport;
    qeReportCertificationData.certificationData = certificationData;
    qeReportCertificationData.qeReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(test::getRawPub(*pckCertPubKeyPtr), qeAuthData.data)));
    qeReportCertificationData.qeReportSignature.signature = signEnclaveReport(qeReportCertificationData.qeReport, *pckCertKeyPtr);

    test::QuoteV4Generator::CertificationData qeCertificationData;
    qeCertificationData.keyDataType = constants::PCK_ID_QE_REPORT_CERTIFICATION_DATA;
    qeCertificationData.keyData = qeReportCertificationData.bytes();
    qeCertificationData.size = static_cast<uint16_t>(qeCertificationData.keyData.size());

    quoteV4Generator.withCertificationData(qeCertificationData);
    quoteV4Generator.getAuthSize() = 134 + (uint32_t) qeCertificationData.keyData.size();
    quoteV4Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);
    quoteV4Generator.getAuthData().ecdsaSignature.signature =
            signAndGetRaw(concat(quoteV4Generator.getHeader().bytes(), quoteV4Generator.getTdReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV4Generator.buildTdxQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveTdxTcbInfoV3JsonBody.begin(), positiveTdxTcbInfoV3JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveTdxTcbInfoV3JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), nullptr);

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}

TEST_F(VerifyQuoteIT, shouldReturnedStatusOKWhenVerifyQuoteV3WithSgxTcbInfoV3Successffuly)
{
    // GIVEN
    auto pckCertPubKeyPtr = EVP_PKEY_get0_EC_KEY(key.get());
    auto pckCertKeyPtr = key.get();

    test::QuoteV3Generator::CertificationData certificationData;
    certificationData.keyDataType = constants::PCK_ID_PLAIN_PPID;
    certificationData.keyData = concat(ppid, concat(cpusvn, pcesvnLE));
    certificationData.size = static_cast<uint16_t>(certificationData.keyData.size());

    quoteV3Generator.withcertificationData(certificationData);
    quoteV3Generator.getAuthSize() += (uint32_t) certificationData.keyData.size();
    quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey = test::getRawPub(*pckCertPubKeyPtr);

    enclaveReport.reportData = assingFirst32(DigestUtils::sha256DigestArray(concat(quoteV3Generator.getAuthData().ecdsaAttestationKey.publicKey,
                                                                                   quoteV3Generator.getAuthData().qeAuthData.data)));

    quoteV3Generator.getAuthData().qeReport = enclaveReport;
    quoteV3Generator.getAuthData().qeReportSignature.signature =
    signEnclaveReport(quoteV3Generator.getAuthData().qeReport, *pckCertKeyPtr);
    quoteV3Generator.getAuthData().ecdsaSignature.signature =
    signAndGetRaw(concat(quoteV3Generator.getHeader().bytes(), quoteV3Generator.getEnclaveReport().bytes()), *pckCertKeyPtr);

    auto quote = quoteV3Generator.buildQuote();
    auto pckPem = certGenerator.x509ToString(cert.get());
    auto pckCrl = getValidCrl(interCert);
    auto tcbInfoBodyBytes = Bytes{};
    tcbInfoBodyBytes.insert(tcbInfoBodyBytes.end(), positiveSgxTcbInfoV3JsonBody.begin(), positiveSgxTcbInfoV3JsonBody.end());
    auto signatureTcb = EcdsaSignatureGenerator::signECDSA_SHA256(tcbInfoBodyBytes, key.get());
    auto tcbInfoJsonWithSignature = tcbInfoJsonGenerator(positiveSgxTcbInfoV3JsonBody,
                                                         EcdsaSignatureGenerator::signatureToHexString(signatureTcb));

    auto qeIdentityBodyBytes = Bytes{};
    qeIdentityBodyBytes.insert(qeIdentityBodyBytes.end(), positiveQEIdentityV2JsonBody.begin(), positiveQEIdentityV2JsonBody.end());
    auto signatureQE = EcdsaSignatureGenerator::signECDSA_SHA256(qeIdentityBodyBytes, key.get());
    auto qeIdentityJsonWithSignature = ::enclaveIdentityJsonWithSignature(positiveQEIdentityV2JsonBody,
                                                                     EcdsaSignatureGenerator::signatureToHexString(
                                                                             signatureQE));

    // WHEN
    auto result = sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckPem.c_str(), pckCrl.c_str(),
                                            tcbInfoJsonWithSignature.c_str(), qeIdentityJsonWithSignature.c_str());

    // THEN
    EXPECT_EQ(STATUS_OK, result);
}
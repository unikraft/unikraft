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
#include <Verifiers/EnclaveReportVerifier.h>
#include <Verifiers/EnclaveIdentityParser.h>
#include <numeric>
#include <iostream>
#include <QuoteV3Generator.h>
#include <QuoteVerification/Quote.h>
#include <EnclaveIdentityGenerator.h>

using namespace testing;
using namespace ::intel::sgx::dcap;
using namespace intel::sgx::dcap::test;
using namespace std;

struct EnclaveIdentityParserUT : public Test
{
    EnclaveIdentityParser parser;
};

TEST_F(EnclaveIdentityParserUT, shouldReturnStatusOkWhenJsonIsOk)
{
    string json = enclaveIdentityJsonWithSignature(EnclaveIdentityVectorModel().toV2JSON());
    auto result = parser.parse(json);

    ASSERT_EQ(STATUS_OK, result->getStatus());
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenMiscselectIsWrong)
{
    EnclaveIdentityVectorModel model;
    model.miscselect = {{1, 1}};
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenOptionalFieldIsInvalid)
{
    EnclaveIdentityVectorModel model;
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    removeWordFromString("mrenclave", json);
    removeWordFromString("mrsigner", json);
    removeWordFromString("isvprodid", json);
    removeWordFromString("isvsvn", json);
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenVerionFieldIsInvalid)
{
    EnclaveIdentityVectorModel model;
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    removeWordFromString("version", json);
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenMiscselectHasIncorrectSize)
{
    EnclaveIdentityVectorModel model;
    model.miscselect= {{1, 1}};
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenMiscselectIsNotHexString)
{
    EnclaveIdentityStringModel model;
    model.miscselect = "xyz00000";
    string json = enclaveIdentityJsonWithSignature(model.toJSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenMiscselectMaskHasIncorrectSize)
{
    EnclaveIdentityVectorModel model;
    model.miscselectMask = {{1, 1}};
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenMiscselectMaskIsNotHexString)
{
    EnclaveIdentityStringModel model;
    model.miscselectMask = "xyz00000";
    string json = enclaveIdentityJsonWithSignature(model.toJSON());

    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenAttributesHasIncorrectSize)
{
    EnclaveIdentityVectorModel model;
    model.attributes = {{1, 1}};
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenAttributesIsNotHexString)
{
    EnclaveIdentityStringModel model;
    model.attributes = "xyz45678900000000000000123456789";
    string json = enclaveIdentityJsonWithSignature(model.toJSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenAttributesMaskHasIncorrectSize)
{
    EnclaveIdentityVectorModel model;
    model.attributesMask = {{1, 1}};
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenAttributesMaskIsNotHexString)
{
    EnclaveIdentityStringModel model;
    model.attributesMask = "xyz45678900000000000000123456789";
    string json = enclaveIdentityJsonWithSignature(model.toJSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenIssuedateIsWrong)
{
    EnclaveIdentityStringModel model;
    model.issueDate = "2018-08-22T10:09:";
    string json = enclaveIdentityJsonWithSignature(model.toJSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityInvalidWhenNextUpdateIsWrong)
{
    EnclaveIdentityStringModel model;
    model.nextUpdate = "2018-08-22T10:09:";
    string json = enclaveIdentityJsonWithSignature(model.toJSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldReturnEnclaveIdentityUnsuportedVersionWhenVersionIsWrong)
{
    EnclaveIdentityVectorModel model;
    model.version = 5;
    string json = enclaveIdentityJsonWithSignature(model.toV2JSON());
    try {
        parser.parse(json);
        FAIL();
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_VERSION, ex.getStatus());
    }
}


TEST_F(EnclaveIdentityParserUT, positiveQE)
{
    auto json = enclaveIdentityJsonWithSignature();

    std::vector<uint8_t> expectedMiscSelect = {0x8f, 0xa6, 0x44, 0x72};
    std::vector<uint8_t> expectedMiscSelectMask = {0x00, 0x00, 0xff, 0xfa};
    std::vector<uint8_t> expectedAttributes = {0x12, 0x54, 0x86, 0x35, 0x48, 0xaf, 0x4a, 0x6b, 0x2f, 0xcc, 0x2d, 0x32, 0x44, 0x78, 0x44, 0x52};
    std::vector<uint8_t> expectedAttributesMask = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    std::vector<uint8_t> expectedMrSigner = {0xaa, 0xff, 0x34, 0xff, 0xa5, 0x19, 0x81, 0x95, 0x1a, 0x61, 0xd6, 0x16, 0xb1,
                                             0x6c, 0x16, 0xf1, 0x65, 0x1c, 0x65, 0x16, 0xe5, 0x1f, 0x65, 0x1d, 0x26, 0xa6, 0x16, 0x6e, 0xd5, 0x67, 0x9c, 0x79};
    uint32_t expectedIsvProdId = 3;
    try
    {
        auto enclaveIdentity = parser.parse(json);
        auto *jsonObject = dynamic_cast<EnclaveIdentityV2 *>(enclaveIdentity.get());
        EXPECT_EQ(jsonObject->getVersion(), 2);
        EXPECT_EQ(jsonObject->getMiscselect(), expectedMiscSelect);
        EXPECT_EQ(jsonObject->getMiscselectMask(), expectedMiscSelectMask);
        EXPECT_EQ(jsonObject->getAttributes(), expectedAttributes);
        EXPECT_EQ(jsonObject->getAttributesMask(), expectedAttributesMask);
        EXPECT_EQ(jsonObject->getMrsigner(), expectedMrSigner);
        EXPECT_EQ(jsonObject->getIsvProdId(), expectedIsvProdId);
        EXPECT_EQ(jsonObject->getID(), EnclaveID::QE);
        EXPECT_EQ(jsonObject->getTcbEvaluationDataNumber(), 0);
        EXPECT_EQ(jsonObject->getTcbStatus(8), TcbStatus::UpToDate);
        EXPECT_EQ(jsonObject->getTcbStatus(7), TcbStatus::OutOfDate);
        EXPECT_EQ(jsonObject->getTcbStatus(6), TcbStatus::ConfigurationNeeded);
        EXPECT_EQ(jsonObject->getTcbStatus(5), TcbStatus::OutOfDateConfigurationNeeded);
        EXPECT_EQ(jsonObject->getTcbStatus(4), TcbStatus::Revoked);
        EXPECT_EQ(jsonObject->getTcbStatus(3), TcbStatus::Revoked);
    }
    catch(const ParserException &ex)
    {
        FAIL() << "Unexpected status: " << ex.getStatus();
    }
}

TEST_F(EnclaveIdentityParserUT, positiveQVE)
{
    auto json = enclaveIdentityJsonWithSignature(R"json({
            "id": "QVE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                },
                {
                    "tcb":{ "isvsvn":7 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"OutOfDate"
                },
                {
                    "tcb":{ "isvsvn":6 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"ConfigurationNeeded"
                },
                {
                    "tcb":{ "isvsvn":5 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"OutOfDateConfigurationNeeded"
                },
                {
                    "tcb":{ "isvsvn":4 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"Revoked"
                }
            ]
        })json");

    std::vector<uint8_t> expectedMiscSelect = {0x8f, 0xa6, 0x44, 0x72};
    std::vector<uint8_t> expectedMiscSelectMask = {0x00, 0x00, 0xff, 0xfa};
    std::vector<uint8_t> expectedAttributes = {0x12, 0x54, 0x86, 0x35, 0x48, 0xaf, 0x4a, 0x6b, 0x2f, 0xcc, 0x2d, 0x32, 0x44, 0x78, 0x44, 0x52};
    std::vector<uint8_t> expectedAttributesMask = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    std::vector<uint8_t> expectedMrSigner = {0xaa, 0xff, 0x34, 0xff, 0xa5, 0x19, 0x81, 0x95, 0x1a, 0x61, 0xd6, 0x16, 0xb1,
                                             0x6c, 0x16, 0xf1, 0x65, 0x1c, 0x65, 0x16, 0xe5, 0x1f, 0x65, 0x1d, 0x26, 0xa6, 0x16, 0x6e, 0xd5, 0x67, 0x9c, 0x79};
    uint32_t expectedIsvProdId = 3;
    try
    {
        auto enclaveIdentity = parser.parse(json);
        auto *jsonObject = dynamic_cast<EnclaveIdentityV2 *>(enclaveIdentity.get());
        EXPECT_EQ(jsonObject->getVersion(), 2);
        EXPECT_EQ(jsonObject->getMiscselect(), expectedMiscSelect);
        EXPECT_EQ(jsonObject->getMiscselectMask(), expectedMiscSelectMask);
        EXPECT_EQ(jsonObject->getAttributes(), expectedAttributes);
        EXPECT_EQ(jsonObject->getAttributesMask(), expectedAttributesMask);
        EXPECT_EQ(jsonObject->getMrsigner(), expectedMrSigner);
        EXPECT_EQ(jsonObject->getIsvProdId(), expectedIsvProdId);
        EXPECT_EQ(jsonObject->getID(), EnclaveID::QVE);
        EXPECT_EQ(jsonObject->getTcbEvaluationDataNumber(), 0);
        EXPECT_EQ(jsonObject->getTcbStatus(8), TcbStatus::UpToDate);
        EXPECT_EQ(jsonObject->getTcbStatus(7), TcbStatus::OutOfDate);
        EXPECT_EQ(jsonObject->getTcbStatus(6), TcbStatus::ConfigurationNeeded);
        EXPECT_EQ(jsonObject->getTcbStatus(5), TcbStatus::OutOfDateConfigurationNeeded);
        EXPECT_EQ(jsonObject->getTcbStatus(4), TcbStatus::Revoked);
        EXPECT_EQ(jsonObject->getTcbStatus(3), TcbStatus::Revoked);
    }
    catch(const ParserException &ex)
    {
        FAIL() << "Unexpected status: " << ex.getStatus();
    }
}

TEST_F(EnclaveIdentityParserUT, positiveTD_QE)
{
    auto json = enclaveIdentityJsonWithSignature(R"json({
            "id": "TD_QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                },
                {
                    "tcb":{ "isvsvn":7 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"OutOfDate"
                },
                {
                    "tcb":{ "isvsvn":6 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"ConfigurationNeeded"
                },
                {
                    "tcb":{ "isvsvn":5 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"OutOfDateConfigurationNeeded"
                },
                {
                    "tcb":{ "isvsvn":4 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"Revoked"
                }
            ]
        })json");

    std::vector<uint8_t> expectedMiscSelect = {0x8f, 0xa6, 0x44, 0x72};
    std::vector<uint8_t> expectedMiscSelectMask = {0x00, 0x00, 0xff, 0xfa};
    std::vector<uint8_t> expectedAttributes = {0x12, 0x54, 0x86, 0x35, 0x48, 0xaf, 0x4a, 0x6b, 0x2f, 0xcc, 0x2d, 0x32, 0x44, 0x78, 0x44, 0x52};
    std::vector<uint8_t> expectedAttributesMask = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    std::vector<uint8_t> expectedMrSigner = {0xaa, 0xff, 0x34, 0xff, 0xa5, 0x19, 0x81, 0x95, 0x1a, 0x61, 0xd6, 0x16, 0xb1,
                                             0x6c, 0x16, 0xf1, 0x65, 0x1c, 0x65, 0x16, 0xe5, 0x1f, 0x65, 0x1d, 0x26, 0xa6, 0x16, 0x6e, 0xd5, 0x67, 0x9c, 0x79};
    uint32_t expectedIsvProdId = 3;
    try
    {
        auto enclaveIdentity = parser.parse(json);
        auto *jsonObject = dynamic_cast<EnclaveIdentityV2 *>(enclaveIdentity.get());
        EXPECT_EQ(jsonObject->getVersion(), 2);
        EXPECT_EQ(jsonObject->getMiscselect(), expectedMiscSelect);
        EXPECT_EQ(jsonObject->getMiscselectMask(), expectedMiscSelectMask);
        EXPECT_EQ(jsonObject->getAttributes(), expectedAttributes);
        EXPECT_EQ(jsonObject->getAttributesMask(), expectedAttributesMask);
        EXPECT_EQ(jsonObject->getMrsigner(), expectedMrSigner);
        EXPECT_EQ(jsonObject->getIsvProdId(), expectedIsvProdId);
        EXPECT_EQ(jsonObject->getID(), EnclaveID::TD_QE);
        EXPECT_EQ(jsonObject->getTcbEvaluationDataNumber(), 0);
        EXPECT_EQ(jsonObject->getTcbStatus(8), TcbStatus::UpToDate);
        EXPECT_EQ(jsonObject->getTcbStatus(7), TcbStatus::OutOfDate);
        EXPECT_EQ(jsonObject->getTcbStatus(6), TcbStatus::ConfigurationNeeded);
        EXPECT_EQ(jsonObject->getTcbStatus(5), TcbStatus::OutOfDateConfigurationNeeded);
        EXPECT_EQ(jsonObject->getTcbStatus(4), TcbStatus::Revoked);
        EXPECT_EQ(jsonObject->getTcbStatus(3), TcbStatus::Revoked);
    }
    catch(const ParserException &ex)
    {
        FAIL() << "Unexpected status: " << ex.getStatus();
    }
}

TEST_F(EnclaveIdentityParserUT, positiveWithExtraField)
{
    auto json = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ],
            "extraField": "ExtraValue"
        })json";

    EXPECT_EQ(STATUS_OK, parser.parse(enclaveIdentityJsonWithSignature(json))->getStatus());
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenInitializedWithEmptyString)
{
    try {
        parser.parse("");
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWHenInitializedWithInvalidJSON)
{
    try {
        parser.parse("Plain string.");
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenQEIdentityFieldIsMissing)
{
    const std::string json = R"json({"signature": "adad"})json";

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenSignatureFieldIsMissing)
{
    auto json = R"json({"enclaveIdentity": {
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        }})json";

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenQeIdentityIsArray)
{
    auto qeidTemplate = R"json([])json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIdFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenVersionFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";

    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIssueDateFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";

    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenNextUpdateFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbEvaluationDataNumberFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectMaskFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesMaskFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMrsignerFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIsvprodidFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsTcbFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsTcbIsvSvnFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsTcbDateFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsTcbStatusFieldIsMissing)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenVersionFieldIsNotEqual1or2)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 23,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_VERSION, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenVersionFieldIsNotANumber)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": "2",
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIdFieldHasInvalidType)
{
    auto qeidTemplate = R"json({
            "id": 0,
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIdFieldHasInvalidValue)
{
    auto qeidTemplate = R"json({
            "id": "QC",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIssueDateIsMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45:00",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIssueDateIsNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": 123,
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenNextUpdateIsMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "219-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenNextUpdateIsNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": 2019,
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectIsMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "qwe-4472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectIsNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": 44,
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectIsTooShort)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa6447",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectIsTooLong)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472f",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectMaskIsMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "asdfgh56",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectMaskIsNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": 234,
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectMaskIsTooShort)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fff",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMiscselectMaskIsTooLong)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "000012345",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesAreMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "qwp4863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesAreNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": true,
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesAreTooShort)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d324478445",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesAreTooLong)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d32447844521",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesMaskIsMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffff****",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesMaskIsNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": 0,
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesMaskIsTooShort)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "fffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenAttributesMaskIsTooLong)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff0",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMrsignerIsMalformed)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "**++lkffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMrsignerIsNotAString)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": 45,
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMrsignerIsTooShort)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c7",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenMrSignerIsTooLong)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c790",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIsvprodidIsNotANumber)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": "3",
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenIsvsvnIsNotANumber)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn": "8" },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsMaskFieldIsAnEmptyArray)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": []
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsTcbStatusFieldInvalidValue)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:29Z",
                    "tcbStatus":"NotUpToDate"
                }
            ]
        })json";
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelsTcbDateFieldInvalidValue)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:290Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json"; // changed ":29Z" to ":290Z"
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

TEST_F(EnclaveIdentityParserUT, shouldFailWhenTcbLevelIsInvalidType)
{
    auto qeidTemplate = R"json({
            "id": "QE",
            "version": 2,
            "issueDate": "2018-10-04T11:10:45Z",
            "nextUpdate": "2019-06-21T12:36:02Z",
            "tcbEvaluationDataNumber":0,
            "miscselect": "8fa64472",
            "miscselectMask": "0000fffa",
            "attributes": "1254863548af4a6b2fcc2d3244784452",
            "attributesMask": "ffffffffffffffffffffffffffffffff",
            "mrsigner": "aaff34ffa51981951a61d616b16c16f1651c6516e51f651d26a6166ed5679c79",
            "isvprodid": 3,
            "tcbLevels": [
                1,
                {
                    "tcb":{ "isvsvn":8 },
                    "tcbDate":"2019-06-23T10:41:290Z",
                    "tcbStatus":"UpToDate"
                }
            ]
        })json"; // changed ":29Z" to ":290Z"
    auto json = enclaveIdentityJsonWithSignature(qeidTemplate);

    try {
        parser.parse(json);
        FAIL() << "Test should throw exception";
    }
    catch (const ParserException &ex)
    {
        EXPECT_EQ(STATUS_SGX_ENCLAVE_IDENTITY_INVALID, ex.getStatus());
    }
}

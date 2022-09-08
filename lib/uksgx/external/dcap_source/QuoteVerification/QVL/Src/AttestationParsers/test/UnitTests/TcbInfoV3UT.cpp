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

#include "TcbInfoGenerator.h"
#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "X509Constants.h"
#include <Utils/TimeUtils.h>

#include <gtest/gtest.h>
#include "rapidjson/document.h"

using namespace testing;
using namespace intel::sgx::dcap;

struct TcbInfoV3UT : public Test
{
};

void expectCommonDefaultTcbInfo(const parser::json::TcbInfo& tcbInfo)
{
    EXPECT_EQ(tcbInfo.getPceId(), DEFAULT_PCEID);
    EXPECT_EQ(tcbInfo.getFmspc(), DEFAULT_FMSPC);
    EXPECT_EQ(tcbInfo.getSignature(), DEFAULT_SIGNATURE);
    EXPECT_EQ(tcbInfo.getTcbType(), DEFAULT_TCB_TYPE);
    EXPECT_EQ(tcbInfo.getTcbEvaluationDataNumber(), DEFAULT_TCB_EVALUATION_DATA_NUMBER);
    EXPECT_EQ(tcbInfo.getIssueDate(), getEpochTimeFromString(DEFAULT_ISSUE_DATE));
    EXPECT_EQ(tcbInfo.getNextUpdate(), getEpochTimeFromString(DEFAULT_NEXT_UPDATE));
    EXPECT_EQ(tcbInfo.getVersion(), 3);

    EXPECT_EQ(1, tcbInfo.getTcbLevels().size());
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());

    EXPECT_EQ(iterator->getSgxTcbComponents().size(), 16);
    auto cpusvn = iterator->getCpuSvn();
    EXPECT_EQ(cpusvn.size(), 16);
    for (uint32_t i=0; i < constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(iterator->getSgxTcbComponentSvn(i), DEFAULT_CPUSVN[i]);
        EXPECT_EQ(iterator->getSgxTcbComponent(i).getSvn(), DEFAULT_CPUSVN[i]);
        EXPECT_EQ(cpusvn[i], DEFAULT_CPUSVN[i]);
    }
    auto component = iterator->getSgxTcbComponent(0);
    EXPECT_EQ(component.getCategory(), "cat1");
    EXPECT_EQ(component.getType(), "type1");

    component = iterator->getSgxTcbComponent(5);
    EXPECT_EQ(component.getCategory(), "cat1");
    EXPECT_EQ(component.getType(), "type2");

    component = iterator->getSgxTcbComponent(6);
    EXPECT_EQ(component.getCategory(), "cat2");
    EXPECT_EQ(component.getType(), "type1");

    EXPECT_EQ(iterator->getTcbDate(), getEpochTimeFromString(DEFAULT_TCB_DATE));
    EXPECT_EQ(iterator->getPceSvn(), DEFAULT_PCESVN);
    EXPECT_EQ(iterator->getStatus(), "UpToDate");
}

TEST_F(TcbInfoV3UT, shouldSuccessfullyParseSgxTcbInfoWhenAllRequiredDataProvided)
{
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validSgxTcbV3));

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);

    EXPECT_EQ(tcbInfo.getId(), parser::json::TcbInfo::SGX_ID);
    expectCommonDefaultTcbInfo(tcbInfo);

    try
    {
        tcbInfo.getTdxModule();
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TdxModule is only valid for TDX TCB Info");
    }
}

TEST_F(TcbInfoV3UT, shouldSuccessfullyParseTdxTcbInfoWhenAllRequiredDataProvided)
{
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);

    EXPECT_EQ(tcbInfo.getId(), parser::json::TcbInfo::TDX_ID);
    expectCommonDefaultTcbInfo(tcbInfo);

    const auto& tdxModule = tcbInfo.getTdxModule();
    EXPECT_EQ(tdxModule.getMrSigner(), DEFAULT_TDXMODULE_MRSIGNER);
    EXPECT_EQ(tdxModule.getAttributes(), DEFAULT_TDXMODULE_ATTRIBUTES);
    EXPECT_EQ(tdxModule.getAttributesMask(), DEFAULT_TDXMODULE_ATTRIBUTESMASK);

    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());

    EXPECT_EQ(iterator->getTdxTcbComponents().size(), 16);

    for (uint32_t i=0; i < constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(iterator->getTdxTcbComponent(i).getSvn(), DEFAULT_CPUSVN[i]);
    }
    auto component = iterator->getTdxTcbComponent(0);
    EXPECT_EQ(component.getCategory(), "cat1");
    EXPECT_EQ(component.getType(), "type1");

    component = iterator->getTdxTcbComponent(5);
    EXPECT_EQ(component.getCategory(), "cat1");
    EXPECT_EQ(component.getType(), "type2");

    component = iterator->getTdxTcbComponent(6);
    EXPECT_EQ(component.getCategory(), "cat2");
    EXPECT_EQ(component.getType(), "type1");
}

TEST_F(TcbInfoV3UT, shouldFailWhenGetTdxTcbComponentsOnSgxTcbInfo) {
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validSgxTcbV3));

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    try
    {
        iterator->getTdxTcbComponents();
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX TCB Components is not a valid field in SGX TCB Info structure");
    }

    try
    {
        iterator->getTdxTcbComponent(0);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX TCB Components is not a valid field in SGX TCB Info structure");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenGetTdxTcbComponentsOnTcbInfoV2) {
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, TcbInfoGenerator::generateTcbLevelV2());

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    try
    {
        iterator->getTdxTcbComponents();
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX TCB Components is not a valid field in TCB Info V1 and V2 structure");
    }

    try
    {
        iterator->getTdxTcbComponent(0);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX TCB Components is not a valid field in TCB Info V1 and V2 structure");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoDoesntHaveTdxModule)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Info JSON for TDX should have [tdxModule] field");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenSgxTcbInfoHaveTdxModule)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "SGX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F",
                "attributes": "0000000000000000",
                "attributesMask": "FFFFFFFFFFFFFFFF"
            },
            "tcbLevels": [%s]
        },
        %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Info JSON for SGX should not have [tdxModule] field");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleMrSignerIsNotHexstring)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "ZZZZ02030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F",
                "attributes": "0000000000000000",
                "attributesMask": "FFFFFFFFFFFFFFFF"
            },
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX Module JSON should have [mrsigner] field and it should be 48 bytes encoded as hexstring");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleMrSignerHasIncorrectLength)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "000102030405060708090A0B0C0D0E0F",
                "attributes": "0000000000000000",
                "attributesMask": "FFFFFFFFFFFFFFFF"
            },
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX Module JSON should have [mrsigner] field and it should be 48 bytes encoded as hexstring");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleAttributesIsNotHexstring)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F",
                "attributes": "ZZ00000000000000",
                "attributesMask": "FFFFFFFFFFFFFFFF"
            },
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX Module JSON should have [attributes] field and it should be 8 bytes encoded as hexstring");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleAttributesHasIncorrectLength)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F",
                "attributes": "0000",
                "attributesMask": "FFFFFFFFFFFFFFFF"
            },
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX Module JSON should have [attributes] field and it should be 8 bytes encoded as hexstring");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleAttributesMaskIsNotHexstring)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F",
                "attributes": "0000000000000000",
                "attributesMask": "ZZFFFFFFFFFFFFFF"
            },
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX Module JSON should have [attributesMask] field and it should be 8 bytes encoded as hexstring");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleAttributesMaskHasIncorrectLength)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": {
                "mrsigner": "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F",
                "attributes": "0000000000000000",
                "attributesMask": "FFFF"
            },
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TDX Module JSON should have [attributesMask] field and it should be 8 bytes encoded as hexstring");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxModuleIsNotObject)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "id": "TDX",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tdxModule": 5,
            "tcbLevels": [%s]
        },
    %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "[tdxModule] field should be an object");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbInfoDoesntHaveId)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "version": 3,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "mrsignerseam": "0A0B0C0D0E0F010203040506",
            "tcbLevels": [%s]
        },
        %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Info JSON should has [id] field");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbInfoHasInvalidId)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "id": "TEST",
            "version": 3,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "mrsignerseam": "0A0B0C0D0E0F010203040506",
            "tcbLevels": [%s]
        },
        %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), "Unsupported id[TEST] value for field of TCB info JSON. Supported identifiers are ["
                                           + parser::json::TcbInfo::SGX_ID + " | " + parser::json::TcbInfo::TDX_ID + "]");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenIdIsNotString)
{
    const std::string tcbInfoJsonTemplate = R"json({
        "tcbInfo": {
            "id": 5,
            "version": 3,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "mrsignerseam": "0A0B0C0D0E0F010203040506",
            "tcbLevels": [%s]
        },
        %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoJsonTemplate, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validTdxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), "Could not parse [id] field of TCB info JSON to string");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoDoesntHaveTdxTcbComponents)
{
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, validSgxTcbV3));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level JSON for TDX should have [tdxtcbcomponents] field");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenSgxTcbInfoDoesntHaveSgxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "pcesvn": 30865
    })json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level JSON should have [sgxtcbcomponents] field");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoDoesntHaveSgxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "tdxtcbcomponents": [
            {"svn": 12, "category": "cat1", "type": "type1"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level JSON should have [sgxtcbcomponents] field");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoHaveWrongNumberOfSgxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 12, "category": "cat1", "type": "type1"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"}
        ],
        "tdxtcbcomponents": [
            {"svn": 12, "category": "cat1", "type": "type1"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level [sgxtcbcomponents] array should have 16 entries");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoHaveWrongNumberOfTdxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 12, "category": "cat1", "type": "type1"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "tdxtcbcomponents": [
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level [tdxtcbcomponents] array should have 16 entries");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenSgxTcbInfoHaveWrongNumberOfSgxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"}
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level [sgxtcbcomponents] array should have 16 entries");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenSgxTcbInfoHaveEmptySgxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level [sgxtcbcomponents] array should have 16 entries");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoHaveEmptySgxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
        ],
        "tdxtcbcomponents": [
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level [sgxtcbcomponents] array should have 16 entries");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbInfoHaveEmptyTdxTcbComponents)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 23 },
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "tdxtcbcomponents": [
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level [tdxtcbcomponents] array should have 16 entries");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenSgxTcbComponentsIsNotArray)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": "test",
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validSgxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level JSON's [sgxtcbcomponents] field should be an array");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTdxTcbComponentsIsNotArray)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 12, "category": "cat1", "type": "type1"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5, "category": "cat2", "type": "type2"},
            {"svn": 6, "category": "cat1", "type": "type3"},
            {"svn": 7 }
        ],
        "tdxtcbcomponents": "test",
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));

    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level JSON's [tdxtcbcomponents] field should be an array");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbComponentCategoryIsNotString)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 12, "category": 5, "type": "type1"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5 },
            {"svn": 6 },
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));
    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Component JSON's [category] field should be string");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbComponentTypeIsNotString)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 12, "category": "test", "type": 4},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5 },
            {"svn": 6 },
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));
    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Component JSON's [type] field should be string");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbComponentSvnIsNotInteger)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": "test", "category": "test", "type": "test"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"svn": 0, "category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5 },
            {"svn": 6 },
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));
    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Component JSON should has [svn] field and it should be unsigned integer");
    }
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbComponentDoesntHaveSvnField)
{
    const std::string tcbJsonTemplate = R"json(
    "tcb": {
        "sgxtcbcomponents": [
            {"svn": 5, "category": "test", "type": "test"},
            {"svn": 23 },
            {"svn": 34 },
            {"svn": 45 },
            {"svn": 100 },
            {"category": "cat1", "type": "type2"},
            {"svn": 1, "category": "cat2", "type": "type1"},
            {"svn": 156 },
            {"svn": 208 },
            {"svn": 255 },
            {"svn": 2 },
            {"svn": 3 },
            {"svn": 4 },
            {"svn": 5 },
            {"svn": 6 },
            {"svn": 7 }
        ],
        "pcesvn": 30865
    })json";

    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTdxTcbInfoV3Template, TcbInfoGenerator::generateTcbLevelV3(validTcbLevelV3Template, tcbJsonTemplate));
    try
    {
        const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB Component JSON should has [svn] field and it should be unsigned integer");
    }
}

TEST_F(TcbInfoV3UT, shouldCompareTrueTdxTcbLevelsV3)
{
    parser::json::TcbLevel tcbLevel1("TDX",
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    parser::json::TcbLevel tcbLevel2("TDX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    auto result = tcbLevel2 > tcbLevel1;
    EXPECT_EQ(result, true);
}

TEST_F(TcbInfoV3UT, shouldCompareTrueTdxTcbLevelsV3WhenOnlyTdxTcbComponentsAreHigher)
{
    parser::json::TcbLevel tcbLevel1("TDX",
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    parser::json::TcbLevel tcbLevel2("TDX",
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    auto result = tcbLevel2 > tcbLevel1;
    EXPECT_EQ(result, true);
}

TEST_F(TcbInfoV3UT, shouldCompareFalseTdxTcbLevelsV3WhenOnlyTdxTcbComponentsAreLower)
{
    parser::json::TcbLevel tcbLevel1("TDX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    parser::json::TcbLevel tcbLevel2("TDX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    auto result = tcbLevel2 > tcbLevel1;
    EXPECT_EQ(result, false);
}

TEST_F(TcbInfoV3UT, shouldCompareFalseSgxTcbLevelsV3WhenOnlyTdxTcbComponentsAreLowerButNotTdx)
{
    parser::json::TcbLevel tcbLevel1("SGX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    parser::json::TcbLevel tcbLevel2("SGX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    auto result = tcbLevel2 > tcbLevel1;
    EXPECT_EQ(result, false);
}

TEST_F(TcbInfoV3UT, shouldCompareTrueSgxTcbLevelsV3WhenPcesvnIsHigher)
{
    parser::json::TcbLevel tcbLevel1("TDX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     0, "test");

    parser::json::TcbLevel tcbLevel2("TDX",
                                     {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                     1, "test");

    auto result = tcbLevel2 > tcbLevel1;
    EXPECT_EQ(result, true);
}

TEST_F(TcbInfoV3UT, shouldFailWhenTcbHasInvalidFormat)
{
    // test case input comes from fuzzing
    std::string tcbInfoJson = "{\"tcbInfo\":{\"id\":\"SGX\",\"tcbType\":0,\"tcbEvaluationDataNumber\":0,\"version\":3,\"issueDate\":\"2021-03-02T10:58:19Z\",\"nextUpdate\":\"2021-03-07T10:58:19Z\",\"fmspc\":\"f9c71eb1d129\",\"pceId\":\"15ce\",\"tcbLevels\":[{\"tcb\":\"2021-03-02T10:58:19Z\",\"next\":{\"sgxtcbcomponents\":[{\"svn\":79},{\"svn\":65432},{\"svn\":106},{\"svn\":116},{\"svn\":65461},{\"svn\":22},{\"svn\":77},{\"svn\":65456},{\"svn\":122},{\"svn\":26},{\"svn\":70},{\"svn\":68},{\"svn\":54},{\"svn\":59},{\"svn\":65534},{\"svq\":65478}],\"pcesvn\":8145},\"tcbStatus\":\"UpToDate\",\"tcbDate\":\"2020-03-02T10:58:19Z\",\"advDs\":[\"INTE-SA076\"]}]},\"signature\":\"040f9bed9d2eca8cf0d8c291dbb66ab1decae50a9d458da65c3355c9f36eb5530c506108bffc498b72ba8ce10e0cc9361ae369a8f080fd01d95157b67a9c3adc\"}";
    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch (const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB level JSON [tcb] field should be an object");
    }
}
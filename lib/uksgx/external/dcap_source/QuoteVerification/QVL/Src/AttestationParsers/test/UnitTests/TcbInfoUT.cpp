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
#include <gmock/gmock.h>

using namespace testing;
using namespace intel::sgx::dcap;

struct TcbInfoUT : public Test
{
};

TEST_F(TcbInfoUT, shouldSuccessfullyParseTcbWhenAllRequiredDataProvided)
{
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo();

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);

    EXPECT_EQ(tcbInfo.getPceId(), DEFAULT_PCEID);
    EXPECT_EQ(tcbInfo.getFmspc(), DEFAULT_FMSPC);
    EXPECT_EQ(tcbInfo.getSignature(), DEFAULT_SIGNATURE);
    EXPECT_EQ(tcbInfo.getInfoBody(), DEFAULT_INFO_BODY);
    EXPECT_EQ(tcbInfo.getTcbType(), DEFAULT_TCB_TYPE);
    EXPECT_EQ(tcbInfo.getTcbEvaluationDataNumber(), DEFAULT_TCB_EVALUATION_DATA_NUMBER);
    EXPECT_EQ(tcbInfo.getIssueDate(), getEpochTimeFromString(DEFAULT_ISSUE_DATE));
    EXPECT_EQ(tcbInfo.getNextUpdate(), getEpochTimeFromString(DEFAULT_NEXT_UPDATE));
    EXPECT_EQ(tcbInfo.getVersion(), 2);
    EXPECT_EQ(1, tcbInfo.getTcbLevels().size());
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    for (uint32_t i=0; i<constants::CPUSVN_BYTE_LEN && iterator != tcbInfo.getTcbLevels().end(); i++)
    {
        EXPECT_EQ(iterator->getSgxTcbComponentSvn(i), DEFAULT_CPUSVN[i]);
        EXPECT_EQ(iterator->getCpuSvn(), DEFAULT_CPUSVN);
    }
    EXPECT_EQ(iterator->getPceSvn(), DEFAULT_PCESVN);
    EXPECT_EQ(iterator->getTcbDate(), getEpochTimeFromString(DEFAULT_TCB_DATE));
    EXPECT_EQ(iterator->getStatus(), "UpToDate");
    EXPECT_EQ(iterator->getAdvisoryIDs().size(), 2);
}

TEST_F(TcbInfoUT, shouldFailWhenInitializedWithEmptyString)
{
    EXPECT_THROW(parser::json::TcbInfo::parse(""), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWHenInitializedWithInvalidJSON)
{
    EXPECT_THROW(parser::json::TcbInfo::parse("Plain string."), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenVersionIsMissing)
{
    const std::string tcbInfoWithoutVersion = R"json({
        "tcbInfo": {
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoWithoutVersion);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenVersionIsNotAnInteger)
{
    const std::string tcbInfoInvalidVersion = R"json({
        "tcbInfo": {
            "version": "asd",
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidVersion);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenVersionIsNotSupported)
{
    const std::string invalidTcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 4,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbLevels": [%s]
        },
        %s})json";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(invalidTcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldSuccessfullyParseMultipleTcbLevels)
{
    std::vector<uint8_t> expectedCpusvn{55, 0, 0, 1, 10, 0, 0, 77, 200, 200, 250, 250, 55, 2, 2, 2};
    uint32_t expectedPcesvn = 66;
    std::vector<uint8_t> expectedRevokedCpusvn{44, 0, 0, 1, 10, 0, 0, 77, 200, 200, 250, 250, 55, 2, 2, 2};
    uint32_t expectedRevokedPcesvn = 65;
    const std::string upToDateTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 55,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 200,
        "sgxtcbcomp11svn": 250,
        "sgxtcbcomp12svn": 250,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 66
    })json";
    const std::string revokedTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 44,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 200,
        "sgxtcbcomp11svn": 250,
        "sgxtcbcomp12svn": 250,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 65
    })json";
    const std::string configurationNeededTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 48,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 200,
        "sgxtcbcomp11svn": 250,
        "sgxtcbcomp12svn": 222,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 66
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, validSgxTcb, validOutOfDateStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, upToDateTcb, validUpToDateStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, revokedTcb, validRevokedStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, configurationNeededTcb, validConfigurationNeededStatus);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    EXPECT_EQ(4, tcbInfo.getTcbLevels().size());
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    for (uint32_t i=0; i<constants::CPUSVN_BYTE_LEN && iterator != tcbInfo.getTcbLevels().end(); i++)
    {
        EXPECT_EQ(expectedCpusvn[i], iterator->getSgxTcbComponentSvn(i));
    }
    EXPECT_EQ(expectedPcesvn, iterator->getPceSvn());
    EXPECT_EQ("UpToDate", iterator->getStatus());
    std::advance(iterator, 2);
    for (uint32_t i=0; i<constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(expectedRevokedCpusvn[i], iterator->getSgxTcbComponentSvn(i));
    }
    EXPECT_EQ(expectedRevokedPcesvn, iterator->getPceSvn());
    EXPECT_EQ("Revoked", iterator->getStatus());
}

TEST_F(TcbInfoUT, shouldSuccessfullyParseMultipleRevokedTcbLevels)
{
    std::vector<uint8_t> expectedRevokedCpusvn{44, 0, 0, 1, 10, 0, 0, 77, 200, 222, 111, 121, 55, 2, 2, 2};
    uint16_t expectedRevokedPcesvn = 66;
    const std::string revokedTcbLatest = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 44,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 222,
        "sgxtcbcomp11svn": 111,
        "sgxtcbcomp12svn": 121,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 66
    })json";
    const std::string otherRevokedTcb1 = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 44,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 222,
        "sgxtcbcomp11svn": 111,
        "sgxtcbcomp12svn": 121,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 65
    })json";
    const std::string otherRevokedTcb2 = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 44,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 0,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 222,
        "sgxtcbcomp11svn": 111,
        "sgxtcbcomp12svn": 121,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 66
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, validSgxTcb, validUpToDateStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, otherRevokedTcb1, validRevokedStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, revokedTcbLatest, validRevokedStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, otherRevokedTcb2, validRevokedStatus);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    EXPECT_EQ(4, tcbInfo.getTcbLevels().size());
    auto iterator = tcbInfo.getTcbLevels().begin();
    std::advance(iterator, 0);
    for (uint32_t i=0; i<constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(expectedRevokedCpusvn[i], iterator->getSgxTcbComponentSvn(i));
    }
    EXPECT_EQ(expectedRevokedPcesvn, iterator->getPceSvn());
    EXPECT_EQ("Revoked", iterator->getStatus());
}

TEST_F(TcbInfoUT, shouldSucceedWhenTcbLevelsContainsOnlyRevokedTcbs)
{
    std::vector<uint8_t> expectedRevokedCpusvn{55, 0, 0, 1, 10, 0, 0, 77, 200, 200, 250, 250, 55, 2, 2, 2};
    uint32_t expectedRevokedPcesvn = 66;
    const std::string revokedTcb1 = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 55,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 200,
        "sgxtcbcomp11svn": 250,
        "sgxtcbcomp12svn": 250,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 66
    })json";
    const std::string revokedTcb2 = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 44,
        "sgxtcbcomp02svn": 0,
        "sgxtcbcomp03svn": 0,
        "sgxtcbcomp04svn": 1,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 77,
        "sgxtcbcomp09svn": 200,
        "sgxtcbcomp10svn": 200,
        "sgxtcbcomp11svn": 250,
        "sgxtcbcomp12svn": 250,
        "sgxtcbcomp13svn": 55,
        "sgxtcbcomp14svn": 2,
        "sgxtcbcomp15svn": 2,
        "sgxtcbcomp16svn": 2,
        "pcesvn": 65
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, revokedTcb1, validRevokedStatus)
        + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, revokedTcb2, validRevokedStatus);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    EXPECT_EQ(2, tcbInfo.getTcbLevels().size());
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    std::advance(iterator, 0);
    for (uint32_t i=0; i<constants::CPUSVN_BYTE_LEN; i++)
    {
        EXPECT_EQ(expectedRevokedCpusvn[i], iterator->getSgxTcbComponentSvn(i));
    }
    EXPECT_EQ(expectedRevokedPcesvn, iterator->getPceSvn());
    EXPECT_EQ("Revoked", iterator->getStatus());
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsContainsNoTcbs)
{
    const std::string tcbLevels = "";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenSignatureIsMissing)
{
    const std::string missingSignature = R"json("missing": "signature")json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, TcbInfoGenerator::generateTcbLevelV2(), missingSignature);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenSignatureIsNotAString)
{
    const std::string invalidSignature = R"json("signature": 555)json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, TcbInfoGenerator::generateTcbLevelV2(), invalidSignature);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenSignatureIsTooLong)
{
    const std::string invalidSignature = R"json("signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA35570")json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, TcbInfoGenerator::generateTcbLevelV2(), invalidSignature);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenSignatureIsTooShort)
{
    const std::string invalidSignature = R"json("signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA355")json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, TcbInfoGenerator::generateTcbLevelV2(), invalidSignature);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenIssueDateIsMissing)
{
    const std::string tcbInfoWithoutDate = R"json({
        "tcbInfo": {
            "version": 2,
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoWithoutDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenIssueDateIsNotAString)
{
    const std::string tcbInfoInvalidDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": true,
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenIssueDateIsNotInValidFormat)
{
    const std::string tcbInfoInvalidDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "20171004T111045Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenIssueDateIsNotInUTC)
{
    const std::string tcbInfoInvalidDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45+01",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenNextUpdateIsMissing)
{
    const std::string tcbInfoWithoutDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoWithoutDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenNextUpdateIsNotAString)
{
    const std::string tcbInfoInvalidDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": true,
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenNextUpdateIsNotInValidFormat)
{
    const std::string tcbInfoInvalidDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "20180621T123602Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenNextUpdateIsNotInUTC)
{
    const std::string tcbInfoInvalidDate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02+01",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoInvalidDate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenFmspcIsMissing)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenFmspcIsNotAString)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": 23,
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenFmspcIsTooLong)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0123456789ABC",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenFmspcIsTooShort)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0123456789A",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenFmspcIsNotAValidHexstring)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "01invalid9AB",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenPceIdIsMissing)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenPceIdIsNotAString)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": 23,
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenPceIdIsTooLong)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "00000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenPceIdIsTooShort)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenPceIdIsNotAValidHexstring)
{
    const std::string tcbInfoTemplate = R"json({
        "tcbInfo": {
            "version": 1,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "xxxx",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}


TEST_F(TcbInfoUT, shouldFailWhenTcbTypeNotExist)
{
    const std::string tcbInfoWithOutTcbType = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";

    auto expResult = "TCB Info JSON should has [tcbType] field";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoWithOutTcbType, TcbInfoGenerator::generateTcbLevelV2());

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbType is not present";
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), expResult);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbTypeInvalid)
{
    const std::string tcbInfoWithOutTcbType = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType" : "1",
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": [%s]
        },
        %s})json";

    auto expResult = "Could not parse [tcbType] field of TCB Info JSON to number";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoWithOutTcbType, TcbInfoGenerator::generateTcbLevelV2());

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbType is invalid";
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), expResult);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbEvaluationDataNumberNotExist)
{
    const std::string tcbInfoTcbEvaluationData = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType" : 1,
            "tcbLevels": [%s]
        },
        %s})json";

    auto expResult = "TCB Info JSON should has [tcbEvaluationDataNumber] field";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTcbEvaluationData, TcbInfoGenerator::generateTcbLevelV2());

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbEvaluationDataNumber is not present";
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), expResult);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbEvaluationDataNumberInvalid)
{
    const std::string tcbInfoTcbEvaluationData = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType" : 1,
            "tcbEvaluationDataNumber": "1",
            "tcbLevels": [%s]
        },
        %s})json";

    auto expResult = "Could not parse [tcbEvaluationDataNumber] field of TCB Info JSON to number";
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTcbEvaluationData, TcbInfoGenerator::generateTcbLevelV2());

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbEvaluationDataNumber is invalid";
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), expResult);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayIsMissing)
{
    const std::string json = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1
        },
        "signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557"})json";
    EXPECT_THROW(parser::json::TcbInfo::parse(json), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsIsNotAnArray)
{
    const std::string json = R"json({
        "tcbInfo": {
            "version": 2,
            "issueDate": "2017-10-04T11:10:45Z",
            "nextUpdate": "2018-06-21T12:36:02Z",
            "fmspc": "0192837465AF",
            "pceId": "0000",
            "tcbType": 1,
            "tcbEvaluationDataNumber": 1,
            "tcbLevels": 0
        },
        "signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557"})json";
    EXPECT_THROW(parser::json::TcbInfo::parse(json), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayIsEmpty)
{
    const std::string tcbLevels = R"json()json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementIsNotAnObject)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(R"json("tcblevelString")json");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementIsEmpty)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2("{}");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementHasIncorrectNumberOfFields)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + R"json(, {"tcbStatus": "UpToDate"})json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementIsMissingTcbField)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, R"json("missing": "tcb")json");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementStatusIsNotAString)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, validSgxTcb, R"json("tcbStatus": 78763124)json");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbIsNotAnObject)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, R"json("tcb": "qwerty")json");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementStatusIsNotAValidValue)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, validSgxTcb, R"json("tcbStatus": "unknown value")json");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbComponentsAreMissing)
{
    const std::string invalidTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 12,
        "sgxtcbcomp02svn": 34,
        "sgxtcbcomp03svn": 56,
        "sgxtcbcomp04svn": 78,
        "sgxtcbcomp08svn": 254,
        "sgxtcbcomp09svn": 9,
        "sgxtcbcomp10svn": 87,
        "sgxtcbcomp11svn": 65,
        "sgxtcbcomp12svn": 43,
        "sgxtcbcomp13svn": 21,
        "sgxtcbcomp14svn": 222,
        "sgxtcbcomp15svn": 184,
        "sgxtcbcomp16svn": 98,
        "pcesvn": 37240
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, invalidTcb);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbComponentIsNotAnInteger)
{
    const std::string invalidTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": "12",
        "pcesvn": 37240
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, invalidTcb);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbComponentIsNegative)
{
    const std::string invalidTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": -23,
        "pcesvn": 37240
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, invalidTcb);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbComponentPcesvnIsMissing)
{
    const std::string invalidTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 12,
        "sgxtcbcomp02svn": 34,
        "sgxtcbcomp03svn": 56,
        "sgxtcbcomp04svn": 78,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 254,
        "sgxtcbcomp09svn": 9,
        "sgxtcbcomp10svn": 87,
        "sgxtcbcomp11svn": 65,
        "sgxtcbcomp12svn": 43,
        "sgxtcbcomp13svn": 21,
        "sgxtcbcomp14svn": 222,
        "sgxtcbcomp15svn": 184,
        "sgxtcbcomp16svn": 98
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, invalidTcb);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbComponentPcesvnIsNegative)
{
    const std::string invalidTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 12,
        "sgxtcbcomp02svn": 34,
        "sgxtcbcomp03svn": 56,
        "sgxtcbcomp04svn": 78,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 254,
        "sgxtcbcomp09svn": 9,
        "sgxtcbcomp10svn": 87,
        "sgxtcbcomp11svn": 65,
        "sgxtcbcomp12svn": 43,
        "sgxtcbcomp13svn": 21,
        "sgxtcbcomp14svn": 222,
        "sgxtcbcomp15svn": 184,
        "sgxtcbcomp16svn": 98,
        "pcesvn": -4
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, invalidTcb);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementTcbComponentPcesvnIsNotANumber)
{
    const std::string invalidTcb = R"json(
    "tcb": {
        "sgxtcbcomp01svn": 12,
        "sgxtcbcomp02svn": 34,
        "sgxtcbcomp03svn": 56,
        "sgxtcbcomp04svn": 78,
        "sgxtcbcomp05svn": 10,
        "sgxtcbcomp06svn": 0,
        "sgxtcbcomp07svn": 0,
        "sgxtcbcomp08svn": 254,
        "sgxtcbcomp09svn": 9,
        "sgxtcbcomp10svn": 87,
        "sgxtcbcomp11svn": 65,
        "sgxtcbcomp12svn": 43,
        "sgxtcbcomp13svn": 21,
        "sgxtcbcomp14svn": 222,
        "sgxtcbcomp15svn": 184,
        "sgxtcbcomp16svn": 98,
        "pcesvn": "78xy"
    })json";
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, invalidTcb);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayHasTwoIdenticalElements)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2();
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayHasTwoElementsWithSameSvnsAndDifferentStatus)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2() + "," + TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, validSgxTcb, validRevokedStatus);
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::InvalidExtensionException);
}


TEST_F(TcbInfoUT, shouldFailWhenGettingSvnComponentOutOfRange)
{
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo();

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    EXPECT_THROW(iterator->getSgxTcbComponentSvn(constants::CPUSVN_BYTE_LEN + 1), parser::FormatException);
    EXPECT_THROW(iterator->getSgxTcbComponentSvn((uint32_t) -1), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbInfoFieldIsMissing)
{
    const std::string json = R"json({"signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557"})json";

    EXPECT_THROW(parser::json::TcbInfo::parse(json), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenJSONRootIsNotAnObject)
{
    const std::string tcbInfoTemplate = R"json([{
        "tcbInfo": {},
        "signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557"}])json";
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(tcbInfoTemplate);

    EXPECT_THROW(parser::json::TcbInfo::parse(tcbInfoJson), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTCBInfoIsNotAnObject)
{
    const std::string json = R"json({"tcbInfo": "text", "signature": "ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557ABBA3557"})json";

    EXPECT_THROW(parser::json::TcbInfo::parse(json), parser::FormatException);
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementIsMissingStatusField)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(validTcbLevelV2Template, validSgxTcb, R"json("missing": "tcbStatus")json");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    auto expErrMsg = "TCB level JSON should has [tcbStatus] field";

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbStatus is not present";
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), expErrMsg);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementIsMissingTcbDateField)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(
            validTcbLevelV2Template, validSgxTcb, R"("tcbStatus": "UpToDate")", R"("missing": "tcbDate")");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    auto expErrMsg = "TCB level JSON should has [tcbDate] field";

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because " << expErrMsg;
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), expErrMsg);
    }
}

TEST_F(TcbInfoUT, shouldSuccessWhenTcbLevelsAdvisoryIDsFieldIsPresent)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(
            validTcbLevelV2Template, validSgxTcb, R"("tcbStatus": "UpToDate")", R"("tcbDate": "2019-05-23T10:36:02Z")", R"("advisoryIDs": ["adv"])");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);

    auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);

    EXPECT_EQ(tcbInfo.getPceId(), DEFAULT_PCEID);
    EXPECT_EQ(tcbInfo.getFmspc(), DEFAULT_FMSPC);
    EXPECT_EQ(tcbInfo.getSignature(), DEFAULT_SIGNATURE);
    EXPECT_EQ(tcbInfo.getTcbType(), DEFAULT_TCB_TYPE);
    EXPECT_EQ(tcbInfo.getTcbEvaluationDataNumber(), DEFAULT_TCB_EVALUATION_DATA_NUMBER);
    EXPECT_EQ(tcbInfo.getIssueDate(), getEpochTimeFromString(DEFAULT_ISSUE_DATE));
    EXPECT_EQ(tcbInfo.getNextUpdate(), getEpochTimeFromString(DEFAULT_NEXT_UPDATE));
    EXPECT_EQ(tcbInfo.getVersion(), 2);
    EXPECT_EQ(1, tcbInfo.getTcbLevels().size());
    auto iterator = tcbInfo.getTcbLevels().begin();
    EXPECT_NE(iterator, tcbInfo.getTcbLevels().end());
    for (uint32_t i=0; i<constants::CPUSVN_BYTE_LEN && iterator != tcbInfo.getTcbLevels().end(); i++)
    {
        EXPECT_EQ(iterator->getSgxTcbComponentSvn(i), DEFAULT_CPUSVN[i]);
    }
    EXPECT_EQ(iterator->getTcbDate(), getEpochTimeFromString(DEFAULT_TCB_DATE));
    EXPECT_EQ(iterator->getPceSvn(), DEFAULT_PCESVN);
    EXPECT_EQ(iterator->getStatus(), "UpToDate");
    EXPECT_EQ(iterator->getAdvisoryIDs().size(), 1);
    EXPECT_EQ(iterator->getAdvisoryIDs()[0], "adv");
}

TEST_F(TcbInfoUT, shouldFailWhenTcbLevelsArrayElementIsMissingTcbIDsField)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(
            validTcbLevelV2Template, R"("missing": "tcb")");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    auto expErrMsg = "TCB level JSON should has [tcb] field";

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because advisoryIDs is not present";
    }
    catch(const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), expErrMsg);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenAdvisoryIDsIsNotArray)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(
            validTcbLevelV2Template, validSgxTcb, R"("tcbStatus": "UpToDate")", R"("tcbDate": "2019-05-23T10:36:02Z")", R"("advisoryIDs": "advisoryIDs")");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    auto expErrMsg = "Could not parse [advisoryIDs] field of TCB info JSON to an array.";

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because advisoryIDs is not array";
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), expErrMsg);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbDateHasWrongFormat)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(
            validTcbLevelV2Template, validSgxTcb, R"("tcbStatus": "UpToDate")", R"("tcbDate": "2019-05-23T10:3")");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    auto expErrMsg = "Could not parse [tcbDate] field of TCB info JSON to date. [tcbDate] should be ISO formatted date";

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbDate has wrong format";
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), expErrMsg);
    }
}

TEST_F(TcbInfoUT, shouldFailWhenTcbDateIsNotString)
{
    const std::string tcbLevels = TcbInfoGenerator::generateTcbLevelV2(
            validTcbLevelV2Template, validSgxTcb, R"("tcbStatus": "UpToDate")", R"("tcbDate": 2019)");
    const auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template, tcbLevels);
    auto expErrMsg = "Could not parse [tcbDate] field of TCB info JSON to date. [tcbDate] should be ISO formatted date";

    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
        FAIL() << "Should throw, because tcbDate is not string";
    }
    catch(const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), expErrMsg);
    }
}

TEST_F(TcbInfoUT, shouldFailOnGetIdFromVersion2) {
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template,
                                                         TcbInfoGenerator::generateTcbLevelV2());

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);

    try
    {
        tcbInfo.getId();
    }
    catch (const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TCB identifier is not a valid field in TCB Info V2 structure");
    }
}

TEST_F(TcbInfoUT, shouldFailOnGetTdxModuleFromVersion2) {
    auto tcbInfoJson = TcbInfoGenerator::generateTcbInfo(validTcbInfoV2Template,
                                                         TcbInfoGenerator::generateTcbLevelV2());

    const auto tcbInfo = parser::json::TcbInfo::parse(tcbInfoJson);

    try
    {
        tcbInfo.getTdxModule();
    }
    catch (const parser::FormatException &err)
    {
        EXPECT_EQ(std::string(err.what()), "TdxModule is not a valid field in TCB Info V1 and V2 structure");
    }
}

TEST_F(TcbInfoUT, shouldFailWhenAdvisoryIdIsNotString)
{
    // test case input comes from fuzzing
    std::string tcbInfoJson = "{\"tcbInfo\":{\"tcbType\":0,\"tcbEvaluationDataNumber\":0,\"version\":2,\"issueDate\":\"2021-03-02T10:58:19Z\",\"nextUpdate\":\"2021-03-07T10:58:19Z\",\"fmspc\":\"f9c71eb1d129\",\"pceId\":\"15ce\",\"tcbLevels\":[{\"tcb\":{\"sgxtcbcomp01svn\":79,\"sgxtcbcomp02svn\":65432,\"sgxtcbcomp03svn\":106,\"sgxtcbcomp04svn\":116,\"sgxtcbcomp05svn\":65461,\"sgxtcbcomp08svn\":65456,\"sgxtcbcsvn\":122,\"sgxtcbcomp10svn\":26,\"sgxtcbcomp12svn\":70,\"sgxtcbcomp12svn\":65408,\"gxtcbcomp13svn\":54,\"svtcbcomp15svn\":65534,\"sgxtcbcomp16svn\":65478,\"pcesvn\":8145},\"tcbStatus\":\"UpToDate\",\"tcbDate\":\"2021-03-02T10:58:19Z\",\"advisoryIDs\":[456,\"sgxA-00079\",\"INTEL-SA-00076\"]}]},\"signature\":\"85d927f55ffc59c7c747f27f600b9d6741bd0bee68ff7a7316a168c4fe5b2a7a2331d1817431b0ce9998d134c622ca61b57f848795aaa06f71ab9e35d283d2f9\"}";
    try
    {
        parser::json::TcbInfo::parse(tcbInfoJson);
    }
    catch (const parser::InvalidExtensionException &err)
    {
        EXPECT_EQ(std::string(err.what()), "Could not parse [advisoryIDs] field of TCB info JSON to an array.");
    }

}
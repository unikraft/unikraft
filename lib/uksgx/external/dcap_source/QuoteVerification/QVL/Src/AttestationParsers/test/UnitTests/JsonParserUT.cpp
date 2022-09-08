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

#include "Json/JsonParser.h"

#include <gtest/gtest.h>
#include <SgxEcdsaAttestation/AttestationParsers.h>

using namespace ::testing;
using namespace intel::sgx::dcap::parser::json;

struct JsonParserTests : public Test
{
    JsonParser jsonParser{};
};

TEST_F(JsonParserTests, shouldParseEmptyJson)
{
    EXPECT_TRUE(jsonParser.parse("{}"));
}

TEST_F(JsonParserTests, shouldFailWhenParsingEmptyString)
{
    EXPECT_FALSE(jsonParser.parse(""));
}

TEST_F(JsonParserTests, shouldFailWhenParsingNonJson)
{
    EXPECT_FALSE(jsonParser.parse("Random string $&^%*("));
}

TEST_F(JsonParserTests, shouldFailWhenParsingNonObjectJson)
{
    EXPECT_FALSE(jsonParser.parse(R"json(["value", "5"])json"));
}

TEST_F(JsonParserTests, shouldReturnMainObjectFields)
{
    std::vector<uint8_t> expectedValue = {0xad, 0xff, 0x09, 0xa7};
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {}, "otherField": 66})json"));
    const auto data = jsonParser.getField("data");
    const auto otherField = jsonParser.getField("otherField");
    const auto missingField = jsonParser.getField("missingField");
    EXPECT_NE(nullptr, data);
    EXPECT_NE(nullptr, otherField);
    EXPECT_EQ(nullptr, missingField);
}

TEST_F(JsonParserTests, shouldParseObjectWithHexstring)
{
    std::vector<uint8_t> expectedValue = {0xad, 0xff, 0x09, 0xa7};
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": "adff09a7"}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
    std::vector<uint8_t> value{};
    std::tie(value, status) = jsonParser.getBytesFieldOf(*data, "v", 8);
    EXPECT_EQ(JsonParser::ParseStatus::OK, status);
    EXPECT_EQ(expectedValue, value);
}

TEST_F(JsonParserTests, shouldParseObjectWithUint)
{
	uint32_t expectedValue = 234;
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": 234}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
	uint32_t value = 0;
    std::tie(value, status) = jsonParser.getUintFieldOf(*data, "v");
    EXPECT_EQ(JsonParser::ParseStatus::OK, status);
    EXPECT_EQ(expectedValue, value);
}

TEST_F(JsonParserTests, shouldParseObjectWithInt)
{
    int expectedValue = -43;
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": -43}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
    int value = 0;
    std::tie(value, status) = jsonParser.getIntFieldOf(*data, "v");
    EXPECT_EQ(JsonParser::ParseStatus::OK, status);
    EXPECT_EQ(expectedValue, value);
}

TEST_F(JsonParserTests, shouldParseObjectWithDate)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"date": "2018-09-29T15:17:22Z"}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
    time_t value = 0;
    std::tie(value, status) = jsonParser.getDateFieldOf(*data, "date");

    EXPECT_EQ(JsonParser::ParseStatus::OK, status);
    EXPECT_EQ(value, 1538234242);
}

TEST_F(JsonParserTests, shouldFailWhenParsingInvalidHexstringField)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": "adff09a732544$#^&%"}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
    std::vector<uint8_t> value{};
    std::tie(value, status) = jsonParser.getBytesFieldOf(*data, "v", 8);
    EXPECT_EQ(JsonParser::ParseStatus::Invalid, status);
}

TEST_F(JsonParserTests, shouldFailWhenParsingWrongLengthHexstringField)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": "ad34"}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
    std::vector<uint8_t> value{};
    std::tie(value, status) = jsonParser.getBytesFieldOf(*data, "v", 5);
    EXPECT_EQ(JsonParser::ParseStatus::Invalid, status);
}

TEST_F(JsonParserTests, shouldFailWhenParsingInvalidIntField)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": "asd"}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
    int value = 0;
    std::tie(value, status) = jsonParser.getIntFieldOf(*data, "v");
    EXPECT_EQ(JsonParser::ParseStatus::Invalid, status);
}

TEST_F(JsonParserTests, shouldFailWhenParsingInvalidUintField)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"data": {"v": -55555}})json"));
    const auto data = jsonParser.getField("data");
    EXPECT_NE(nullptr, data);
    JsonParser::ParseStatus status = JsonParser::Missing;
	uint32_t value = 0;
    std::tie(value, status) = jsonParser.getUintFieldOf(*data, "v");
    EXPECT_EQ(JsonParser::ParseStatus::Invalid, status);
}

TEST_F(JsonParserTests, getStringFieldOfShouldThrowFormatExceptionWhenParentIsNotAnObject)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"parent": "test"})json"));
    const auto parent = jsonParser.getField("parent");
    EXPECT_NE(nullptr, parent);
    try
    {
        jsonParser.getStringFieldOf(*parent, "test");
    }
    catch(const intel::sgx::dcap::parser::FormatException& ex)
    {
        EXPECT_EQ("Fields can only be get from objects. Parent should be an object", std::string(ex.what()));
    }
}

TEST_F(JsonParserTests, getStringVecFieldOfShouldThrowFormatExceptionWhenParentIsNotAnObject)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"parent": "test"})json"));
    const auto parent = jsonParser.getField("parent");
    EXPECT_NE(nullptr, parent);
    try
    {
        jsonParser.getStringVecFieldOf(*parent, "test");
    }
    catch(const intel::sgx::dcap::parser::FormatException& ex)
    {
        EXPECT_EQ("Fields can only be get from objects. Parent should be an object", std::string(ex.what()));
    }
}

TEST_F(JsonParserTests, getBytesFieldOfShouldThrowFormatExceptionWhenParentIsNotAnObject)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"parent": "test"})json"));
    const auto parent = jsonParser.getField("parent");
    EXPECT_NE(nullptr, parent);
    try
    {
        jsonParser.getBytesFieldOf(*parent, "test", 0);
    }
    catch(const intel::sgx::dcap::parser::FormatException& ex)
    {
        EXPECT_EQ("Fields can only be get from objects. Parent should be an object", std::string(ex.what()));
    }
}

TEST_F(JsonParserTests, getDateFieldOfShouldThrowFormatExceptionWhenParentIsNotAnObject)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"parent": "test"})json"));
    const auto parent = jsonParser.getField("parent");
    EXPECT_NE(nullptr, parent);
    try
    {
        jsonParser.getDateFieldOf(*parent, "test");
    }
    catch(const intel::sgx::dcap::parser::FormatException& ex)
    {
        EXPECT_EQ("Fields can only be get from objects. Parent should be an object", std::string(ex.what()));
    }
}

TEST_F(JsonParserTests, getUintFieldOfShouldThrowFormatExceptionWhenParentIsNotAnObject)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"parent": "test"})json"));
    const auto parent = jsonParser.getField("parent");
    EXPECT_NE(nullptr, parent);
    try
    {
        jsonParser.getUintFieldOf(*parent, "test");
    }
    catch(const intel::sgx::dcap::parser::FormatException& ex)
    {
        EXPECT_EQ("Fields can only be get from objects. Parent should be an object", std::string(ex.what()));
    }
}

TEST_F(JsonParserTests, getIntFieldOfShouldThrowFormatExceptionWhenParentIsNotAnObject)
{
    ASSERT_TRUE(jsonParser.parse(R"json({"parent": "test"})json"));
    const auto parent = jsonParser.getField("parent");
    EXPECT_NE(nullptr, parent);
    try
    {
        jsonParser.getIntFieldOf(*parent, "test");
    }
    catch(const intel::sgx::dcap::parser::FormatException& ex)
    {
        EXPECT_EQ("Fields can only be get from objects. Parent should be an object", std::string(ex.what()));
    }
}

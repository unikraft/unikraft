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

#include "JsonParser.h"

#include "OpensslHelpers/Bytes.h"
#include "Utils/TimeUtils.h"

#include <rapidjson/stringbuffer.h>

#include <tuple>
#include <algorithm>
#include <SgxEcdsaAttestation/AttestationParsers.h>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace json {

bool JsonParser::parse(const std::string& json)
{
    if(json.empty())
    {
        return false;
    }
    jsonDocument.Parse(json.c_str());
    return !jsonDocument.HasParseError() && jsonDocument.IsObject();
}

const rapidjson::Value* JsonParser::getField(const std::string& fieldName) const
{
    if(!jsonDocument.HasMember(fieldName.c_str()))
    {
        return nullptr;
    }
    return &jsonDocument[fieldName.c_str()];
}

std::pair<std::string, JsonParser::ParseStatus> JsonParser::getStringFieldOf(const ::rapidjson::Value &parent, const std::string &fieldName) const
{
    if(!parent.IsObject())
    {
        throw intel::sgx::dcap::parser::FormatException("Fields can only be get from objects. Parent should be an object");
    }
    if(!parent.HasMember(fieldName.c_str()))
    {
        return std::make_pair("", ParseStatus::Missing);
    }
    const ::rapidjson::Value& property_v = parent[fieldName.c_str()];
    if(!property_v.IsString())
    {
        return std::make_pair("", ParseStatus::Invalid);
    }

    const std::string propertyStr = property_v.GetString();
    return std::make_pair(propertyStr, ParseStatus::OK);
}

std::pair<std::vector<std::string>, JsonParser::ParseStatus> JsonParser::getStringVecFieldOf(
        const ::rapidjson::Value& parent, const std::string& fieldName) const
{
    if(!parent.IsObject())
    {
        throw intel::sgx::dcap::parser::FormatException("Fields can only be get from objects. Parent should be an object");
    }
    std::vector<std::string> advisoryIDs;
    if(!parent.HasMember(fieldName.c_str()))
    {
        return std::make_pair(advisoryIDs, ParseStatus::Missing);
    }
    const ::rapidjson::Value& property_v = parent[fieldName.c_str()];
    if(!property_v.IsArray())
    {
        return std::make_pair(advisoryIDs, ParseStatus::Invalid);
    }

    for (rapidjson::SizeType i = 0; i < property_v.Size(); i++)
    {
        if(!property_v[i].IsString())
        {
            return std::make_pair(advisoryIDs, ParseStatus::Invalid);
        }
        advisoryIDs.push_back(property_v[i].GetString());
    }

    return std::make_pair(advisoryIDs, ParseStatus::OK);
}

std::pair<std::vector<uint8_t>, JsonParser::ParseStatus> JsonParser::getBytesFieldOf(
        const ::rapidjson::Value &parent, const std::string &fieldName, size_t length) const
{
    if(!parent.IsObject())
    {
        throw intel::sgx::dcap::parser::FormatException("Fields can only be get from objects. Parent should be an object");
    }
    if(!parent.HasMember(fieldName.c_str()))
    {
        return std::make_pair(std::vector<uint8_t>{}, ParseStatus::Missing);
    }
    const ::rapidjson::Value& property_v = parent[fieldName.c_str()];
    if(!property_v.IsString())
    {
        return std::make_pair(std::vector<uint8_t>{}, ParseStatus::Invalid);
    }

    const std::string propertyStr = property_v.GetString();
    if(propertyStr.length() == length && isValidHexstring(propertyStr))
    {
        return std::make_pair(hexStringToBytes(propertyStr), ParseStatus::OK);
    }
    return std::make_pair(std::vector<uint8_t>{}, ParseStatus::Invalid);
}

std::pair<time_t, JsonParser::ParseStatus> JsonParser::getDateFieldOf(
        const ::rapidjson::Value& parent, const std::string& fieldName) const
{
    if(!parent.IsObject())
    {
        throw intel::sgx::dcap::parser::FormatException("Fields can only be get from objects. Parent should be an object");
    }
    if(!parent.HasMember(fieldName.c_str()))
    {
        return std::make_pair(time_t{}, ParseStatus::Missing);
    }
    const auto& date = parent[fieldName.c_str()];
    if(!date.IsString() || !isValidTimeString(date.GetString()))
    {
        return std::make_pair(time_t{}, ParseStatus::Invalid);
    }
    return std::make_pair(getEpochTimeFromString(date.GetString()), ParseStatus::OK);
}

std::pair<uint32_t, JsonParser::ParseStatus> JsonParser::getUintFieldOf(
        const ::rapidjson::Value& parent, const std::string& fieldName) const
{
    if(!parent.IsObject())
    {
        throw intel::sgx::dcap::parser::FormatException("Fields can only be get from objects. Parent should be an object");
    }
    if(!parent.HasMember(fieldName.c_str()))
    {
        return std::make_pair(0u, ParseStatus::Missing);
    }
    const ::rapidjson::Value& value = parent[fieldName.c_str()];
    if(!value.IsUint())
    {
        return std::make_pair(0u, ParseStatus::Invalid);
    }
    return std::make_pair(value.GetUint(), ParseStatus::OK);
}

std::pair<int, JsonParser::ParseStatus> JsonParser::getIntFieldOf(
        const ::rapidjson::Value& parent, const std::string& fieldName) const
{
    if(!parent.IsObject())
    {
        throw intel::sgx::dcap::parser::FormatException("Fields can only be get from objects. Parent should be an object");
    }
    if(!parent.HasMember(fieldName.c_str()))
    {
        return std::make_pair(0, ParseStatus::Missing);
    }
    const ::rapidjson::Value& value = parent[fieldName.c_str()];
    if(!value.IsInt())
    {
        return std::make_pair(0, ParseStatus::Invalid);
    }
    return std::make_pair(value.GetInt(), ParseStatus::OK);
}

bool JsonParser::isValidHexstring(const std::string& hexString) const
{
    return std::find_if(hexString.cbegin(), hexString.cend(),
        [](const char c){return !::isxdigit(static_cast<unsigned char>(c));}) == hexString.cend();
}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace json {

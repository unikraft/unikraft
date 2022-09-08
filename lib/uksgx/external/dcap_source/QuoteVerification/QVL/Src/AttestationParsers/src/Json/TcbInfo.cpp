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

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "OpensslHelpers/Bytes.h"
#include "X509Constants.h"
#include "JsonParser.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <tuple>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace json {

const std::string TcbInfo::SGX_ID = "SGX";
const std::string TcbInfo::TDX_ID = "TDX";

TcbInfo TcbInfo::parse(const std::string& json)
{
    return TcbInfo(json);
}

std::string TcbInfo::getId() const
{
    switch (_version)
    {
        case (Version::V2):
            throw FormatException("TCB identifier is not a valid field in TCB Info V2 structure");
        case (Version::V3):
            return _id;
        default:
            throw FormatException("TCB identifier is not a valid field in TCB Info structure");
    }
}

uint32_t TcbInfo::getVersion() const
{
    return static_cast<uint32_t>(_version);
}

std::time_t TcbInfo::getIssueDate() const
{
    return _issueDate;
}

std::time_t TcbInfo::getNextUpdate() const
{
    return _nextUpdate;
}

const std::vector<uint8_t>& TcbInfo::getFmspc() const
{
    return _fmspc;
}

const std::vector<uint8_t>& TcbInfo::getPceId() const
{
    return _pceId;
}

const std::set<TcbLevel, std::greater<TcbLevel>>& TcbInfo::getTcbLevels() const
{
    return _tcbLevels;
}

const std::vector<uint8_t>& TcbInfo::getSignature() const
{
    return _signature;
}

const std::vector<uint8_t>& TcbInfo::getInfoBody() const
{
    return _infoBody;
}

int TcbInfo::getTcbType() const
{
    return _tcbType;
}

uint32_t TcbInfo::getTcbEvaluationDataNumber() const
{
    return _tcbEvaluationDataNumber;
}

const TdxModule& TcbInfo::getTdxModule() const
{
    if (_version < Version::V3)
    {
        throw FormatException("TdxModule is not a valid field in TCB Info V1 and V2 structure");
    }

    if (_id != TDX_ID)
    {
        throw FormatException("TdxModule is only valid for TDX TCB Info");
    }

    return _tdxModule;
}
// private

TcbInfo::TcbInfo(const std::string& jsonString)
{
    JsonParser jsonParser;
    if(!jsonParser.parse(jsonString))
    {
        throw FormatException("Could not parse TCB info JSON");
    }

    const auto* tcbInfo = jsonParser.getField("tcbInfo");
    if(tcbInfo == nullptr)
    {
        throw FormatException("Missing [tcbInfo] field of TCB info JSON");
    }

    if(!tcbInfo->IsObject())
    {
        throw FormatException("[tcbInfo] field of TCB info JSON should be an object");
    }

    const auto* signatureField = jsonParser.getField("signature");
    if(signatureField == nullptr)
    {
        throw InvalidExtensionException("Missing [signature] field of TCB info JSON");
    }

    auto version = jsonParser.getUintFieldOf(*tcbInfo, "version");
    JsonParser::ParseStatus status = version.second;
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [version] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [version] field of TCB info JSON to integer");
        case JsonParser::ParseStatus::OK:
            break;
    }

    _version = static_cast<Version>(version.first);

    if (_version != Version::V2 && _version != Version::V3)
    {
        std::string err = "Unsupported version[" + std::to_string(static_cast<uint32_t>(_version))
                          + "] value for field of TCB info JSON. Supported versions are ["
                          + std::to_string(static_cast<uint32_t>(Version::V2)) + " | "
                          + std::to_string(static_cast<uint32_t>(Version::V3)) + "]";
        throw InvalidExtensionException(err);
    }

    if (_version == Version::V3)
    {
        std::tie(_id, status) = jsonParser.getStringFieldOf(*tcbInfo, "id");
        switch (status)
        {
            case JsonParser::ParseStatus::Missing:
                throw FormatException("TCB Info JSON should has [id] field");
            case JsonParser::ParseStatus::Invalid:
                throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
            case JsonParser::ParseStatus::OK:
                break;
            default:
                throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
        }

        if (_id != SGX_ID && _id != TDX_ID)
        {
            std::string err = "Unsupported id[" + _id + "] value for field of TCB info JSON. Supported identifiers are ["
                              + SGX_ID + " | " + TDX_ID + "]";
            throw InvalidExtensionException(err);
        }
    }
    else
    {
        _id = SGX_ID;
    }

    std::tie(_issueDate, status) = jsonParser.getDateFieldOf(*tcbInfo, "issueDate");
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [issueDate] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [issueDate] field of TCB info JSON to date. [issueDate] should be ISO formatted date");
        case JsonParser::ParseStatus::OK:
            break;
        default:
            throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
    }

    std::tie(_nextUpdate, status) = jsonParser.getDateFieldOf(*tcbInfo, "nextUpdate");
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [nextUpdate] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [nextUpdate] field of TCB info JSON to date. [nextUpdate] should be ISO formatted date");
        case JsonParser::ParseStatus::OK:
            break;
        default:
            throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
    }

    std::tie(_fmspc, status) = jsonParser.getBytesFieldOf(*tcbInfo, "fmspc", constants::FMSPC_BYTE_LEN * 2);
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [fmspc] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [fmspc] field of TCB info JSON to bytes");
        case JsonParser::ParseStatus::OK:
            break;
        default:
            throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
    }

    std::tie(_pceId, status) = jsonParser.getBytesFieldOf(*tcbInfo, "pceId", constants::PCEID_BYTE_LEN * 2);
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [pceId] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [pceId] field of TCB info JSON to bytes");
        case JsonParser::ParseStatus::OK:
            break;
        default:
            throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
    }

    if(!signatureField->IsString() || signatureField->GetStringLength() != constants::ECDSA_P256_SIGNATURE_BYTE_LEN * 2)
    {
        throw InvalidExtensionException("Could not parse [signature] field of TCB info JSON to bytes");
    }
    _signature = hexStringToBytes(signatureField->GetString());

    if(!tcbInfo->HasMember("tcbLevels"))
    {
        throw InvalidExtensionException("Missing [tcbLevels] field of TCB info JSON");
    }

    if(_version >= Version::V2)
    {
        parsePartV2(*tcbInfo, jsonParser);
    }

    if(_version >= Version::V3)
    {
        parsePartV3(*tcbInfo);
    }

    const auto& tcbs = (*tcbInfo)["tcbLevels"];
    if(!tcbs.IsArray())
    {
        throw InvalidExtensionException("[tcbLevels] field of TCB info JSON should be a nonempty array");
    }

    for(uint32_t tcbLevelIndex = 0; tcbLevelIndex < tcbs.Size(); ++tcbLevelIndex)
    {
        bool inserted = false;
        std::tie(std::ignore, inserted) = _tcbLevels.emplace(TcbLevel(tcbs[tcbLevelIndex], static_cast<uint32_t>(_version), _id));
        if (!inserted)
        {
            throw InvalidExtensionException("Detected duplicated TCB levels");
        }
    }

    if(_tcbLevels.empty())
    {
        throw InvalidExtensionException("Number of parsed [tcbLevels] should not be 0");
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.SetMaxDecimalPlaces(25);
    tcbInfo->Accept(writer);

    _infoBody = std::vector<uint8_t>{ buffer.GetString(),
                                      &buffer.GetString()[buffer.GetSize()] };
}

void TcbInfo::parsePartV2(const ::rapidjson::Value &tcbInfo, JsonParser &jsonParser)
{
    JsonParser::ParseStatus status = JsonParser::Missing;

    std::tie(_tcbType, status) = jsonParser.getIntFieldOf(tcbInfo, "tcbType");
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [tcbType] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [tcbType] field of TCB Info JSON to number");
        case JsonParser::ParseStatus::OK:
            break;
        default:
            throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
    }

    std::tie(_tcbEvaluationDataNumber, status) = jsonParser.getUintFieldOf(tcbInfo, "tcbEvaluationDataNumber");
    switch (status)
    {
        case JsonParser::ParseStatus::Missing:
            throw FormatException("TCB Info JSON should has [tcbEvaluationDataNumber] field");
        case JsonParser::ParseStatus::Invalid:
            throw InvalidExtensionException("Could not parse [tcbEvaluationDataNumber] field of TCB Info JSON to number");
        case JsonParser::ParseStatus::OK:
            break;
        default:
            throw InvalidExtensionException("Could not parse [id] field of TCB info JSON to string");
    }
}

void TcbInfo::parsePartV3(const ::rapidjson::Value &tcbInfo)
{
    auto tdxModuleExists = tcbInfo.HasMember("tdxModule");

    if (_id == TcbInfo::SGX_ID && tdxModuleExists)
    {
        throw InvalidExtensionException("TCB Info JSON for SGX should not have [tdxModule] field");
    }
    else if (_id == TcbInfo::TDX_ID)
    {
        if (!tdxModuleExists)
        {
            throw InvalidExtensionException("TCB Info JSON for TDX should have [tdxModule] field");
        }
        const auto tdxModuleJson = &tcbInfo["tdxModule"];
        if (!tdxModuleJson->IsObject())
        {
            throw FormatException("[tdxModule] field should be an object");
        }
        _tdxModule = TdxModule(*tdxModuleJson);
    }
}
}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace json {

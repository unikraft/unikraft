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

#include "EnclaveIdentityV2.h"
#include "CertVerification/X509Constants.h"
#include "Utils/TimeUtils.h"
#include "QuoteVerification/QuoteConstants.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <tuple>

namespace intel { namespace sgx { namespace dcap {
    EnclaveIdentityV2::EnclaveIdentityV2(const ::rapidjson::Value &p_body)
            : tcbEvaluationDataNumber(0)
    {
        if(!p_body.IsObject())
        {
            status = STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT;
            return;
        }

        /// 4.1.2.9.3
        if(!parseVersion(p_body)
           || !parseIssueDate(p_body) || !parseNextUpdate(p_body)
           || !parseMiscselect(p_body) || !parseMiscselectMask(p_body)
           || !parseAttributes(p_body) || !parseAttributesMask(p_body)
           || !parseMrsigner(p_body) || !parseIsvprodid(p_body)
           || !parseID(p_body) || !parseTcbEvaluationDataNumber(p_body)
           || !parseTcbLevels(p_body))
        {
            status = STATUS_SGX_ENCLAVE_IDENTITY_INVALID;
            return;
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        p_body.Accept(writer);

        this->body = std::vector<uint8_t>{buffer.GetString(), &buffer.GetString()[buffer.GetSize()]};
        status = STATUS_OK;
    }
    void EnclaveIdentityV2::setSignature(std::vector<uint8_t> &p_signature)
    {
        signature = p_signature;
    }

    std::vector<uint8_t> EnclaveIdentityV2::getBody() const
    {
        return body;
    }

    std::vector<uint8_t> EnclaveIdentityV2::getSignature() const
    {
        return signature;
    }

    const std::vector<uint8_t>& EnclaveIdentityV2::getMiscselect() const
    {
        return miscselect;
    }

    const std::vector<uint8_t>& EnclaveIdentityV2::getMiscselectMask() const
    {
        return miscselectMask;
    }

    const std::vector<uint8_t>& EnclaveIdentityV2::getAttributes() const
    {
        return attributes;
    }

    const std::vector<uint8_t>& EnclaveIdentityV2::getAttributesMask() const
    {
        return attributesMask;
    }

    const std::vector<uint8_t>& EnclaveIdentityV2::getMrsigner() const
    {
        return mrsigner;
    }

    uint32_t EnclaveIdentityV2::getIsvProdId() const
    {
        return isvProdId;
    }

    int EnclaveIdentityV2::getVersion() const
    {
        return version;
    }

    time_t EnclaveIdentityV2::getIssueDate() const
    {
        return issueDate;
    }

    time_t EnclaveIdentityV2::getNextUpdate() const
    {
        return nextUpdate;
    }

    EnclaveID EnclaveIdentityV2::getID() const
    {
        return id;
    }

    bool EnclaveIdentityV2::parseVersion(const rapidjson::Value &input)
    {
        auto l_status = JsonParser::ParseStatus::Missing;
        std::tie(version, l_status) = jsonParser.getIntFieldOf(input, "version");
        return l_status == JsonParser::OK;
    }

    bool EnclaveIdentityV2::parseIssueDate(const rapidjson::Value &input)
    {
        auto l_status = JsonParser::ParseStatus::Missing;
        struct tm issueDateTm{};
        std::tie(issueDateTm, l_status) = jsonParser.getDateFieldOf(input, "issueDate");
        issueDate = dcap::mktime(&issueDateTm);
        return l_status == JsonParser::OK;
    }

    bool EnclaveIdentityV2::parseNextUpdate(const rapidjson::Value &input)
    {
        auto l_status = JsonParser::ParseStatus::Missing;
        struct tm nextUpdateTm{};
        std::tie(nextUpdateTm, l_status) = jsonParser.getDateFieldOf(input, "nextUpdate");
        nextUpdate = dcap::mktime(&nextUpdateTm);
        return l_status == JsonParser::OK;
    }

    bool EnclaveIdentityV2::parseMiscselect(const rapidjson::Value &input)
    {
        return parseHexstringProperty(input, "miscselect", constants::MISCSELECT_BYTE_LEN * 2, miscselect);
    }

    bool EnclaveIdentityV2::parseMiscselectMask(const rapidjson::Value &input)
    {
        return parseHexstringProperty(input, "miscselectMask", constants::MISCSELECT_BYTE_LEN * 2, miscselectMask);
    }

    bool EnclaveIdentityV2::parseAttributes(const rapidjson::Value &input)
    {
        return parseHexstringProperty(input, "attributes", constants::ATTRIBUTES_BYTE_LEN * 2, attributes);
    }

    bool EnclaveIdentityV2::parseAttributesMask(const rapidjson::Value &input)
    {
        return parseHexstringProperty(input, "attributesMask", constants::ATTRIBUTES_BYTE_LEN * 2, attributesMask);
    }

    bool EnclaveIdentityV2::parseMrsigner(const rapidjson::Value &input)
    {
        return parseHexstringProperty(input, "mrsigner", constants::MRSIGNER_BYTE_LEN * 2, mrsigner);
    }

    bool EnclaveIdentityV2::parseHexstringProperty(const rapidjson::Value &object, const std::string &propertyName, const size_t length, std::vector<uint8_t> &saveAs)
    {
        auto parseSuccessful = JsonParser::ParseStatus::Missing;
        std::tie(saveAs, parseSuccessful) = jsonParser.getHexstringFieldOf(object, propertyName, length);
        return parseSuccessful == JsonParser::OK;
    }

    bool EnclaveIdentityV2::parseIsvprodid(const rapidjson::Value &input)
    {
        return parseUintProperty(input, "isvprodid", isvProdId);
    }

    bool EnclaveIdentityV2::parseUintProperty(const rapidjson::Value &object, const std::string &propertyName, uint32_t &saveAs)
    {
        auto parseSuccessful = JsonParser::ParseStatus::Missing;
        std::tie(saveAs, parseSuccessful) = jsonParser.getUintFieldOf(object, propertyName);
        return parseSuccessful == JsonParser::OK;
    }

    bool EnclaveIdentityV2::checkDateCorrectness(const time_t expirationDate) const
    {
        if (expirationDate > nextUpdate)
        {
            return false;
        }

        if (expirationDate <= issueDate)
        {
            return false;
        }

        return true;
    }

    Status EnclaveIdentityV2::getStatus() const
    {
        return status;
    }


    bool EnclaveIdentityV2::parseID(const rapidjson::Value &input)
    {
        auto parseSuccessful = JsonParser::ParseStatus::Missing;
        std::string idString;
        std::tie(idString, parseSuccessful) = jsonParser.getStringFieldOf(input, "id");
        if (idString == "QE")
        {
            id = QE;
        }
        else if (idString == "QVE")
        {
            id = QVE;
        }
        else if (idString == "TD_QE")
        {
            id = TD_QE;
        }
        else
        {
            return false;
        }
        return parseSuccessful == JsonParser::OK;
    }

    bool EnclaveIdentityV2::parseTcbEvaluationDataNumber(const rapidjson::Value &input)
    {
        return parseUintProperty(input, "tcbEvaluationDataNumber", tcbEvaluationDataNumber);
    }

    bool EnclaveIdentityV2::parseTcbLevels(const rapidjson::Value &input)
    {
        if (!input.HasMember("tcbLevels"))
        {
            return false;
        }

        const ::rapidjson::Value& l_tcbLevels = input["tcbLevels"];

        if (!l_tcbLevels.IsArray() || l_tcbLevels.Empty()) // must be a non empty array
        {
            return false;
        }

        auto l_status = JsonParser::ParseStatus::Missing;
        for (rapidjson::Value::ConstValueIterator itr = l_tcbLevels.Begin(); itr != l_tcbLevels.End(); itr++)
        {
            struct tm tcbDate{};
            std::string tcbStatus;
            uint32_t isvsvn = 0;

            std::tie(tcbDate, l_status) = jsonParser.getDateFieldOf(*itr, "tcbDate");
            if (l_status != JsonParser::OK)
            {
                return false;
            }
            std::tie(tcbStatus, l_status) = jsonParser.getStringFieldOf(*itr, "tcbStatus");
            if (l_status != JsonParser::OK)
            {
                return false;
            }

            if (!(*itr).HasMember("tcb"))
            {
                return false;
            }

            const ::rapidjson::Value& tcb = (*itr)["tcb"];

            if (!tcb.IsObject())
            {
                return false;
            }

            std::tie(isvsvn, l_status) = jsonParser.getUintFieldOf(tcb, "isvsvn");
            if (l_status != JsonParser::OK)
            {
                return false;
            }


            TcbStatus tcbStatusEnum;
            try
            {
                tcbStatusEnum = parseStringToTcbStatus(tcbStatus);
            }
            catch (const std::runtime_error &)
            {
                return false;
            }

            this->tcbLevels.emplace_back(isvsvn, tcbDate, tcbStatusEnum);
        }
        return true;
    }

    TcbStatus EnclaveIdentityV2::getTcbStatus(uint32_t p_isvSvn) const
    {
        for(const auto & tcbLevel : tcbLevels)
        {
            if (tcbLevel.getIsvsvn() <= p_isvSvn)
            {
                return tcbLevel.getTcbStatus();
            }
        }
        return TcbStatus::Revoked;
    }

    uint32_t EnclaveIdentityV2::getTcbEvaluationDataNumber() const
    {
        return tcbEvaluationDataNumber;
    }

    const std::vector<TCBLevel>& EnclaveIdentityV2::getTcbLevels() const
    {
        return tcbLevels;
    }

    uint32_t TCBLevel::getIsvsvn() const
    {
        return isvsvn;
    }

    struct tm TCBLevel::getTcbDate() const
    {
        return tcbDate;
    }

    TcbStatus TCBLevel::getTcbStatus() const
    {
        return tcbStatus;
    }

}}}
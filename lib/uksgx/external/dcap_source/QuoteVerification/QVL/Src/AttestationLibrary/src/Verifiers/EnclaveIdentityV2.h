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

#ifndef SGXECDSAATTESTATION_ENCLAVEIDENTITYV2_H
#define SGXECDSAATTESTATION_ENCLAVEIDENTITYV2_H

#include "OpensslHelpers/Bytes.h"
#include "Utils/JsonParser.h"
#include "TcbStatus.h"

#include <SgxEcdsaAttestation/QuoteVerification.h>

#include <rapidjson/document.h>

#include <string>
#include <vector>
#include <cstdint>

namespace intel { namespace sgx { namespace dcap {

    enum EnclaveID
    {
        QE, QVE, TD_QE
    };

    class TCBLevel
    {
    public:
        TCBLevel(const uint32_t p_isvsvn,
                 const struct tm &p_tcbDate,
                 const TcbStatus p_tcbStatus):
                isvsvn(p_isvsvn), tcbDate(p_tcbDate), tcbStatus(p_tcbStatus) {};
        uint32_t getIsvsvn() const;
        struct tm getTcbDate() const;
        TcbStatus getTcbStatus() const;
    protected:
        uint32_t isvsvn;
        struct tm tcbDate;
        TcbStatus tcbStatus;
    };

    class EnclaveIdentityV2
    {
    public:
        enum Version
        {
            V2 = 2,
        };

        explicit EnclaveIdentityV2(const ::rapidjson::Value &p_body);
        virtual ~EnclaveIdentityV2() = default;

        virtual void setSignature(std::vector<uint8_t> &p_signature);
        virtual std::vector<uint8_t> getBody() const;
        virtual std::vector<uint8_t> getSignature() const;

        virtual time_t getIssueDate() const;
        virtual time_t getNextUpdate() const;
        virtual const std::vector<uint8_t>& getMiscselect() const;
        virtual const std::vector<uint8_t>& getMiscselectMask() const;
        virtual const std::vector<uint8_t>& getAttributes() const;
        virtual const std::vector<uint8_t>& getAttributesMask() const;
        virtual const std::vector<uint8_t>& getMrsigner() const;
        virtual uint32_t getIsvProdId() const;
        virtual int getVersion() const;
        virtual bool checkDateCorrectness(time_t expirationDate) const;
        virtual Status getStatus() const;
        virtual EnclaveID getID() const;

        virtual TcbStatus getTcbStatus(uint32_t isvSvn) const;
        virtual uint32_t getTcbEvaluationDataNumber() const;
        virtual const std::vector<TCBLevel>& getTcbLevels() const;

    protected:
        EnclaveIdentityV2() = default;

        std::vector<uint8_t> signature;
        std::vector<uint8_t> body;

        bool parseMiscselect(const rapidjson::Value &input);
        bool parseMiscselectMask(const rapidjson::Value &input);
        bool parseAttributes(const rapidjson::Value &input);
        bool parseAttributesMask(const rapidjson::Value &input);
        bool parseMrsigner(const rapidjson::Value &input);
        bool parseIsvprodid(const rapidjson::Value &input);
        bool parseVersion(const rapidjson::Value &input);
        bool parseIssueDate(const rapidjson::Value &input);
        bool parseNextUpdate(const rapidjson::Value &input);

        bool parseHexstringProperty(const rapidjson::Value &object, const std::string &propertyName, size_t length, std::vector<uint8_t> &saveAs);
        bool parseUintProperty(const rapidjson::Value &object, const std::string &propertyName, uint32_t &saveAs);

        bool parseID(const rapidjson::Value &input);
        bool parseTcbEvaluationDataNumber(const rapidjson::Value &input);
        bool parseTcbLevels(const rapidjson::Value &input);

        JsonParser jsonParser;
        std::vector<uint8_t> miscselect;
        std::vector<uint8_t> miscselectMask;
        std::vector<uint8_t> attributes;
        std::vector<uint8_t> attributesMask;
        std::vector<uint8_t> mrsigner;
        time_t issueDate = {};
        time_t nextUpdate = {};
        uint32_t isvProdId = 0;
        int version = Version::V2;
        EnclaveID id = EnclaveID::QE;
        uint32_t tcbEvaluationDataNumber;
        std::vector<TCBLevel> tcbLevels;

        Status status = STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT;
    };
}}}

#endif //SGXECDSAATTESTATION_ENCLAVEIDENTITYV2_H

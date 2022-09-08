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

#include <tuple>
#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "JsonParser.h"

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace json {
    uint8_t TcbComponent::getSvn() const {
        return _svn;
    }

    const std::string &TcbComponent::getCategory() const {
        return _category;
    }

    const std::string &TcbComponent::getType() const {
        return _type;
    }

    TcbComponent::TcbComponent(const ::rapidjson::Value& tcbComponent) {
        if (!tcbComponent.IsObject())
        {
            throw FormatException("TCB Component should be an object");
        }
        _svn = 0;
        JsonParser jsonParser;

        auto status = JsonParser::Missing;
        uint32_t svnTemporary = 0;
        std::tie(svnTemporary, status)  = jsonParser.getUintFieldOf(tcbComponent, "svn");

        if (status != JsonParser::OK)
        {
            throw FormatException("TCB Component JSON should has [svn] field and it should be unsigned integer");
        }

        _svn = static_cast<uint8_t>(svnTemporary);

        std::tie(_category, status)  = jsonParser.getStringFieldOf(tcbComponent, "category");
        if (status == JsonParser::Invalid)
        {
            throw FormatException("TCB Component JSON's [category] field should be string");
        }

        std::tie(_type, status)  = jsonParser.getStringFieldOf(tcbComponent, "type");
        if (status == JsonParser::Invalid)
        {
            throw FormatException("TCB Component JSON's [type] field should be string");
        }
    }

    bool TcbComponent::operator>(const TcbComponent& other) const
    {
        return _svn > other._svn;
    }

    bool TcbComponent::operator<(const TcbComponent& other) const
    {
        return _svn < other._svn;
    }
}}}}}
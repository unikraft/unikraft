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
/**
 * File: pccs_response_object.cpp 
 *  
 * Description: Object class to save PCCS/PCS response message
 *
 */
#include "pccs_response_object.h"
#include "qcnl_def.h"
#include "qcnl_util.h"

PccsResponseObject::PccsResponseObject() : is_body_json_(false) {
}

PccsResponseObject::~PccsResponseObject() {
}

PccsResponseObject &PccsResponseObject::set_raw_header(const char *header, uint32_t header_size) {
    // Set raw header
    if (header && header_size > 0) {
        header_raw_ = header;
        // Convert raw header to unordered_map
        http_header_to_map(header, header_size, header_map_);
    }

    return *this;
}
PccsResponseObject &PccsResponseObject::set_raw_body(const char *body, uint32_t body_size) {
    // Set raw body
    if (body && body_size > 0) {
        body_raw_.assign(body, body_size);
        // Parse response body
        ParseResult ok = body_json_.Parse(body_raw_.c_str());
        if (ok) {
            is_body_json_ = true;
        }
    }

    return *this;
}

string &PccsResponseObject::get_raw_body() {
    return body_raw_;
}

string PccsResponseObject::get_header_key_value(const char *key) {
    unordered_map<string, string>::const_iterator it;
    it = header_map_.find(key);
    if (it != header_map_.end()) {
        return it->second;
    } else {
        return "";
    }
}

string PccsResponseObject::get_body_key_value(const char *key) {
    if (is_body_json_ && body_json_.HasMember(key)) {
        Value &val = body_json_[key];
        if (val.IsString()) {
            return val.GetString();
        } else {
            return "";
        }

    } else {
        return "";
    }
}

string PccsResponseObject::get_real_response_body(const char *key) {
    string body = get_body_key_value(key);
    if (body.empty())
        return body_raw_;
    else return body;
}

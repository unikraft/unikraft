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
/** File: pccs_response_object.h 
 *  
 * Description: Header file of PccsResponseObject class and its sub-classes
 *
 */
#ifndef PCCSRESPONSEOBJECT_H_
#define PCCSRESPONSEOBJECT_H_
#pragma once

#include "document.h"
#include "qcnl_def.h"
#include <string>
#include <unordered_map>

using namespace std;
using namespace rapidjson;

class PccsResponseObject {
private:
public:
    PccsResponseObject();
    ~PccsResponseObject();

    PccsResponseObject &set_raw_header(const char *header, uint32_t header_size);
    PccsResponseObject &set_raw_body(const char *body, uint32_t body_size);
    string &get_raw_body();
    string get_header_key_value(const char *key);
    string get_body_key_value(const char *key);
    string get_real_response_body(const char *key);

protected:
    string header_raw_;
    unordered_map<string, string> header_map_;
    string body_raw_;
    Document body_json_;
    bool is_body_json_;
};

class PckCertResponseObject : public PccsResponseObject {
private:
public:
    PckCertResponseObject() {}
    ~PckCertResponseObject() {}
    string get_tcbm() {
        string tcbm = this->get_header_key_value(intelpcs::SGX_TCBM);
        return tcbm.empty() ? this->get_body_key_value(azurepccs::SGX_TCBM) : tcbm;
    }
    string get_pckcert_issuer_chain() {
        string chain = this->get_header_key_value(intelpcs::PCK_CERT_ISSUER_CHAIN);
        return chain.empty() ? this->get_body_key_value(azurepccs::PCK_CERT_ISSUER_CHAIN) : chain;
    }
    string get_pckcert() {
        return this->get_real_response_body(azurepccs::PCK_CERT);
    }
};

class PckCrlResponseObject : public PccsResponseObject {
private:
public:
    PckCrlResponseObject() {}
    ~PckCrlResponseObject() {}
    string get_pckcrl_issuer_chain() {
        // string chain = this->get_header_key_value(intelpcs::CRL_ISSUER_CHAIN);
        // return chain.empty() ? this->get_body_key_value(azurepccs::CRL_ISSUER_CHAIN) : chain;
        return this->get_header_key_value(intelpcs::CRL_ISSUER_CHAIN);
    }
    string get_pckcrl() {
        //return this->get_real_response_body();
        return this->body_raw_;
    }
};

class TcbInfoResponseObject : public PccsResponseObject {
private:
public:
    TcbInfoResponseObject() {}
    ~TcbInfoResponseObject() {}
    string get_tcbinfo_issuer_chain() {
        string chain = this->get_header_key_value(intelpcs::SGX_TCB_INFO_ISSUER_CHAIN);
        if (!chain.empty())
            return chain;

        chain = this->get_header_key_value(intelpcs::TCB_INFO_ISSUER_CHAIN);
        if (!chain.empty())
            return chain;

        // chain = this->get_body_key_value(azurepccs::SGX_TCB_INFO_ISSUER_CHAIN);
        // if (!chain.empty())
        //     return chain;

        // return this->get_body_key_value(azurepccs::TCB_INFO_ISSUER_CHAIN);

        return "";
    }
    string get_tcbinfo() {
        //return this->get_real_response_body();
        return body_raw_;
    }
};

class QeIdentityResponseObject : public PccsResponseObject {
private:
public:
    QeIdentityResponseObject() {}
    ~QeIdentityResponseObject() {}
    string get_enclave_id_issuer_chain() {
        // string chain = this->get_header_key_value(intelpcs::ENCLAVE_ID_ISSUER_CHAIN);
        // return chain.empty() ? this->get_body_key_value(azurepccs::ENCLAVE_ID_ISSUER_CHAIN) : chain;
        return this->get_header_key_value(intelpcs::ENCLAVE_ID_ISSUER_CHAIN);
    }
    string get_qeidentity() {
        //return this->get_real_response_body();
        return body_raw_;
    }
};

class QveIdentityResponseObject : public PccsResponseObject {
private:
public:
    QveIdentityResponseObject() {}
    ~QveIdentityResponseObject() {}
    string get_enclave_id_issuer_chain() {
        // string chain = this->get_header_key_value(intelpcs::ENCLAVE_ID_ISSUER_CHAIN);
        // return chain.empty() ? this->get_body_key_value(azurepccs::ENCLAVE_ID_ISSUER_CHAIN) : chain;
        return this->get_header_key_value(intelpcs::ENCLAVE_ID_ISSUER_CHAIN);
    }
    string get_qveidentity() {
        // return this->get_real_response_body();
        return body_raw_;
    }
};

#endif
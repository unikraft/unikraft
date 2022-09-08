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
 * File: qcnl_config.cpp 
 *  
 * Description: Read configuration data
 *
 */

#include "qcnl_config.h"
#include "sgx_default_qcnl_wrapper.h"
#include <fstream>
#include <istreamwrapper.h>

using namespace std;

std::shared_ptr<QcnlConfig> QcnlConfig::myInstance;

std::shared_ptr<QcnlConfig> QcnlConfig::Instance() {
    if (!myInstance) {
        QcnlConfigJson *pConfigJson = new QcnlConfigJson();
        if (pConfigJson->load_config()) {
            myInstance.reset(pConfigJson);
        } else {
            delete pConfigJson;
            pConfigJson = nullptr;
            QcnlConfigLegacy *pConfigLegacy = new QcnlConfigLegacy();
            pConfigLegacy->load_config();
            myInstance.reset(pConfigLegacy);
        }
    }

    return myInstance;
}

bool QcnlConfig::load_config_json(const TCHAR *json_file) {
    ifstream ifs(json_file);
    IStreamWrapper isw(ifs);

    Document config;
    ParseResult ok = config.ParseStream<kParseCommentsFlag>(isw);

    if (!ok) {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Failed to load config file in JSON. Either the legacy format config file is used, \n");
        qcnl_log(SGX_QL_LOG_INFO, "       or there is something wrong with it (should be JSON format). \n");
        return false;
    }
    else {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] JSON config file %s is loaded successfully. \n", json_file);
    }

    if (config.HasMember("pccs_url")) {
        Value &val = config["pccs_url"];
        if (val.IsString()) {
            this->server_url_ = val.GetString();
            this->collateral_service_url_ = this->server_url_;
        }
    }

    if (config.HasMember("use_secure_cert")) {
        Value &val = config["use_secure_cert"];
        if (val.IsBool())
            this->use_secure_cert_ = val.GetBool();
    }

    if (config.HasMember("collateral_service")) {
        // will overwrite the previous value
        Value &val = config["collateral_service"];
        if (val.IsString()) {
            this->collateral_service_url_ = val.GetString();
        }
    }

    if (config.HasMember("pccs_api_version")) {
        Value &val = config["pccs_api_version"];
        if (val.IsString()) {
            this->collateral_version_ = val.GetString();
        }
    }

    if (config.HasMember("retry_times")) {
        Value &val = config["retry_times"];
        if (val.IsInt()) {
            this->retry_times_ = val.GetInt();
        }
    }

    if (config.HasMember("retry_delay")) {
        Value &val = config["retry_delay"];
        if (val.IsInt()) {
            this->retry_delay_ = val.GetInt();
        }
    }

    if (config.HasMember("local_pck_url")) {
        Value &val = config["local_pck_url"];
        if (val.IsString()) {
            this->local_pck_url_ = val.GetString();
        }
    }

    if (config.HasMember("pck_cache_expire_hours")) {
        Value &val = config["pck_cache_expire_hours"];
        if (val.IsInt()) {
            this->cache_expire_hour_ = val.GetInt();
            if (this->cache_expire_hour_ > CACHE_MAX_EXPIRY_HOURS)
                this->cache_expire_hour_ = CACHE_MAX_EXPIRY_HOURS;
        }
    }

    if (config.HasMember("custom_request_options")) {
        Value &val = config["custom_request_options"];
        if (val.IsObject())
            custom_request_options_.CopyFrom(val, custom_request_options_.GetAllocator());
    }

    return true;
}
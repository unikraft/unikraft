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
/** File: qcnl_config.h 
 *  
 * Description: Configurations for QCNL library
 *
 */
#ifndef QCNLCONFIG_H_
#define QCNLCONFIG_H_
#pragma once

#include "document.h"
#include <string>
#include <memory>

using namespace std;
using namespace rapidjson;

#ifndef _MSC_VER
#define TCHAR char
#define _T(x) (x)
#else
#include <tchar.h>
#include <windows.h>
#endif

#define CACHE_MAX_EXPIRY_HOURS  2160

class QcnlConfig {
protected:
    // Default URL for PCCS server if configuration file doesn't exist
    string server_url_;
    // Use secure HTTPS certificate or not
    bool use_secure_cert_;
    // If defined in config file, will use this URL to get collateral
    string collateral_service_url_;
    // PCCS's API version
    string collateral_version_;
    // Max retry times
    uint32_t retry_times_;
    // Retry delay time in seconds
    uint32_t retry_delay_;
    // Local URL address
    string local_pck_url_;
    // Cache expire hours
    uint32_t cache_expire_hour_;
    // custom request options for Azure
    Document custom_request_options_;

    QcnlConfig() : server_url_("https://localhost:8081/sgx/certification/v3/"),
                   use_secure_cert_(true),
                   collateral_service_url_(server_url_),
                   collateral_version_("3.0"),
                   retry_times_(0),
                   retry_delay_(0),
                   local_pck_url_(""),
                   cache_expire_hour_(0) {}
    // To define the virtual destructor outside the class:
    virtual ~QcnlConfig() {
    };

public:
    QcnlConfig(QcnlConfig const &) = delete;
    QcnlConfig(QcnlConfig &&) = delete;
    QcnlConfig &operator=(QcnlConfig const &) = delete;
    QcnlConfig &operator=(QcnlConfig &&) = delete;

    static std::shared_ptr<QcnlConfig> myInstance;
    static std::shared_ptr<QcnlConfig> Instance();

    string getServerUrl() {
        return server_url_;
    }

    bool is_server_secure() {
        return use_secure_cert_;
    }

    string getCollateralServiceUrl() {
        return collateral_service_url_;
    }

    string getCollateralVersion() {
        return collateral_version_;
    }

    uint32_t getRetryTimes() {
        return retry_times_;
    }

    uint32_t getRetryDelay() {
        return retry_delay_;
    }

    string getLocalPckUrl() {
        return local_pck_url_;
    }

    uint32_t getCacheExpireHour() {
        return cache_expire_hour_;
    }

    Document &getCustomRequestOptions() {
        return custom_request_options_;
    }

    bool load_config_json(const TCHAR *json_file);
};

class QcnlConfigLegacy : public QcnlConfig {
public:
    QcnlConfigLegacy() {}
    ~QcnlConfigLegacy() {}

public:
    bool load_config();
};

class QcnlConfigJson : public QcnlConfig {
public:
    QcnlConfigJson() {}
    ~QcnlConfigJson() {}

public:
    bool load_config();
};

#endif //QCNLCONFIG_H_
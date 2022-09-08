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
#include <tchar.h>

using namespace std;

#define MAX_URL_LENGTH 2083
#define REG_KEY_SGX_QCNL _T("SOFTWARE\\Intel\\SGX\\QCNL")
#define REG_VALUE_QCNL_PCCS_URL _T("PCCS_URL")
#define REG_VALUE_QCNL_USE_SECURE_CERT _T("USE_SECURE_CERT")
#define REG_VALUE_QCNL_COLLATERAL_SERVICE _T("COLLATERAL_SERVICE")
#define REG_VALUE_QCNL_PCCS_VERSION _T("PCCS_API_VERSION")
#define REG_VALUE_QCNL_RETRY_TIMES _T("RETRY_TIMES")
#define REG_VALUE_QCNL_RETRY_DELAY _T("RETRY_DELAY")
#define REG_VALUE_QCNL_LOCAL_PCK_URL _T("LOCAL_PCK_URL")
#define REG_VALUE_QCNL_CACHE_EXPIRE_HOURS _T("PCK_CACHE_EXPIRE_HOURS")
#define REG_VALUE_QCNL_CONFIG_FILE _T("CONFIG_FILE")


bool QcnlConfigLegacy::load_config() {
    // read registry
    // Read configuration data from registry
    // Open the Registry Key
    HKEY key = NULL;
    LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_SGX_QCNL, 0, KEY_READ, &key);
    if (ERROR_SUCCESS != status) {
        return false;
    }

    DWORD type, count;
    TCHAR url[MAX_URL_LENGTH] = {0};

    // Get PCCS URL
    count = MAX_URL_LENGTH * sizeof(TCHAR);
    status = RegQueryValueEx(key, REG_VALUE_QCNL_PCCS_URL, NULL, &type, (LPBYTE)url, &count);
    if (ERROR_SUCCESS == status && type == REG_SZ) {
        size_t input_len = _tcsnlen(url, MAX_URL_LENGTH);
        size_t output_len = 0;
        char url_multi[MAX_URL_LENGTH];

        if (wcstombs_s(&output_len, url_multi, MAX_URL_LENGTH, url, input_len) != 0) {
            // Use default value
        } else {
            server_url_ = url_multi;
        }
    }

    // Get Collateral Service URL
    memset(url, 0, sizeof(url));
    status = RegQueryValueEx(key, REG_VALUE_QCNL_COLLATERAL_SERVICE, NULL, &type, (LPBYTE)url, &count);
    if (ERROR_SUCCESS == status && type == REG_SZ) {
        size_t input_len = _tcsnlen(url, MAX_URL_LENGTH);
        size_t output_len = 0;
        char collateral_service_url_multi[MAX_URL_LENGTH];

        if (wcstombs_s(&output_len, collateral_service_url_multi, MAX_URL_LENGTH, url, input_len) != 0) {
            // Use default value
        } else {
            collateral_service_url_ = collateral_service_url_multi;
        }
    } else {
        // If collateral service url is not defined, use the default service url
        collateral_service_url_ = server_url_;
    }

    // Get PCCS Version
    const DWORD vlen = 20;
    TCHAR collateral_version[vlen] = {0};
    count = vlen * sizeof(TCHAR);
    status = RegQueryValueEx(key, REG_VALUE_QCNL_PCCS_VERSION, NULL, &type, (LPBYTE)collateral_version, &count);
    if (ERROR_SUCCESS == status && type == REG_SZ) {
        size_t input_len = _tcsnlen(collateral_version, vlen);
        size_t output_len = 0;
        char collateral_version_multi[20];

        if (wcstombs_s(&output_len, collateral_version_multi, vlen, collateral_version, input_len) != 0) {
            // Use default value
        } else {
            collateral_version_ = collateral_version_multi;
        }
    }

    count = sizeof(DWORD);
    DWORD dwSecureCert = 0;
    status = RegQueryValueEx(key, REG_VALUE_QCNL_USE_SECURE_CERT, NULL, &type, (LPBYTE)&dwSecureCert, &count);
    if (ERROR_SUCCESS == status && type == REG_DWORD) {
        use_secure_cert_ = (dwSecureCert != 0);
    }

    DWORD dwRetryTimes = 0;
    status = RegQueryValueEx(key, REG_VALUE_QCNL_RETRY_TIMES, NULL, &type, (LPBYTE)&dwRetryTimes, &count);
    if (ERROR_SUCCESS == status && type == REG_DWORD) {
        retry_times_ = (uint32_t)dwRetryTimes;
    }

    DWORD dwRetryDelay = 0;
    status = RegQueryValueEx(key, REG_VALUE_QCNL_RETRY_DELAY, NULL, &type, (LPBYTE)&dwRetryDelay, &count);
    if (ERROR_SUCCESS == status && type == REG_DWORD) {
        retry_delay_ = (uint32_t)dwRetryDelay;
    }

    // Get LOCAL_PCK_URL
    memset(url, 0, sizeof(url));
    status = RegQueryValueEx(key, REG_VALUE_QCNL_LOCAL_PCK_URL, NULL, &type, (LPBYTE)url, &count);
    if (ERROR_SUCCESS == status && type == REG_SZ) {
        size_t input_len = _tcsnlen(url, MAX_URL_LENGTH);
        size_t output_len = 0;
        char url_multi[MAX_URL_LENGTH];

        if (wcstombs_s(&output_len, url_multi, MAX_URL_LENGTH, url, input_len) != 0) {
            // Use default value
        } else {
            local_pck_url_ = url_multi;
        }
    }

    DWORD dwCacheExpireHours = 0;
    status = RegQueryValueEx(key, REG_VALUE_QCNL_CACHE_EXPIRE_HOURS, NULL, &type, (LPBYTE)&dwCacheExpireHours, &count);
    if (ERROR_SUCCESS == status && type == REG_DWORD) {
        cache_expire_hour_ = (uint32_t)dwCacheExpireHours;
        if (cache_expire_hour_ > CACHE_MAX_EXPIRY_HOURS)
            cache_expire_hour_ = CACHE_MAX_EXPIRY_HOURS;
    }

    RegCloseKey(key);

    return true;
}

bool QcnlConfigJson::load_config() {

    // read registry to get config file location
    // Open the Registry Key
    HKEY key = NULL;
    LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_SGX_QCNL, 0, KEY_READ, &key);
    if (ERROR_SUCCESS != status) {
        return false;
    }

    DWORD type, count;
    TCHAR config_path[MAX_PATH] = { 0 };

    // Get config file path
    count = MAX_PATH * sizeof(TCHAR);
    status = RegQueryValueEx(key, REG_VALUE_QCNL_CONFIG_FILE, NULL, &type, (LPBYTE)config_path, &count);
    RegCloseKey(key);

    if (ERROR_SUCCESS == status && type == REG_SZ) {
        // Load JSON config
        return this->load_config_json(config_path);
    }
    else {
        return false;
    }
}
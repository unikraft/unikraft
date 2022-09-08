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
 * File: AgentConfigurationWin.cpp
 *  
 * Description: Windows implementation for retrieving the MP Agent
 *              configurations.  
 *
 */
#include <Windows.h>
#include <tchar.h>
#include "AgentConfiguration.h"
#include "agent_logger.h"
#include "common.h"

static const TCHAR *get_registry_entry_path(mp_config_type entry)
{
    switch (entry) {
    case MP_PROXY_CONF:
        return _T("SOFTWARE\\Intel\\SGX_RA\\RAProxy");
    case MP_LOG_LEVEL_CONF:
        return _T("SOFTWARE\\Intel\\SGX_RA\\RALog");
    case MP_SUBSCRIPTION_KEY_CONF:
        return _T("SOFTWARE\\Intel\\SGX_RA\\RASubscriptionKey");
    default:
        return NULL;
    }
}

MpResult mpa_read_registry_dword(mp_config_type entry, const TCHAR *name, uint32_t *result)
{
    const TCHAR *entry_path = get_registry_entry_path(entry);
    if (entry_path == NULL) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Cannot find registry entry path\n");
        return MP_INVALID_PARAMETER;
    }
    HKEY key = NULL;
    LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, entry_path, 0, KEY_READ, &key);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to open registry key (%s), return value %ld\n", entry_path, status);
        return MP_UNEXPECTED_ERROR;
    }
    DWORD type, count;
    count = sizeof(uint32_t);
    status = RegQueryValueEx(key, name, NULL, &type, (LPBYTE)result, &count);
    RegCloseKey(key);
    if (ERROR_SUCCESS != status ||
        type != REG_DWORD) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to query value %s:status=%d, type=%d\n", name, (int)status, (int)type);
        return MP_UNEXPECTED_ERROR;
    }
    return MP_SUCCESS;
}

MpResult mpa_create_registry_and_set_default_value_dword(mp_config_type entry, const TCHAR *name, uint32_t result)
{
    const TCHAR *entry_path = get_registry_entry_path(entry);
    if (entry_path == NULL) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Cannot find registry entry path\n");
        return MP_INVALID_PARAMETER;
    }
    HKEY key = NULL;
    LSTATUS status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, entry_path, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to open registry key (%s), return value %ld\n", entry_path, status);
        return MP_UNEXPECTED_ERROR;
    }
    DWORD type, count;
    type = REG_DWORD;
    count = sizeof(uint32_t);
    status = RegSetValueEx(key, name, NULL, type, (LPBYTE)&result, count);
    RegCloseKey(key);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to set value %s:status=%d\n", name, (int)status);
        return MP_UNEXPECTED_ERROR;
    }
    return MP_SUCCESS;
}


MpResult mpa_create_registry_string_subscriptionkey(mp_config_type entry)
{
    const TCHAR* entry_path = get_registry_entry_path(entry);
    if (entry_path == NULL) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Cannot find registry entry path\n");
        return MP_INVALID_PARAMETER;
    }
   
    HKEY key = NULL;
    LSTATUS status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, entry_path, 0, NULL,0, KEY_WRITE, NULL, &key, NULL);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to create registry key (%s), return value %ld\n", entry_path, status);
        return MP_UNEXPECTED_ERROR;
    }

    TCHAR subscriptionkey[] = _T("12345678901234567890123456789012");
    status = RegSetValueEx(key, _T("token"), NULL, REG_SZ, (LPBYTE)subscriptionkey, sizeof(subscriptionkey));
    RegCloseKey(key);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to set the sub scription key, and the status=%d\n", (int)status);
        return MP_UNEXPECTED_ERROR;
    }

    return MP_SUCCESS;
}

MpResult mpa_create_registry_string_proxy_url(mp_config_type entry)
{
    const TCHAR* entry_path = get_registry_entry_path(entry);
    if (entry_path == NULL) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Cannot find registry entry path\n");
        return MP_INVALID_PARAMETER;
    }

    HKEY key = NULL;
    LSTATUS status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, entry_path, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to create registry key (%s), return value %ld\n", entry_path, status);
        return MP_UNEXPECTED_ERROR;
    }

    TCHAR proxy_url[] = _T("http://proxy-url:proxy-port");
    status = RegSetValueEx(key, _T("url"), NULL, REG_SZ, (LPBYTE)proxy_url, sizeof(proxy_url));
    RegCloseKey(key);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to set proxy url for manual proxy type, and the status=%ld\n", status);
        return MP_UNEXPECTED_ERROR;
    }
    return MP_SUCCESS;
}




MpResult mpa_read_registry_value(const TCHAR * registry_path, const TCHAR *name, TCHAR value[], uint32_t tchar_num)
{
    HKEY key = NULL;
    if (!registry_path || !name || !value || !tchar_num)
        return MP_INVALID_PARAMETER;
    LSTATUS status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_path, 0, KEY_READ, &key);
    if (ERROR_SUCCESS != status) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to open registry key (%s), return value %ld\n", registry_path, status);
        return MP_UNEXPECTED_ERROR;
    }
    DWORD type, count;
    count = tchar_num * sizeof(TCHAR);
    status = RegQueryValueEx(key, name, NULL, &type, (LPBYTE)value, &count);
    RegCloseKey(key);
    if (ERROR_SUCCESS != status ||
        type != REG_SZ) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to query value s: name (%s) status=%d, type=%d\n", name, (int)status, (int)type);
        return MP_UNEXPECTED_ERROR;
    }
    if (strnlen(value, tchar_num) >= tchar_num) {
        return MP_UNEXPECTED_ERROR;
    }
    return MP_SUCCESS;
}

MpResult mpa_read_registry_string(mp_config_type entry, const TCHAR *name, TCHAR value[], uint32_t tchar_num)
{
    const TCHAR *entry_path = get_registry_entry_path(entry);
    if (entry_path == NULL) {
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Cannot find registry entry path\n");
        return MP_UNEXPECTED_ERROR;
    }
    return mpa_read_registry_value(entry_path, name, value, tchar_num);
}


bool AgentConfiguration::read(MPConfigurations& conf)
{
    MpResult res = MP_UNEXPECTED_ERROR;
    uint32_t value = 0;
    TCHAR valueStr[MAX_PATH_SIZE] = { 0 };
    memset(&conf, 0, sizeof(MPConfigurations));
    conf.log_level = MP_REG_LOG_LEVEL_ERROR;//default log level

    res = mpa_read_registry_dword(MP_PROXY_CONF, _T("type"), &value);
    if (MP_SUCCESS == res) {
        conf.proxy.proxy_type = (ProxyType)value;
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Found proxy type reg key: %d\n", value);
    } else {
        uint32_t proxy_type = MP_REG_PROXY_TYPE_DEFAULT_PROXY;
        conf.proxy.proxy_type = (ProxyType) proxy_type;
        mpa_create_registry_and_set_default_value_dword(MP_PROXY_CONF, _T("type"), proxy_type);
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Using deafult proxy type settings: %d.\n", value);
    }

    res = mpa_read_registry_string(MP_PROXY_CONF, _T("url"), valueStr, (uint32_t)sizeof(valueStr));
    if (MP_SUCCESS == res) {
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Found proxy url reg key: %s\n", valueStr);
        memcpy(conf.proxy.proxy_url, valueStr, strnlen_s(valueStr, sizeof(valueStr)));
    } else {
        if (MP_REG_PROXY_TYPE_MANUAL_PROXY == conf.proxy.proxy_type) {
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Found manual proxy type reg key without url. Using deafult proxy configuration.\n");
            conf.proxy.proxy_type = MP_REG_PROXY_TYPE_DEFAULT_PROXY;
        }
        mpa_create_registry_string_proxy_url(MP_PROXY_CONF);
    }

    res = mpa_read_registry_dword(MP_LOG_LEVEL_CONF, _T("level"), &value);
    if (MP_SUCCESS == res) {
        conf.log_level = (LogLevel)value;
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Found log level reg key: %d\n", value);
    } else {
        uint32_t log_level = MP_REG_LOG_LEVEL_ERROR;
        conf.log_level = (LogLevel)log_level;
        mpa_create_registry_and_set_default_value_dword(MP_LOG_LEVEL_CONF, _T("level"), log_level);
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Using deafult log level settings: %d.\n", log_level);
    }

    res = mpa_read_registry_string(MP_SUBSCRIPTION_KEY_CONF, _T("token"), valueStr, (uint32_t)sizeof(valueStr));
    if (MP_SUCCESS == res) {
        memcpy_s(conf.server_add_package_subscription_key, SUBSCRIPTION_KEY_SIZE, valueStr, strnlen_s(valueStr, sizeof(valueStr)));
        conf.server_add_package_subscription_key[SUBSCRIPTION_KEY_SIZE] = _T('\0');
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Found subscription token reg key: %s\n", valueStr);
    }
    else {
        //the sub scription key should be applied from Inte's registration server, user need provide this key
        //https://api.portal.trustedservices.intel.com/registration
        mpa_create_registry_string_subscriptionkey(MP_SUBSCRIPTION_KEY_CONF);
    }
    return true;
}

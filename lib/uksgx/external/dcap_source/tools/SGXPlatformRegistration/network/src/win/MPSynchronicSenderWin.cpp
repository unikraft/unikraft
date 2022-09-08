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
 * File: MPSynchronicSenderWin.cpp
 *   
 * Description: Windows specific implementation for the MPSynchronicSender 
 * class to send network requests.	 
 */
#include <windows.h>
#include <winhttp.h>
#include <assert.h>
#include <tchar.h>
#include <versionhelpers.h>
#include "network_logger.h"
#include "MPSynchronicSender.h"
#include "common.h"

#define DEFAULT_CONNECT_TIME_OUT_VALUE (5*1000)
#define REQUEST_ID_STR_RESPONSE_HEADER _T(L"Request-ID")
#define ERROR_CODE_STR_RESPONSE_HEADER _T(L"Error-Code")
#define ERROR_MSG_STR_RESPONSE_HEADER _T(L"Error-Message")
#define MAX_HEADER_SIZE 1024
#define MAX_HEADER_LINE_SIZE 512
#define NUM_OF_DESIRED_CHIPERS 26
#define TLS_FAILURE_TIME_TO_WAIT 200

#pragma comment(lib, "winhttp")

typedef struct _http_network_info_t {
    HINTERNET internet;
    HINTERNET session;
    HINTERNET http_handle;
    DWORD     proxy_auth_scheme;
    LPCWSTR   proxy_username;
    LPCWSTR   proxy_password;
    LogLevel  log_level;
}http_network_info_t;

static MpResult http_network_init(http_network_info_t *info, const char *url, ProxyConf &proxy)
{
    MpResult ret = MP_UNEXPECTED_ERROR;
    LPCWSTR p_proxy = WINHTTP_NO_PROXY_NAME;
    LPCWSTR p_bypass = WINHTTP_NO_PROXY_BYPASS;
    DWORD proxy_access_type;
    bool need_test_auto_proxy = false;
    WCHAR wurl[MAX_PATH] = { 0 };
    WCHAR purl[MAX_PATH] = { 0 };
    WCHAR whostname[MAX_PATH] = { 0 };
    WCHAR phostname[MAX_PATH] = { 0 };
    LPCWSTR octet_accept_type[] = { L"application/octet-stream", NULL };
    LPCWSTR *accept_type = octet_accept_type;
    URL_COMPONENTS urlComp;
    DWORD request_flags = 0;
    DWORD temp_value = 0;
    DWORD options_flags = 0;
    DWORD dwEnableSSLRevocOpt = 0;
    size_t count = 0;
    LogLevel m_logLevel = info->log_level;

	/* configure proxy settings */
    switch (proxy.proxy_type) {
    case MP_REG_PROXY_TYPE_DEFAULT_PROXY:
        proxy_access_type = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
        if (!IsWindows8Point1OrGreater()) {
            proxy_access_type = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        }
        need_test_auto_proxy = true;
        break;
    case MP_REG_PROXY_TYPE_MANUAL_PROXY:
		proxy_access_type = WINHTTP_ACCESS_TYPE_NAMED_PROXY;

        //WinHTTP API explicitly use UNICODE so that we should use WCHAR instead of TCHAR
        if (mbstowcs_s(&count, purl, proxy.proxy_url, strlen(proxy.proxy_url)) != 0) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "proxy mbstowcs failed for url: %s\n", url);
            goto out;
        }
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = phostname;//we will only crack hostname, urlpath 
        urlComp.dwHostNameLength = MAX_PATH;//copy hostname to a buffer to get 0-terminated string
        urlComp.dwUrlPathLength = (DWORD)-1;

        if (!WinHttpCrackUrl(purl, (DWORD)wcsnlen_s(purl, MAX_PATH), 0, &urlComp)) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "proxy could not be parsed: %ls, last error: %d\n", purl, GetLastError());
            goto out;
        }

        p_proxy = purl;
        if (urlComp.dwUserNameLength > 0) {
            info->proxy_auth_scheme = urlComp.nScheme;
            info->proxy_username = urlComp.lpszUserName;
        }
        if (urlComp.dwPasswordLength > 0) {
            info->proxy_password = urlComp.lpszPassword;
        }
        network_log_message(MP_REG_LOG_LEVEL_INFO, "Using manual proxy settings, proxy url: %s\n", proxy.proxy_url);
        break;
    case MP_REG_PROXY_TYPE_DIRECT_ACCESS:
        proxy_access_type = WINHTTP_ACCESS_TYPE_NO_PROXY;
        break;
    default:
        proxy_access_type = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
        network_log_message(MP_REG_LOG_LEVEL_INFO, "Unrecognized proxy type, using default windows configurations. %d\n", proxy.proxy_type);
        break;
    }

    //WinHTTP API explicitly use UNICODE so that we should use WCHAR instead of TCHAR
    if (mbstowcs_s(&count, wurl, url, strlen(url)) != 0) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "url mbstowcs failed for url: %s\n", url);
        goto out;
    }
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = whostname;//we will only crack hostname, urlpath 
    urlComp.dwHostNameLength = MAX_PATH;//copy hostname to a buffer to get 0-terminated string
    urlComp.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(wurl, (DWORD)wcsnlen_s(wurl, MAX_PATH), 0, &urlComp)){
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "url could not be parsed: %ls, last error: %d\n", wurl, GetLastError());
        ret = MP_NETWORK_ERROR;
        goto out;
    }

    network_log_message(MP_REG_LOG_LEVEL_INFO, "HTTP network connect, server: %ls, port %u, path: %ls\n",
                        urlComp.lpszHostName, urlComp.nPort, urlComp.lpszUrlPath);
    info->internet = WinHttpOpen(L"SGXRegistrationService", proxy_access_type, p_proxy, p_bypass, 0);
    if (info->internet == NULL) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "failure during WinHttpOpen, last error: %d\n", GetLastError());
        ret = MP_NETWORK_ERROR;
        goto out;
    }

    // disable insecure tls protocols.
    options_flags = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    if (!WinHttpSetOption(info->internet, WINHTTP_OPTION_SECURE_PROTOCOLS, &options_flags, sizeof(options_flags))) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed setting secure crypto protocols with error code: %d\n", GetLastError());
        goto out;
    }

    // connect to registration server
    info->session = WinHttpConnect(info->internet, urlComp.lpszHostName, static_cast<INTERNET_PORT>(urlComp.nPort), 0);
    if (info->session == NULL) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to connect to server %ls, port %d, last error: %d\n", urlComp.lpszHostName, static_cast<int>(urlComp.nPort), GetLastError());
        ret = MP_NETWORK_ERROR;
        goto out;
    }

    request_flags |= WINHTTP_FLAG_SECURE;
    request_flags |= WINHTTP_FLAG_BYPASS_PROXY_CACHE;

    // HTTP/1.0 POST protocol used
    info->http_handle = WinHttpOpenRequest(info->session, L"POST", urlComp.lpszUrlPath, L"HTTP/1.0", WINHTTP_NO_REFERER,
                                           accept_type, request_flags);
    if (info->http_handle == NULL) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to open HTTP Request, last error: %d\n", GetLastError());
        ret = MP_NETWORK_ERROR;
        goto out;
    }

    temp_value = DEFAULT_CONNECT_TIME_OUT_VALUE;//set default connection timeout value
    if (!WinHttpSetOption(info->http_handle, WINHTTP_OPTION_CONNECT_TIMEOUT, &temp_value, sizeof(DWORD))) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to set timeout information, last error: %d\n", GetLastError());
        ret = MP_NETWORK_ERROR;
        goto out;
    }

    // Enable the certificate revocation check
    dwEnableSSLRevocOpt = WINHTTP_ENABLE_SSL_REVOCATION;
    if (!WinHttpSetOption(info->http_handle, WINHTTP_OPTION_ENABLE_FEATURE, &dwEnableSSLRevocOpt, sizeof(dwEnableSSLRevocOpt))) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "Error enabing SSL revocation check with error code: %d\n", GetLastError());
        ret = MP_NETWORK_ERROR;
        goto out;
    }

    if (need_test_auto_proxy) {
        // Set up the autoproxy call.
        WINHTTP_AUTOPROXY_OPTIONS  AutoProxyOptions;
        WINHTTP_PROXY_INFO         ProxyInfo;
        DWORD                      cbProxyInfoSize = sizeof(ProxyInfo);
        WCHAR                      wpurl[MAX_PATH];
        size_t                     len;
        network_log_message(MP_REG_LOG_LEVEL_INFO, "try auto proxy\n");
        if (mbstowcs_s(&len, wpurl, MAX_PATH, url, MAX_PATH - 1) != 0) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "url size too large: %ls\n", url);
            goto out;
        }

        ZeroMemory(&AutoProxyOptions, sizeof(AutoProxyOptions));
        ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));
        AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
        // Use DHCP and DNS-based auto-detection.
        AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        AutoProxyOptions.fAutoLogonIfChallenged = TRUE;

        int proxy_set_succ = 0;
        if (WinHttpGetProxyForUrl(info->internet, wpurl, &AutoProxyOptions, &ProxyInfo)) {
            if (WinHttpSetOption(info->http_handle,WINHTTP_OPTION_PROXY,&ProxyInfo,cbProxyInfoSize)) {
                proxy_set_succ = 1;
                network_log_message(MP_REG_LOG_LEVEL_INFO, "succ to use proxy type %d, url: %ls, bypass: %ls\n", ProxyInfo.dwAccessType, ProxyInfo.lpszProxy, ProxyInfo.lpszProxyBypass);
            }else {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "fail to set proxy type %d, url: %ls, bypass: %ls, last error: %d\n", ProxyInfo.dwAccessType, ProxyInfo.lpszProxy, ProxyInfo.lpszProxyBypass, GetLastError());
            }
            if (ProxyInfo.lpszProxy != NULL) {
                GlobalFree(ProxyInfo.lpszProxy);
            }
            if (ProxyInfo.lpszProxyBypass != NULL) {
                GlobalFree(ProxyInfo.lpszProxyBypass);
            }
        }
        if (!proxy_set_succ) {
            //ignored proxy setting error for verified proxy and fall through to default proxy setting
            network_log_message(MP_REG_LOG_LEVEL_INFO, "auto proxy detection failed, default proxy used\n");
        }
    }

    ret = MP_SUCCESS;
out:
    return ret;
}

static void http_network_fini(http_network_info_t *info)
{
    if (info->http_handle) {
        WinHttpCloseHandle(info->http_handle);
    }
    if (info->session) {
        WinHttpCloseHandle(info->session);
    }
    if (info->internet) {
        WinHttpCloseHandle(info->internet);
    }
    memset(info, 0, sizeof(*info));
}

//Function to send data and get resp from server. The proxy_auth_scheme in info could be modified if proxy response with a proxy_auth_scheme
static MpResult http_network_send_data(http_network_info_t *info, const string& subscription_key, 
                                       const uint8_t *req_msg, const uint16_t &msg_size, 
                                       uint8_t *resp_msg, uint16_t &resp_size,
                                       HttpStatusCode& http_response_code, string& errorCodeStr)
{
    WCHAR header[MAX_HEADER_SIZE];
    uint8_t response[MAX_RESPONSE_SIZE];
    uint8_t *response_ptr = response;
    WCHAR *p = header;
    uint32_t left_size = MAX_HEADER_SIZE;
    DWORD ret_size = 0;
    MpResult ret = MP_UNEXPECTED_ERROR;
    BOOL win_http_ret = FALSE;
    BOOL proxy_auth_has_been_required = FALSE;
    DWORD dwHttpSecurityFlags = 0;
    DWORD dwHttpSecurityFlagsSize = sizeof(dwHttpSecurityFlags);
    LogLevel m_logLevel = info->log_level;

    do {

        // Set autenticated proxy
        if (info->proxy_auth_scheme != 0) {//proxy authentication required
            network_log_message(MP_REG_LOG_LEVEL_INFO, "proxy scheme %d used\n", info->proxy_auth_scheme);
            (void)WinHttpSetCredentials(info->http_handle, WINHTTP_AUTH_TARGET_PROXY, info->proxy_auth_scheme,
                info->proxy_username, info->proxy_password, NULL);//ignore the return value if proxy authentication setting failed
        }

        // Set headers
        (void)wcscpy_s(p, left_size, L"Content-Type: application/octet-stream\r\n");
        p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
        left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);;
        assert(wcsnlen_s(header, MAX_HEADER_SIZE) < MAX_HEADER_SIZE);

        if (subscription_key.length() == SUBSCRIPTION_KEY_SIZE) {
            (void)wcscpy_s(p, left_size, L"Ocp-Apim-Subscription-Key: ");
            p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
            left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);;
            assert(wcsnlen_s(header, MAX_HEADER_SIZE) < MAX_HEADER_SIZE);

            wstring wcsubscription_key = wstring(subscription_key.begin(), subscription_key.end());
            (void)wcscpy_s(p, left_size, wcsubscription_key.c_str());
            p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
            left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);;
            assert(wcsnlen_s(header, MAX_HEADER_SIZE) < MAX_HEADER_SIZE);

            (void)wcscpy_s(p, left_size, L"\r\n");
            p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
            left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);;
            assert(wcsnlen_s(header, MAX_HEADER_SIZE) < MAX_HEADER_SIZE);
        }
        (void)wcscpy_s(p, left_size, L"Connection: Keep-Alive\r\nContent-Length: ");//Need to send Keep-Alive for proxy authentication
        p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
        left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);
        assert(wcsnlen_s(header, MAX_HEADER_SIZE) < MAX_HEADER_SIZE);

        (void)_itow_s((int)msg_size, p, left_size, 10);
        p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
        left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);

        assert(left_size >= 3);
        (void)wcscat_s(header, L"\r\n");

        network_log_message(MP_REG_LOG_LEVEL_INFO, "HTTP header: \n%ls", header);
        //send request message
        if (!WinHttpSendRequest(info->http_handle, header, (DWORD)wcsnlen_s(header, MAX_HEADER_SIZE), WINHTTP_NO_REQUEST_DATA, 0, (DWORD)msg_size, 0)) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to send http request, last error: %d\n", GetLastError());
            ret = MP_NETWORK_ERROR;
            break;
        }

        //send message body
        if (!WinHttpWriteData(info->http_handle, req_msg, (DWORD)msg_size, &ret_size)) {
            ret = MP_NETWORK_ERROR;
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to write HTTP Data, last error: %d\n", GetLastError());
            break;
        }
        network_log_message(MP_REG_LOG_LEVEL_INFO, "Data sent to server and wait for response\n");


        if (!WinHttpQueryOption(info->http_handle, WINHTTP_OPTION_SECURITY_FLAGS, &dwHttpSecurityFlags, &dwHttpSecurityFlagsSize)) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to query WinHttp connection, last error: %d\n", GetLastError());
            ret = MP_NETWORK_ERROR;
            break;
        }

        //verify connection is strong (128bits key)
        if (!(dwHttpSecurityFlags & SECURITY_FLAG_STRENGTH_STRONG)) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Secure channel is too weak, closing connection.\n");
            ret = MP_NETWORK_ERROR;
            break;
        }

        //wait for response
        win_http_ret = WinHttpReceiveResponse(info->http_handle, NULL);
        if (!win_http_ret) {
            ret = MP_NETWORK_ERROR;
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to receive HTTP Response, last error: %d\n", GetLastError());
            break;
        }
        DWORD dwStatus = 0;
        DWORD dwSize = sizeof(dwStatus);
        DWORD dwFirstScheme = 0;
        DWORD dwTarget = 0;
        win_http_ret = WinHttpQueryHeaders(info->http_handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &dwStatus, &dwSize, 0);//query for server response error code
        if (!win_http_ret) {
            ret = MP_NETWORK_ERROR;
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to query http response header, last error: %d\n", GetLastError());
            break;
        }
        http_response_code = (HttpStatusCode)dwStatus;

        ret = MP_SUCCESS;
        break;
    } while (1);

    uint16_t old_size = 0;
    //using a loop since the response may arrive as multiple packages
    while (ret == MP_SUCCESS)
    {
        //query size of response message in bytes
        if (!WinHttpQueryDataAvailable(info->http_handle, &ret_size))
        {
            ret = MP_NETWORK_ERROR;
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail in query HTTP response Data, last error: %d\n", GetLastError());
            break;
        }

        if (ret_size == 0) {
            TCHAR header_line_str[MAX_HEADER_LINE_SIZE + 1] = { 0 };
            char header_line_str_char[MAX_HEADER_LINE_SIZE + 1] = { 0 };
            DWORD dwSize = sizeof(header_line_str);
            // query for server request ID
            win_http_ret = WinHttpQueryHeaders(info->http_handle, WINHTTP_QUERY_CUSTOM,
                REQUEST_ID_STR_RESPONSE_HEADER, header_line_str, &dwSize, WINHTTP_NO_HEADER_INDEX);
            if (win_http_ret) {
                size_t count_coverted = 0;
                errno_t err_covert = wcstombs_s(&count_coverted, header_line_str_char, (const wchar_t *)header_line_str, sizeof(header_line_str_char));
                if (0 == err_covert && wcslen((const wchar_t *)header_line_str) + 1 == count_coverted) {
                    network_log_message(MP_REG_LOG_LEVEL_INFO, "Request ID: %s\n", header_line_str_char);
                } else {
                    network_log_message(MP_REG_LOG_LEVEL_ERROR, "Error during equest-ID convertion: %d, last error: %d\n", err_covert, GetLastError());
                    break;
                }
            } else {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "Couldn't find Request-ID, last error: %d\n", GetLastError());
                break;
            }

            // query for Error-Code, if existing
            memset(header_line_str, 0, sizeof(header_line_str));
            dwSize = sizeof(header_line_str);
            win_http_ret = WinHttpQueryHeaders(info->http_handle, WINHTTP_QUERY_CUSTOM,
                ERROR_CODE_STR_RESPONSE_HEADER, header_line_str, &dwSize, WINHTTP_NO_HEADER_INDEX);
            if (win_http_ret) {
                memset(header_line_str_char, 0, sizeof(header_line_str_char));
                size_t count_coverted = 0;
                errno_t err_covert = wcstombs_s(&count_coverted, header_line_str_char, (const wchar_t *)header_line_str, sizeof(header_line_str_char));
                if (0 == err_covert && wcslen((const wchar_t *)header_line_str) + 1 == count_coverted) {
                    network_log_message(MP_REG_LOG_LEVEL_ERROR, "Found response Error-Code: %s\n", header_line_str_char);
                    errorCodeStr = string(header_line_str_char);
                } else {
                    network_log_message(MP_REG_LOG_LEVEL_ERROR, "Error during Error-Code convertion: %d, last error: %d\n", err_covert, GetLastError());
                    break;
                }
            }

            // query for Error-Message, if existing
            memset(header_line_str, 0, sizeof(header_line_str));
            dwSize = sizeof(header_line_str);
            win_http_ret = WinHttpQueryHeaders(info->http_handle, WINHTTP_QUERY_CUSTOM,
                ERROR_MSG_STR_RESPONSE_HEADER, header_line_str, &dwSize, WINHTTP_NO_HEADER_INDEX);
            if (win_http_ret) {
                memset(header_line_str_char, 0, sizeof(header_line_str_char));
                size_t count_coverted = 0;
                errno_t err_covert = wcstombs_s(&count_coverted, header_line_str_char, (const wchar_t *)header_line_str, sizeof(header_line_str_char));
                if (0 == err_covert && wcslen((const wchar_t *)header_line_str) + 1 == count_coverted) {
                    network_log_message(MP_REG_LOG_LEVEL_ERROR, "Found response Error-Message: %s\n", header_line_str_char);
                }
                else {
                    network_log_message(MP_REG_LOG_LEVEL_ERROR, "Error during Error-Message convertion: %d, last error: %d\n", err_covert, GetLastError());
                    break;
                }
            }

            break;
        } else {
            assert(old_size + ret_size < MAX_RESPONSE_SIZE);
            memset(response_ptr + old_size, 0, ret_size);
            DWORD download_size = 0;
            //get response message from server
            if (!WinHttpReadData(info->http_handle, response_ptr + old_size, ret_size, &download_size)) {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "Fail to read HTTP response data, last error: %d\n", GetLastError());
                break;
            }
            else {
                old_size += (uint16_t)download_size;
            }
        }
    };

    if ((0 < old_size) && (resp_msg)) {
        if (old_size > resp_size) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Reponse buffer too small for pending response, required size: %d\n", old_size);
            ret = MP_USER_INSUFFICIENT_MEM;
        } else {
            memcpy(resp_msg, response, old_size);
        }
    }

    // set response size
    resp_size = old_size;

    return ret;
}

MpResult MPSynchronicSender::sendBinaryRequest(const string& serverURL, const string& subscriptionKey, const uint8_t *request, const uint16_t requestSize,
    uint8_t *response, uint16_t &responseSize, HttpStatusCode& http_response_code, string& errorCodeStr) {
    http_network_info_t info;
    memset(&info, 0, sizeof(info));

    info.log_level = m_logLevel;

    MpResult ret = http_network_init(&info, serverURL.c_str(), m_proxy);
    if (ret != MP_SUCCESS) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "http network init failed: %d\n", ret);
        goto out;
    }

    ret = http_network_send_data(&info, subscriptionKey, request, requestSize,
        response, responseSize, http_response_code, errorCodeStr);
    if (ret != MP_SUCCESS) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "http send data failed: %d\n", ret);
        goto out;
    }
out:
    http_network_fini(&info);
    return ret;
}

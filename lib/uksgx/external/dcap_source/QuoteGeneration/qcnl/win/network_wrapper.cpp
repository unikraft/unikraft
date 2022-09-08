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
 * File: network_wrapper.cpp 
 *  
 * Description: Network access logic
 *
 */
#include "sgx_default_qcnl_wrapper.h"
#include "network_wrapper.h"
#include "qcnl_config.h"
#include <winhttp.h>
#include <atlstr.h>

#define DEFAULT_CONNECT_TIME_OUT_VALUE (5 * 1000)
#define HTTP_DOWNLOAD_MAX_SIZE (100 * 1024 * 1024) //http response msg should be not more than 100M
#define MAX_URL 2083

extern bool g_isWin81OrLater;

static sgx_qcnl_error_t windows_last_error_to_qcnl_error(void) {
    DWORD ec = GetLastError();
    switch (ec) {
    case ERROR_WINHTTP_CONNECTION_ERROR:
    case ERROR_WINHTTP_CANNOT_CONNECT:
    case ERROR_WINHTTP_SECURE_FAILURE:
        return SGX_QCNL_NETWORK_COULDNT_CONNECT;
    case ERROR_WINHTTP_TIMEOUT:
        return SGX_QCNL_NETWORK_OPERATION_TIMEDOUT; //All network related error
    case ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR:
    case ERROR_WINHTTP_AUTODETECTION_FAILED:
    case ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT:
        return SGX_QCNL_NETWORK_PROXY_FAIL;
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
    case ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY:
    case ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY:
    case ERROR_WINHTTP_HEADER_ALREADY_EXISTS:
    case ERROR_WINHTTP_LOGIN_FAILURE:
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        return SGX_QCNL_NETWORK_ERROR;
    case ERROR_WINHTTP_INVALID_URL:
        return SGX_QCNL_INVALID_PARAMETER;
    default:
        return SGX_QCNL_UNEXPECTED_ERROR;
    }
}

/**
* This method converts PCCS HTTP status codes to QCNL error codes
*
* @param pccs_status_code PCCS HTTP status codes
*
* @return Collateral Network Library Error Codes
*/
static sgx_qcnl_error_t pccs_status_to_qcnl_error(DWORD pccs_status_code) {
    switch (pccs_status_code) {
    case 200: // PCCS_STATUS_SUCCESS
        return SGX_QCNL_SUCCESS;
    case 404: // PCCS_STATUS_NO_CACHE_DATA
        return SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA;
    case 461: // PCCS_STATUS_PLATFORM_UNKNOWN
        return SGX_QCNL_ERROR_STATUS_PLATFORM_UNKNOWN;
    case 462: // PCCS_STATUS_CERTS_UNAVAILABLE
        return SGX_QCNL_ERROR_STATUS_CERTS_UNAVAILABLE;
    case 503: // PCCS_STATUS_SERVICE_UNAVAILABLE;
        return SGX_QCNL_ERROR_STATUS_SERVICE_UNAVAILABLE;
    default:
        return SGX_QCNL_ERROR_STATUS_UNEXPECTED;
    }
}

/**
* This method calls curl library to perform https POST request with raw body in JSON format and returns response body and header
*
* @param url HTTPS GET/POST URL
* @param req_body Request body in raw JSON format
* @param req_body_size Size of request body
* @param user_token user token to access PCCS v3/platforms API
* @param user_token_size Size of user token
* @param resp_msg Output buffer of response body
* @param resp_size Size of response body
* @param resp_header Output buffer of response header
* @param header_size Size of response header
*
* @return SGX_QCNL_SUCCESS Call https post successfully. Other return codes indicate an error occured.
*/
sgx_qcnl_error_t qcnl_https_request_once(const char *url,
                                         http_header_map& header_map,
                                         const char *req_body,
                                         uint32_t req_body_size,
                                         const uint8_t *user_token,
                                         uint16_t user_token_size,
                                         char **resp_msg,
                                         uint32_t &resp_size,
                                         char **resp_header,
                                         uint32_t &header_size) {
    sgx_qcnl_error_t ret = SGX_QCNL_UNEXPECTED_ERROR;
    HINTERNET hSession = NULL,
              hConnect = NULL,
              hRequest = NULL;

    do {
        //WinHTTP API explicitly use UNICODE so that we should use WCHAR instead of TCHAR
        WCHAR wurl[MAX_URL];
        WCHAR whostname[MAX_URL];
        size_t count;
        count = 0;
        if (mbstowcs_s(&count, wurl, url, strnlen_s(url, MAX_URL)) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        URL_COMPONENTS urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = whostname;    //we will only crack hostname, urlpath
        urlComp.dwHostNameLength = MAX_PATH; //copy hostname to a buffer to get 0-terminated string
        urlComp.dwUrlPathLength = (DWORD)-1;

        // Crack the URL
        if (!WinHttpCrackUrl(wurl, (DWORD)wcsnlen_s(wurl, MAX_URL), 0, &urlComp)) {
            ret = SGX_QCNL_INVALID_PARAMETER;
            break;
        }

        DWORD dwAutoProxy = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
        if (!g_isWin81OrLater)
            dwAutoProxy = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        // Use WinHttpOpen to obtain a session handle.
        hSession = WinHttpOpen(L"SGX default qcnl",
                               dwAutoProxy,
                               WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        // Specify an HTTP server.
        hConnect = WinHttpConnect(hSession, urlComp.lpszHostName,
                                  static_cast<INTERNET_PORT>(urlComp.nPort), 0);
        if (!hConnect) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        LPCWSTR pwszVerb = L"GET";
        if (req_body && req_body_size > 0) {
            pwszVerb = L"POST";
        }
        // Create an HTTP request handle.
        hRequest = WinHttpOpenRequest(hConnect, pwszVerb, urlComp.lpszUrlPath,
                                      L"HTTP/1.0", WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        //set default connection timeout value
        DWORD dwTimeout = DEFAULT_CONNECT_TIME_OUT_VALUE;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(dwTimeout))) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        if (!QcnlConfig::Instance()->is_server_secure()) {
            DWORD dwFlags =
                SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
            if (!WinHttpSetOption(
                    hRequest,
                    WINHTTP_OPTION_SECURITY_FLAGS,
                    &dwFlags,
                    sizeof(dwFlags))) {
                ret = windows_last_error_to_qcnl_error();
                break;
            }
        }

        // Enable the certificate revocation check
        DWORD dwEnableSSLRevocOpt = WINHTTP_ENABLE_SSL_REVOCATION;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_ENABLE_FEATURE, &dwEnableSSLRevocOpt, sizeof(dwEnableSSLRevocOpt))) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        // add custom headers
        http_header_map::iterator it = header_map.begin();
        while (it != header_map.end()) {
            string key = it->first;
            string value = it->second;
            string headerline = key + ": " + value;
            CA2W w_headerline(headerline.c_str(), CP_UTF8);
            if (!WinHttpAddRequestHeaders(hRequest, w_headerline, (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
                ret = windows_last_error_to_qcnl_error();
                goto cleanup;
            }
            it++;
        }

        if (req_body && req_body_size > 0) {
            // Set Content-Type in header
            if (!WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
                ret = windows_last_error_to_qcnl_error();
                break;
            }

            std::string user_token_header("user-token: ");
            user_token_header.append(reinterpret_cast<const char *>(user_token), user_token_size);
            CA2W w_user_token_header(user_token_header.c_str());
            if (!WinHttpAddRequestHeaders(hRequest, w_user_token_header, (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
                ret = windows_last_error_to_qcnl_error();
                break;
            }
        }

        // Send a request.
        if (!WinHttpSendRequest(hRequest,
                                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0,
                                (DWORD)req_body_size, 0)) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        if (req_body && req_body_size > 0) {
            //send request body
            DWORD write_bytes = 0;
            if (!WinHttpWriteData(hRequest, req_body, (DWORD)req_body_size, &write_bytes)) {
                ret = windows_last_error_to_qcnl_error();
                break;
            }
        }

        // End the request.
        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        LPVOID lpOutBuffer = NULL;
        DWORD dwStatus = 0;
        DWORD dwSize = sizeof(dwStatus);
        // Query response code
        if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 NULL, &dwStatus, &dwSize, 0)) //query for server response error code
        {
            ret = windows_last_error_to_qcnl_error();
            break;
        }

        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] HTTP status code: %ld \n", dwStatus);

        if (dwStatus == HTTP_STATUS_OK) // 200
        {
            // Get response header
            (void)WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                      WINHTTP_HEADER_NAME_BY_INDEX, NULL,
                                      &dwSize, WINHTTP_NO_HEADER_INDEX);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                ret = SGX_QCNL_UNEXPECTED_ERROR;
                break;
            }

            // Allocate memory for the buffer.
            lpOutBuffer = new WCHAR[dwSize / sizeof(WCHAR)];

            // Now, use WinHttpQueryHeaders to retrieve the header.
            if (!WinHttpQueryHeaders(hRequest,
                                     WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                     WINHTTP_HEADER_NAME_BY_INDEX,
                                     lpOutBuffer, &dwSize,
                                     WINHTTP_NO_HEADER_INDEX)) {
                delete[] lpOutBuffer;
                ret = windows_last_error_to_qcnl_error();
                break;
            }
            header_size = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)lpOutBuffer, -1, NULL, 0, NULL, NULL);
            if (header_size == 0) {
                delete[] lpOutBuffer;
                ret = windows_last_error_to_qcnl_error();
                break;
            }
            *resp_header = static_cast<char *>(malloc((size_t)header_size + 1));
            if (NULL == *resp_header) {
                delete[] lpOutBuffer;
                ret = SGX_QCNL_OUT_OF_MEMORY;
                break;
            }
            if (WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)lpOutBuffer, -1, *resp_header, header_size, NULL, NULL) == 0) {
                delete[] lpOutBuffer;
                ret = windows_last_error_to_qcnl_error();
                break;
            }
            (*resp_header)[header_size] = 0;
            header_size++;
            delete[] lpOutBuffer;
        } else if (dwStatus == HTTP_STATUS_NOT_FOUND) // 404
        {
            ret = SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA;
            break;
        } else {
            ret = pccs_status_to_qcnl_error(dwStatus);
            break;
        }

        resp_size = 0;
        // Keep checking for data until there is nothing left.
        do {
            // Check for available data.
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                ret = windows_last_error_to_qcnl_error();
                break;
            }
            if (dwSize == 0) {
                ret = SGX_QCNL_SUCCESS;
                break;
            }

            if (*resp_msg == NULL) {
                *resp_msg = static_cast<char *>(malloc(dwSize));
                if (*resp_msg == NULL) {
                    ret = SGX_QCNL_OUT_OF_MEMORY;
                    break;
                }
            } else {
                if (UINT32_MAX - resp_size < dwSize || resp_size + dwSize > HTTP_DOWNLOAD_MAX_SIZE) {
                    free(*resp_msg);
                    *resp_msg = NULL;
                    ret = SGX_QCNL_OUT_OF_MEMORY;
                    break;
                }
                char *p_buffer_expanded = static_cast<char *>(malloc((size_t)resp_size + dwSize));
                if (p_buffer_expanded == NULL) {
                    ret = SGX_QCNL_OUT_OF_MEMORY;
                    break;
                }

                (void)memcpy_s(p_buffer_expanded, (size_t)resp_size + dwSize, *resp_msg, resp_size);
                free(*resp_msg);
                *resp_msg = p_buffer_expanded;
            }

            memset(*resp_msg + resp_size, 0, dwSize);
            DWORD download_size = 0;
            //get response message from server
            if (!WinHttpReadData(hRequest, *resp_msg + resp_size, dwSize, &download_size)) {
                ret = windows_last_error_to_qcnl_error();
                free(*resp_msg);
                *resp_msg = NULL;
                break;
            } else {
                resp_size += download_size;
            }
        } while (TRUE);
    } while (0);

cleanup:
    // Close any open handles.
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);

    // free allocated buffers in case this function retuns error
    if (ret != SGX_QCNL_SUCCESS) {
        if (*resp_msg) {
            free(*resp_msg);
            *resp_msg = NULL;
            resp_size = 0;
        }
        if (*resp_header) {
            free(*resp_header);
            *resp_header = NULL;
            header_size = 0;
        }
    }

    return ret;
}

sgx_qcnl_error_t qcnl_https_request(const char *url,
                                    http_header_map& header_map,
                                    const char *req_body,
                                    uint32_t req_body_size,
                                    const uint8_t *user_token,
                                    uint16_t user_token_size,
                                    char **resp_msg,
                                    uint32_t &resp_size,
                                    char **resp_header,
                                    uint32_t &header_size) {
    sgx_qcnl_error_t ret = SGX_QCNL_UNEXPECTED_ERROR;
    uint32_t retry_times = QcnlConfig::Instance()->getRetryTimes() + 1;
    uint32_t retry_delay = QcnlConfig::Instance()->getRetryDelay();
    uint32_t current_delay_time = 1; // wait 1 second before first retry

    while (retry_times > 0) {
        ret = qcnl_https_request_once(url, header_map, req_body, req_body_size, user_token, user_token_size,
                                      resp_msg, resp_size, resp_header, header_size);

        if ((ret >= SGX_QCNL_NETWORK_ERROR && ret <= SGX_QCNL_NETWORK_HTTPS_ERROR) ||
            ret == SGX_QCNL_ERROR_STATUS_SERVICE_UNAVAILABLE) {
            retry_times--;
            if (retry_times > 0) {
                if (retry_delay != 0)
                    Sleep(retry_delay * 1000);
                else {
                    Sleep(current_delay_time * 1000);
                    current_delay_time *= 2;
                }
            }
        } else {
            break;
        }
    }

    return ret;
}
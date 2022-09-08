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
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <algorithm>
#include <winhttp.h>
#include <tchar.h>
#include <assert.h>
#include "network_wrapper.h"
#include "utility.h"

using namespace std;

#define DEFAUT_CONNECT_TIME_OUT_VAUE  (5*1000)
#define MAX_URL_LENGTH  2083     
#define MAX_PATH_SIZE   256  
#define MAX_HEADER_SIZE 1024
#define LOCAL_NETWORK_SETTING "./network_setting.conf"

static bool g_use_secure_cert = true;
typedef enum _ProxyType
{
    PROXY_TYPE_DEFAULT_PROXY   = 0,
    PROXY_TYPE_AUTOMATIC      = 1,
    PROXY_TYPE_DIRECT_ACCESS  = 2,
    PROXY_TYPE_MANUAL_PROXY   = 3,
} ProxyType;

extern string server_url_string;
extern string proxy_type_string;
extern string proxy_url_string ;
extern string user_token_string ;
extern string use_secure_cert_string ;

static network_post_error_t windows_last_error_to_network_post_error(void)
{
    DWORD ec = GetLastError();
    switch (ec) {
    case ERROR_WINHTTP_CONNECTION_ERROR:
    case ERROR_WINHTTP_CANNOT_CONNECT:
        return POST_NETWORK_ERROR;
    case ERROR_WINHTTP_TIMEOUT:
        return POST_NETWORK_ERROR;//All network related error
    case ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR:
    case ERROR_WINHTTP_AUTODETECTION_FAILED:
    case ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT:
        return POST_NETWORK_ERROR;
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
    case ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY:
    case ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY:
    case ERROR_WINHTTP_HEADER_ALREADY_EXISTS:
    case ERROR_WINHTTP_LOGIN_FAILURE:
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        return POST_NETWORK_ERROR;
    case ERROR_WINHTTP_INVALID_URL:
        return POST_INVALID_PARAMETER_ERROR;
    default:
        return POST_UNEXPECTED_ERROR;
    }
}
                                                                                                  

static bool process_configuration_setting(const char *config_file_name, string& url, ProxyType &proxy_type, string &proxy_url, string& user_token)
{
    bool ret = true;
    ifstream ifs(config_file_name);
    string line;
    if (ifs.is_open()) {
        auto f = [](unsigned char const c) { return isspace(c); };
        while (getline(ifs, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), f), line.end());
            if (line[0] == '#' || line.empty())
                continue;
            size_t pos = line.find("=");
            string name = line.substr(0, pos);
            std::transform(name.begin(), name.end(), name.begin(), [](auto ch) {return static_cast<char>(::towupper(ch)); });
            string value = line.substr(pos + 1);
            if (name.compare("PCCS_URL") == 0) {
                if (server_url_string.empty() == true) {
                    url = value;
                }
                else {
                    url = server_url_string + "/sgx/certification/v4/platforms";
                }
            }
            else if (name.compare("USE_SECURE_CERT") == 0) {
                if (use_secure_cert_string.empty() == true) {
                    std::transform(value.begin(), value.end(), value.begin(), [](auto ch) {return static_cast<char>(::towupper(ch)); });
                    if (value.compare("FALSE") == 0) {
                        g_use_secure_cert = false;
                    }
                }
                else if (use_secure_cert_string.compare("FALSE") == 0 || use_secure_cert_string.compare("false") == 0) {
                    g_use_secure_cert = false;
                }
            }
            else if (name.compare("PROXY_TYPE") == 0) {
                if (proxy_type_string.empty() == true) {
                    std::transform(value.begin(), value.end(), value.begin(), [](auto ch) {return static_cast<char>(::towupper(ch)); });
                    if (value.compare("DIRECT") == 0) {
                        proxy_type = PROXY_TYPE_DIRECT_ACCESS;
                    }
                    else if (value.compare("MANUAL") == 0) {
                        proxy_type = PROXY_TYPE_MANUAL_PROXY;
                    }
                    else if (value.compare("AUTO") == 0) {
                        proxy_type = PROXY_TYPE_AUTOMATIC;
                    }
                    else {
                        proxy_type = PROXY_TYPE_DEFAULT_PROXY;
                    }
                }
                else {
                    if (proxy_type_string.compare("DIRECT") == 0 || proxy_type_string.compare("direct") == 0) {
                        proxy_type = PROXY_TYPE_DIRECT_ACCESS;
                    }
                    else if (proxy_type_string.compare("MANUAL") == 0 || proxy_type_string.compare("manual") == 0) {
                        proxy_type = PROXY_TYPE_MANUAL_PROXY;
                    }
                    else if (proxy_type_string.compare("AUTO") == 0 || proxy_type_string.compare("auto") == 0) {
                        proxy_type = PROXY_TYPE_DIRECT_ACCESS;
                    }
                    else  {
                        proxy_type = PROXY_TYPE_DEFAULT_PROXY;
                    }         
                }
            }
            else if (name.compare("PROXY_URL") == 0) {
                if (proxy_url_string.empty() == true) {
                    proxy_url = value;
                }
            } 
            else if (name.compare("USER_TOKEN") == 0) {
                if (user_token_string.empty() == true) {
                    user_token = value;
                }
            } 
            else {
                continue;
            }
        }
    }
    else {
        if (use_secure_cert_string.compare("FALSE") == 0 || use_secure_cert_string.compare("false") == 0) {
            g_use_secure_cert = true;
        }

        if (proxy_type_string.compare("DIRECT") == 0 || proxy_type_string.compare("direct") == 0) {
            proxy_type = PROXY_TYPE_DIRECT_ACCESS;
        }
        else if (proxy_type_string.compare("MANUAL") == 0 || proxy_type_string.compare("manual") == 0) {
            proxy_type = PROXY_TYPE_MANUAL_PROXY;
        }
        else if (proxy_type_string.compare("AUTO") == 0 || proxy_type_string.compare("auto") == 0) {
            proxy_type = PROXY_TYPE_DIRECT_ACCESS;
        }
        else {
            proxy_type = PROXY_TYPE_DEFAULT_PROXY;
        }
        url = server_url_string + "/sgx/certification/v2/platforms";
        ret = false;
    }
    return ret;
}

/**
*  Read configuration data
*/
static void network_configuration(string &url, ProxyType &proxy_type, string &proxy_url, string& user_token)
{
	//firstly read local configuration File
	process_configuration_setting(LOCAL_NETWORK_SETTING, url, proxy_type, proxy_url, user_token);	
}

/**
* Method converts byte containing value from 0x00-0x0F into its corresponding ASCII code,
* e.g. converts 0x00 to '0', 0x0A to 'A'.
* Note: This is mainly a helper method for internal use in byte_array_to_hex_string().
*
* @param in byte to be converted (allowed values: 0x00-0x0F)
*
* @return ASCII code representation of the byte or 0 if method failed (e.g input value was not in provided range).
*/
static uint8_t convert_value_to_ascii(uint8_t in)
{
	if (in <= 0x09)
	{
		return (uint8_t)(in + '0');
	}
	else if (in <= 0x0F)
	{
		return (uint8_t)(in - 10 + 'A');
	}

	return 0;
}

//Function to do HEX encoding of array of bytes
//@param in_buf, bytes array whose length is in_size
//       out_buf, output the HEX encoding of in_buf on success.
//@return true on success and false on error
//The out_size must always be 2*in_size since each byte into encoded by 2 characters
static bool byte_array_to_hex_string(const uint8_t *in_buf, uint32_t in_size, uint8_t *out_buf, uint32_t out_size)
{
	if (in_size>UINT32_MAX / 2)return false;
	if (in_buf == NULL || out_buf == NULL || out_size != in_size * 2)return false;

	for (uint32_t i = 0; i< in_size; i++)
	{
		*out_buf++ = convert_value_to_ascii(static_cast<uint8_t>(*in_buf >> 4));
		*out_buf++ = convert_value_to_ascii(static_cast<uint8_t>(*in_buf & 0xf));
		in_buf++;
	}
	return true;
}
                     

/**
* This function appends request parameters of byte array type to the UR in HEX string format
*
* @param url Request UR
* @param request  Request parameter in byte array
* @param request_size Size of byte array
*
* @return true If the byte array was appended to the UR successfully
*/
static network_post_error_t append_body_context(string& url, const uint8_t* request, const uint32_t request_size)
{
	if (request_size >= UINT32_MAX / 2)
		return POST_INVALID_PARAMETER_ERROR;

	uint8_t* hex = (uint8_t*)malloc(request_size * 2);
	if (!hex)
		return POST_OUT_OF_MEMORY_ERROR;
	if (!byte_array_to_hex_string(request, request_size, hex, request_size * 2)) {
		free(hex);
		return POST_UNEXPECTED_ERROR;
	}
	url.append(reinterpret_cast<const char*>(hex), request_size * 2);
	free(hex);
	return POST_SUCCESS;
}

static network_post_error_t generate_json_message_body(const uint8_t *raw_data, 
                                                       const uint32_t raw_data_size,
                                                       const uint16_t platform_id_length,
                                                       const bool non_enclave_mode, 
                                                       string &jsonString)
{
    network_post_error_t ret = POST_SUCCESS;
    const uint8_t *position = raw_data;

    jsonString = "{";
    if (true == non_enclave_mode) {
        jsonString += "\"pce_id\": \"";
        if ((ret = append_body_context(jsonString, position, PCE_ID_LENGTH)) != POST_SUCCESS) {
            return ret;
        }
        jsonString += "\" ,\"qe_id\": \"";
        position = position + PCE_ID_LENGTH;
        if ((ret = append_body_context(jsonString, position, platform_id_length)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"platform_manifest\": \"";
        position = position + platform_id_length;
        if ((ret = append_body_context(jsonString, position, raw_data_size - PCE_ID_LENGTH - platform_id_length)) != POST_SUCCESS) {
            return ret;
        }
    }
    else {
        uint32_t left_size = raw_data_size - platform_id_length - CPU_SVN_LENGTH - ISV_SVN_LENGTH - PCE_ID_LENGTH - ENCRYPTED_PPID_LENGTH;
        jsonString += "\"enc_ppid\": \"";
        if ((ret = append_body_context(jsonString, position, ENCRYPTED_PPID_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"pce_id\": \"";
        position = position + ENCRYPTED_PPID_LENGTH;
        if ((ret = append_body_context(jsonString, position, PCE_ID_LENGTH)) != POST_SUCCESS) {
            return ret;
        }
        jsonString += "\" ,\"cpu_svn\": \"";
        position = position + PCE_ID_LENGTH;
        if ((ret = append_body_context(jsonString, position, CPU_SVN_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"pce_svn\": \"";
        position = position + CPU_SVN_LENGTH;
        if ((ret = append_body_context(jsonString, position, ISV_SVN_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"qe_id\": \"";
        position = position + ISV_SVN_LENGTH;
        if ((ret = append_body_context(jsonString, position, platform_id_length)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"platform_manifest\": \"";
        if (left_size != 0) {
            position = position + platform_id_length;
            if ((ret = append_body_context(jsonString, position, left_size)) != POST_SUCCESS) {
                return ret;
            }
        }

    }
    jsonString += "\" }";
    return ret;
}

network_post_error_t network_https_post(const uint8_t* raw_data, const uint32_t raw_data_size, const uint16_t platform_id_length, const bool non_enclave_mode)
{
    if (raw_data_size < platform_id_length + static_cast<uint32_t>(PCE_ID_LENGTH )) {
        return POST_INVALID_PARAMETER_ERROR;
    }

	network_post_error_t ret = POST_UNEXPECTED_ERROR;  
    string strJson("");
    ret = generate_json_message_body(raw_data, raw_data_size, platform_id_length, non_enclave_mode, strJson);
    if (ret != POST_SUCCESS) {
        printf("Error: unexpected error happens during generate json message body.\n");
        return ret;
    } 
    
    ret = POST_UNEXPECTED_ERROR;

	// initialize https request url
	string url(server_url_string);
    ProxyType proxy_type = PROXY_TYPE_DEFAULT_PROXY;
    string proxy_url(proxy_url_string);
    string user_token(user_token_string);
	// initialize network configuration
	network_configuration(url, proxy_type, proxy_url, user_token);
    HINTERNET  hSession = NULL, hConnect = NULL, hRequest = NULL;

	//WinHTTP API explicitly use UNICODE so that we should use WCHAR instead of TCHAR
	WCHAR wurl[MAX_PATH];
	WCHAR wproxyurl[MAX_PATH];
	WCHAR whostname[MAX_PATH];
	size_t count = 0;
	if (mbstowcs_s(&count, wurl, url.c_str(), url.size()) != 0) {
		return POST_UNEXPECTED_ERROR;
	}

    do {
        //WinHTTP API explicitly use UNICODE so that we should use WCHAR instead of TCHAR
        URL_COMPONENTS urlCompccs;
        ZeroMemory(&urlCompccs, sizeof(urlCompccs));
		URL_COMPONENTS proxyurlCompccs;
		ZeroMemory(&urlCompccs, sizeof(proxyurlCompccs));
        urlCompccs.dwStructSize = sizeof(urlCompccs);
        urlCompccs.lpszHostName = whostname;//we will only crack hostname, urlpath 
        urlCompccs.dwHostNameLength = MAX_PATH;//copy hostname to a buffer to get 0-terminated string
        urlCompccs.dwUrlPathLength = (DWORD)-1;

        // Crack the URL
        if (!WinHttpCrackUrl(wurl, (DWORD)wcsnlen_s(wurl, MAX_URL_LENGTH), 0, &urlCompccs)) {
            ret = POST_INVALID_PARAMETER_ERROR;
            break;
        }

        DWORD dwProxyType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
		proxyurlCompccs.lpszUrlPath = NULL;
		switch (proxy_type) {
		case PROXY_TYPE_DIRECT_ACCESS:
			dwProxyType = WINHTTP_ACCESS_TYPE_NO_PROXY;
			break;
		case PROXY_TYPE_DEFAULT_PROXY:
			dwProxyType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
			break;
		case PROXY_TYPE_MANUAL_PROXY:
			dwProxyType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
			proxyurlCompccs.dwStructSize = sizeof(proxyurlCompccs);
			proxyurlCompccs.lpszHostName = whostname;//we will only crack hostname, urlpath 
			proxyurlCompccs.dwHostNameLength = MAX_PATH;//copy hostname to a buffer to get 0-terminated string
			proxyurlCompccs.dwUrlPathLength = (DWORD)-1;

			if (mbstowcs_s(&count, wproxyurl, proxy_url.c_str(), proxy_url.size()) != 0) {
				return POST_UNEXPECTED_ERROR;
			}
			// Crack the URL
			if (!WinHttpCrackUrl(wproxyurl, (DWORD)wcsnlen_s(wproxyurl, MAX_URL_LENGTH), 0, &proxyurlCompccs)) {
				return  POST_INVALID_PARAMETER_ERROR;
			}
			break;
		default:
			dwProxyType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
		}

        // Use WinHttpOpen to obtain a session handle.
        hSession = WinHttpOpen(L"SGX connect pccs",
            dwProxyType,
            proxyurlCompccs.lpszUrlPath,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            ret = windows_last_error_to_network_post_error();
            break;
        }

        // Specify an HTTP server.
        hConnect = WinHttpConnect(hSession, urlCompccs.lpszHostName,
            static_cast<INTERNET_PORT>(urlCompccs.nPort), 0);
        if (!hConnect) {
            ret = windows_last_error_to_network_post_error();
            break;
        }

        // Create an HTTP request handle.
        hRequest = WinHttpOpenRequest(hConnect, L"POST", urlCompccs.lpszUrlPath,
            L"HTTP/1.0", WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            ret = windows_last_error_to_network_post_error();
            break;
        }

        //set default connection timeout value
        DWORD value = DEFAUT_CONNECT_TIME_OUT_VAUE;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value))) {
            ret = windows_last_error_to_network_post_error();
            break;
        }

        if (!g_use_secure_cert) {
            DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA       | 
                            SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                            SECURITY_FLAG_IGNORE_CERT_CN_INVALID  | 
                            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
            if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags))) {
                ret = windows_last_error_to_network_post_error();
                break;
            }
        }


        // Enable the certificate revocation check
        DWORD dwEnableSSLRevocOpt = WINHTTP_ENABLE_SSL_REVOCATION;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_ENABLE_FEATURE, &dwEnableSSLRevocOpt, sizeof(dwEnableSSLRevocOpt))) {
            ret = windows_last_error_to_network_post_error();
            break;
        }

		WCHAR header[MAX_HEADER_SIZE];
		WCHAR *p = header;
		uint32_t left_size = MAX_HEADER_SIZE;
		DWORD ret_size = 0;
		BOOL win_http_ret = FALSE;

        // Set headers
        (void)wcscpy_s(p, left_size, L"Content-Type: application/json\r\n");
        p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
        left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);;

        if (user_token.empty()) {
            printf("\n Please input the pccs password, and use \"Enter key\" to end\n");
            string token = "user-token: ";
            char ch;
            do {
                ch = static_cast<char>(_getch());
                token = token + ch;
            } while (ch != '\r');

            token = token + "\r\n";
            wstring wtoken = wstring(token.begin(), token.end());
            (void)wcscpy_s(p, left_size, wtoken.c_str());
            p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
            left_size = left_size - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);
        }
        else {
            string token = "user-token: " + user_token + "\r\n";
            wstring wtoken = wstring(token.begin(), token.end());
            (void)wcscpy_s(p, left_size, wtoken.c_str());
            p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
            left_size = left_size - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);
        }
        (void)wcscpy_s(p, left_size, L"Connection: Keep-Alive\r\nContent-Length: ");
        p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
        left_size = left_size - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);

        (void)_itow_s((int)strJson.size(), p, left_size, 10);
        p = header + wcsnlen_s(header, MAX_HEADER_SIZE);
        left_size = MAX_HEADER_SIZE - (uint32_t)wcsnlen_s(header, MAX_HEADER_SIZE);

        assert(left_size >= 3);
        (void)wcscat_s(header, L"\r\n");

        //send request message
        if (!WinHttpSendRequest(hRequest, header, (DWORD)wcsnlen_s(header, MAX_HEADER_SIZE), WINHTTP_NO_REQUEST_DATA, 0, (DWORD)strJson.size(), 0)) {
            ret = POST_NETWORK_ERROR;
            break;
        }

        //send message body
        if (!WinHttpWriteData(hRequest, strJson.c_str(), (DWORD)strJson.size(), &ret_size)) {
            ret = POST_NETWORK_ERROR;
            printf("Fail to write HTTP Data, error code is: %d\n", GetLastError());
            break;
        }

        //wait for response
        win_http_ret = WinHttpReceiveResponse(hRequest, NULL);
        if (!win_http_ret) {
            ret = POST_NETWORK_ERROR;
            printf("Fail to receive HTTP Response, error code is: %d\n", GetLastError());
            break;
        }
        DWORD dwStatus = 0;
        DWORD dwSize = sizeof(dwStatus);
        win_http_ret = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, &dwStatus, &dwSize, 0);//query for server response error code
        if (!win_http_ret) {
            ret = POST_NETWORK_ERROR;
            printf("Fail to query http response header, error code is: %d\n", GetLastError());
            break;
        }
        if (dwStatus == 200) {
            ret = POST_SUCCESS;
        }
        else if (dwStatus == 401) {
            ret = POST_AUTHENTICATION_ERROR;
        }
        else {
            ret = POST_UNEXPECTED_ERROR;
        }    
    }
    while (0);

    // Close any open handles.
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return ret;
}


bool is_server_url_available() {
    ifstream ifs_local(LOCAL_NETWORK_SETTING);
    string line;
    if (ifs_local.is_open()) {
        auto f = [](unsigned char const c) { return isspace(c); };
        while (getline(ifs_local, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), f), line.end());
            if (line[0] == '#' || line.empty())
                continue;
            size_t pos = line.find("=");
            string name = line.substr(0, pos);
            std::transform(name.begin(), name.end(), name.begin(), [](auto ch) {return static_cast<char>(::towupper(ch)); });
            string value = line.substr(pos + 1);
            if (name.compare("PCCS_URL") == 0) {
                if (value.empty() == false) {
                    return true;
                }
            }
        }
    }
    return false;
}

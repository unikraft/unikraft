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
/** File: qcnl_util.cpp
 *  
 * Description: Utility functions for QCNL
 *
 */

#include "qcnl_util.h"
#include "qcnl_config.h"
#include <algorithm>

#ifdef _MSC_VER
#define sscanf sscanf_s
#endif

/**
* Method converts char containing ASCII code into its corresponding value,
* e.g. converts '0' to 0x00, 'A' to 0x0A.
*
* @param in char containing ASCII code (allowed values: '0-9', 'a-f', 'A-F')
* @param val output parameter containing converted value, if method succeeds.
*
* @return true if conversion succeeds, false otherwise
*/
bool convert_ascii_to_value(uint8_t in, uint8_t &val) {
    if (in >= '0' && in <= '9') {
        val = static_cast<uint8_t>(in - '0');
    } else if (in >= 'A' && in <= 'F') {
        val = static_cast<uint8_t>(in - 'A' + 10);
    } else if (in >= 'a' && in <= 'f') {
        val = static_cast<uint8_t>(in - 'a' + 10);
    } else {
        return false;
    }

    return true;
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
uint8_t convert_value_to_ascii(uint8_t in) {
    if (in <= 0x09) {
        return (uint8_t)(in + '0');
    } else if (in <= 0x0F) {
        return (uint8_t)(in - 10 + 'A');
    }

    return 0;
}

//Function to do HEX decoding
//@param in_buf, character strings which are HEX encoding of a byte array
//       out_buf, output the decode byte array on success
//@return true on success and false on error
//The in_size must be even number and equals 2*out_size
bool hex_string_to_byte_array(const uint8_t *in_buf, uint32_t in_size, uint8_t *out_buf, uint32_t out_size) {
    if (out_size > UINT32_MAX / 2)
        return false;
    if (in_buf == NULL || out_buf == NULL || out_size * 2 != in_size)
        return false;

    for (uint32_t i = 0; i < out_size; i++) {
        uint8_t value_first, value_second;
        if (!convert_ascii_to_value(in_buf[i * 2], value_first))
            return false;
        if (!convert_ascii_to_value(in_buf[i * 2 + 1], value_second))
            return false;
        out_buf[i] = static_cast<uint8_t>(value_second + (value_first << 4));
    }
    return true;
}

//Function to do HEX encoding of array of bytes
//@param in_buf, bytes array whose length is in_size
//       out_buf, output the HEX encoding of in_buf on success.
//@return true on success and false on error
//The out_size must always be 2*in_size since each byte into encoded by 2 characters
bool byte_array_to_hex_string(const uint8_t *in_buf, uint32_t in_size, uint8_t *out_buf, uint32_t out_size) {
    if (in_size > UINT32_MAX / 2)
        return false;
    if (in_buf == NULL || out_buf == NULL || out_size != in_size * 2)
        return false;

    for (uint32_t i = 0; i < in_size; i++) {
        *out_buf++ = convert_value_to_ascii(static_cast<uint8_t>(*in_buf >> 4));
        *out_buf++ = convert_value_to_ascii(static_cast<uint8_t>(*in_buf & 0xf));
        in_buf++;
    }
    return true;
}

// This function is used to unescpae URL Codes, for example, %20 to SPACE character(0x20)
string unescape(string &src) {
    if (src.length() < 3) {
        return src;
    }

    string dst;
    char ch;
    int i, value;
    for (i = 0; i < (int)(src.length() - 2); i++) {
        if (int(src[i]) == '%') {
            sscanf(src.substr(i + 1, 2).c_str(), "%x", &value);
            ch = static_cast<char>(value);
            dst += ch;
            i += 2;
        } else {
            dst += src[i];
        }
    }
    return dst;
}

/**
* This function appends request parameters of byte array type to the URL in HEX string format
*
* @param url Request URL
* @param ba  Request parameter in byte array
* @param ba_size Size of byte array
*
* @return true If the byte array was appended to the URL successfully 
*/
bool concat_string_with_hex_buf(string &url, const uint8_t *ba, const uint32_t ba_size) {
    if (ba_size >= UINT32_MAX / 2)
        return false;

    uint8_t *hex = (uint8_t *)malloc(ba_size * 2);
    if (!hex)
        return false;
    if (!byte_array_to_hex_string(ba, ba_size, hex, ba_size * 2)) {
        free(hex);
        return false;
    }
    url.append(reinterpret_cast<const char *>(hex), ba_size * 2);
    free(hex);
    return true;
}

/**
* This function appends appends request parameters of byte array type to the JSON request body in HEX string format
*
* @param req_body   Request body in JSON string format
* @param para_name  The name of the Request parameter as JSON key
* @param para       The Request parameter in byte array which will be converted into HEX string as JSON value
* @param para_size  Size of para in byte array
*
* @return true If the byte array was appended to the Request body successfully 
*/
bool req_body_append_para(string &req_body, const string &para_name, const uint8_t *para, const uint32_t para_size) {
    if (para_size >= UINT32_MAX / 2)
        return false;

    uint8_t *hex = (uint8_t *)malloc(para_size * 2);
    if (!hex)
        return false;
    if (!byte_array_to_hex_string(para, para_size, hex, para_size * 2)) {
        free(hex);
        return false;
    }
    string temp(req_body.substr(1, req_body.size() - 2));
    temp.append(para_name + ":\"");
    temp.append(reinterpret_cast<const char *>(hex), para_size * 2);
    free(hex);
    req_body = "{" + temp + "\"}";

    return true;
}

// Convert http header string to a <field,value> map
// HTTP 1.1 header specification
//       message-header = field-name ":" [ field-value ]
//       field-name     = token
//       field-value    = *( field-content | LWS )
//       field-content  = <the OCTETs making up the field-value
//                        and consisting of either *TEXT or combinations
//                        of token, separators, and quoted-string>
//@param resp_header, http header string
//       header_map, a <string,string> map that stores fields and values
void http_header_to_map(const char *resp_header, uint32_t header_size, unordered_map<string, string> &header_map) {
    size_t length = header_size;
    size_t start = 0, end = 0;

    while (start < length) {
        while (end < length && resp_header[end] != '\r' && resp_header[end] != '\n') {
            end++;
        }
        if (end == start) {
            start++;
            end++;
        } else {
            // parse one line
            string str((unsigned char *)resp_header + start, (unsigned char *)resp_header + end);
            size_t pos = str.find(": ");
            if (pos != string::npos) {
                // HTTP headers are case-insensitive. Convert to lower case
                // for convenience.
                string header_lc = str.substr(0, pos);
                transform(header_lc.begin(), header_lc.end(), header_lc.begin(),
                          [](unsigned char c) { return (unsigned char)::tolower(c); });
                header_map.insert(pair<string, string>(header_lc, str.substr(pos + 2)));
            }
            start = end;
        }
    }
}

/**
* Method to check the collateral service is PCCS or PCS
*
* @return true if the URL contains trustedservices.intel.com, otherwise false.
*/
bool is_collateral_service_pcs() {
    if (QcnlConfig::Instance()->getCollateralServiceUrl().find("trustedservices.intel.com") != string::npos)
        return true;
    else
        return false;
}

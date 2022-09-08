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
/** File: network_wrapper.h 
 *  
 * Description: Definitions of network access interfaces
 *
 */

#ifndef _NETWORK_WRAPPER_H_
#define _NETWORK_WRAPPER_H_


typedef enum  _cache_server_delivery_status_t {
    DELIVERY_SUCCESS = 0,
    DELIVERY_FAIL,
    DELIVERY_ERROR_MAX = 0xff,
} cache_server_delivery_status_t;

typedef enum  _network_post_error_t {
    POST_SUCCESS = 0,
    POST_UNEXPECTED_ERROR,
    POST_INVALID_PARAMETER_ERROR,
    POST_OUT_OF_MEMORY_ERROR,
    POST_AUTHENTICATION_ERROR,
    POST_NETWORK_ERROR
} network_post_error_t;

network_post_error_t network_https_post(const uint8_t* raw_data, const uint32_t raw_data_size, const uint16_t platform_id_length, const bool non_enclave_mode);

bool is_server_url_available();

#endif /* !_NETWORK_WRAPPER_H_ */


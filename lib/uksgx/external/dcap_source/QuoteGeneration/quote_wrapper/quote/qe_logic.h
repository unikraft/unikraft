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
 * File: qe_logic.h 
 *  
 * Description: Definitions and class 
 * defintion for reference ECDSA quoting 
 * interface. 
 *
 */
#ifndef _QE3_LOGIC_H_
#define _QE3_LOGIC_H_

#include "sgx_urts.h"
#include "sgx_quote_3.h"
#include "sgx_ql_quote.h"
#include <time.h>
#include <string.h>

typedef quote3_error_t (*sgx_get_quote_config_func_t)(const sgx_ql_pck_cert_id_t *p_pck_cert_id, 
                                                      sgx_ql_config_t **pp_quote_config);

typedef quote3_error_t (*sgx_free_quote_config_func_t)(sgx_ql_config_t *p_quote_config);

typedef quote3_error_t (*sgx_write_persistent_data_func_t)(const uint8_t *p_buf,
                                                           uint32_t buf_size,  
                                                           const char *p_label);

typedef quote3_error_t (*sgx_read_persistent_data_func_t)(const uint8_t *p_buf,
                                                          uint32_t *p_buf_size,  
                                                          const char *p_label);

typedef quote3_error_t (*sgx_ql_set_logging_callback_t)(sgx_ql_logging_callback_t logger);

#endif


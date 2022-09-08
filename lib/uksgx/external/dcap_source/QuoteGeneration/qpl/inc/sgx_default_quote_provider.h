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
 * File: sgx_dcap_qpl.h 
 *  
 * Description: Definitions and prototypes for the Quote Provider Library
 *
 */

#ifndef _SGX_DCAP_QPL_H_
#define _SGX_DCAP_QPL_H_

#include "sgx_ql_lib_common.h"
#include <cstddef>

#if defined(__cplusplus)
extern "C" {
#endif

quote3_error_t sgx_ql_get_quote_config(const sgx_ql_pck_cert_id_t *p_pck_cert_id, sgx_ql_config_t **pp_quote_config);
quote3_error_t sgx_ql_free_quote_config(sgx_ql_config_t *p_quote_config);
quote3_error_t sgx_ql_get_quote_verification_collateral(const uint8_t *fmspc,
                                                        uint16_t fmspc_size,
                                                        const char *pck_ca,
                                                        sgx_ql_qve_collateral_t **pp_quote_collateral);
quote3_error_t sgx_ql_get_quote_verification_collateral_with_params(const uint8_t *fmspc,
                                                                    const uint16_t fmspc_size,
                                                                    const char *pck_ca,
                                                                    const void* custom_param,
                                                                    const uint16_t custom_param_length,
                                                                    sgx_ql_qve_collateral_t **pp_quote_collateral);
quote3_error_t sgx_ql_free_quote_verification_collateral(sgx_ql_qve_collateral_t *p_quote_collateral);
quote3_error_t tdx_ql_get_quote_verification_collateral(const uint8_t *fmspc, 
                                                        uint16_t fmspc_size, 
                                                        const char *pck_ca,
                                                        tdx_ql_qve_collateral_t **pp_quote_collateral);
quote3_error_t tdx_ql_free_quote_verification_collateral(tdx_ql_qve_collateral_t *p_quote_collateral);
quote3_error_t sgx_ql_get_qve_identity(char **pp_qve_identity,
                                       uint32_t *p_qve_identity_size,
                                       char **pp_qve_identity_issuer_chain,
                                       uint32_t *p_qve_identity_issuer_chain_size);
quote3_error_t sgx_ql_free_qve_identity(char *p_qve_identity, char *p_qve_identity_issuer_chain);
quote3_error_t sgx_ql_get_root_ca_crl(uint8_t **pp_root_ca_crl, uint16_t *p_root_ca_crl_size);
quote3_error_t sgx_ql_free_root_ca_crl(uint8_t *p_root_ca_crl);
quote3_error_t sgx_ql_set_logging_callback(sgx_ql_logging_callback_t logger);

#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_DCAP_QPL_H_*/

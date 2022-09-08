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
/** File: qv_collateral_provider.h 
 *  
 * Description: Header file of QvCollateralProvider class
 *
 */
#ifndef QVCOLLATERALPROVIDER_H_
#define QVCOLLATERALPROVIDER_H_
#pragma once

#include "sgx_default_qcnl_wrapper.h"
#include <string>
#include <vector>

using namespace std;

class QvCollateralProvider {
private:
    string custom_param_;

    string get_custom_param_string();
    sgx_qcnl_error_t build_pck_crl_url(const char *ca,
                                       string &url);
    sgx_qcnl_error_t build_tcbinfo_url(sgx_prod_type_t prod_type,
                                       const char *fmspc,
                                       uint16_t fmspc_size,
                                       string &url);
    sgx_qcnl_error_t build_qeidentity_url(sgx_qe_type_t qe_type, string &url);
    sgx_qcnl_error_t build_qveidentity_url(string &url);
    sgx_qcnl_error_t build_root_ca_crl_url(const char *root_ca_cdp_url, string &url);
public:
    QvCollateralProvider();
    QvCollateralProvider(const char* custom_param);
    ~QvCollateralProvider();

    sgx_qcnl_error_t get_pck_crl_chain(const char *ca,
                                       uint16_t ca_size,
                                       uint8_t **p_crl_chain,
                                       uint16_t *p_crl_chain_size);
    sgx_qcnl_error_t get_tcbinfo(sgx_prod_type_t prod_type,
                                 const char *fmspc,
                                 uint16_t fmspc_size,
                                 uint8_t **p_tcbinfo,
                                 uint16_t *p_tcbinfo_size);
    sgx_qcnl_error_t get_qe_identity(sgx_qe_type_t qe_type,
                                     uint8_t **p_qe_identity,
                                     uint16_t *p_qe_identity_size);
    sgx_qcnl_error_t get_qve_identity(char **pp_qve_identity,
                                      uint32_t *p_qve_identity_size,
                                      char **pp_qve_identity_issuer_chain,
                                      uint32_t *p_qve_identity_issuer_chain_size);
    sgx_qcnl_error_t get_root_ca_crl(const char *root_ca_cdp_url,
                                     uint8_t **p_root_ca_crl,
                                     uint16_t *p_root_ca_crl_size);
};
#endif

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
#ifndef _UAE_OAL_H
#define _UAE_OAL_H

#include "sgx_quote.h"
#include "sgx_error.h"
#include "sgx_urts.h"
#include <arch.h>
#include <aesm_error.h>

typedef enum{
    UAE_OAL_SUCCESS                 =   0,
    UAE_OAL_ERROR_UNEXPECTED            ,
    UAE_OAL_ERROR_AESM_UNAVAILABLE      ,
    UAE_OAL_ERROR_TIMEOUT               ,
    UAE_OAL_ERROR_INVALID               ,
} uae_oal_status_t;

/*OAL methods from here forward */

extern "C"
{

uae_oal_status_t SGXAPI oal_get_launch_token(
    const enclave_css_t*        signature,
    const sgx_attributes_t*     attribute,
    sgx_launch_token_t*         launch_token,
    uint32_t                    timeout_usec,
    aesm_error_t                *result);


uae_oal_status_t SGXAPI oal_init_quote(
    sgx_target_info_t       *p_target_info,
    sgx_epid_group_id_t     *p_gid,
    uint32_t                timeout_usec,
    aesm_error_t            *result);


uae_oal_status_t SGXAPI oal_get_quote(
    const sgx_report_t      *p_report,
    sgx_quote_sign_type_t   quote_type,
    const sgx_spid_t        *p_spid,
    const sgx_quote_nonce_t *p_nonce,
    const uint8_t           *p_sig_rl,
    uint32_t                sig_rl_size,
    sgx_report_t            *p_qe_report,
    sgx_quote_t             *p_quote,
    uint32_t                quote_size,
    uint32_t                timeout_usec,
    aesm_error_t            *result);

uae_oal_status_t SGXAPI oal_report_attestation_status(
    const sgx_platform_info_t*  p_platform_info,
    int                         attestation_status,
    sgx_update_info_bit_t*      p_update_info,
    uint32_t                    timeout_usec,
    aesm_error_t                *result);

uae_oal_status_t SGXAPI oal_check_update_status(
    const sgx_platform_info_t*  p_platform_info,
    sgx_update_info_bit_t*      p_update_info,
    uint32_t                    config,
    uint32_t*                   status,
    uint32_t                    timeout_usec,
    aesm_error_t                *result);

uae_oal_status_t oal_get_whitelist_size(
    uint32_t* p_whitelist_size,
    uint32_t timeout_usec,
    aesm_error_t* result);

uae_oal_status_t oal_get_whitelist(
    uint8_t* p_whitelist,
    uint32_t whitelist_size,
    uint32_t timeout_usec,
    aesm_error_t *result);

uae_oal_status_t oal_get_extended_epid_group_id(
    uint32_t* extended_group_id,
    uint32_t timeout_usec,
    aesm_error_t *result);

uae_oal_status_t oal_switch_extended_epid_group(
    uint32_t x_group_id,
    uint32_t timeout_usec,
    aesm_error_t *result);

uae_oal_status_t oal_register_common(
    uint8_t* buf,
    uint32_t buf_size,
    uint32_t data_type,
    uint32_t timeout_usec,
    aesm_error_t *result);

uae_oal_status_t oal_select_att_key_id(const uint8_t *att_key_id_list,
                uint32_t att_key_id_list_size,
                sgx_att_key_id_t *selected_key_id,
                uint32_t timeout_usec, aesm_error_t *result);

uae_oal_status_t oal_init_quote_ex(const sgx_att_key_id_t *att_key_id,
                sgx_target_info_t *target_info,
                size_t *pub_key_id_size, size_t buf_size, uint8_t *pub_key_id,
                uint32_t timeout_usec, aesm_error_t *result);

uae_oal_status_t oal_get_quote_size_ex(
                const sgx_att_key_id_t *att_key_id,
                uint32_t *quote_size,
                uint32_t timeout_usec, aesm_error_t *result);

uae_oal_status_t oal_get_quote_ex(
                const sgx_report_t *p_report,
                const sgx_att_key_id_t *att_key_id,
                sgx_qe_report_info_t *qe_report_info,
                uint32_t quote_size, uint8_t *p_quote,
                uint32_t timeout_usec,
                aesm_error_t *result);

uae_oal_status_t oal_get_supported_att_key_id_num(
    uint32_t *p_att_key_id_num,
    uint32_t timeout_usec, aesm_error_t *result);

uae_oal_status_t oal_get_supported_att_key_ids(
    sgx_att_key_id_ext_t *p_att_key_id_list,
    uint32_t att_key_id_list_size,
    uint32_t timeout_usec, aesm_error_t *result);

sgx_status_t    oal_map_status(uae_oal_status_t status);
sgx_status_t    oal_map_result(aesm_error_t result);

} /* end of extern "C" */
#endif

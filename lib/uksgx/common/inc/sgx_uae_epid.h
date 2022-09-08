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

#ifndef _SGX_UAE_EPID_H_
#define _SGX_UAE_EPID_H_

#include <stdint.h>

#include "sgx_quote.h"
#include "sgx_error.h"
#include "sgx_urts.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Function used to initialize the process of quoting.
 *
 * @param p_target_info[out] Target info of quoting enclave.
 * @param p_gid[out] ID of platform's current EPID group.
 * @return If outputs are generated, return SGX_SUCCESS, otherwise return general error code
 *             or SGX_ERROR_AE_INVALID_EPIDBLOB to indicate special error condition.
 */
sgx_status_t SGXAPI sgx_init_quote(
    sgx_target_info_t *p_target_info,
    sgx_epid_group_id_t *p_gid);


/*
 * Function used to calculate quote size.
 *
 * @param p_sig_rl[in] OPTIONAL Signature Revocation List.
 * @param sig_rl_size[in] Signature Revocation List size, in bytes.
 * @param p_quote_size[out] Quote size, in bytes.
 * @return If quote size is calculated,return SGX_SUCCESS, otherwise return
 *            SGX_ERROR_INVALID_PARAMETER to indicate special error condition.
 */
sgx_status_t SGXAPI sgx_calc_quote_size(
    const uint8_t *p_sig_rl,
    uint32_t sig_rl_size,
    uint32_t* p_quote_size);

/*
 * [DEPRECATED] Use sgx_calc_quote_size function instead of this one
 * Function used to get quote size.
 *
 * @param p_sig_rl[in] OPTIONAL Signature Revocation List.
 * @param p_quote_size[out] Quote size, in bytes.
 * @return If quote size is calculated,return SGX_SUCCESS, otherwise return
 *            SGX_ERROR_INVALID_PARAMETER to indicate special error condition.
 */
SGX_DEPRECATED
sgx_status_t SGXAPI sgx_get_quote_size(
    const uint8_t *p_sig_rl,
    uint32_t* p_quote_size);

/*
 * Function used to get quote.
 *
 * @param p_report[in] Report of enclave for which quote is being calculated.
 * @param quote_type[in] Linkable or unlinkable quote.
 * @param p_spid[in] Pointer of SPID.
 * @param p_nonce[in] OPTIONAL nonce.
 * @param p_sig_rl[in] OPTIONAL list of signature made fore EPID.
 * @param sig_rl_size[in] The size of p_sig_rl, in bytes.
 * @param p_qe_report[out] OPTIONAL The QE report.
 * @param p_quote[out] The quote buffer, can not be NULL.
 * @param quote_size[in] Quote buffer size, in bytes.
 * @return If quote is generated,return SGX_SUCCESS,
 *         error code or SGX_ERROR_AE_INVALID_EPIDBLOB,
 *         SGX_ERROR_INVALID_PARAMETER to indicate special error condition.
 *         SGX_ERROR_EPID_MEMBER_REVOKED to indicate the EPID group membership has been revoked.
 */
sgx_status_t SGXAPI sgx_get_quote(
    const sgx_report_t *p_report,
    sgx_quote_sign_type_t quote_type,
    const sgx_spid_t *p_spid,
    const sgx_quote_nonce_t *p_nonce,
    const uint8_t *p_sig_rl,
    uint32_t sig_rl_size,
    sgx_report_t *p_qe_report,
    sgx_quote_t *p_quote,
    uint32_t quote_size);

/**
 * Get the extended epid group id
 *
 * @param p_extended_epid_group_id Save the extended epid group id.
 * @return if OK, return SGX_SUCCESS
 */
sgx_status_t SGXAPI sgx_get_extended_epid_group_id(uint32_t* p_extended_epid_group_id);

/*
 * Function used to report the status of the attestation.
 *
 * @param p_platform_info[in] platform information received from Intel Attestation Server.
 * @param attestation_status[in] Value representing status during attestation. 0 if attestation succeeds.
 * @param p_update_info[out] update information of the SGX platform.
 * @return If OK, return SGX_SUCCESS. If update is needed, return SGX_ERROR_UPDATE_NEEDED and update_info contains update information.
 */

sgx_status_t SGXAPI sgx_report_attestation_status(
    const sgx_platform_info_t* p_platform_info,
    int attestation_status,
    sgx_update_info_bit_t* p_update_info);


/*
 * Function used to learn whether update is available from platform information.
 *
 * @param p_platform_info[in] platform information received from Intel Attestation Server.
 * @param p_update_info[out] update information of the SGX platform.
 * @param config[in] Caller can use config to indicate whether they want to address pending EPID or PSE provisioning.
 *   bit 0: reserved (MBZ)
 *   bit 1: set if caller wants to trigger EPID provisioning if it is needed/pending. 
 *   bit 2: set if caller wants to trigger PSE provisioning/long-term pairing if it is needed/pending.
 *   bit 31..3: reserved (MBZ)
 *   if bit [2:1] == 0, then never trigger either EPID or PSE provisioning.
 * @param p_status[out] fills in p_status as follows:
 *   bit 0: set if any update is available. (Caller inspects p_update_info to learn details)
 *   bit 1: set if EPID provisioning is or was needed/pending. Set or cleared independent of config input.
 *   bit 2: set if PSE provisioning/long-term pairing is or was needed/pending. Set or cleared independent of config input.
 *   bit 31..3: reserved
 *
 * @return SGX_ERROR_UPDATE_NEEDED if update is needed, and update_info contains update information.
 * @return SGX_ERROR_UNSUPPORTED_CONFIG  if any unsupported bits in config input are set.
 * @return SGX_ERROR_INVALID_PARAMETER  Invalid parameter if p_platform_info is NULL and p_update_info is not NULL, or 
 *         p_platform_info is NULL and config is 0 (nothing to do).
 * @return SGX_ERROR_UNEXPECTED Unexpected internal error.
 * @return SGX_SUCCESS otherwise.
 */
sgx_status_t SGXAPI sgx_check_update_status(
    const sgx_platform_info_t* p_platform_info,
    sgx_update_info_bit_t* p_update_info,
    uint32_t config,
    uint32_t* p_status);

#ifdef  __cplusplus
}
#endif

#endif

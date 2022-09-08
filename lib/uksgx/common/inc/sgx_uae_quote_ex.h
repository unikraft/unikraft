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

#ifndef _SGX_UAE_QUOTE_EX_H_
#define _SGX_UAE_QUOTE_EX_H_

#include <stdint.h>

#include "sgx_quote.h"
#include "sgx_error.h"
#include "sgx_urts.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Function use to select the attestation key. The function will select a att_key_id_list of attestation keys supported
 * by the quote verifier if the platform can support one in the list.  If the platform cannot support one in the list,
 * the API will return error SGX_ERROR_UNSUPPORTED_ATT_KEY_ID. If multiple attestation keys are supported
 * by both quote verifier and the platform software, the "default quoting type" in config file will be used. Alternatively,
 * if the quote provider doesn't supply a list if attestation keys supported by the quote verifier (p_att_key_id_list == NULL),
 * then the platform software will select the attestation key by the internal logic.
 * 
 *
 * @param p_att_key_id_list [In] List of the supported attestation key IDs provided by the quote verifier. Can be
 *                          NULL, in such case a default att key supported by PSW will be returned.
 * @param att_key_id_list_size The size of attestation key ID list.
 * @param p_selected_key_id [In, Out] Pointer to the selected attestation key. This should be used by the
 *                           application as input to the quoting and remote attestation APIs.  Must not be NULL.
 *
 * @return SGX_SUCCESS Successfully return an attestation key. The p_selected_key_id will be filled with selected
 *         attestation key ID.
 * @return SGX_ERROR_INVALID_PARAMETER Invalid parameter if p_selected_key_id is NULL,
 *         list header is incorrect, or the number of key IDs in the list exceed the maximum.
 * @return SGX_ERROR_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support any of the keys in the
 *         list.  This can be because it doesn't carry the QE that owns the attestation key or the platform is in a
 *         mode that doesn't allow any of the listed keys; for example, for privacy reasons. It also returns such eror
 *         if the platform software only supports a key that is not supported by the current launch control policy.
 * @return SGX_ERROR_UNEXPECTED Unexpected internal error.
 */
sgx_status_t SGXAPI sgx_select_att_key_id(const uint8_t *p_att_key_id_list, uint32_t att_key_id_list_size,
                                                   sgx_att_key_id_t *p_selected_key_id);



/**
 * The application calls this API to request the selected platform's attestation key owner to generate or obtain
 * the attestation key.  Once called, the QE that owns the attestation key described by the inputted attestation
 * key id will do what is required to get this platform's attestation including getting any certification data
 * required from the PCE.  Depending on the type of attestation key and the attestation key owner, this API will
 * return the same attestation key public ID or generate a new one.  The caller can request that the attestation
 * key owner "refresh" the key.  This will cause the owner to either re-get the key or generate a new one.  The
 * platform's attestation key owner is expected to store the key in persistent memory and use it in the
 * subsequent quote generation APIs described below.
 *
 * In an environment where attestation key provisioning and certification needs to take place during a platform
 * deployment phase, an application can generate the attestation key, certify it with the PCK Cert and register
 * it with the attestation owners cloud infrastructure.  That way, the key is available during the run time
 * phase to generate code without requiring re-certification.
 *
 * The QE's target info is also returned by this API that will allow the application's enclave to generate a
 * REPORT that the attestation key owner's QE will verify using local REPORT-based attestation when generating a
 * quote.
 *
 * In order to allow the application to allocate the public key id buffer first, the application can call this
 * function with the p_pub_key_id set to NULL and the p_pub_key_id_size to a valid size_t pointer.  In this
 * case, the function will return the required buffer size to contain the p_pub_key_id_size and ignore the other
 * parameters.  The application can then call this API again with the correct p_pub_key_size and the pointer to
 * the allocated buffer in p_pub_key_id.
 *
 *
 * @param p_att_key_id The selected att_key_id from the quote verifier's list.  It includes the QE identity as
 *                     well as the attestation key's algorithm type. It cannot be NULL.
 * @param p_qe_target_info Pointer to QE's target info required by the application to generate an enclave REPORT
 *                         targeting the selected QE. Must not be NULL when p_pub_key_id is not NULL.
 * @param p_pub_key_id_size This parameter can be used in 2 ways.  When p_pub_key_id is NULL, the API will
 *                          return the buffer size required to hold the attestation's public key ID.  The
 *                          application can then allocate the buffer and call it again with p_pub_key_id not set
 *                          to NULL and the other parameters valid.  If p_pub_key_id is not NULL, p_pub_key_id_size
 *                          must be large enough to hold the return attestation's public key ID.  Must not be
 *                          NULL.
 * @param p_pub_key_id This parameter can be used in 2 ways. When it is passed in as NULL and p_pub_key_id_size
 *                     is not NULL, the API will return the buffer size required to hold the attestation's
 *                     public key ID.  The other parameters will be ignored.  When it is not NULL, it must point
 *                     to a buffer which is at least a long as the value passed in by p_pub_key_id_size. API will
 *                     return the attestation key's public identifier if no error occured.

 * @return SGX_SUCCESS Successfully selected an attestation key.  Either returns the required attestation's
 *         public key ID size in p_pub_key_id_size when p_pub_key_id is passed in as NULL.  When p_pub_key_id is
 *         not NULL, p_qe_target_info will contain the attestation key's QE target info for REPORT generation
 *         and p_pub_key_id will contain the attestation's public key ID.
 * @return SGX_ERROR_INVALID_PARAMETER Invalid parameter if p_pub_key_id_size, p_att_key_id is NULL.
 *         If p_pub_key_id_size is not NULL, the other parameters must be valid.
 * @return SGX_ERROR_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support the key described
 *         in p_att_key_id.
 * @return SGX_ERROR_ATT_KEY_CERTIFICATION_FAILURE Failed to generate and certify the attestation key.
 *
 */
sgx_status_t SGXAPI sgx_init_quote_ex(const sgx_att_key_id_t* p_att_key_id,
                                            sgx_target_info_t *p_qe_target_info,
                                            size_t* p_pub_key_id_size,
                                            uint8_t* p_pub_key_id);

/**
 * The application needs to call this function before generating a quote.  The quote size is variable
 * depending on the type of attestation key selected and other platform or key data required to generate the
 * quote.  Once the application calls this API, it will use the returned p_quote_size to allocate the buffer
 * required to hold the generated quote.  A pointer to this buffer is provided to the sgx_get_quote_ex() API.
 *
 * If the key is not available, this API may return an error (SGX_ERROR_ATT_KEY_UNINITIALIZED) depending on
 * the algorithm.  In this case, the caller must call sgx_init_quote_ex() to re-generate and certify the attestation key.
 *
 * @param p_att_key_id The selected attestation key ID from the quote verifier's list.  It includes the QE
 *                     identity as well as the attestation key's algorithm type. It cannot be NULL.
 * @param p_quote_size Pointer to the location where the required quote buffer size will be returned. Must
 *                     not be NULL.
 *
 * @return SGX_SUCCESS Successfully calculated the required quote size. The required size in bytes is returned in the
 *         memory pointed to by p_quote_size.
 * @return SGX_ERROR_INVALID_PARAMETER Invalid parameter. p_quote_size and p_att_key_id must not be NULL.
 * @return SGX_ERROR_ATT_KEY_UNINITIALIZED The platform quoting infrastructure does not have the attestation
 *         key available to generate quotes.  sgx_init_quote_ex() must be called again.
 * @return SGX_ERROR_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support the key
 *         described in p_att_key_id.
 */
sgx_status_t SGXAPI sgx_get_quote_size_ex(const sgx_att_key_id_t *p_att_key_id,
                                                uint32_t* p_quote_size);

/**
 * The function will take the application enclave's REPORT that will be converted into a quote after the QE verifies
 * the REPORT.  Once verified it will sign it with platform's attestation key matching the selected attestation key ID. 
 * If the key is not available, this API may return an error (SGX_ERROR_ATT_KEY_UNINITIALIZED) depending on the algorithm.
 * In this case, the caller must call sgx_init_quote_ex() to re-generate and certify the attestation key. an attestation key.
 *
 *
 * The caller can request a REPORT from the QE using a supplied nonce.  This will allow the enclave requesting the quote
 * to verify the QE used to generate the quote. This makes it more difficult for something to spoof a QE and allows the
 * app enclave to catch it earlier.  But since the authenticity of the QE lies in knowledge of the Quote signing key,
 * such spoofing will ultimately be detected by the quote verifier.  QE REPORT.ReportData =
 * SHA256(*p_nonce||*p_quote)||32-0x00's.
 *
 * @param p_app_report Pointer to the enclave report that needs the quote. The report needs to be generated using the
 *                     QE's target info returned by the sgx_init_quote_ex() API.  Must not be NULL.
 * @param p_att_key_id The selected attestation key ID from the quote verifier's list.  It includes the QE identity as
 *                     well as the attestation key's algorithm type. It cannot be NULL.
 * @param p_qe_report_info Pointer to a data structure that will contain the information required for the QE to generate
 *                         a REPORT that can be verified by the application enclave.  The inputted data structure
 *                         contains the application's TARGET_INFO, a nonce and a buffer to hold the generated report.
 *                         The QE Report will be generated using the target information and the QE's REPORT.ReportData =
 *                         SHA256(*p_nonce||*p_quote)||32-0x00's.  This parameter is used when the application wants to
 *                         verify the QE's REPORT to provide earlier detection that the QE is not being spoofed by
 *                         untrusted code.  A spoofed QE will ultimately be rejected by the remote verifier.   This
 *                         parameter is optional and will be ignored when NULL.
 * @param p_quote Pointer to the buffer that will contain the quote.
 * @param quote_size Size of the buffer pointed to by p_quote.
 *
 * @return SGX_SUCCESS Successfully generated the quote.
 * @return SGX_ERROR_INVALID_PARAMETER If either p_app_report or p_quote is null. Or, if quote_size isn't large
 *         enough, p_att_key_id is NULL.
 * @return SGX_ERROR_ATT_KEY_UNINITIALIZED The platform quoting infrastructure does not have the attestation key
 *         available to generate quotes.  sgx_init_quote_ex() must be called again.
 * @return SGX_ERROR_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support the key described in
 *         p_att_key_id.
 * @return SGX_ERROR_MAC_MISMATCH Report MAC check failed on application report.
 */
sgx_status_t SGXAPI  sgx_get_quote_ex(const sgx_report_t *p_app_report,
                                           const sgx_att_key_id_t *p_att_key_id,
                                           sgx_qe_report_info_t *p_qe_report_info,
                                           uint8_t *p_quote,
                                           uint32_t quote_size);

/**
 * The application needs to call this function before getting supported_attestation key IDs.  The number is variable
 * depending on the platform. Once the application calls this API, it will use the returned p_att_key_id_num to allocate
 * the buffer required to hold the supported_attestation key IDs.  A pointer to this buffer is provided to the
 * sgx_get_supported_att_key_ids() API.
 *
 * @param p_att_key_id_num Pointer to the location where the required number will be returned. Must not be NULL.
 *
 * @return SGX_SUCCESS Successfully calculated the required number.
 * @return SGX_ERROR_INVALID_PARAMETER Invalid parameter. p_att_key_id_num must not be NULL.
 */
sgx_status_t SGXAPI sgx_get_supported_att_key_id_num(uint32_t *p_att_key_id_num);

/**
 * The function will generate an array of all supported attestation key IDs.
 * The application needs to call sgx_get_supported_att_key_id_num() API first to get the number of key IDs. Then
 * the application needs to allocate the buffer whose size is sizeof sgx_att_key_id_ext_t * att_key_id_num.
 *
 * @param p_att_key_id_list Pointer to the buffer that will contain supported_attestation key IDs. It cannot be NULL.
 * @param att_key_id_num  the number of supported key IDs.
 *
 * @return SGX_SUCCESS Successfully generated the supported attestation key IDs.
 * @return SGX_ERROR_INVALID_PARAMETER Invalid parameter. p_att_key_id_list must not be NULL.
 */
sgx_status_t SGXAPI sgx_get_supported_att_key_ids(sgx_att_key_id_ext_t *p_att_key_id_list,
                                                    uint32_t att_key_id_num);

#ifdef  __cplusplus
}
#endif

#endif

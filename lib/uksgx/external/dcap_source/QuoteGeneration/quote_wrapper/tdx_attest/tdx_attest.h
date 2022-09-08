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
 * File: tdx_attest.h
 *
 * Description: API definitions for TDX Attestation library
 *
 */
#ifndef _TDX_ATTEST_H_
#define _TDX_ATTEST_H_
#include <stdint.h>

typedef enum _tdx_attest_error_t {
    TDX_ATTEST_SUCCESS = 0x0000,                        ///< Success
    TDX_ATTEST_ERROR_MIN = 0x0001,                      ///< Indicate min error to allow better translation.
    TDX_ATTEST_ERROR_UNEXPECTED = 0x0001,               ///< Unexpected error
    TDX_ATTEST_ERROR_INVALID_PARAMETER = 0x0002,        ///< The parameter is incorrect
    TDX_ATTEST_ERROR_OUT_OF_MEMORY = 0x0003,            ///< Not enough memory is available to complete this operation
    TDX_ATTEST_ERROR_VSOCK_FAILURE = 0x0004,            ///< vsock related failure
    TDX_ATTEST_ERROR_REPORT_FAILURE = 0x0005,           ///< Failed to get the TD Report
    TDX_ATTEST_ERROR_EXTEND_FAILURE = 0x0006,           ///< Failed to extend rtmr
    TDX_ATTEST_ERROR_NOT_SUPPORTED = 0x0007,            ///< Request feature is not supported
    TDX_ATTEST_ERROR_QUOTE_FAILURE = 0x0008,            ///< Failed to get the TD Quote
    TDX_ATTEST_ERROR_BUSY = 0x0009,                     ///< The device driver return busy
    TDX_ATTEST_ERROR_DEVICE_FAILURE = 0x000a,           ///< Failed to acess tdx attest device
    TDX_ATTEST_ERROR_INVALID_RTMR_INDEX = 0x000b,       ///< Only supported RTMR index is 2 and 3
    TDX_ATTEST_ERROR_MAX
} tdx_attest_error_t;

#define TDX_UUID_SIZE 16

#pragma pack(push, 1)

#define TDX_UUID_SIZE 16
typedef struct tdx_uuid_t
{
    uint8_t d[TDX_UUID_SIZE];
} tdx_uuid_t;

#define TDX_SGX_ECDSA_ATTESTATION_ID                   \
{                                                      \
    0xe8, 0x6c, 0x04, 0x6e, 0x8c, 0xc4, 0x4d, 0x95,    \
    0x81, 0x73, 0xfc, 0x43, 0xc1, 0xfa, 0x4f, 0x3f     \
}

#define TDX_REPORT_DATA_SIZE 64
typedef struct _tdx_report_data_t
{
    uint8_t d[TDX_REPORT_DATA_SIZE];
} tdx_report_data_t;

#define TDX_REPORT_SIZE 1024
typedef struct _tdx_report_t
{
    uint8_t d[TDX_REPORT_SIZE];
} tdx_report_t;

typedef struct _tdx_rtmr_event_t {
    uint32_t	version;
    uint64_t 	rtmr_index;
    uint8_t 	extend_data[48];
    uint32_t 	event_type;
    uint32_t 	event_data_size;
    uint8_t 	event_data[];
} tdx_rtmr_event_t;

#pragma pack(pop)

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Request a Quote of the calling TD.
 *
 * The caller provides data intended to be cryptographically bound to the
 * resulting Quote. (This data should not require confidentiality protection.)
 * The caller also provides information about the type of Quote signing that
 * should be used.
 *
 * In general, a given platform can create Quotes using
 * different cryptographic algorithms or using different vendors’ code/enclaves.
 * The att_key_id_list parameter is related to this. It is a list of key IDs
 * supported by the eventual verifier of the Quote. How the caller of this
 * function obtains this list is outside the scope of the R3AAL.
 *
 * A default key ID is supported and will be used when att_key_id_list == NULL.
 * In this case, the default key ID is returned via the p_att_key_id parameter.
 *
 * When the function returns successfully, p_quote will point to a buffer
 * containing the Quote. This buffer is allocated by the function. Use
 * tdx_att_free_quote to free this buffer.
 *
 * @param p_tdx_report_data [in] Pointer to data that the caller/TD wants to
 *                               cryptographically bind to the Quote,
 *                               typically a hash. May be NULL, in which case,
 *                               all zeros will be used for the Report data.
 * @param att_key_id_list [in] List (array) of the attestation key IDs supported
 *                             by the Quote verifier. The function compares the
 *                             key IDs in att_key_id_list to the key IDs that
 *                             the platform supports and uses the first match.
 *                             May be NULL. If NULL, the API will use the
 *                             platform’s default key ID. The uuid_t
 *                             corresponding to the key ID that’s used is
 *                             pointed to by p_att_key_id when the function
 *                             returns unless p_att_key_id == NULL.
 * @param list_size [in] Size of att_key_id_list in entries.
 * @param p_att_key_id [out] The selected attestation key ID when the function
 *                           returns. May be NULL indicating the platform’s
 *                           default key ID
 * @param pp_quote [out] Pointer to a pointer that the function will set equal
 *                       to the address of the buffer containing the Quote. The
 *                       function also allocates this buffer. Use
 *                       tdx_att_free_quote to free this buffe
 * @param p_quote_size [out] This function will place the size of the Quote, in
 *                           bytes, in the uint32_t pointed to by the
 *                           p_quote_size parameter. May be NULL.
 * @param flags [in] Reserved, must be zero.
 * @return TDX_ATTEST_SUCCESS: Successfully generated the Quote.
 * @return TDX_ATTEST_ERROR_UNEXPECTED: An unexpected internal error occurred.
 * @return TDX_ATTEST_ERROR_INVALID_PARAMETER: The parameter is incorrect
 * @return TDX_ATTEST_ERROR_REPORT_FAILURE: Failed to get TD report.
 * @return TDX_ATTEST_ERROR_VSOCK_FAILURE: Failed read/write in vsock mode
 * @return TDX_ATTEST_ERROR_QUOTE_FAILURE: Failed to get quote from QGS
 * @return TDX_ATTEST_UNSUPPORTED_ATT_KEY_ID: The platform Quoting
 *         infrastructure does not support any of the keys described in
 *         att_key_id_list.
 * @return TDX_ATTEST_OUT_OF_MEMORY: Heap memory allocation error in library or
 *                                enclave.
 */
tdx_attest_error_t tdx_att_get_quote(
    const tdx_report_data_t *p_tdx_report_data,
    const tdx_uuid_t att_key_id_list[],
    uint32_t list_size,
    tdx_uuid_t *p_att_key_id,
    uint8_t **pp_quote,
    uint32_t *p_quote_size,
    uint32_t flags);


/**
 * @brief Free the Quote buffer allocated by tdx_att_get_quote.
 *
 * @param p_quote [in] The value of *p_quote returned by tdx_att_get_quote.
 * @return TDX_ATTEST_SUCCESS: Successfully freed the p_quote.
 */
tdx_attest_error_t tdx_att_free_quote(
    uint8_t *p_quote);

/**
 * @brief Request a TDX Report of the calling TD.
 *
 * The caller provides data intended to be cryptographically bound to the
 * resulting Report.
 *
 * @param p_tdx_report_data [in] Pointer to data that the caller/TD wants to
 *                               cryptographically bind to the Quote, typically
 *                               a hash. May be NULL, in which case, all zeros
 *                               will be used for the Report data.
 * @param p_tdx_report [out] Pointer to the buffer that will contain the
 *                           generated TDX Report. Must not be NULL.
 * @return TDX_ATTEST_SUCCESS: Successfully generated the Report.
 * @return TDX_ATTEST_ERROR_INVALID_PARAMETER: p_tdx_report == NULL
 * @return TDX_ATTEST_ERROR_REPORT_FAILURE: Failed to get TD report.
 */
tdx_attest_error_t tdx_att_get_report(
    const tdx_report_data_t *p_tdx_report_data,
    tdx_report_t* p_tdx_report);


/**
 * @brief Extend one of the TDX runtime measurement registers (RTMRs).
 *
 * RTMR[rtmr_index] = SHA384(RTMR[rtmr_index] || extend_data)
 * rtmr_index and extend_data are fields in the structure that is an input of
 * this API.
 * This API does not return either the new or old value of the specified RTMR.
 * The tdx_att_get_report API may be used for this.
 * The input to this API includes a description of the “extend data”. This is
 * intended to facilitate reconstruction of the RTMR value. This, in turn,
 * suggests maintenance of an event log by the callee. Currently, event_data is
 * not supported.
 *
 * @param p_rtmr_event [in] Pointer to structure that contains the index of the
 *                          RTMR to extend, the data with which to extend it and
 *                          a description of the data.
 * @return TDX_ATTEST_SUCCESS: Successfully extended the RTMR.
 * @return TDX_ATTEST_ERROR_INVALID_PARAMETER: p_rtmr_event == NULL
 * @return TDX_ATTEST_ERROR_UNEXPECTED: An unexpected internal error occurred.
 * @return TDX_ATTEST_ERROR_EXTEND_FAILURE: Failed to extend data.
 * @return TDX_ATTEST_ERROR_NOT_SUPPORTED: p_rtmr_event->event_data_size != 0
 */
tdx_attest_error_t tdx_att_extend(
    const tdx_rtmr_event_t *p_rtmr_event);


/**
 * @brief Retrieve the list of attestation key IDs supported by the platform.
 *
 * Specify p_att_key_id_list = NULL to learn the number of entries in the list.
 *
 * @param p_att_key_id_list [out] List of the attestation key IDs that the
 *                                platform supports. May be NULL. If NULL, the
 *                                API will return the number of entries in the
 *                                list in the uint32_t pointed to by p_list_size
 * @param p_list_size [in/out] As input, pointer to a uint32_t specifying the
 *                             size of p_att_key_id_list in entries. As output,
 *                             this function will place the required size, in
 *                             entries, in the uint32_t pointed to by the
 *                             p_list_size parameter. If this value changes, the
 *                             new value will be the required size
 * @return TDX_ATTEST_SUCCESS: att_key_id_list populated and p_list_size points
 *                             to a uint32_t that indicates the number of
 *                             entries.
 * @return TDX_ATTEST_ERROR_INVALID_PARAMETER: The parameter is incorrect
 */
tdx_attest_error_t tdx_att_get_supported_att_key_ids(
    tdx_uuid_t *p_att_key_id_list,
    uint32_t *p_list_size);
#if defined(__cplusplus)
}
#endif


#endif


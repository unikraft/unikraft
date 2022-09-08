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
 * File: sgx_ql_core_wrapper.cpp
 *
 * Description: SGX core Quote Library wrapper to implement generic quote generation. Only implements the ECDSA
 * P256 quote algorithm type.
 *
 */

#include <string.h>
#include <stdio.h>

#include "user_types.h"
#include "sgx_report.h"
#include "sgx_ql_ecdsa_quote.h"
#include "sgx_ql_core_wrapper.h"
#include "qe3.h"
#include "se_memcpy.h"

#ifdef USE_DEBUG_MRSIGNER
// This is the mrsigner for debug key
uint8_t g_qe_mrsigner[32] = { 0xe4, 0xbd, 0x9f, 0xc2, 0x12, 0x98, 0x35, 0xba, 0x1d, 0xcc, 0xa1, 0x93, 0x9d, 0x57, 0xd0, 0x8e,
							  0x88, 0x56, 0x1b, 0x34, 0x74, 0x0d, 0x59, 0x40, 0x59, 0x72, 0xd4, 0xba, 0x25, 0xa7, 0xe5, 0xf7 };
#else
uint8_t g_qe_mrsigner[32] = { 0x8c, 0x4f, 0x57, 0x75, 0xd7, 0x96, 0x50, 0x3e, 0x96, 0x13, 0x7f, 0x77, 0xc6, 0x8a, 0x82, 0x9a,
                              0x00, 0x56, 0xac, 0x8d, 0xed, 0x70, 0x14, 0x0b, 0x08, 0x1b, 0x09, 0x44, 0x90, 0xc5, 0x7b, 0xff };
#endif
uint8_t g_qe_ext_prod_id[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t g_qe_config_id[64] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    // QE's Config ID
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t g_qe_family_id[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};   // QE's family_id


/* Set the default Attestation Key Identity for the DCAP Quoting Library.  This is the ECDSA QE3's identity and
   ECDSA-256 */
extern const sgx_ql_att_key_id_t g_default_ecdsa_p256_att_key_id =
{
    0,                                                                                                   // ID
    0,                                                                                                   // Version
    32,                                                                                                  // Number of bytes in MRSIGNER
#ifdef USE_DEBUG_MRSIGNER
    { 0xe4, 0xbd, 0x9f, 0xc2, 0x12, 0x98, 0x35, 0xba, 0x1d, 0xcc, 0xa1, 0x93, 0x9d, 0x57, 0xd0, 0x8e,
	  0x88, 0x56, 0x1b, 0x34, 0x74, 0x0d, 0x59, 0x40, 0x59, 0x72, 0xd4, 0xba, 0x25, 0xa7, 0xe5, 0xf7,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // DEBUG QE3's MRSIGNER
#else
    { 0x8c, 0x4f, 0x57, 0x75, 0xd7, 0x96, 0x50, 0x3e, 0x96, 0x13, 0x7f, 0x77, 0xc6, 0x8a, 0x82, 0x9a,
      0x00, 0x56, 0xac, 0x8d, 0xed, 0x70, 0x14, 0x0b, 0x08, 0x1b, 0x09, 0x44, 0x90, 0xc5, 0x7b, 0xff,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // Production QE3's MRSIGNER
#endif
    1,                                                                                                   // QE3's Legacy Prod ID
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // QE's extended_prod_id
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    // QE's Config ID
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // QE's family_id
    SGX_QL_ALG_ECDSA_P256                                                                                   // Supported QE3's algorithm_id
};

/**
 * When the Quoting Library is linked to a process, it needs to know the proper enclave loading policy.  The library
 * may be linked with a long lived process, such as a service, where it can load the enclaves and leave them loaded
 * (persistent).  This better ensures that the enclaves will be available upon quote requests and not subject to EPC
 * limitations if loaded on demand. However, if the Quoting library is linked with an application process, there may be
 * many applications with the Quoting library and a better utilization of EPC is to load and unloaded the quoting
 * enclaves on demand (ephemeral).  The library will be shipped with a default policy of loading enclaves and leaving
 * them loaded until the library is unloaded (SGX_QL_PERSISTENT).
 *
 * If the policy is set to SGX_QL_EPHEMERAL, then the QE and PCE will be loaded and unloaded on-demand.  If either
 * enclave is already loaded when the policy is change to SGX_QL_EPHEMERAL, the enclaves will be unloaded before
 * returning.
 *
 * @param policy Sets the requested enclave loading policy to either SGX_QL_PERSISTENT, SGX_QL_EPHEMERAL or
 *                SGX_QL_DEFAULT.
 *
 * @return SGX_QL_SUCCESS Successfully set the enclave loading policy for the quoting library's enclaves.
 * @return SGX_QL_UNSUPPORTED_LOADING_POLICY
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 *
 */
quote3_error_t sgx_ql_set_enclave_load_policy(sgx_ql_request_policy_t policy)
{
    quote3_error_t ret_val = SGX_QL_ERROR_UNEXPECTED;
    ECDSA256Quote ecdsa_quote;

    if((SGX_QL_PERSISTENT != policy) &&
       (SGX_QL_EPHEMERAL != policy)) {
        ret_val = SGX_QL_UNSUPPORTED_LOADING_POLICY;
    }

    ret_val = ecdsa_quote.set_enclave_load_policy(policy);
    if(SGX_QL_SUCCESS != ret_val) {
        if((ret_val < SGX_QL_ERROR_MIN) ||
           (ret_val > SGX_QL_ERROR_MAX))
        {
                ret_val = SGX_QL_ERROR_UNEXPECTED;
        }
    }

    return(ret_val);
}


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
 * REPORT that the attestation key owner's QE can verify using local REPORT-based attestation when generating a
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
 *                         targeting the selected QE.  Must not be NULL when p_pub_key_id is not NULL.
 * @param refresh_att_key A flag indicating the attestation key owner should re-generated and certify or
 *                        otherwise attempt to re-provision the attestation key.  For example, for ECDSDA, the
 *                        platform will generate a new key and request the PCE to recertify it.  For EPID, the
 *                        platform will attempt to re-provision the EPID key.  The behavior is dependent on the
 *                        key type and the key owner, but it should make an attempt to refresh the key typically
 *                        to update the key to the current platform's TCB.
 * @param p_pub_key_id_size This parameter can be used in 2 ways.  When p_pub_key_id is NULL, the API will
 *                          return the buffer size required to hold the attestation's public key ID.  The
 *                          application can then allocate the buffer and call it again with p_pub_key_id not set
 *                          to NULL and the other parameters valid.  If p_pub_key_id is not NULL, p_pub_key_size
 *                          must be large enough to hold the return attestation's public key ID.  Must not be
 *                          NULL.
 * @param p_pub_key_id This parameter can be used in 2 ways. When it is passed in as NULL and p_pub_key_id_size
 *                     is not NULL, the API will return the buffer size required to hold the attestation's
 *                     public key ID.  The other parameters will be ignored.  When it is not NULL, it must point
 *                     to a buffer which is at least a long as the value passed in by p_pub_key_id.  Can either
 *                     be NULL or point to the correct buffer.his will point to the buffer that will contain the
 *                     attestation key's public identifier. If first called with a NULL pointer, the API will
 *                     return the required length of the buffer in p_pub_key_id_size.
 *
 * @return SGX_QL_SUCCESS Successfully selected an attestation key.  Either returns the required attestation's
 *                        public key ID size in p_pub_key_id_size when p_pub_key_id is passed in as NULL.  When
 *                        p_pub_key_id is not NULL, p_qe_target_info will contain the attestation key's QE
 *                        target info for REPORT generation and p_pub_key_id will contain the attestation's
 *                        public key ID.
 * @return SGX_QL_ERROR_INVALID_PARAMETER Invalid parameter if p_pub_key_id_size is NULL.  If p_pub_key_size is
 *                                        not NULL, the other parameters must be valid.
 * @return SGX_QL_ERROR_UNEXPECTED Invalid parameter if p_pub_key_id_size is NULL.  If
 *                                 p_pub_key_size is not NULL, the other parameters must be valid
 *
 *
 * @return SGX_QL_SUCCESS Successfully selected an attestation key.  Either returns the required attestation's
 *         public key ID size in p_pub_key_id_size when p_pub_key_id is passed in as NULL.  When p_pub_key_id is
 *         not NULL, p_qe_target_info will contain the attestation key's QE target info for REPORT generation
 *         and p_pub_key_id will contain the attestation's public key ID.
 * @return SGX_QL_ERROR_INVALID_PARAMETER Invalid parameter if p_pub_key_id_size is NULL, p_att_key_id is NULL.
 *         If p_pub_key_size is not NULL, the other parameters must be valid.
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 * @return SGX_QL_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support the key described
 *         in p_att_key_id.
 * @return SGX_QL_OUT_OF_EPC There is not enough EPC memory to load one of the Architecture Enclaves needed to
 *         complete this operation.
 * @return SGX_QL_ERROR_OUT_OF_MEMORY Heap memory allocation error in library or enclave.
 * @return SGX_QL_ATTESTATION_KEY_CERTIFCATION_ERROR Failed to generate and certify the attestation key.
 * @return SGX_QL_ENCLAVE_LOST Enclave lost after power transition or used in child process created by
 *         linux:fork().
 * @return SGX_QL_ENCLAVE_LOAD_ERROR Unable to load the enclaves required to initialize the attestation key.
 *         Could be due to file I/O error, loading infrastructure error or insufficient enclave memory.
 *
 */
extern "C" quote3_error_t sgx_ql_init_quote(sgx_ql_att_key_id_t* p_att_key_id,
                                            sgx_target_info_t *p_qe_target_info,
                                            bool refresh_att_key,
                                            size_t* p_pub_key_id_size,
                                            uint8_t* p_pub_key_id)
{
    quote3_error_t ret_val = SGX_QL_ERROR_UNEXPECTED;
    sgx_status_t sgx_status;
    qe3_error_t qe3_error;
    ECDSA256Quote ecdsa_quote;
    sgx_ql_att_key_id_t *p_local_att_key_id = NULL;

    // Verify inputs
    if(NULL == p_pub_key_id_size)
    {
        return(SGX_QL_ERROR_INVALID_PARAMETER);
    }

    if(NULL != p_att_key_id)
    {
        // Verify the Attestation key identity is supported.
        if (0 != p_att_key_id->id)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (0 != p_att_key_id->version)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (32 != p_att_key_id->mrsigner_length)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (0 != memcmp(p_att_key_id->mrsigner, g_qe_mrsigner, 32))
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        else
        {
            if(1 != p_att_key_id->prod_id)
            {
                return(SGX_QL_ERROR_INVALID_PARAMETER);
            }
            if (SGX_QL_ALG_ECDSA_P256 != p_att_key_id->algorithm_id)
            {
                return(SGX_QL_ERROR_INVALID_PARAMETER);
            }
        }
        p_local_att_key_id = p_att_key_id;
    }
    else
    {
        p_local_att_key_id = (sgx_ql_att_key_id_t*)&g_default_ecdsa_p256_att_key_id;
    }
    // Choose the default certification key type supported by the reference.
    sgx_ql_cert_key_type_t certification_key_type = SGX_QL_CERT_TYPE;

    ret_val = ecdsa_quote.init_quote(p_local_att_key_id,  ///todo: make parameter const.
                                     certification_key_type,
                                     p_qe_target_info,
                                     refresh_att_key,
                                     p_pub_key_id_size,
                                     p_pub_key_id);

    if(SGX_QL_SUCCESS != ret_val) {
        if((ret_val < SGX_QL_ERROR_MIN) ||
           (ret_val > SGX_QL_ERROR_MAX))
        {
            sgx_status = (sgx_status_t)ret_val;
            qe3_error = (qe3_error_t)ret_val;

            // Translate QE3 errors
            switch(qe3_error)
            {
            case REFQE3_ERROR_INVALID_PARAMETER:
                ret_val = SGX_QL_ERROR_INVALID_PARAMETER;
                break;

            case REFQE3_ERROR_OUT_OF_MEMORY:
                ret_val =  SGX_QL_ERROR_OUT_OF_MEMORY;
                break;

            case REFQE3_ERROR_UNEXPECTED:
            case REFQE3_ERROR_CRYPTO:       // Error generating the QE_ID (or decypting PPID not supported in release).  Unexpected error.
            case REFQE3_ERROR_ATT_KEY_GEN:  // Error generating the ECDSA Attestation key.
            case REFQE3_ECDSABLOB_ERROR:    // Should be unexpected since the blob was either generated or regenerated during this call
                ret_val = SGX_QL_ERROR_UNEXPECTED;
                break;

            default:
                // Translate SDK errors
                switch (sgx_status)
                {
                case SGX_ERROR_INVALID_PARAMETER:
                    ret_val = SGX_QL_ERROR_INVALID_PARAMETER;
                    break;

                case SGX_ERROR_OUT_OF_MEMORY:
                    ret_val = SGX_QL_ERROR_OUT_OF_MEMORY;
                    break;

                case SGX_ERROR_ENCLAVE_FILE_ACCESS:
                    ret_val = SGX_QL_ENCLAVE_LOAD_ERROR;
                    break;

                case SGX_ERROR_ENCLAVE_LOST:
                    ret_val = SGX_QL_ENCLAVE_LOST;
                    break;

                    // Unexpected enclave loading errorsReturn codes from load_qe
                case SGX_ERROR_INVALID_ENCLAVE:
                case SGX_ERROR_UNDEFINED_SYMBOL:
                case SGX_ERROR_MODE_INCOMPATIBLE:
                case SGX_ERROR_INVALID_METADATA:
                case SGX_ERROR_MEMORY_MAP_CONFLICT:
                case SGX_ERROR_INVALID_VERSION:
                case SGX_ERROR_INVALID_ATTRIBUTE:
                case SGX_ERROR_NDEBUG_ENCLAVE:
                case SGX_ERROR_INVALID_MISC:
                    //case SE_ERROR_INVALID_LAUNCH_TOKEN:     ///todo: Internal error should be scrubbed before here.
                case SGX_ERROR_DEVICE_BUSY:
                case SGX_ERROR_NO_DEVICE:
                case SGX_ERROR_INVALID_SIGNATURE:
                    //case SE_ERROR_INVALID_MEASUREMENT:      ///todo: Internal error should be scrubbed before here.
                    //case SE_ERROR_INVALID_ISVSVNLE:         ///todo: Internal error should be scrubbed before here.
                case SGX_ERROR_INVALID_ENCLAVE_ID:
                    ret_val = SGX_QL_ENCLAVE_LOAD_ERROR;
                    break;
                case SGX_ERROR_SERVICE_INVALID_PRIVILEGE:
                    ret_val = SGX_QL_ERROR_INVALID_PRIVILEGE;
                    break;

                case SGX_ERROR_UNEXPECTED:
                    ret_val = SGX_QL_ERROR_UNEXPECTED;
                    break;

                default:
                    ret_val = SGX_QL_ERROR_UNEXPECTED;
                    break;
                }
                break;
            }
        }
    }

    return(ret_val);
}

/**
 * The application needs to call this function before generating a quote.  The quote size is variable
 * depending on the type of attestation key selected and other platform or key data required to generate the
 * quote.  Once the application calls this API, it will use the returned p_quote_size to allocate the buffer
 * required to hold the generated quote.  A pointer to this buffer is provided to the ref_get_quote() API.
 *
 * If the key is not available, this API may return an error (SGX_QL_ATT_KEY_NOT_INITIALIZED) depending on
 * the algorithm.  In this case, the caller must call sgx_ql_init_quote() to re-generate and certify the
 * attestation key.
 *
 * @param p_att_key_id The selected attestation key ID from the quote verifier's list.  It includes the QE
 *                     identity as well as the attestation key's algorithm type. It cannot be NULL.
 * @param p_quote_size Pointer to the location where the required quote buffer size will be returned. Must
 *                     not be NULL.
 *
 * @return SGX_QL_SUCCESS Successfully calculated the required quote size. The required size in bytes is returned in the
 *         memory pointed to by p_quote_size.
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 * @return SGX_QL_ERROR_INVALID_PARAMETER Invalid parameter.  p_quote_size and p_att_key_id must not be NULL.
 * @return SGX_QL_ATT_KEY_NOT_INITIALIZED The platform quoting infrastructure does not have the attestation
 *         key available to generate quotes.  sgx_ql_init_quote() must be called again.
 * @return SGX_QL_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support the key
 *         described in p_att_key_id.  ///todo:  Add support for this error.
 * @return SGX_QL_ATT_KEY_CERT_DATA_INVALID The data returned by the platform library's sgx_ql_get_quote_config() is
 *         invalid.
 * @return SGX_QL_OUT_OF_EPC There is not enough EPC memory to load one of the Architecture Enclaves needed to complete
 *         this operation.
 * @return SGX_QL_ERROR_OUT_OF_MEMORY Heap memory allocation error in library or enclave.
 * @return SGX_QL_ENCLAVE_LOAD_ERROR Unable to load the enclaves required to initialize the attestation key.  Could be
 *         due to file I/O error, loading infrastructure error or insufficient enclave memory.
 * @return SGX_QL_ENCLAVE_LOST Enclave lost after power transition or used in child process created by linux:fork().
 *
 */
extern "C" quote3_error_t sgx_ql_get_quote_size(sgx_ql_att_key_id_t *p_att_key_id,
                                                uint32_t* p_quote_size)
{
    quote3_error_t ret_val = SGX_QL_ERROR_UNEXPECTED;
    sgx_status_t sgx_status;
    ECDSA256Quote ecdsa_quote;
    sgx_ql_att_key_id_t *p_local_att_key_id = NULL;

    //Verify inputs
    if(NULL == p_quote_size)
    {
        return(SGX_QL_ERROR_INVALID_PARAMETER);
    }

    // Dispatch the call based on the key_id.  Ref only supports ecdsa with ref QE.
    // Verify the Attestation key identity is supported.
    if(NULL != p_att_key_id)
    {
        if (0 != p_att_key_id->id)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (0 != p_att_key_id->version)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (32 != p_att_key_id->mrsigner_length)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (0 != memcmp(p_att_key_id->mrsigner, g_qe_mrsigner, 32))
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        else
        {
            if(1 != p_att_key_id->prod_id)
            {
                return(SGX_QL_ERROR_INVALID_PARAMETER);
            }
            if (SGX_QL_ALG_ECDSA_P256 != p_att_key_id->algorithm_id)
            {
                return(SGX_QL_ERROR_INVALID_PARAMETER);
            }
        }
        p_local_att_key_id = p_att_key_id;
    }
    else
    {
        p_local_att_key_id = (sgx_ql_att_key_id_t*)&g_default_ecdsa_p256_att_key_id;
    }

    // Choose the certification key type supported by the reference.
    sgx_ql_cert_key_type_t certification_key_type = SGX_QL_CERT_TYPE;

    ret_val = ecdsa_quote.get_quote_size(p_local_att_key_id,
                                         certification_key_type,
                                         p_quote_size);


    if(SGX_QL_SUCCESS != ret_val) {
        if((ret_val < SGX_QL_ERROR_MIN) ||
           (ret_val > SGX_QL_ERROR_MAX))
        {
            sgx_status = (sgx_status_t)ret_val;

            // Translate SDK errors
            switch(sgx_status)
            {
            case SGX_ERROR_OUT_OF_MEMORY:
                ret_val =  SGX_QL_ERROR_OUT_OF_MEMORY;
                break;

            case SGX_ERROR_ENCLAVE_FILE_ACCESS:
                ret_val = SGX_QL_ENCLAVE_LOAD_ERROR;
                break;

            // Unexpected enclave loading errorsReturn codes from load_qe
            case SGX_ERROR_INVALID_ENCLAVE:
            case SGX_ERROR_UNDEFINED_SYMBOL:
            case SGX_ERROR_MODE_INCOMPATIBLE:
            case SGX_ERROR_INVALID_METADATA:
            case SGX_ERROR_MEMORY_MAP_CONFLICT:
            case SGX_ERROR_INVALID_VERSION:
            case SGX_ERROR_INVALID_ATTRIBUTE:
            case SGX_ERROR_NDEBUG_ENCLAVE:
            case SGX_ERROR_INVALID_MISC:
            //case SE_ERROR_INVALID_LAUNCH_TOKEN:     ///todo: Internal error should be scrubbed before here.
            case SGX_ERROR_DEVICE_BUSY:
            case SGX_ERROR_NO_DEVICE:
            case SGX_ERROR_INVALID_SIGNATURE:
            //case SE_ERROR_INVALID_MEASUREMENT:      ///todo: Internal error should be scrubbed before here.
            //case SE_ERROR_INVALID_ISVSVNLE:         ///todo: Internal error should be scrubbed before here.
            case SGX_ERROR_INVALID_ENCLAVE_ID:
                ret_val = SGX_QL_ENCLAVE_LOAD_ERROR;
                break;
            case SGX_ERROR_SERVICE_INVALID_PRIVILEGE:
                ret_val = SGX_QL_ERROR_INVALID_PRIVILEGE;
                break;

            case SGX_ERROR_ENCLAVE_LOST:
                ret_val = SGX_QL_ENCLAVE_LOST;
                break;


            case SGX_ERROR_UNEXPECTED:
                ret_val = SGX_QL_ERROR_UNEXPECTED;
                break;


            default:
                ret_val = SGX_QL_ERROR_UNEXPECTED;
                break;
            }
        }
    }

    return(ret_val);
}

/**
 * This function is c-code wrapper for getting the quote. The function will take the application enclave's REPORT that
 * will be converted into a quote after the QE verifies the REPORT.  Once verified it will sign it with platform's
 * attestation key matching the selected attestation key ID.  If the key is not available, this API may return an error
 * (SGX_QL_ATT_KEY_NOT_INITIALIZED) depending on the algorithm.  In this case, the caller must call sgx_ql_init_quote()
 * to re-generate and certify the attestation key. an attestation key.
 *
 * The caller can request a REPORT from the QE using a supplied nonce.  This will allow the enclave requesting the quote
 * to verify the QE used to generate the quote. This makes it more difficult for something to spoof a QE and allows the
 * app enclave to catch it earlier.  But since the authenticity of the QE lies in knowledge of the Quote signing key,
 * such spoofing will ultimately be detected by the quote verifier.  QE REPORT.ReportData =
 * SHA256(*p_nonce||*p_quote)||32-0x00's.
 *
 * @param p_app_report Pointer to the enclave report that needs the quote. The report needs to be generated using the
 *                     QE's target info returned by the sgx_ql_init_quote() API.  Must not be NULL.
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
 * @return SGX_QL_SUCCESS Successfully generated the quote.
 * @return SGX_QL_ERROR_UNEXPECTED An unexpected internal error occurred.
 * @return SGX_QL_ERROR_INVALID_PARAMETER If either p_app_report or p_quote is null. Or, if quote_size isn't large
 *         enough, p_att_key_id is NULL.
 * @return SGX_QL_ATT_KEY_NOT_INITIALIZED The platform quoting infrastructure does not have the attestation key
 *         available to generate quotes.  sgx_ql_init_quote() must be called again.
 * @return SGX_QL_UNSUPPORTED_ATT_KEY_ID The platform quoting infrastructure does not support the key described in
 *         p_att_key_id.
 * @return SGX_QL_ATT_KEY_CERT_DATA_INVALID The data returned by the platform library's sgx_ql_get_quote_config() is
 *         invalid.
 * @return SGX_QL_OUT_OF_EPC There is not enough EPC memory to load one of the Architecture Enclaves needed to complete
 *         this operation.
 * @return SGX_QL_ERROR_OUT_OF_MEMORY Heap memory allocation error in library or enclave.
 * @return SGX_QL_ENCLAVE_LOAD_ERROR Unable to load the enclaves required to initialize the attestation key.  Could be
 *         due to file I/O error, loading infrastructure error or insufficient enclave memory.
 * @return SGX_QL_ENCLAVE_LOST Enclave lost after power transition or used in child process created by linux:fork().
 * @return SGX_QL_INVALID_REPORT Report MAC check failed on application report.
 * @return SGX_QL_UNABLE_TO_GENERATE_QE_REPORT The QE was unable to generate its own report targeting the application
 *         enclave either because the QE doesn't support this feature or there is an enclave compatibility issue.
 *         Please call again with the p_qe_report_info set to NULL.
 */
extern "C" quote3_error_t sgx_ql_get_quote(const sgx_report_t *p_app_report,
                                           sgx_ql_att_key_id_t *p_att_key_id,
                                           sgx_ql_qe_report_info_t *p_qe_report_info,
                                           uint8_t *p_quote,
                                           uint32_t quote_size)
{
    quote3_error_t ret_val = SGX_QL_ERROR_UNEXPECTED;
    sgx_status_t sgx_status;
    qe3_error_t qe3_error;
    ECDSA256Quote ecdsa_quote;
    sgx_ql_att_key_id_t *p_local_att_key_id = NULL;

    // Verify Inputs

    // Dispatch the call based on the key_id.  Ref only supports ecdsa with ref QE.
    // Verify the Attestation key identity is supported.
    if(NULL != p_att_key_id)
    {
        if (0 != p_att_key_id->id)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (0 != p_att_key_id->version)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (32 != p_att_key_id->mrsigner_length)
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        if (0 != memcmp(p_att_key_id->mrsigner, g_qe_mrsigner, 32))
        {
            return(SGX_QL_ERROR_INVALID_PARAMETER);
        }
        else
        {
            if(1 != p_att_key_id->prod_id)
            {
                return(SGX_QL_ERROR_INVALID_PARAMETER);
            }
            if (SGX_QL_ALG_ECDSA_P256 != p_att_key_id->algorithm_id)
            {
                return(SGX_QL_ERROR_INVALID_PARAMETER);
            }
        }
        p_local_att_key_id = p_att_key_id;
    }
    else
    {
        p_local_att_key_id = (sgx_ql_att_key_id_t*)&g_default_ecdsa_p256_att_key_id;
    }

    ret_val = ecdsa_quote.get_quote(p_app_report,
                                    p_local_att_key_id,
                                    p_qe_report_info,
                                    (sgx_quote3_t*)p_quote,
                                    quote_size);

    if(SGX_QL_SUCCESS != ret_val) {
        if((ret_val < SGX_QL_ERROR_MIN) ||
           (ret_val > SGX_QL_ERROR_MAX))
        {
            sgx_status = (sgx_status_t)ret_val;
            qe3_error = (qe3_error_t)ret_val;

            // Translate QE3 errors
            switch(qe3_error)
            {

            case REFQE3_ERROR_INVALID_PARAMETER:
                ret_val = SGX_QL_ERROR_INVALID_PARAMETER;
                break;

            case REFQE3_ERROR_INVALID_REPORT:
                ret_val = SGX_QL_INVALID_REPORT;
                break;

            case REFQE3_ERROR_CRYPTO:
                // Error generating QE_ID.  Shouldn't happen
                ret_val = SGX_QL_ERROR_UNEXPECTED;
                break;

            case REFQE3_ERROR_OUT_OF_MEMORY:
                ret_val = SGX_QL_ERROR_OUT_OF_MEMORY;
                break;

            case REFQE3_UNABLE_TO_GENERATE_QE_REPORT:
                ret_val = SGX_QL_UNABLE_TO_GENERATE_QE_REPORT;
                break;

            default:
                // Translate SDK errors
                switch (sgx_status)
                {
                case SGX_ERROR_INVALID_PARAMETER:
                    ret_val = SGX_QL_ERROR_INVALID_PARAMETER;
                    break;

                case SGX_ERROR_ENCLAVE_FILE_ACCESS:
                    ret_val = SGX_QL_ENCLAVE_LOAD_ERROR;
                    break;

                case SGX_ERROR_OUT_OF_MEMORY:
                    ret_val = SGX_QL_ERROR_OUT_OF_MEMORY;
                    break;

                case SGX_ERROR_ENCLAVE_LOST:
                    ret_val = SGX_QL_ENCLAVE_LOST;
                    break;

                    // Unexpected enclave loading errorsReturn codes from load_qe
                case SGX_ERROR_INVALID_ENCLAVE:
                case SGX_ERROR_UNDEFINED_SYMBOL:
                case SGX_ERROR_MODE_INCOMPATIBLE:
                case SGX_ERROR_INVALID_METADATA:
                case SGX_ERROR_MEMORY_MAP_CONFLICT:
                case SGX_ERROR_INVALID_VERSION:
                case SGX_ERROR_INVALID_ATTRIBUTE:
                case SGX_ERROR_NDEBUG_ENCLAVE:
                case SGX_ERROR_INVALID_MISC:
                    //case SE_ERROR_INVALID_LAUNCH_TOKEN:     ///todo: Internal error should be scrubbed before here.
                case SGX_ERROR_DEVICE_BUSY:
                case SGX_ERROR_NO_DEVICE:
                case SGX_ERROR_INVALID_SIGNATURE:
                    //case SE_ERROR_INVALID_MEASUREMENT:      ///todo: Internal error should be scrubbed before here.
                    //case SE_ERROR_INVALID_ISVSVNLE:         ///todo: Internal error should be scrubbed before here.
                case SGX_ERROR_INVALID_ENCLAVE_ID:
                    ret_val = SGX_QL_ENCLAVE_LOAD_ERROR;
                    break;
                case SGX_ERROR_SERVICE_INVALID_PRIVILEGE:
                    ret_val = SGX_QL_ERROR_INVALID_PRIVILEGE;
                    break;

                case SGX_ERROR_UNEXPECTED:
                    ret_val = SGX_QL_ERROR_UNEXPECTED;
                    break;

                default:
                    ret_val = SGX_QL_ERROR_UNEXPECTED;
                    break;
                }
                break;
            }
        }
    }

    return(ret_val);
}

extern "C" quote3_error_t sgx_ql_get_keyid(sgx_att_key_id_ext_t *p_att_key_id_ext)
{
    if(NULL == p_att_key_id_ext)
        return SGX_QL_ERROR_INVALID_PARAMETER;

    memset(p_att_key_id_ext, 0, sizeof(*p_att_key_id_ext));
    memcpy_s(&p_att_key_id_ext->base, sizeof(p_att_key_id_ext->base),
        &g_default_ecdsa_p256_att_key_id, sizeof(g_default_ecdsa_p256_att_key_id));
    p_att_key_id_ext->base.algorithm_id = SGX_QL_ALG_ECDSA_P256;
    p_att_key_id_ext->base.prod_id = 1;
    //p_att_key_id_ext.att_key_type and p_att_key_id_ext.reserved should be 0
    return SGX_QL_SUCCESS;
}


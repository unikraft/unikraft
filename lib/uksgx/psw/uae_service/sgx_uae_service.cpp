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

#include <stddef.h>
#include <oal/uae_oal_api.h>
#include <aesm_error.h>
#include "sgx_uae_launch.h"
#include "sgx_uae_epid.h"
#include "sgx_uae_quote_ex.h"
#include "uae_service_internal.h"
#include "config.h"

#include "stdint.h"
#include "se_sig_rl.h"

#if !defined(ntohl)
#define ntohl(u32)                                      \
  ((uint32_t)(((((const unsigned char*)&(u32))[0]) << 24)     \
            + ((((const unsigned char*)&(u32))[1]) << 16)     \
            + ((((const unsigned char*)&(u32))[2]) << 8)      \
            + (((const unsigned char*)&(u32))[3])))
#endif


#define GET_LAUNCH_TOKEN_TIMEOUT_MSEC (IPC_LATENCY)
#define SE_INIT_QUOTE_TIMEOUT_MSEC (IPC_LATENCY)
//add 3 millisecond per sig_rl entry
#define SE_GET_QUOTE_TIMEOUT_MSEC(p_sig_rl) (IPC_LATENCY + ((p_sig_rl) ? 3*ntohl(((const se_sig_rl_t*)p_sig_rl)->sig_rl.n2) : 0))
#define SE_REPORT_REMOTE_ATTESTATION_FAILURE_TIMEOUT_MSEC  (IPC_LATENCY)
#define SE_CHECK_UPDATE_STATUS_TIMEOUT_MSEC  (IPC_LATENCY)

#define GET_WHITE_LIST_SIZE_MSEC (IPC_LATENCY)
#define GET_WHITE_LIST_MSEC (IPC_LATENCY)
#define SGX_GET_EXTENDED_GROUP_ID_MSEC (IPC_LATENCY)
#define SGX_SWITCH_EXTENDED_GROUP_MSEC (IPC_LATENCY)
#define REG_WL_CERT_CHAIN_MSEC (IPC_LATENCY)
#define SE_CALC_QUOTE_SIZE_TIMEOUT_MSEC (IPC_LATENCY)
#define SE_SELECT_ATT_KEY_ID_TIMEOUT_MSEC (IPC_LATENCY)
#define SE_GET_SUPPORTED_ATT_ID_NUM_TIMEOUT_MSEC  (IPC_LATENCY)
#define SE_GET_SUPPORTED_ATT_IDS_TIMEOUT_MSEC (IPC_LATENCY)

extern "C" {

sgx_status_t get_launch_token(
    const enclave_css_t*        signature,
    const sgx_attributes_t*     attribute,
    sgx_launch_token_t*         launch_token)
{
    if (signature == NULL || attribute == NULL || launch_token == NULL)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t status = oal_get_launch_token(signature, attribute, launch_token, GET_LAUNCH_TOKEN_TIMEOUT_MSEC*1000, &result);

    /*common mappings */
    sgx_status_t mapped = oal_map_status(status);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        /*operation specific mapping */
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
                case AESM_NO_DEVICE_ERROR:
                    mapped = SGX_ERROR_NO_DEVICE;
                    break;
                case AESM_GET_LICENSETOKEN_ERROR:
                    mapped = SGX_ERROR_SERVICE_INVALID_PRIVILEGE;
                    break;
                case AESM_OUT_OF_EPC:
                    mapped = SGX_ERROR_OUT_OF_EPC;
                    break;
                default:
                    mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;
}

sgx_status_t sgx_init_quote(
    sgx_target_info_t       *p_target_info,
    sgx_epid_group_id_t     *p_gid)
{
    if (p_target_info == NULL || p_gid == NULL)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;

    uae_oal_status_t status = oal_init_quote(p_target_info, p_gid, SE_INIT_QUOTE_TIMEOUT_MSEC*1000, &result);

    sgx_status_t mapped = oal_map_status(status);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        /*operation specific mapping */
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
                case AESM_EPIDBLOB_ERROR:
                    mapped = SGX_ERROR_AE_INVALID_EPIDBLOB;
                    break;
                case AESM_EPID_REVOKED_ERROR:
                    mapped = SGX_ERROR_EPID_MEMBER_REVOKED;
                    break;
                case AESM_BACKEND_SERVER_BUSY:
                    mapped = SGX_ERROR_BUSY;
                    break;
                case AESM_SGX_PROVISION_FAILED:
                    mapped = SGX_ERROR_UNEXPECTED;
                    break;
                case AESM_OUT_OF_EPC:
                    mapped = SGX_ERROR_OUT_OF_EPC;
                    break;
                default:
                    mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;
}


sgx_status_t sgx_get_quote(
    const sgx_report_t      *p_report,
    sgx_quote_sign_type_t   quote_type,
    const sgx_spid_t        *p_spid,
    const sgx_quote_nonce_t *p_nonce,
    const uint8_t           *p_sig_rl,
    uint32_t                sig_rl_size,
    sgx_report_t            *p_qe_report,
    sgx_quote_t             *p_quote,
    uint32_t                quote_size)
{

    if (p_report == NULL || p_spid == NULL || p_quote == NULL || quote_size == 0 )
        return SGX_ERROR_INVALID_PARAMETER;
    if ((p_sig_rl == NULL && sig_rl_size != 0) ||
        (p_sig_rl != NULL && sig_rl_size == 0) )
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;

    uae_oal_status_t status = oal_get_quote(p_report, quote_type, p_spid, p_nonce, p_sig_rl, sig_rl_size, p_qe_report,
                                            p_quote, quote_size, SE_GET_QUOTE_TIMEOUT_MSEC(p_sig_rl)*1000, &result);

    sgx_status_t mapped = oal_map_status(status);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        /*operation specific mapping */
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
                case AESM_EPIDBLOB_ERROR:
                    mapped = SGX_ERROR_AE_INVALID_EPIDBLOB;
                    break;
                case AESM_EPID_REVOKED_ERROR:
                    mapped = SGX_ERROR_EPID_MEMBER_REVOKED;
                    break;
                case AESM_BACKEND_SERVER_BUSY:
                    mapped = SGX_ERROR_BUSY;
                    break;
                case AESM_SGX_PROVISION_FAILED:
                    mapped = SGX_ERROR_UNEXPECTED;
                    break;
                case AESM_OUT_OF_EPC:
                    mapped = SGX_ERROR_OUT_OF_EPC;
                    break;
                default:
                    mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;

}

sgx_status_t sgx_report_attestation_status(
    const sgx_platform_info_t*  p_platform_info,
    int                         attestation_status,
    sgx_update_info_bit_t*          p_update_info)
{
    if (p_platform_info == NULL || p_update_info == NULL)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;

    uae_oal_status_t status = oal_report_attestation_status(p_platform_info, attestation_status, p_update_info, SE_REPORT_REMOTE_ATTESTATION_FAILURE_TIMEOUT_MSEC*1000, &result);

    sgx_status_t mapped = oal_map_status(status);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        /*operation specific mapping */
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
                case AESM_BACKEND_SERVER_BUSY:
                    mapped = SGX_ERROR_BUSY;
                    break;
                case AESM_PLATFORM_INFO_BLOB_INVALID_SIG:
                    mapped = SGX_ERROR_INVALID_PARAMETER;
                    break;
                case AESM_EPIDBLOB_ERROR:
                    mapped = SGX_ERROR_AE_INVALID_EPIDBLOB;
                    break;
                case AESM_OUT_OF_EPC:
                    mapped = SGX_ERROR_OUT_OF_EPC;
                    break;
                case AESM_SGX_PROVISION_FAILED:
                default:
                    mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;
}

#define CHECK_UPDATE_STATUS_EPID_PROV		0x2
#define CHECK_UPDATE_STATUS_CERT_PROV_LTP	0x4
sgx_status_t SGXAPI sgx_check_update_status(
    const sgx_platform_info_t* p_platform_info,
    sgx_update_info_bit_t* p_update_info,
    uint32_t config,
    uint32_t* p_status)
{
    if ((NULL == p_platform_info && NULL != p_update_info) ||  // can't determine update status w/o PIB
        (NULL == p_platform_info && 0 == config)) {  // nothing to do without platform info
        return SGX_ERROR_INVALID_PARAMETER;
    }

    if (0 != (config & ~(CHECK_UPDATE_STATUS_EPID_PROV | CHECK_UPDATE_STATUS_CERT_PROV_LTP))) { // any unsupported bits in config input
        return SGX_ERROR_UNSUPPORTED_CONFIG;
    }

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;

    uae_oal_status_t status = oal_check_update_status(p_platform_info, p_update_info, config, p_status,
        SE_CHECK_UPDATE_STATUS_TIMEOUT_MSEC*1000, &result);

    sgx_status_t mapped = oal_map_status(status);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        /*operation specific mapping */
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
                case AESM_BACKEND_SERVER_BUSY:
                    mapped = SGX_ERROR_BUSY;
                    break;
                case AESM_PLATFORM_INFO_BLOB_INVALID_SIG:
                    mapped = SGX_ERROR_INVALID_PARAMETER;
                    break;
                case AESM_EPIDBLOB_ERROR:
                    mapped = SGX_ERROR_AE_INVALID_EPIDBLOB;
                    break;
                case AESM_OUT_OF_EPC:
                    mapped = SGX_ERROR_OUT_OF_EPC;
                    break;
                case AESM_CONFIG_UNSUPPORTED:
                    mapped = SGX_ERROR_UNSUPPORTED_CONFIG;
                    break;
                case AESM_SGX_PROVISION_FAILED:
                default:
                    mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;
}


sgx_status_t sgx_get_whitelist_size(
    uint32_t* p_whitelist_size)
{
    if (p_whitelist_size == NULL)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t ret = UAE_OAL_ERROR_UNEXPECTED;
    ret = oal_get_whitelist_size(p_whitelist_size, GET_WHITE_LIST_SIZE_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;
}


sgx_status_t sgx_get_whitelist(
    uint8_t* p_whitelist,
    uint32_t whitelist_size)
{
    if (p_whitelist == NULL || whitelist_size == 0)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t ret = UAE_OAL_ERROR_UNEXPECTED;

    ret = oal_get_whitelist(p_whitelist, whitelist_size, GET_WHITE_LIST_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }

    return mapped;
}

sgx_status_t sgx_get_extended_epid_group_id(
    uint32_t* p_extended_epid_group_id)
{
    if (p_extended_epid_group_id == NULL)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t ret = UAE_OAL_ERROR_UNEXPECTED;
    ret = oal_get_extended_epid_group_id(p_extended_epid_group_id, SGX_GET_EXTENDED_GROUP_ID_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;
}

sgx_status_t sgx_switch_extended_epid_group(uint32_t extended_epid_group_id)
{
    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t ret = UAE_OAL_ERROR_UNEXPECTED;
    ret = oal_switch_extended_epid_group(extended_epid_group_id, SGX_SWITCH_EXTENDED_GROUP_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;
}


typedef enum _sgx_register_type_t {SGX_REGISTER_WHITE_LIST_CERT} sgx_register_type_t;

sgx_status_t sgx_register_wl_cert_chain(uint8_t* p_wl_cert_chain, uint32_t wl_cert_chain_size)
{
    if (p_wl_cert_chain == NULL || wl_cert_chain_size == 0)
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_register_common(p_wl_cert_chain, wl_cert_chain_size, SGX_REGISTER_WHITE_LIST_CERT,
            REG_WL_CERT_CHAIN_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;
}

sgx_status_t sgx_select_att_key_id(const uint8_t *p_att_key_id_list, uint32_t att_key_id_list_size,
                                   sgx_att_key_id_t *p_selected_key_id)
{
    if (((NULL == p_att_key_id_list) && (0 != att_key_id_list_size))
        ||(NULL == p_selected_key_id))
        return SGX_ERROR_INVALID_PARAMETER;

    aesm_error_t result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_select_att_key_id(p_att_key_id_list, att_key_id_list_size, p_selected_key_id,
            SE_SELECT_ATT_KEY_ID_TIMEOUT_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            case AESM_UNSUPPORTED_ATT_KEY_ID:
                mapped = SGX_ERROR_UNSUPPORTED_ATT_KEY_ID;
                break;
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;
 }

sgx_status_t sgx_init_quote_ex(const sgx_att_key_id_t* p_att_key_id,
                                            sgx_target_info_t *p_qe_target_info,
                                            size_t* p_pub_key_id_size,
                                            uint8_t* p_pub_key_id)
{
    // Verify inputs
    if(NULL == p_pub_key_id_size || NULL == p_qe_target_info || NULL == p_att_key_id ||
        (NULL != p_pub_key_id && (0 == *p_pub_key_id_size || *p_pub_key_id_size >= UINT32_MAX)))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_init_quote_ex((sgx_att_key_id_t*)p_att_key_id, p_qe_target_info, p_pub_key_id_size, *p_pub_key_id_size, p_pub_key_id,
            SE_INIT_QUOTE_TIMEOUT_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            case AESM_UNSUPPORTED_ATT_KEY_ID:
                mapped = SGX_ERROR_UNSUPPORTED_ATT_KEY_ID;
                break;
            case AESM_KEY_CERTIFICATION_ERROR:
                mapped = SGX_ERROR_ATT_KEY_CERTIFICATION_FAILURE;
                break;
            case AESM_NO_PLATFORM_CERT_DATA:
                mapped = SGX_ERROR_PLATFORM_CERT_UNAVAILABLE;
                break;
            case AESM_EPIDBLOB_ERROR:
                mapped = SGX_ERROR_AE_INVALID_EPIDBLOB;
                break;
            case AESM_EPID_REVOKED_ERROR:
                mapped = SGX_ERROR_EPID_MEMBER_REVOKED;
                break;
            case AESM_BACKEND_SERVER_BUSY:
                mapped = SGX_ERROR_BUSY;
                break;
            case AESM_SGX_PROVISION_FAILED:
                mapped = SGX_ERROR_UNEXPECTED;
                break;
            case AESM_OUT_OF_EPC:
                mapped = SGX_ERROR_OUT_OF_EPC;
                break;
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;

}

sgx_status_t SGXAPI sgx_get_quote_size_ex(const sgx_att_key_id_t *p_att_key_id,
                                                uint32_t* p_quote_size)
{
    //Verify inputs
    if(NULL == p_quote_size)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_get_quote_size_ex((sgx_att_key_id_t*)p_att_key_id, p_quote_size,
            SE_CALC_QUOTE_SIZE_TIMEOUT_MSEC*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            case AESM_ATT_KEY_NOT_INITIALIZED:
                mapped = SGX_ERROR_ATT_KEY_UNINITIALIZED;
                break;
            case AESM_ATT_KEY_CERT_DATA_INVALID:
                mapped = SGX_ERROR_UNEXPECTED;
                break;
            case AESM_UNSUPPORTED_ATT_KEY_ID:
                mapped = SGX_ERROR_UNSUPPORTED_ATT_KEY_ID;
                break;
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;

}

sgx_status_t SGXAPI  sgx_get_quote_ex(const sgx_report_t *p_app_report,
                                           const sgx_att_key_id_t *p_att_key_id,
                                           sgx_qe_report_info_t *p_qe_report_info,
                                           uint8_t *p_quote,
                                           uint32_t quote_size)
{
    //Verify inputs
    if(NULL == p_app_report || NULL == p_att_key_id
        || NULL == p_quote || 0 == quote_size)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_get_quote_ex(p_app_report, (sgx_att_key_id_t*)p_att_key_id, p_qe_report_info, quote_size, p_quote,
            SE_GET_QUOTE_TIMEOUT_MSEC(NULL)*1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //operation specific mapping
        if (mapped == SGX_ERROR_UNEXPECTED && result != AESM_UNEXPECTED_ERROR)
        {
            switch (result)
            {
            case AESM_ATT_KEY_NOT_INITIALIZED:
                mapped = SGX_ERROR_ATT_KEY_UNINITIALIZED;
                break;
            case AESM_ATT_KEY_CERT_DATA_INVALID:
                mapped = SGX_ERROR_UNEXPECTED;
                break;
            case AESM_UNSUPPORTED_ATT_KEY_ID:
                mapped = SGX_ERROR_UNSUPPORTED_ATT_KEY_ID;
                break;
            case AESM_ERROR_REPORT:
            case AESM_INVALID_REPORT:
                mapped = SGX_ERROR_MAC_MISMATCH;
                break;
            case AESM_UNABLE_TO_GENERATE_QE_REPORT:
                mapped = SGX_ERROR_UNEXPECTED;
                break;
            case AESM_EPIDBLOB_ERROR:
                mapped = SGX_ERROR_AE_INVALID_EPIDBLOB;
                break;
            case AESM_EPID_REVOKED_ERROR:
                mapped = SGX_ERROR_EPID_MEMBER_REVOKED;
                break;
            case AESM_BACKEND_SERVER_BUSY:
                mapped = SGX_ERROR_BUSY;
                break;
            case AESM_SGX_PROVISION_FAILED:
                mapped = SGX_ERROR_UNEXPECTED;
                break;
            case AESM_OUT_OF_EPC:
                mapped = SGX_ERROR_OUT_OF_EPC;
                break;
            default:
                mapped = SGX_ERROR_UNEXPECTED;
            }
        }
    }
    return mapped;
}

sgx_status_t SGXAPI sgx_get_supported_att_key_id_num(uint32_t *p_att_key_id_num)
{
    //Verify inputs
    if (NULL == p_att_key_id_num)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_get_supported_att_key_id_num(p_att_key_id_num,
        SE_GET_SUPPORTED_ATT_ID_NUM_TIMEOUT_MSEC * 1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
         //No specific error code mapping
    }
    return mapped;

}

sgx_status_t SGXAPI sgx_get_supported_att_key_ids(sgx_att_key_id_ext_t *p_att_key_id_list, uint32_t att_key_id_num)
{
    //Verify inputs
    if (NULL == p_att_key_id_list || 0 == att_key_id_num ||
        UINT32_MAX / sizeof(sgx_att_key_id_ext_t) < att_key_id_num)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    uint32_t  att_key_id_list_size = (uint32_t)(att_key_id_num * sizeof(sgx_att_key_id_ext_t));

    aesm_error_t    result = AESM_UNEXPECTED_ERROR;
    uae_oal_status_t oal_ret = UAE_OAL_ERROR_UNEXPECTED;
    oal_ret = oal_get_supported_att_key_ids(p_att_key_id_list, att_key_id_list_size,
        SE_GET_SUPPORTED_ATT_IDS_TIMEOUT_MSEC * 1000, &result);

    //common mappings
    sgx_status_t mapped = oal_map_status(oal_ret);
    if (mapped != SGX_SUCCESS)
        return mapped;

    mapped = oal_map_result(result);
    if (mapped != SGX_SUCCESS)
    {
        //No specific error code mapping
    }
    return mapped;
}


// common mapper function for all OAL specific error codes

sgx_status_t    oal_map_status(uae_oal_status_t status)
{
    sgx_status_t retVal;

    switch (status)
    {
        case UAE_OAL_SUCCESS:
            retVal = SGX_SUCCESS;
            break;
        case UAE_OAL_ERROR_UNEXPECTED:
            retVal = SGX_ERROR_UNEXPECTED;
            break;
        case UAE_OAL_ERROR_AESM_UNAVAILABLE:
            retVal = SGX_ERROR_SERVICE_UNAVAILABLE;
            break;
        case UAE_OAL_ERROR_TIMEOUT:
            retVal = SGX_ERROR_SERVICE_TIMEOUT;
            break;
        case UAE_OAL_ERROR_INVALID:
            retVal = SGX_ERROR_INVALID_PARAMETER;
            break;
        default:
            retVal = SGX_ERROR_UNEXPECTED;
    }

    return retVal;
}

sgx_status_t    oal_map_result(aesm_error_t result)
{
    sgx_status_t retVal = SGX_ERROR_UNEXPECTED;

    switch (result)
    {
        case AESM_SUCCESS:
            retVal = SGX_SUCCESS;
            break;
        case AESM_UPDATE_AVAILABLE:
            retVal = SGX_ERROR_UPDATE_NEEDED;
            break;
        case AESM_UNEXPECTED_ERROR:
            retVal = SGX_ERROR_UNEXPECTED;
            break;
        case AESM_PARAMETER_ERROR:
            retVal = SGX_ERROR_INVALID_PARAMETER;
            break;
        case AESM_SERVICE_STOPPED:
        case AESM_SERVICE_UNAVAILABLE:
        case AESM_ENCLAVE_LOAD_ERROR:
        case AESM_ENCLAVE_LOST:
            retVal = SGX_ERROR_SERVICE_UNAVAILABLE;
            break;
        case AESM_OUT_OF_MEMORY_ERROR:
            retVal = SGX_ERROR_OUT_OF_MEMORY;
            break;
        case AESM_BUSY:
            retVal = SGX_ERROR_BUSY;
            break;
        case AESM_UNRECOGNIZED_PLATFORM:
            retVal = SGX_ERROR_UNRECOGNIZED_PLATFORM;
            break;
        case AESM_NETWORK_ERROR:
        case AESM_NETWORK_BUSY_ERROR:
        case AESM_PROXY_SETTING_ASSIST:
            retVal = SGX_ERROR_NETWORK_FAILURE;
            break;
        case AESM_NO_DEVICE_ERROR:
            retVal = SGX_ERROR_NO_DEVICE;
            break;
        default:
            retVal = SGX_ERROR_UNEXPECTED;
    }

    return retVal;

}

} /* extern "C" */

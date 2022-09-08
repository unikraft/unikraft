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



#include "ref_le.h"
#include "byte_order.h"
#include "sgx_utils.h"
#include "sgx_trts.h"
#include "metadata.h"
#include "se_memcpy.h"

#include "ref_le_t.h"
#include <stdlib.h> /* for malloc/free etc */

#define REF_LE_WL_MAX_NUM_OF_RECORDS        512
#define REF_LE_WL_CURRENT_VERSION           0x0100 /*big endian*/

// staticly allocate the white list cache - all the values in the cache are in little endian
static uint8_t g_buffer[REF_LE_WL_SIZE(REF_LE_WL_MAX_NUM_OF_RECORDS)] = { 0 };
static ref_le_white_list_t *g_ref_le_white_list_cache = (ref_le_white_list_t*)g_buffer;

static void copy_reversed_byte_array(uint8_t *dst, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        dst[i] = src[size - i - 1];
    }
}

// calculate the white list expected size based on entry count
static size_t ref_le_get_white_list_size(const ref_le_white_list_t* val)
{
    if (val == NULL) {
        return 0;
    }

    uint16_t entries_count = _ntohs(val->entries_count);

    if (entries_count > REF_LE_WL_MAX_NUM_OF_RECORDS || entries_count == 0) {
        // list cannot be empty nor can be larget than max records
        return 0;
    }

    return REF_LE_WL_SIZE(entries_count);
}

// this function gets a white list and signature, verifies the correctness and signature of the
// white list and updates the white list cache in case of success
int ref_le_init_white_list(const ref_le_white_list_t* p_ref_le_white_list, uint32_t ref_le_white_list_size, const sgx_rsa3072_signature_t* p_ref_le_white_list_signature)
{
    sgx_status_t sgx_stat = SGX_SUCCESS;

    if (p_ref_le_white_list == NULL || ref_le_white_list_size < (uint32_t)sizeof(ref_le_white_list_t) || p_ref_le_white_list_signature == NULL)
    {
        return LE_INVALID_PARAMETER;
    }

    uint32_t data_size = (uint32_t)ref_le_get_white_list_size(p_ref_le_white_list);
    if (data_size == 0 || data_size != ref_le_white_list_size ||
        data_size > REF_LE_WL_SIZE(REF_LE_WL_MAX_NUM_OF_RECORDS)) 
    {
        // calculated size must match the declared one and 0 is illegal
        // also checking that it is no more than the max size as a defence in depth
        return LE_INVALID_PARAMETER;
    }

    if (!sgx_is_within_enclave(p_ref_le_white_list, ref_le_white_list_size) || 
        !sgx_is_within_enclave(p_ref_le_white_list_signature, sizeof(sgx_rsa3072_signature_t)))
    {
        return LE_INVALID_PARAMETER;
    }

    // Format version should be 1 (big endian)
    if (p_ref_le_white_list->version != REF_LE_WL_CURRENT_VERSION)
    {
        return LE_INVALID_PARAMETER;
    }

    // ===== version checking =====

    uint32_t wl_version = _ntohl(p_ref_le_white_list->wl_version);
    if (g_ref_le_white_list_cache->wl_version >= wl_version)
    {
        // white list version must be newer or equal to the existing
        return LE_WHITE_LIST_ALREADY_UPDATED;
    }

    // ===== verify list signer =====

    sgx_rsa3072_public_key_t temp_public_key;
    copy_reversed_byte_array((uint8_t*)&(temp_public_key.exp), (const uint8_t*)&(p_ref_le_white_list->signer_pubkey.exp), sizeof(temp_public_key.exp));
    copy_reversed_byte_array((uint8_t*)&(temp_public_key.mod), (const uint8_t*)&(p_ref_le_white_list->signer_pubkey.mod), sizeof(temp_public_key.mod));

    // calculate the hash of the provided public key
    sgx_sha256_hash_t signer_hash;
    sgx_stat = sgx_sha256_msg(temp_public_key.mod, SGX_RSA3072_KEY_SIZE, &signer_hash);
    if (sgx_stat != SGX_SUCCESS)
    {
        return LE_UNEXPECTED_ERROR;
    }

    // get self report - signer pubkey must match the self signing key aquired in report
    sgx_report_t report;
    sgx_stat = sgx_create_report(NULL, NULL, &report);
    if (sgx_stat != SGX_SUCCESS)
    {
        return LE_UNEXPECTED_ERROR;
    }

    // compare the signer of the LE acquired from the report to the one provided in the list
    if (memcmp(&(report.body.mr_signer), &signer_hash, sizeof(signer_hash)) != 0)
    {
        return LE_INVALID_PARAMETER;
    }

    // ===== verify signature =====

    // create a little-endian copy of the signature 
    sgx_rsa3072_signature_t temp_signature;
    copy_reversed_byte_array((uint8_t*)&temp_signature, (const uint8_t*)p_ref_le_white_list_signature, sizeof(*p_ref_le_white_list_signature));

    // verify the signed white list
	sgx_rsa_result_t verify_result = SGX_RSA_INVALID_SIGNATURE;
    sgx_stat = sgx_rsa3072_verify((const uint8_t *)p_ref_le_white_list, data_size, &temp_public_key, &temp_signature, &verify_result);
    if (sgx_stat != SGX_SUCCESS)
    {
        return LE_UNEXPECTED_ERROR;
    }
    if (verify_result != SGX_RSA_VALID)
    {
        return LE_INVALID_PARAMETER;
    }


    // ===== update the white list cache =====

    // clear the existing records
    memset(g_ref_le_white_list_cache, 0, REF_LE_WL_SIZE(REF_LE_WL_MAX_NUM_OF_RECORDS));

    // copy white list as little endian to the cache
    // the count was verified earilier in this function when verified the size
    uint16_t entries_count = _ntohs(p_ref_le_white_list->entries_count);

    for (uint32_t i = 0; i < entries_count; ++i)
    {
        (g_ref_le_white_list_cache->wl_entries[i]).provision_key = ((p_ref_le_white_list->wl_entries[i]).provision_key) ? 1 : 0;
        (g_ref_le_white_list_cache->wl_entries[i]).match_mr_enclave = ((p_ref_le_white_list->wl_entries[i]).match_mr_enclave) ? 1 : 0;
        copy_reversed_byte_array((uint8_t*)&((g_ref_le_white_list_cache->wl_entries[i]).mr_signer), 
            (const uint8_t*)&((p_ref_le_white_list->wl_entries[i]).mr_signer), 
            sizeof((g_ref_le_white_list_cache->wl_entries[i]).mr_signer));

        if ((g_ref_le_white_list_cache->wl_entries[i]).match_mr_enclave)
        {
            copy_reversed_byte_array((uint8_t*)&((g_ref_le_white_list_cache->wl_entries[i]).mr_enclave),
                (const uint8_t*)&((p_ref_le_white_list->wl_entries[i]).mr_enclave), 
                sizeof((g_ref_le_white_list_cache->wl_entries[i]).mr_enclave));
        }
    }

    g_ref_le_white_list_cache->entries_count = entries_count;
    g_ref_le_white_list_cache->wl_version = wl_version;

    return AE_SUCCESS;
}

// this function gets the enclave information and provides token if priviledge is valid
int ref_le_get_launch_token(const sgx_measurement_t* mrenclave, const sgx_measurement_t* mrsigner, const sgx_attributes_t* se_attributes, token_t* lictoken)
{
    int result = AE_SUCCESS;

    if (NULL == mrenclave || NULL == mrsigner || NULL == se_attributes || NULL == lictoken)
    {
        return LE_INVALID_PARAMETER;
    }

    if (!sgx_is_within_enclave(mrenclave, sizeof(sgx_measurement_t)) ||
        !sgx_is_within_enclave(mrsigner, sizeof(sgx_measurement_t)) ||
        !sgx_is_within_enclave(se_attributes, sizeof(sgx_attributes_t)) ||
        !sgx_is_within_enclave(lictoken, sizeof(token_t)))
    {
        return LE_INVALID_PARAMETER;
    }

    // ===== verify priviledge =====

    // lookup for launch priviledges
    bool valid_priviledge = false;

    uint8_t provision = (se_attributes->flags & SGX_FLAGS_PROVISION_KEY) ? 1 : 0;

    for (uint16_t i = 0; i < g_ref_le_white_list_cache->entries_count; ++i)
    {
        ref_le_white_list_entry_t *current_entry = &(g_ref_le_white_list_cache->wl_entries[i]);
        if ((current_entry->provision_key || !provision) &&
            (memcmp(&(current_entry->mr_signer), mrsigner, sizeof(sgx_measurement_t)) == 0) &&
                (!current_entry->match_mr_enclave ||
                (memcmp(&(current_entry->mr_enclave), mrenclave, sizeof(sgx_measurement_t)) == 0))
            )
        {
            valid_priviledge = true;
            break;
        }
    }

    if (!valid_priviledge)
    {
        return LE_INVALID_PRIVILEGE_ERROR;
    }

    // ===== init EINIT token values =====

    // initial EINIT Token and set 0 for all reserved area
    memset(lictoken, 0, sizeof(*lictoken));

    // set EINIT Token valid
    lictoken->body.valid = 1;

    // set EINIT Token mrenclave
    memcpy(&lictoken->body.mr_enclave, mrenclave, sizeof(lictoken->body.mr_enclave));

    // set EINIT Token mrsigner
    memcpy(&lictoken->body.mr_signer, mrsigner, sizeof(lictoken->body.mr_signer));

    // set EINIT Token attributes
    memcpy(&lictoken->body.attributes, se_attributes, sizeof(lictoken->body.attributes));

    // set EINIT Token with platform information from EREPORT

    // create report to get current cpu_svn and isv_svn.
    sgx_report_t report;
    memset(&report, 0, sizeof(report));
    sgx_status_t sgx_stat = sgx_create_report(NULL, NULL, &report);
    if (sgx_stat != SGX_SUCCESS)
    {
        return LE_UNEXPECTED_ERROR;
    }

    memcpy(&lictoken->cpu_svn_le, &(report.body.cpu_svn), sizeof(lictoken->cpu_svn_le));
    lictoken->isv_svn_le = report.body.isv_svn;
    lictoken->isv_prod_id_le = report.body.isv_prod_id;
    lictoken->masked_misc_select_le = report.body.misc_select & DEFAULT_MISC_MASK;
    lictoken->attributes_le.flags = report.body.attributes.flags & ~SGX_FLAGS_MODE64BIT;
    lictoken->attributes_le.xfrm = 0;

    // equire random
    sgx_stat = sgx_read_rand((uint8_t*)&lictoken->key_id, sizeof(sgx_key_id_t));
    if (sgx_stat != SGX_SUCCESS)
    {
        memset_s(lictoken, sizeof(token_t), 0, sizeof(*lictoken));
        return LE_UNEXPECTED_ERROR;
    }

    // Create key request
    sgx_key_request_t key_request;
    memset(&key_request, 0, sizeof(key_request));
    key_request.key_name = SGX_KEYSELECT_EINITTOKEN;
    key_request.attribute_mask.xfrm = 0;
    key_request.attribute_mask.flags = ~SGX_FLAGS_MODE64BIT;
    key_request.misc_mask = DEFAULT_MISC_MASK;
    key_request.isv_svn = lictoken->isv_svn_le;
    memcpy(&key_request.key_id, &lictoken->key_id, sizeof(key_request.key_id));
    memcpy(&key_request.cpu_svn, &(lictoken->cpu_svn_le), sizeof(key_request.cpu_svn));

    sgx_cmac_state_handle_t p_cmac_handle = NULL;
    sgx_key_128bit_t launch_key;

    // ===== calculate the EINIT token =====

    do
    {
        // call EGETKEY
        sgx_stat = sgx_get_key(&key_request, &launch_key);
        if (sgx_stat != SGX_SUCCESS)
        {
            result = LE_GET_EINITTOKEN_KEY_ERROR;
            break;
        }

        // generate MAC for the token with the aquired key
        sgx_stat = sgx_cmac128_init(&launch_key, &p_cmac_handle);
        if (sgx_stat != SGX_SUCCESS)
        {
            result = AE_FAILURE;
            break;
        }

        sgx_stat = sgx_cmac128_update((uint8_t*)&lictoken->body, sizeof(lictoken->body), p_cmac_handle);
        if (sgx_stat != SGX_SUCCESS)
        {
            result = AE_FAILURE;
            break;
        }

        sgx_stat = sgx_cmac128_final(p_cmac_handle, (sgx_cmac_128bit_tag_t*)&lictoken->mac);
        if (sgx_stat != SGX_SUCCESS)
        {
            result = AE_FAILURE;
            break;
        }
    } while (0);

    // ===== wrap up =====

    // clear launch_key after being used
    memset_s(&launch_key, sizeof(sgx_key_128bit_t), 0, sizeof(launch_key));

    // close CMAC handle
    if (p_cmac_handle != NULL)
    {
        sgx_cmac128_close(p_cmac_handle);
    }

    // on failure, clear the EINIT token
    if (result != AE_SUCCESS)
    {
        memset_s(lictoken, sizeof(token_t), 0, sizeof(*lictoken));
    }

    return result;
}



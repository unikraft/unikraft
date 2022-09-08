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

#include <sgx_secure_align.h>
#include <sgx_random_buffers.h>
#include "pce_cert.h"
#include "pce_t.c"
#include "aeerror.h"
#include "sgx_utils.h"
#include "sgx_lfence.h"
#include "byte_order.h"
#include "pve_qe_common.h"
#include "arch.h"
#include <assert.h>

ae_error_t get_ppid(ppid_t* ppid);
ae_error_t get_pce_priv_key(const psvn_t* psvn, sgx_ec256_private_t* wrap_key);
#define PCE_RSA_SEED_SIZE 32

#define RSA_MOD_SIZE 384 //hardcode n size to be 384
#define RSA_E_SIZE 4 //hardcode e size to be 4

se_static_assert(RSA_MOD_SIZE == PEK_MOD_SIZE);

//Function to generate Current isvsvn from REPORT
static ae_error_t get_isv_svn(sgx_isv_svn_t* isv_svn)
{
    sgx_status_t se_ret = SGX_SUCCESS;

    sgx_report_t report;
    memset(&report, 0, sizeof(report));
    se_ret = sgx_create_report(NULL, NULL, &report);
    if(SGX_SUCCESS != se_ret){
        (void)memset_s(&report,sizeof(report), 0, sizeof(report));
        return PCE_UNEXPECTED_ERROR;
    }
    memcpy(isv_svn, &report.body.isv_svn, sizeof(report.body.isv_svn));
    (void)memset_s(&report, sizeof(report), 0, sizeof(report));
    return AE_SUCCESS;
}

//always assume the format of public_key is module n of RSA public key followed by 4 bytes e and both n and e are in Big Endian
uint32_t get_pc_info(const sgx_report_t* report,
    const uint8_t *public_key, uint32_t key_size,
    uint8_t crypto_suite,
    uint8_t *encrypted_ppid, uint32_t encrypted_ppid_buf_size,
    uint32_t *encrypted_ppid_out_size,
    pce_info_t *pce_info,
    uint8_t *signature_scheme)
{
    if (report == NULL ||
        public_key == NULL ||
        encrypted_ppid == NULL ||
        encrypted_ppid_out_size == NULL ||
        pce_info == NULL||
        signature_scheme == NULL){
            return AE_INVALID_PARAMETER;
    }
    if(ALG_RSA_OAEP_3072!=crypto_suite){//The only crypto suite supported in RSA 3072 where 384 bytes module n is used
        return AE_INVALID_PARAMETER;
    }

    //RSA public key is mod || e
    if (RSA_MOD_SIZE + RSA_E_SIZE != key_size)
    {
        return AE_INVALID_PARAMETER;
    }

	//
	// if this mispredicts, we might go past end of 
	// public_key below
	//
    sgx_lfence();

    *encrypted_ppid_out_size = 0;//set out_size to 0. IFF all operations pass, the out_size will be set to RSA_MOD_SIZE
    if (encrypted_ppid_buf_size < RSA_MOD_SIZE){
        return AE_INSUFFICIENT_DATA_IN_BUFFER;
    }
    if(SGX_SUCCESS != sgx_verify_report(report)){
        return PCE_INVALID_REPORT;
    }
    //only PvE and QE3 could use the interface which has flag SGX_FLAGS_PROVISION_KEY
    if((report->body.attributes.flags & SGX_FLAGS_PROVISION_KEY) != SGX_FLAGS_PROVISION_KEY){
        return PCE_INVALID_PRIVILEGE;
    }
    uint8_t hash_buf[SGX_REPORT_DATA_SIZE];//hash value only use 32 bytes but data in report has 64 bytes size
    se_static_assert(sizeof(hash_buf)>=sizeof(sgx_sha256_hash_t));
    memset(hash_buf, 0, sizeof(hash_buf));

    sgx_sha_state_handle_t sha_handle = NULL;
    sgx_status_t sgx_ret = SGX_ERROR_UNEXPECTED;
    do
    {
        sgx_ret = sgx_sha256_init(&sha_handle);
        if (SGX_SUCCESS != sgx_ret)
            break;
        sgx_ret = sgx_sha256_update(&crypto_suite, sizeof(uint8_t), sha_handle);
        if (SGX_SUCCESS != sgx_ret)
            break;
        sgx_ret = sgx_sha256_update(public_key, RSA_MOD_SIZE + RSA_E_SIZE, sha_handle);
        if (SGX_SUCCESS != sgx_ret)
            break;
        sgx_ret = sgx_sha256_get_hash(sha_handle, reinterpret_cast<sgx_sha256_hash_t *>(hash_buf));
    } while (0);
    if (sha_handle != NULL)
        sgx_sha256_close(sha_handle);
    if (SGX_ERROR_OUT_OF_MEMORY == sgx_ret){
        return AE_OUT_OF_MEMORY_ERROR;
    }
    else if (SGX_SUCCESS != sgx_ret){
        return AE_FAILURE;
    }

    //verify the report data is SHA256(crypto_suite||public_key)||0-padding
    if(memcmp(hash_buf, &report->body.report_data, sizeof(report->body.report_data))!=0){
        return PCE_CRYPTO_ERROR;
    }

    ppid_t ppid_buf;
    uint32_t little_endian_e = 0;
    uint8_t *le_n = NULL;
    void *key = NULL;
    ae_error_t ae_ret = AE_FAILURE;
    size_t temp_encrypted_size = 0;
    do {
        //get ppid
        //
        if (get_ppid(&ppid_buf) != AE_SUCCESS) {
            break;
        }

        //create public exponent value represented in little endian
        //
        little_endian_e = lv_ntohl(*(public_key + RSA_MOD_SIZE));

        le_n = (uint8_t *)malloc(RSA_MOD_SIZE);
        if (le_n == NULL) {
            ae_ret = AE_OUT_OF_MEMORY_ERROR;
            break;
        }

        for (size_t i = 0; i<RSA_MOD_SIZE; i++) {
            le_n[i] = *(public_key + RSA_MOD_SIZE - 1 - i);//create little endian n
        }

        //create RSA public key with le_n modulus and little_endian_e exponent
        //
        if (sgx_create_rsa_pub1_key(RSA_MOD_SIZE, RSA_E_SIZE, (const unsigned char *)le_n,
            (const unsigned char *)(&little_endian_e), &key) != SGX_SUCCESS) {
            ae_ret = AE_FAILURE;
            break;
        }

        //get expected cipher text length
        //
        if (sgx_rsa_pub_encrypt_sha256(key, NULL, &temp_encrypted_size,
            (ppid_buf.ppid), sizeof(ppid_buf.ppid)) != SGX_SUCCESS) {
            ae_ret = AE_FAILURE;
            break;
        }

        //validate out size match RSA_MOD_SIZE
        //
        if (temp_encrypted_size != RSA_MOD_SIZE) {
            ae_ret = AE_FAILURE;
            break;
        }

        //encrypt ppid, using RSA public key into encrypted_ppid buffer
        //
        if (sgx_rsa_pub_encrypt_sha256(key, encrypted_ppid, &temp_encrypted_size,
            (ppid_buf.ppid), sizeof(ppid_buf.ppid)) != SGX_SUCCESS) {
            ae_ret = AE_FAILURE;
            break;
        }

        //validate out size match RSA_MOD_SIZE
        //
        if (temp_encrypted_size != RSA_MOD_SIZE) {
            ae_ret = AE_FAILURE;
            break;
        }

        //get ISV and SVN values
        //
        if ((ae_ret = get_isv_svn(&pce_info->pce_isvn)) != AE_SUCCESS) {
            break;
    }

    *encrypted_ppid_out_size = RSA_MOD_SIZE;
    pce_info->pce_id = CUR_PCE_ID;
    *signature_scheme = NIST_P256_ECDSA_SHA256;
    ae_ret = AE_SUCCESS;
    } while (0);

    //free public modulus buffer, clear temporary ppid buffer, clear output buffer in case of any error occured
    //
    if (le_n != NULL)
        free(le_n);
    sgx_free_rsa_key(key, SGX_RSA_PUBLIC_KEY, RSA_MOD_SIZE, RSA_E_SIZE);
    memset_s(&ppid_buf, sizeof(ppid_buf), 0, sizeof(ppid_t));
    if (AE_SUCCESS != ae_ret) {
        memset_s(encrypted_ppid, encrypted_ppid_buf_size, 0, encrypted_ppid_buf_size);
    }
    return ae_ret;
}

uint32_t get_pc_info_without_ppid(pce_info_t *pce_info)
{
    if (pce_info == NULL) {
        return AE_INVALID_PARAMETER;
    }

    ae_error_t ae_ret = AE_FAILURE;
    if ((ae_ret = get_isv_svn(&pce_info->pce_isvn)) != AE_SUCCESS) {
        return ae_ret;
    }

    pce_info->pce_id = CUR_PCE_ID;
    
    return ae_ret;
}


uint32_t certify_enclave(const psvn_t* cert_psvn,
                         const sgx_report_t* report,
                         uint8_t *signature,
                         uint32_t signature_buf_size,
                         uint32_t *signature_out_size)
{
    if(cert_psvn==NULL||
        report==NULL||
        signature == NULL||
        signature_out_size == NULL){
            return AE_INVALID_PARAMETER;
    }
    if(signature_buf_size < sizeof(sgx_ec256_signature_t)){
        *signature_out_size = sizeof(sgx_ec256_signature_t);
        return AE_INSUFFICIENT_DATA_IN_BUFFER;
    }

    ae_error_t ae_ret = AE_FAILURE;
    sgx_ecc_state_handle_t handle=NULL;

    using cec_prv_key = randomly_placed_object<sgx::custom_alignment_aligned < sgx_ec256_private_t, sizeof(sgx_ec256_private_t), 0, sizeof(sgx_ec256_private_t)>>;
    cec_prv_key oec_prv_key_buf;	
    auto* oec_prv_key = oec_prv_key_buf.instantiate_object();	
    auto* pec_prv_key = &oec_prv_key->v;
    sgx_ec256_public_t  ec_pub_key;
    uint8_t verify_result = SGX_EC_INVALID_SIGNATURE;

    sgx_status_t sgx_status = SGX_SUCCESS;

    if(SGX_SUCCESS != sgx_verify_report(report)){
        return PCE_INVALID_REPORT;
    }
    //only PvE and QE3 could use the interface which has flag SGX_FLAGS_PROVISION_KEY
    if((report->body.attributes.flags & SGX_FLAGS_PROVISION_KEY) != SGX_FLAGS_PROVISION_KEY){
        return PCE_INVALID_PRIVILEGE;
    }
    ae_ret = random_stack_advance(get_pce_priv_key, cert_psvn, pec_prv_key);
    if(AE_SUCCESS!=ae_ret){
        goto ret_point;
    }
    SWAP_ENDIAN_32B(pec_prv_key);
    sgx_status = sgx_ecc256_open_context(&handle);
    if (SGX_ERROR_OUT_OF_MEMORY == sgx_status)
    {
        ae_ret = AE_OUT_OF_MEMORY_ERROR;
        goto ret_point;
    }
    else if (SGX_SUCCESS != sgx_status) {
        ae_ret = AE_FAILURE;
        goto ret_point;
    }

    sgx_status = sgx_ecdsa_sign(reinterpret_cast<const uint8_t *>(&report->body), sizeof(report->body),
        pec_prv_key, reinterpret_cast<sgx_ec256_signature_t *>(signature), handle);
    if (SGX_ERROR_OUT_OF_MEMORY == sgx_status)
    {
        ae_ret = AE_OUT_OF_MEMORY_ERROR;
        goto ret_point;
    }
    else if (SGX_SUCCESS != sgx_status) {
        ae_ret = AE_FAILURE;
        goto ret_point;
    }
    sgx_status = sgx_ecc256_calculate_pub_from_priv(pec_prv_key, &ec_pub_key);
    if (SGX_SUCCESS != sgx_status) {
        ae_ret = AE_FAILURE;
        goto ret_point;
    }

    sgx_status = sgx_ecdsa_verify(reinterpret_cast<const uint8_t *>(&report->body),
        sizeof(report->body),
        &ec_pub_key,
        reinterpret_cast<sgx_ec256_signature_t *>(signature),
        &verify_result,
        handle);
    if (SGX_SUCCESS != sgx_status || SGX_EC_VALID != verify_result) {
        ae_ret = AE_FAILURE;
        goto ret_point;
    }
    //swap from little endian used in sgx_crypto to big endian used in network byte order
    SWAP_ENDIAN_32B(reinterpret_cast<sgx_ec256_signature_t *>(signature)->x);
    SWAP_ENDIAN_32B(reinterpret_cast<sgx_ec256_signature_t *>(signature)->y);

    *signature_out_size = sizeof(sgx_ec256_signature_t);
    ae_ret = AE_SUCCESS;
ret_point:
    (void)memset_s(pec_prv_key, sizeof(*pec_prv_key),0,sizeof(*pec_prv_key));
    if(handle!=NULL){
        sgx_ecc256_close_context(handle);
    }
    if(AE_SUCCESS != ae_ret){
        if(SGX_SUCCESS != sgx_read_rand((unsigned char*)signature, signature_buf_size))
            (void)memset_s(signature, signature_buf_size, 0, sizeof(sgx_ec256_signature_t));
    }
    return ae_ret;
}

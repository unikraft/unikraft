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
* File:
*     sgx_rsa_encryption.cpp
* Description:
*     Wrapper for rsa operation functions
*
*/
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "sgx_error.h"
#include "sgx_trts.h"
#include "ipp_wrapper.h"


sgx_status_t sgx_create_rsa_key_pair(int n_byte_size, int e_byte_size, unsigned char *p_n, unsigned char *p_d, unsigned char *p_e,
    unsigned char *p_p, unsigned char *p_q, unsigned char *p_dmp1,
    unsigned char *p_dmq1, unsigned char *p_iqmp)
{
    if (n_byte_size <= 0 || e_byte_size <= 0 || p_n == NULL || p_d == NULL || p_e == NULL ||
        p_p == NULL || p_q == NULL || p_dmp1 == NULL || p_dmq1 == NULL || p_iqmp == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsRSAPrivateKeyState *p_pri_key = NULL;
    IppStatus error_code = ippStsNoErr;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    IppsPrimeState *p_prime = NULL;
    Ipp8u * scratch_buffer = NULL;
    int pri_size = 0, scratch_buffer_size = 0;
    IppsBigNumState *bn_n = NULL, *bn_e = NULL, *bn_d = NULL, *bn_e_s = NULL, *bn_p = NULL, *bn_q = NULL, *bn_dmp1 = NULL, *bn_dmq1 = NULL, *bn_iqmp = NULL;
    int size = 0;
    IppsBigNumSGN sgn = IppsBigNumPOS;

    do {

        //create a new prime number generator
        //
        error_code = sgx_ipp_newPrimeGen(n_byte_size * 8 / 2, &p_prime);
        ERROR_BREAK(error_code);

        //allocate and init private key of type 2
        //
        error_code = ippsRSA_GetSizePrivateKeyType2(n_byte_size / 2 * 8, n_byte_size / 2 * 8, &pri_size);
        ERROR_BREAK(error_code);
        p_pri_key = (IppsRSAPrivateKeyState *)malloc(pri_size);
        if (!p_pri_key)
        {
            error_code = ippStsMemAllocErr;
            break;
        }
        error_code = ippsRSA_InitPrivateKeyType2(n_byte_size / 2 * 8, n_byte_size / 2 * 8, p_pri_key, pri_size);
        ERROR_BREAK(error_code);

        //allocate scratch buffer, to be used as temp buffer
        //
        error_code = ippsRSA_GetBufferSizePrivateKey(&scratch_buffer_size, p_pri_key);
        ERROR_BREAK(error_code);
        scratch_buffer = (Ipp8u *)malloc(scratch_buffer_size);
        if (!scratch_buffer)
        {
            error_code = ippStsMemAllocErr;
            break;
        }
        memset(scratch_buffer, 0, scratch_buffer_size);

        //allocate and initialize RSA BNs
        //
        error_code = sgx_ipp_newBN((const Ipp32u*)p_e, e_byte_size, &bn_e_s);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size, &bn_n);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, e_byte_size, &bn_e);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size, &bn_d);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size / 2, &bn_p);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size / 2, &bn_q);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size / 2, &bn_dmp1);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size / 2, &bn_dmq1);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN(NULL, n_byte_size / 2, &bn_iqmp);
        ERROR_BREAK(error_code);

        //generate RSA key components with n_byte_size modulus and p_e public exponent
        //
        do {
            // generate keys
            // ippsRSA_GenerateKeys() may return ippStsInsufficientEntropy.
            // In that case, we need to retry the API
            error_code = ippsRSA_GenerateKeys(bn_e_s,
                bn_n,
                bn_e,
                bn_d,
                p_pri_key,
                scratch_buffer,
                1,
                p_prime,
                sgx_ipp_DRNGen,
                NULL);
        } while (error_code == ippStsInsufficientEntropy);
        ERROR_BREAK(error_code);

        //extract private key components into BNs
        //
        error_code = ippsRSA_GetPrivateKeyType2(bn_p,
            bn_q,
            bn_dmp1,
            bn_dmq1,
            bn_iqmp,
            p_pri_key);
        ERROR_BREAK(error_code);

        //extract RSA components from BNs into output buffers
        //
        error_code = ippsGetSize_BN(bn_n, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_n, bn_n);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_e, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_e, bn_e);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_d, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_d, bn_d);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_p, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_p, bn_p);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_q, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_q, bn_q);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_dmp1, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_dmp1, bn_dmp1);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_dmq1, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_dmq1, bn_dmq1);
        ERROR_BREAK(error_code);
        error_code = ippsGetSize_BN(bn_iqmp, &size);
        ERROR_BREAK(error_code);
        error_code = ippsGet_BN(&sgn, &size, (Ipp32u*)p_iqmp, bn_iqmp);
        ERROR_BREAK(error_code);

        ret_code = SGX_SUCCESS;

    } while (0);

    sgx_ipp_secure_free_BN(bn_e_s, e_byte_size);
    sgx_ipp_secure_free_BN(bn_e, e_byte_size);
    sgx_ipp_secure_free_BN(bn_d, n_byte_size);
    sgx_ipp_secure_free_BN(bn_n, n_byte_size);
    sgx_ipp_secure_free_BN(bn_p, n_byte_size / 2);
    sgx_ipp_secure_free_BN(bn_q, n_byte_size / 2);
    sgx_ipp_secure_free_BN(bn_dmp1, n_byte_size / 2);
    sgx_ipp_secure_free_BN(bn_dmq1, n_byte_size / 2);
    sgx_ipp_secure_free_BN(bn_iqmp, n_byte_size / 2);

    SAFE_FREE_MM(p_prime);
    secure_free_rsa_pri_key(p_pri_key);
    CLEAR_FREE_MEM(scratch_buffer, scratch_buffer_size);

    if (error_code == ippStsMemAllocErr)
        ret_code = SGX_ERROR_OUT_OF_MEMORY;
    return ret_code;
}

sgx_status_t sgx_create_rsa_priv2_key(int mod_size, int exp_size, const unsigned char *p_rsa_key_e, const unsigned char *p_rsa_key_p, const unsigned char *p_rsa_key_q,
    const unsigned char *p_rsa_key_dmp1, const unsigned char *p_rsa_key_dmq1, const unsigned char *p_rsa_key_iqmp,
    void **new_pri_key2)
{
    (void)(exp_size);
    (void)(p_rsa_key_e);
    IppsRSAPrivateKeyState *p_rsa2 = NULL;
    IppsBigNumState *p_p = NULL, *p_q = NULL, *p_dmp1 = NULL, *p_dmq1 = NULL, *p_iqmp = NULL;
    int rsa2_size = 0;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    if (mod_size <= 0 || p_rsa_key_p == NULL || p_rsa_key_q == NULL || p_rsa_key_dmp1 == NULL || p_rsa_key_dmq1 == NULL || p_rsa_key_iqmp == NULL || new_pri_key2 == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppStatus error_code = ippStsNoErr;
    do {

        //generate and assign RSA components BNs
        //
        error_code = sgx_ipp_newBN((const Ipp32u*)p_rsa_key_p, mod_size / 2, &p_p);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN((const Ipp32u*)p_rsa_key_q, mod_size / 2, &p_q);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN((const Ipp32u*)p_rsa_key_dmp1, mod_size / 2, &p_dmp1);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN((const Ipp32u*)p_rsa_key_dmq1, mod_size / 2, &p_dmq1);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN((const Ipp32u*)p_rsa_key_iqmp, mod_size / 2, &p_iqmp);
        ERROR_BREAK(error_code);

        //allocate and initialize private key of type 2
        //
        error_code = ippsRSA_GetSizePrivateKeyType2(mod_size / 2 * 8, mod_size / 2 * 8, &rsa2_size);
        ERROR_BREAK(error_code);
        p_rsa2 = (IppsRSAPrivateKeyState *)malloc(rsa2_size);
        if (!p_rsa2)
        {
            error_code = ippStsMemAllocErr;
            break;
        }
        error_code = ippsRSA_InitPrivateKeyType2(mod_size / 2 * 8, mod_size / 2 * 8, p_rsa2, rsa2_size);
        ERROR_BREAK(error_code);

        //setup private key with values of input components
        //
        error_code = ippsRSA_SetPrivateKeyType2(p_p, p_q, p_dmp1, p_dmq1, p_iqmp, p_rsa2);
        ERROR_BREAK(error_code);
        *new_pri_key2 = (void*)p_rsa2;

        ret_code = SGX_SUCCESS;
    } while (0);

    sgx_ipp_secure_free_BN(p_p, mod_size / 2);
    sgx_ipp_secure_free_BN(p_q, mod_size / 2);
    sgx_ipp_secure_free_BN(p_dmp1, mod_size / 2);
    sgx_ipp_secure_free_BN(p_dmq1, mod_size / 2);
    sgx_ipp_secure_free_BN(p_iqmp, mod_size / 2);

    if (error_code == ippStsMemAllocErr) {
        ret_code = SGX_ERROR_OUT_OF_MEMORY;
    }

    if (ret_code != SGX_SUCCESS) {
        secure_free_rsa_pri_key(p_rsa2);
    }
    return ret_code;
}

sgx_status_t sgx_create_rsa_pub1_key(int mod_size, int exp_size, const unsigned char *le_n, const unsigned char *le_e, void **new_pub_key1)
{
    if (new_pub_key1 == NULL || mod_size <= 0 || exp_size <= 0 || le_n == NULL || le_e == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsRSAPublicKeyState *p_pub_key = NULL;
    IppsBigNumState *p_n = NULL, *p_e = NULL;
    int rsa_size = 0;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    IppStatus error_code = ippStsNoErr;
    do {

        //generate and assign RSA components BNs
        //
        error_code = sgx_ipp_newBN((const Ipp32u*)le_n, mod_size, &p_n);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN((const Ipp32u*)le_e, exp_size, &p_e);
        ERROR_BREAK(error_code);

        //allocate and initialize public key
        //
        error_code = ippsRSA_GetSizePublicKey(mod_size * 8, exp_size * 8, &rsa_size);
        ERROR_BREAK(error_code);
        p_pub_key = (IppsRSAPublicKeyState *)malloc(rsa_size);
        if (!p_pub_key)
        {
            error_code = ippStsMemAllocErr;
            break;
        }
        error_code = ippsRSA_InitPublicKey(mod_size * 8, exp_size * 8, p_pub_key, rsa_size);
        ERROR_BREAK(error_code);

        //setup public key with values of input components
        //
        error_code = ippsRSA_SetPublicKey(p_n, p_e, p_pub_key);
        ERROR_BREAK(error_code);

        *new_pub_key1 = (void*)p_pub_key;
        ret_code = SGX_SUCCESS;
    } while (0);

    sgx_ipp_secure_free_BN(p_n, mod_size);
    sgx_ipp_secure_free_BN(p_e, exp_size);

    if (error_code == ippStsMemAllocErr)
        ret_code = SGX_ERROR_OUT_OF_MEMORY;

    if (ret_code != SGX_SUCCESS) {
        secure_free_rsa_pub_key(mod_size, exp_size, p_pub_key);
    }

    return ret_code;
}

sgx_status_t sgx_rsa_pub_encrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data,
    const size_t pin_len) {
    if (rsa_key == NULL || pout_len == NULL || pin_data == NULL || pin_len < 1 || pin_len >= INT_MAX) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsBigNumState* p_modulus = NULL;
    int mod_len = 0;
    uint8_t *p_scratch_buffer = NULL;
    Ipp8u seeds[RSA_SEED_SIZE_SHA256] = { 0 };
    int scratch_buff_size = 0;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;

    do {
        //
        //create a new BN
        //
        if (sgx_ipp_newBN(NULL, MAX_IPP_BN_LENGTH, &p_modulus) != ippStsNoErr) {
            break;
        }
        //get public key modulus
        //
        if (ippsRSA_GetPublicKey(p_modulus, NULL, (IppsRSAPublicKeyState*)rsa_key) != ippStsNoErr) {
            break;
        }
        //get modulus length in bits
        //
        if (ippsExtGet_BN(0, &mod_len, 0, p_modulus) != ippStsNoErr) {
            break;
        }
        if (pout_data == NULL) {
            // return required pout_data buffer size
            *pout_len = mod_len / 8;
            ret_code = SGX_SUCCESS;
            break;
        }
        else if (*pout_len < (size_t)(mod_len / 8)) {
            ret_code = SGX_ERROR_INVALID_PARAMETER;
            break;
        }

        //get scratch buffer size, to be used as temp buffer, and allocate it
        //
        if (ippsRSA_GetBufferSizePublicKey(&scratch_buff_size, (IppsRSAPublicKeyState*)rsa_key) != ippStsNoErr) {
            break;
        }
        p_scratch_buffer = (uint8_t *)malloc(scratch_buff_size);
        if (!p_scratch_buffer)
        {
            ret_code = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }
        memset(p_scratch_buffer, 0, scratch_buff_size);

        //get random seed
        //
        if (sgx_read_rand(seeds, RSA_SEED_SIZE_SHA256) != SGX_SUCCESS) {
            break;
        }

        //encrypt input data with public rsa_key and SHA256 padding
        //
        if (ippsRSAEncrypt_OAEP_rmf(pin_data, (int)pin_len, NULL, 0, seeds,
            pout_data, (IppsRSAPublicKeyState*)rsa_key, ippsHashMethod_SHA256_TT(), p_scratch_buffer) != ippStsNoErr) {
            break;
        }
	*pout_len = mod_len / 8;

        ret_code = SGX_SUCCESS;
    } while (0);

    memset_s(seeds, RSA_SEED_SIZE_SHA256, 0, RSA_SEED_SIZE_SHA256);
    CLEAR_FREE_MEM(p_scratch_buffer, scratch_buff_size);
    sgx_ipp_secure_free_BN(p_modulus, MAX_IPP_BN_LENGTH);
    return ret_code;
}
sgx_status_t sgx_rsa_priv_decrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data,
    const size_t pin_len) {
    (void)(pin_len);
    if (rsa_key == NULL || pout_len == NULL || pin_data == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsBigNumState* p_bn = NULL;
    int dataLen = 0;
    int factor = 1;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    uint8_t *p_scratch_buffer = NULL;
    int scratch_buff_size = 0;

    do {
        //create a new BN
        //
        if (sgx_ipp_newBN(NULL, MAX_IPP_BN_LENGTH, &p_bn) != ippStsNoErr)
        {
            break;
        }
        //get private key modulus or prime factor P
        //
        if (ippsRSA_GetPrivateKeyType1(p_bn, NULL, (IppsRSAPrivateKeyState*)rsa_key) != ippStsNoErr)
        {
            if (ippsRSA_GetPrivateKeyType2(p_bn, NULL, NULL, NULL, NULL, (IppsRSAPrivateKeyState*)rsa_key) != ippStsNoErr)
            {
                break;
            }
            else
            {
                //we're working with prime number and not modulus, need to multiply length by 2
                //
                factor = 2;
            }
        }
        //get modulus or prime factor P bits length
        //
        if (ippsExtGet_BN(0, &dataLen, 0, p_bn) != ippStsNoErr)
        {
            break;
        }
        
        // output buffer is NULL, return required pout_data buffer size
        //
        if (pout_data == NULL) {
            //calculate pout_len based on RSA factors size and return.
            // convert bits to bytes, in case of working with P, multiply by factor=2.
            //
            *pout_len = dataLen / 8 * factor;
            ret_code = SGX_SUCCESS;
            break;
        }
        else if(*pout_len < (size_t)(dataLen / 8 * factor))
        {
            ret_code = SGX_ERROR_INVALID_PARAMETER;
            break;
        }

        //get scratch buffer size, to be used as temp buffer, and allocate it
        //
        if (ippsRSA_GetBufferSizePrivateKey(&scratch_buff_size, (IppsRSAPrivateKeyState*)rsa_key) != ippStsNoErr) {
            break;
        }
        p_scratch_buffer = (uint8_t *)malloc(scratch_buff_size);
        if (!p_scratch_buffer)
        {
            ret_code = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        //decrypt input ciphertext using private key rsa_key
        if (ippsRSADecrypt_OAEP_rmf(pin_data, NULL, 0, pout_data, (int*)pout_len, (IppsRSAPrivateKeyState*)rsa_key,
            ippsHashMethod_SHA256_TT(), p_scratch_buffer) != ippStsNoErr) {
            break;
        }
        ret_code = SGX_SUCCESS;

    } while (0);
    CLEAR_FREE_MEM(p_scratch_buffer, scratch_buff_size);
    sgx_ipp_secure_free_BN(p_bn, MAX_IPP_BN_LENGTH);

    return ret_code;
}

sgx_status_t sgx_create_rsa_priv1_key(int n_byte_size, int e_byte_size, int d_byte_size, const unsigned char *le_n, const unsigned char *le_e,
    const unsigned char *le_d, void **new_pri_key1)
{
    if (n_byte_size <= 0 || e_byte_size <= 0 || d_byte_size <= 0 || new_pri_key1 == NULL ||
        le_n == NULL || le_e == NULL || le_d == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsRSAPrivateKeyState *p_rsa1 = NULL;
    IppsBigNumState *p_n = NULL, *p_d = NULL;
    int rsa1_size = 0;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;

    IppStatus error_code = ippStsErr;
    do {

        //generate and assign RSA components BNs
        //
        error_code = sgx_ipp_newBN((const Ipp32u*)le_n, n_byte_size, &p_n);
        ERROR_BREAK(error_code);
        error_code = sgx_ipp_newBN((const Ipp32u*)le_d, d_byte_size, &p_d);
        ERROR_BREAK(error_code);

        //allocate and init private key of type 1
        //
        error_code = ippsRSA_GetSizePrivateKeyType1(n_byte_size * 8, d_byte_size * 8, &rsa1_size);
        if (error_code != ippStsNoErr || rsa1_size <= 0) {
            break;
        }
        p_rsa1 = (IppsRSAPrivateKeyState *)malloc(rsa1_size);
        if (!p_rsa1)
        {
            error_code = ippStsMemAllocErr;
            break;
        }
        error_code = ippsRSA_InitPrivateKeyType1(n_byte_size * 8, d_byte_size * 8, p_rsa1, rsa1_size);
        ERROR_BREAK(error_code);

        //setup private key with values of input components
        //
        error_code = ippsRSA_SetPrivateKeyType1(p_n, p_d, p_rsa1);
        ERROR_BREAK(error_code);

        *new_pri_key1 = p_rsa1;
        ret_code = SGX_SUCCESS;

    } while (0);

    sgx_ipp_secure_free_BN(p_n, n_byte_size);
    sgx_ipp_secure_free_BN(p_d, d_byte_size);
    if (ret_code != SGX_SUCCESS) {
        secure_free_rsa_pri_key(p_rsa1);
        if (error_code == ippStsMemAllocErr) {
            ret_code = SGX_ERROR_OUT_OF_MEMORY;
        }
    }

    return ret_code;
}


sgx_status_t sgx_free_rsa_key(void *p_rsa_key, sgx_rsa_key_type_t key_type, int mod_size, int exp_size) {
	if (key_type == SGX_RSA_PRIVATE_KEY) {
		(void)(exp_size);
		secure_free_rsa_pri_key((IppsRSAPrivateKeyState*)p_rsa_key);
	} else if (key_type == SGX_RSA_PUBLIC_KEY) {
		secure_free_rsa_pub_key(mod_size, exp_size, (IppsRSAPublicKeyState*)p_rsa_key);
	}

	return SGX_SUCCESS;
}


sgx_status_t sgx_calculate_ecdsa_priv_key(const unsigned char* hash_drg, int hash_drg_len,
    const unsigned char* sgx_nistp256_r_m1, int sgx_nistp256_r_m1_len,
    unsigned char* out_key, int out_key_len) {

    if (out_key == NULL || hash_drg_len <= 0 || sgx_nistp256_r_m1_len <= 0 ||
        out_key_len <= 0 || hash_drg == NULL || sgx_nistp256_r_m1 == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    IppStatus ipp_status = ippStsNoErr;
    IppsBigNumState *bn_d = NULL;
    IppsBigNumState *bn_m = NULL;
    IppsBigNumState *bn_o = NULL;
    IppsBigNumState *bn_one = NULL;
    Ipp32u i = 1;

    do {

        //allocate and initialize BNs
        //
        ipp_status = sgx_ipp_newBN(reinterpret_cast<const Ipp32u *>(hash_drg), hash_drg_len, &bn_d);
        ERROR_BREAK(ipp_status);

        //generate mod to be n-1 where n is order of ECC Group
        //
        ipp_status = sgx_ipp_newBN(reinterpret_cast<const Ipp32u *>(sgx_nistp256_r_m1), sgx_nistp256_r_m1_len, &bn_m);
        ERROR_BREAK(ipp_status);

        //allocate memory for output BN
        //
        ipp_status = sgx_ipp_newBN(NULL, sgx_nistp256_r_m1_len, &bn_o);
        ERROR_BREAK(ipp_status);

        //create big number with value of 1
        //
        ipp_status = sgx_ipp_newBN(&i, sizeof(Ipp32u), &bn_one);
        ERROR_BREAK(ipp_status);

        //calculate output's BN value
        ipp_status = ippsMod_BN(bn_d, bn_m, bn_o);
        ERROR_BREAK(ipp_status)

        //increase by 1
        //
        ipp_status = ippsAdd_BN(bn_o, bn_one, bn_o);
        ERROR_BREAK(ipp_status);

        /*Unmatched size*/
        if (sgx_nistp256_r_m1_len != sizeof(sgx_ec256_private_t)) {
            break;
        }

        //convert BN_o into octet string
        ipp_status = ippsGetOctString_BN(reinterpret_cast<Ipp8u *>(out_key), sgx_nistp256_r_m1_len, bn_o);//output data in bigendian order
        ERROR_BREAK(ipp_status);

        ret_code = SGX_SUCCESS;
    } while (0);

    sgx_ipp_secure_free_BN(bn_d, hash_drg_len);
    sgx_ipp_secure_free_BN(bn_m, sgx_nistp256_r_m1_len);
    sgx_ipp_secure_free_BN(bn_o, sgx_nistp256_r_m1_len);
    sgx_ipp_secure_free_BN(bn_one, sizeof(uint32_t));

    if (ipp_status == ippStsMemAllocErr)
        ret_code = SGX_ERROR_OUT_OF_MEMORY;

    if (ret_code != SGX_SUCCESS) {
        (void)memset_s(out_key, out_key_len, 0, out_key_len);
    }

    return ret_code;
}

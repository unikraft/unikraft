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

#include "ssl_crypto.h"
#include "ssl_compat_wrapper.h"
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <assert.h>
#include <se_memcpy.h>

sgx_status_t sgx_create_rsa_pub1_key(int mod_size, int exp_size, const unsigned char *le_n, const unsigned char *le_e, void **new_pub_key)
{
    if (new_pub_key == NULL || mod_size <= 0 || exp_size <= 0 || le_n == NULL || le_e == NULL) 
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    EVP_PKEY *rsa_key = NULL;
    RSA *rsa_ctx = NULL;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    BIGNUM* n = NULL;
    BIGNUM* e = NULL;

    do 
    {
        //convert input buffers to BNs
        //
        n = BN_lebin2bn(le_n, mod_size, n);
        BN_CHECK_BREAK(n);
        e = BN_lebin2bn(le_e, exp_size, e);
        BN_CHECK_BREAK(e);

        // allocates and initializes an RSA key structure
        //
        rsa_ctx = RSA_new();
        rsa_key = EVP_PKEY_new();

        if (rsa_ctx == NULL || rsa_key == NULL || !EVP_PKEY_assign_RSA(rsa_key, rsa_ctx))
        {
            RSA_free(rsa_ctx);
            rsa_ctx = NULL;
            break;
        }

        //set n, e values of RSA key
        //Calling set functions transfers the memory management of input BNs to the RSA object,
        //and therefore the values that have been passed in should not be freed by the caller after these functions has been called.
        //
        if (!RSA_set0_key(rsa_ctx, n, e, NULL))
        {
                break;
        }
        *new_pub_key = rsa_key;
        ret_code = SGX_SUCCESS;
    } while (0);

    if (ret_code != SGX_SUCCESS)
    {
        EVP_PKEY_free(rsa_key);
        BN_clear_free(n);
        BN_clear_free(e);
    }

    return ret_code;
}


sgx_status_t sgx_free_rsa_key(void *p_rsa_key, sgx_rsa_key_type_t key_type, int mod_size, int exp_size)
{
    (void)(key_type);
    (void)(mod_size);
    (void)(exp_size);
    if (p_rsa_key != NULL)
    {
        EVP_PKEY_free((EVP_PKEY*)p_rsa_key);
    }
    return SGX_SUCCESS;
}

sgx_status_t rsa3072_verify(const uint8_t *p_data,
    uint32_t data_size,
    const void *p_pub_key,
    const sgx_rsa3072_signature_t *p_signature,
    sgx_rsa_result_t *p_result)
{
    if ((p_data == NULL) || (data_size < 1) || (p_pub_key == NULL) ||
        (p_signature == NULL) || (p_result == NULL))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    *p_result = SGX_RSA_INVALID_SIGNATURE;

    sgx_status_t retval = SGX_ERROR_UNEXPECTED;
    int verified = 0;
    const EVP_MD* sha256_md = NULL;
    EVP_MD_CTX *ctx = NULL;

    do {
        // allocates, initializes and returns a digest context
        //
        ctx = EVP_MD_CTX_new();
        if (ctx == NULL)
        {
            retval = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        // return EVP_MD structures for SHA256 digest algorithm */
        //
        sha256_md = EVP_sha256();
        if (sha256_md == NULL)
        {
            break;
        }

        // sets up verification context ctx to use digest type
        //
        if (EVP_DigestVerifyInit(ctx, NULL, sha256_md, NULL, reinterpret_cast<EVP_PKEY *>(const_cast<void*>(p_pub_key))) <= 0)
        {
            break;
        }

        // hashes data_size bytes of data at p_data into the verification context ctx.
        // this function can be called several times on the same ctx to hash additional data
        //
        if (EVP_DigestVerifyUpdate(ctx, (const void *)p_data, data_size) <= 0)
        {
            break;
        }

        // verifies the data in ctx against the signature in p_signature of length SGX_RSA3072_KEY_SIZE
        //
        verified = EVP_DigestVerifyFinal(ctx, (const unsigned char *)p_signature, SGX_RSA3072_KEY_SIZE);
        if (verified == 1 || verified == 0)
        {
            retval = SGX_SUCCESS;
            if(verified == 1)
            {
                *p_result = SGX_RSA_VALID;
            }

        }
    } while (0);

    if (ctx)
        EVP_MD_CTX_free(ctx);
    
    return retval;
}


sgx_status_t sgx_rsa_pub_encrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data, const size_t pin_len) 
{
    if (rsa_key == NULL || pout_len == NULL || pin_data == NULL || pin_len < 1 || pin_len >= INT_MAX)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
    EVP_PKEY_CTX *ctx = NULL;

    do {
        //allocate and init PKEY_CTX
        //
        ctx = EVP_PKEY_CTX_new((EVP_PKEY*)rsa_key, NULL);
        if ((ctx == NULL) || (EVP_PKEY_encrypt_init(ctx) < 1))
        {
                break;
        }

        //set the RSA padding mode, init it to use SHA256
        //
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());

        if (EVP_PKEY_encrypt(ctx, pout_data, pout_len, pin_data, pin_len) <= 0)
        {
                break;
        }

        ret_code = SGX_SUCCESS;
    } while (0);

    EVP_PKEY_CTX_free(ctx);
    return ret_code;
}


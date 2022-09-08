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
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>

/*
* Elliptic Curve Cryptography - Based on GF(p), 256 bit
*/
/* Allocates and initializes ecc context
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
*   Output: sgx_ecc_state_handle_t *p_ecc_handle - Pointer to the handle of ECC crypto system  */
sgx_status_t sgx_ecc256_open_context(sgx_ecc_state_handle_t* p_ecc_handle)
{
    if (p_ecc_handle == NULL)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    sgx_status_t retval = SGX_SUCCESS;

    /* construct a curve p-256 */
    EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    if (NULL == ec_group)
    {
        retval = SGX_ERROR_UNEXPECTED;
    }
    else
    {
        *p_ecc_handle = (void*)ec_group;
    }
    return retval;
}

/* Cleans up ecc context
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
*   Output: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system  */
sgx_status_t sgx_ecc256_close_context(sgx_ecc_state_handle_t ecc_handle)
{
    if (ecc_handle == NULL)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    EC_GROUP_free((EC_GROUP*)ecc_handle);

    return SGX_SUCCESS;
}

/* Verifies the signature for the given data based on the public key
*
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
*   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system
*           sgx_ec256_public_t *p_public - Pointer to the public key
*           uint8_t *p_data - Pointer to the data to be signed
*           uint32_t data_size - Size of the data to be signed
*           sgx_ec256_signature_t *p_signature - Pointer to the signature
*   Output: uint8_t *p_result - Pointer to the result of verification check  */
sgx_status_t sgx_ecdsa_verify(const uint8_t *p_data,
                              uint32_t data_size,
                              const sgx_ec256_public_t *p_public,
                              const sgx_ec256_signature_t *p_signature,
                              uint8_t *p_result,
                              sgx_ecc_state_handle_t ecc_handle)
{
    if ((ecc_handle == NULL) || (p_public == NULL) || (p_signature == NULL) ||
        (p_data == NULL) || (data_size < 1) || (p_result == NULL))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    EC_KEY *public_key = NULL;
    BIGNUM *bn_pub_x = NULL;
    BIGNUM *bn_pub_y = NULL;
    BIGNUM *bn_r = NULL;
    BIGNUM *bn_s = NULL;
    EC_POINT *public_point = NULL;
    ECDSA_SIG *ecdsa_sig = NULL;
    unsigned char digest[SGX_SHA256_HASH_SIZE] = { 0 };
    sgx_status_t retval = SGX_ERROR_UNEXPECTED;
    int valid = 0;

    *p_result = SGX_EC_INVALID_SIGNATURE;


    do {
        // converts the x value of public key, represented as positive integer in little-endian into a BIGNUM
        //
        bn_pub_x = BN_lebin2bn((unsigned char*)p_public->gx, sizeof(p_public->gx), 0);
        if (NULL == bn_pub_x)
        {
            break;
        }

        // converts the y value of public key, represented as positive integer in little-endian into a BIGNUM
        //
        bn_pub_y = BN_lebin2bn((unsigned char*)p_public->gy, sizeof(p_public->gy), 0);
        if (NULL == bn_pub_y)
        {
            break;
        }

        // converts the x value of the signature, represented as positive integer in little-endian into a BIGNUM
        //
        bn_r = BN_lebin2bn((unsigned char*)p_signature->x, sizeof(p_signature->x), 0);
        if (NULL == bn_r)
        {
            break;
        }

        // converts the y value of the signature, represented as positive integer in little-endian into a BIGNUM
        //
        bn_s = BN_lebin2bn((unsigned char*)p_signature->y, sizeof(p_signature->y), 0);
        if (NULL == bn_s)
        {
            break;
        }

        // creates new point and assigned the group object that the point relates to
        //
        public_point = EC_POINT_new((EC_GROUP*)ecc_handle);
        if (public_point == NULL)
        {
            retval = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        // sets point based on public key's x,y coordinates
        //
        if (1 != EC_POINT_set_affine_coordinates_GFp((EC_GROUP*)ecc_handle, public_point, bn_pub_x, bn_pub_y, NULL))
        {
            break;
        }

        // check point if the point is on curve
        //
        if (1 != EC_POINT_is_on_curve((EC_GROUP*)ecc_handle, public_point, NULL)) 
        {
            break;
        }

        // create empty ecc key
        //
        public_key = EC_KEY_new();
        if (NULL == public_key)
        {
            retval = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        // sets ecc key group (set curve)
        //
        if (1 != EC_KEY_set_group(public_key, (EC_GROUP*)ecc_handle))
        {
            break;
        }

        // uses the created point to set the public key value
        //
        if (1 != EC_KEY_set_public_key(public_key, public_point))
        {
            break;
        }

        /* generates digest of p_data */
        if (NULL == SHA256((const unsigned char *)p_data, data_size, (unsigned char *)digest))
        {
            break;
        }

        // allocates a new ECDSA_SIG structure (note: this function also allocates the BIGNUMs) and initialize it
        //
        ecdsa_sig = ECDSA_SIG_new();
        if (NULL == ecdsa_sig)
        {
            retval = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        // setes the r and s values of ecdsa_sig
        // calling this function transfers the memory management of the values to the ECDSA_SIG object,
        // and therefore the values that have been passed in should not be freed directly after this function has been called
        //
        if (1 != ECDSA_SIG_set0(ecdsa_sig, bn_r, bn_s))
        {
            ECDSA_SIG_free(ecdsa_sig);
            ecdsa_sig = NULL;
            break;
        }

        // verifies that the signature ecdsa_sig is a valid ECDSA signature of the hash value digest of size SGX_SHA256_HASH_SIZE using the public key public_key
        //
        valid = ECDSA_do_verify(digest, SGX_SHA256_HASH_SIZE, ecdsa_sig, public_key);
        if (-1 == valid)
        {
            break;
        }

        // sets the p_result based on ECDSA_do_verify result
        //
        if (valid)
        {
            *p_result = SGX_EC_VALID;
        }

        retval = SGX_SUCCESS;
    } while(0);

    if (bn_pub_x)
        BN_clear_free(bn_pub_x);
    if (bn_pub_y)
        BN_clear_free(bn_pub_y);
    if (public_point)
        EC_POINT_clear_free(public_point);
    if (ecdsa_sig)
    {
        ECDSA_SIG_free(ecdsa_sig);
        bn_r = NULL;
        bn_s = NULL;
    }
    if (public_key)
        EC_KEY_free(public_key);
    if (bn_r)
        BN_clear_free(bn_r);
    if (bn_s)
        BN_clear_free(bn_s);

    return retval;
}


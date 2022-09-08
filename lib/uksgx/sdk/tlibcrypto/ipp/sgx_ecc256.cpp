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


#include "ipp_wrapper.h"

#define ECC_FIELD_SIZE 256

/*
* Elliptic Curve Crytpography - Based on GF(p), 256 bits
*/
/* Allocates and initializes ecc context
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
*   Output: sgx_ecc_state_handle_t *p_ecc_handle - Pointer to the handle of ECC crypto system  */
sgx_status_t sgx_ecc256_open_context(sgx_ecc_state_handle_t* p_ecc_handle)
{
    IppStatus ipp_ret = ippStsNoErr;
    IppsECCPState* p_ecc_state = NULL;
    // default use 256r1 parameter
    int ctx_size = 0;

    if (p_ecc_handle == NULL)
        return SGX_ERROR_INVALID_PARAMETER;
    ipp_ret = ippsECCPGetSize(ECC_FIELD_SIZE, &ctx_size);
    if (ipp_ret != ippStsNoErr)
        return SGX_ERROR_UNEXPECTED;
    p_ecc_state = (IppsECCPState*)(malloc(ctx_size));
    if (p_ecc_state == NULL)
        return SGX_ERROR_OUT_OF_MEMORY;
    ipp_ret = ippsECCPInit(ECC_FIELD_SIZE, p_ecc_state);
    if (ipp_ret != ippStsNoErr)
    {
        CLEAR_FREE_MEM(p_ecc_state, ctx_size);
        *p_ecc_handle = NULL;
        return SGX_ERROR_UNEXPECTED;
    }
    ipp_ret = ippsECCPSetStd256r1(p_ecc_state);
    if (ipp_ret != ippStsNoErr)
    {
        CLEAR_FREE_MEM(p_ecc_state, ctx_size);
        *p_ecc_handle = NULL;
        return SGX_ERROR_UNEXPECTED;
    }
    *p_ecc_handle = p_ecc_state;
    return SGX_SUCCESS;
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
    IppsECCPState* p_ecc_state = (IppsECCPState*)ecc_handle;
    int ctx_size = 0;
    IppStatus ipp_ret = ippsECCPGetSize(ECC_FIELD_SIZE, &ctx_size);
    if (ipp_ret != ippStsNoErr)
    {
        free(p_ecc_state);
        return SGX_SUCCESS;
    }
    CLEAR_FREE_MEM(p_ecc_state, ctx_size);
    return SGX_SUCCESS;
}

/* Populates private/public key pair - caller code allocates memory
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
*   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system
*   Outputs: sgx_ec256_private_t *p_private - Pointer to the private key
*            sgx_ec256_public_t *p_public - Pointer to the public key  */
sgx_status_t sgx_ecc256_create_key_pair(sgx_ec256_private_t *p_private,
    sgx_ec256_public_t *p_public,
    sgx_ecc_state_handle_t ecc_handle)
{
    if ((ecc_handle == NULL) || (p_private == NULL) || (p_public == NULL))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsBigNumState*    dh_priv_BN = NULL;
    IppsECCPPointState* point_pub = NULL;
    IppsBigNumState*    pub_gx = NULL;
    IppsBigNumState*    pub_gy = NULL;
    IppStatus           ipp_ret = ippStsNoErr;
    int                 ecPointSize = 0;
    IppsECCPState* p_ecc_state = (IppsECCPState*)ecc_handle;

    do
    {
        //init eccp point
        ipp_ret = ippsECCPPointGetSize(ECC_FIELD_SIZE, &ecPointSize);
        ERROR_BREAK(ipp_ret);
        point_pub = (IppsECCPPointState*)(malloc(ecPointSize));
        if (!point_pub)
        {
            ipp_ret = ippStsNoMemErr;
            break;
        }
        ipp_ret = ippsECCPPointInit(ECC_FIELD_SIZE, point_pub);
        ERROR_BREAK(ipp_ret);

        ipp_ret = sgx_ipp_newBN(NULL, SGX_ECP256_KEY_SIZE, &dh_priv_BN);
        ERROR_BREAK(ipp_ret);
        // Use the true random number (DRNG)
        // Notice that IPP ensures the private key generated is non-zero
        ipp_ret = ippsECCPGenKeyPair(dh_priv_BN, point_pub, p_ecc_state, (IppBitSupplier)sgx_ipp_DRNGen, NULL);
        ERROR_BREAK(ipp_ret);

        //convert point_result to oct string
        ipp_ret = sgx_ipp_newBN(NULL, SGX_ECP256_KEY_SIZE, &pub_gx);
        ERROR_BREAK(ipp_ret);
        ipp_ret = sgx_ipp_newBN(NULL, SGX_ECP256_KEY_SIZE, &pub_gy);
        ERROR_BREAK(ipp_ret);
        ipp_ret = ippsECCPGetPoint(pub_gx, pub_gy, point_pub, p_ecc_state);
        ERROR_BREAK(ipp_ret);

        IppsBigNumSGN sgn = IppsBigNumPOS;
        Ipp32u *pdata = NULL;
        // ippsRef_BN is in bits not bytes (versus old ippsGet_BN)
        int length = 0;
        ipp_ret = ippsRef_BN(&sgn, &length, &pdata, pub_gx);
        ERROR_BREAK(ipp_ret);
        memset(p_public->gx, 0, sizeof(p_public->gx));
        ipp_ret = check_copy_size(sizeof(p_public->gx), ROUND_TO(length, 8) / 8);
        ERROR_BREAK(ipp_ret);
        memcpy(p_public->gx, pdata, ROUND_TO(length, 8) / 8);
        ipp_ret = ippsRef_BN(&sgn, &length, &pdata, pub_gy);
        ERROR_BREAK(ipp_ret);
        memset(p_public->gy, 0, sizeof(p_public->gy));
        ipp_ret = check_copy_size(sizeof(p_public->gy), ROUND_TO(length, 8) / 8);
        ERROR_BREAK(ipp_ret);
        memcpy(p_public->gy, pdata, ROUND_TO(length, 8) / 8);
        ipp_ret = ippsRef_BN(&sgn, &length, &pdata, dh_priv_BN);
        ERROR_BREAK(ipp_ret);
        memset(p_private->r, 0, sizeof(p_private->r));
        ipp_ret = check_copy_size(sizeof(p_private->r), ROUND_TO(length, 8) / 8);
        ERROR_BREAK(ipp_ret);
        memcpy(p_private->r, pdata, ROUND_TO(length, 8) / 8);
    } while (0);

    //Clear temp buffer before free.
    if (point_pub) memset_s(point_pub, ecPointSize, 0, ecPointSize);
    SAFE_FREE(point_pub);
    sgx_ipp_secure_free_BN(pub_gx, SGX_ECP256_KEY_SIZE);
    sgx_ipp_secure_free_BN(pub_gy, SGX_ECP256_KEY_SIZE);
    sgx_ipp_secure_free_BN(dh_priv_BN, SGX_ECP256_KEY_SIZE);

    switch (ipp_ret)
    {
    case ippStsNoErr: return SGX_SUCCESS;
    case ippStsNoMemErr:
    case ippStsMemAllocErr: return SGX_ERROR_OUT_OF_MEMORY;
    case ippStsNullPtrErr:
    case ippStsLengthErr:
    case ippStsOutOfRangeErr:
    case ippStsSizeErr:
    case ippStsBadArgErr: return SGX_ERROR_INVALID_PARAMETER;
    default: return SGX_ERROR_UNEXPECTED;
    }
}

/* Checks whether the input point is a valid point on the given elliptic curve
* Parameters:
*   Return: sgx_status_t - SGX_SUCCESS or failure as defined sgx_error.h
*   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system
*           sgx_ec256_public_t *p_point - Pointer to perform validity check on - LITTLE ENDIAN
*   Output: int *p_valid - Return 0 if the point is an invalid point on ECC curve */
sgx_status_t sgx_ecc256_check_point(const sgx_ec256_public_t *p_point,
                                    const sgx_ecc_state_handle_t ecc_handle,
                                    int *p_valid)
{
    if ((ecc_handle == NULL) || (p_point == NULL) || (p_valid == NULL))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsECCPPointState* point2check = NULL;
    IppStatus           ipp_ret = ippStsNoErr;
    IppsECCPState* p_ecc_state = (IppsECCPState*)ecc_handle;
    IppECResult ipp_result = ippECValid;
    int                 ecPointSize = 0;
    IppsBigNumState*    BN_gx = NULL;
    IppsBigNumState*    BN_gy = NULL;

    // Intialize return to false
    *p_valid = 0;

    do
    {
        ipp_ret = ippsECCPPointGetSize(ECC_FIELD_SIZE, &ecPointSize);
        ERROR_BREAK(ipp_ret);
        point2check = (IppsECCPPointState*)malloc(ecPointSize);
        if (!point2check)
        {
            ipp_ret = ippStsNoMemErr;
            break;
        }
        ipp_ret = ippsECCPPointInit(ECC_FIELD_SIZE, point2check);
        ERROR_BREAK(ipp_ret);

        ipp_ret = sgx_ipp_newBN((const Ipp32u *)p_point->gx, sizeof(p_point->gx), &BN_gx);
        ERROR_BREAK(ipp_ret);
        ipp_ret = sgx_ipp_newBN((const Ipp32u *)p_point->gy, sizeof(p_point->gy), &BN_gy);
        ERROR_BREAK(ipp_ret);

        ipp_ret = ippsECCPSetPoint(BN_gx, BN_gy, point2check, p_ecc_state);
        ERROR_BREAK(ipp_ret);

        // Check to see if the point is a valid point on the Elliptic curve and is not infinity
        ipp_ret = ippsECCPCheckPoint(point2check, &ipp_result, p_ecc_state);
        ERROR_BREAK(ipp_ret);
        if (ipp_result == ippECValid)
        {
            *p_valid = 1;
        }
    } while (0);

    // Clear temp buffer before free.
    if (point2check)
        memset_s(point2check, ecPointSize, 0, ecPointSize);
    SAFE_FREE(point2check);

    sgx_ipp_secure_free_BN(BN_gx, sizeof(p_point->gx));
    sgx_ipp_secure_free_BN(BN_gy, sizeof(p_point->gy));

    switch (ipp_ret)
    {
    case ippStsNoErr: return SGX_SUCCESS;
    case ippStsNoMemErr:
    case ippStsMemAllocErr: return SGX_ERROR_OUT_OF_MEMORY;
    case ippStsNullPtrErr:
    case ippStsLengthErr:
    case ippStsOutOfRangeErr:
    case ippStsSizeErr:
    case ippStsBadArgErr: return SGX_ERROR_INVALID_PARAMETER;
    default: return SGX_ERROR_UNEXPECTED;
    }
}

/* Computes DH shared key based on private B key (local) and remote public Ga Key
* Parameters:
*   Return: sgx_status_t - SGX_SUCCESS or failure as defined sgx_error.h
*   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system
*           sgx_ec256_private_t *p_private_b - Pointer to the local private key - LITTLE ENDIAN
*           sgx_ec256_public_t *p_public_ga - Pointer to the remote public key - LITTLE ENDIAN
*   Output: sgx_ec256_dh_shared_t *p_shared_key - Pointer to the shared DH key - LITTLE ENDIAN
x-coordinate of (privKeyB - pubKeyA) */
sgx_status_t sgx_ecc256_compute_shared_dhkey(const sgx_ec256_private_t *p_private_b,
                                             const sgx_ec256_public_t *p_public_ga,
                                             sgx_ec256_dh_shared_t *p_shared_key,
                                             sgx_ecc_state_handle_t ecc_handle)
{
    if ((ecc_handle == NULL) || (p_private_b == NULL) || (p_public_ga == NULL) || (p_shared_key == NULL))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsBigNumState*    BN_dh_privB = NULL;
    IppsBigNumState*    BN_dh_share = NULL;
    IppsBigNumState*    pubA_gx = NULL;
    IppsBigNumState*    pubA_gy = NULL;
    IppsECCPPointState* point_pubA = NULL;
    IppStatus           ipp_ret = ippStsNoErr;
    int                 ecPointSize = 0;
    IppsECCPState* p_ecc_state = (IppsECCPState*)ecc_handle;
    IppECResult ipp_result = ippECValid;

    do
    {
        ipp_ret = sgx_ipp_newBN((Ipp32u*)p_private_b->r, sizeof(sgx_ec256_private_t), &BN_dh_privB);
        ERROR_BREAK(ipp_ret);
        ipp_ret = sgx_ipp_newBN((uint32_t*)p_public_ga->gx, sizeof(p_public_ga->gx), &pubA_gx);
        ERROR_BREAK(ipp_ret);
        ipp_ret = sgx_ipp_newBN((uint32_t*)p_public_ga->gy, sizeof(p_public_ga->gy), &pubA_gy);
        ERROR_BREAK(ipp_ret);
        ipp_ret = ippsECCPPointGetSize(ECC_FIELD_SIZE, &ecPointSize);
        ERROR_BREAK(ipp_ret);
        point_pubA = (IppsECCPPointState*)(malloc(ecPointSize));
        if (!point_pubA)
        {
            ipp_ret = ippStsNoMemErr;
            break;
        }
        ipp_ret = ippsECCPPointInit(ECC_FIELD_SIZE, point_pubA);
        ERROR_BREAK(ipp_ret);
        ipp_ret = ippsECCPSetPoint(pubA_gx, pubA_gy, point_pubA, p_ecc_state);
        ERROR_BREAK(ipp_ret);

        // Check to see if the point is a valid point on the Elliptic curve and is not infinity
        ipp_ret = ippsECCPCheckPoint(point_pubA, &ipp_result, p_ecc_state);
        if (ipp_result != ippECValid)
        {
            break;
        }
        ERROR_BREAK(ipp_ret);

        ipp_ret = sgx_ipp_newBN(NULL, sizeof(sgx_ec256_dh_shared_t), &BN_dh_share);
        ERROR_BREAK(ipp_ret);
        /* This API generates shareA = x-coordinate of (privKeyB*pubKeyA) */
        ipp_ret = ippsECCPSharedSecretDH(BN_dh_privB, point_pubA, BN_dh_share, p_ecc_state);
        ERROR_BREAK(ipp_ret);
        IppsBigNumSGN sgn = IppsBigNumPOS;
        int length = 0;
        Ipp32u * pdata = NULL;
        ipp_ret = ippsRef_BN(&sgn, &length, &pdata, BN_dh_share);
        ERROR_BREAK(ipp_ret);
        memset(p_shared_key->s, 0, sizeof(p_shared_key->s));
        ipp_ret = check_copy_size(sizeof(p_shared_key->s), ROUND_TO(length, 8) / 8);
        ERROR_BREAK(ipp_ret);
        memcpy(p_shared_key->s, pdata, ROUND_TO(length, 8) / 8);
    } while (0);

    // Clear temp buffer before free.
    if (point_pubA) memset_s(point_pubA, ecPointSize, 0, ecPointSize);
    SAFE_FREE(point_pubA);
    sgx_ipp_secure_free_BN(pubA_gx, sizeof(p_public_ga->gx));
    sgx_ipp_secure_free_BN(pubA_gy, sizeof(p_public_ga->gy));
    sgx_ipp_secure_free_BN(BN_dh_privB, sizeof(sgx_ec256_private_t));
    sgx_ipp_secure_free_BN(BN_dh_share, sizeof(sgx_ec256_dh_shared_t));


    if (ipp_result != ippECValid)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    switch (ipp_ret)
    {
    case ippStsNoErr: return SGX_SUCCESS;
    case ippStsNoMemErr:
    case ippStsMemAllocErr: return SGX_ERROR_OUT_OF_MEMORY;
    case ippStsNullPtrErr:
    case ippStsLengthErr:
    case ippStsOutOfRangeErr:
    case ippStsSizeErr:
    case ippStsBadArgErr: return SGX_ERROR_INVALID_PARAMETER;
    default: return SGX_ERROR_UNEXPECTED;
    }
}

/** Create an ECC public key based on a given ECC private key.
*
* Parameters:
*   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
*   Input: p_att_priv_key - Input private key
*   Output: p_att_pub_key - Output public key - LITTLE ENDIAN
*
*/
sgx_status_t sgx_ecc256_calculate_pub_from_priv(const sgx_ec256_private_t *p_att_priv_key, sgx_ec256_public_t  *p_att_pub_key)
{
    if ((p_att_priv_key == NULL) || (p_att_pub_key == NULL)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    IppsECCPState* p_ecc_state = NULL;
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int ctx_size = 0;
    int point_size = 0;
    IppsECCPPointState* public_key = NULL;
    IppsBigNumState*    bn_o = NULL;
    IppsBigNumState*    bn_x = NULL;
    IppsBigNumState*    bn_y = NULL;
    sgx_ec256_private_t att_priv_key_be;
    uint8_t* p_temp;
    int size = 0;
    IppsBigNumSGN sgn;

    do {
        //get the size of the IppsECCPState context
        //
        if (ippsECCPGetSize(ECC_FIELD_SIZE, &ctx_size) != ippStsNoErr) {
            break;
        }

        //allocate ecc ctx
        //
        p_ecc_state = (IppsECCPState*)(malloc(ctx_size));
        if (NULL == p_ecc_state) {
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        //init ecc ctx
        //
        if (ippsECCPInit(ECC_FIELD_SIZE, p_ecc_state) != ippStsNoErr) {
            break;
        }

        //set up elliptic curve domain parameters over GF(p)
        //
        if (ippsECCPSetStd(IppECCPStd256r1, p_ecc_state) != ippStsNoErr) {
            break;
        }

        //get point (public key) size
        //
        if (ippsECCPPointGetSize(ECC_FIELD_SIZE, &point_size) != ippStsNoErr) {
            break;
        }

        //allocate point of point_size size
        //
        public_key = (IppsECCPPointState*)(malloc(point_size));
        if (NULL == public_key) {
            ret = SGX_ERROR_OUT_OF_MEMORY;
            break;
        }

        //init point
        //
        if (ippsECCPPointInit(ECC_FIELD_SIZE, public_key) != ippStsNoErr) {
            break;
        }

        //allocate bn_o, will be used for private key
        //
        if (sgx_ipp_newBN(NULL, sizeof(sgx_ec256_private_t), &bn_o) != ippStsNoErr) {
            break;
        }

        //convert private key into big endian
        //
        p_temp = (uint8_t*)p_att_priv_key;
        for (uint32_t i = 0; i<sizeof(att_priv_key_be); i++) {
            att_priv_key_be.r[i] = *(p_temp + sizeof(att_priv_key_be) - 1 - i);
        }

        //assign private key into bn_o
        //
        if (ippsSetOctString_BN(reinterpret_cast<Ipp8u *>(&att_priv_key_be), sizeof(sgx_ec256_private_t), bn_o) != ippStsNoErr) {
            break;
        }

        //compute public key from the given private key (bn_o) of the elliptic cryptosystem (p_ecc_state) over GF(p).
        //
        if (ippsECCPPublicKey(bn_o, public_key, p_ecc_state) != ippStsNoErr) {
            break;
        }

        //allocate BNs
        //
        if (sgx_ipp_newBN(NULL, sizeof(sgx_ec256_private_t), &bn_x) != ippStsNoErr) {
            break;
        }

        if (sgx_ipp_newBN(NULL, sizeof(sgx_ec256_private_t), &bn_y) != ippStsNoErr) {
            break;
        }
        //assign public key into BNs
        //
        if (ippsECCPGetPoint(bn_x, bn_y, public_key, p_ecc_state) != ippStsNoErr) {
            break;
        }

        //output key in little endian order
        //
        //gx value
        if (ippsGetSize_BN(bn_x, &size) != ippStsNoErr) {
            break;
        }
        if (ippsGet_BN(&sgn, &size, reinterpret_cast<Ipp32u *>(p_att_pub_key->gx), bn_x) != ippStsNoErr) {
            break;
        }
        //gy value
        //
        if (ippsGetSize_BN(bn_y, &size) != ippStsNoErr) {
            break;
        }
        if (ippsGet_BN(&sgn, &size, reinterpret_cast<Ipp32u *>(p_att_pub_key->gy), bn_y) != ippStsNoErr) {
            break;
        }

        ret = SGX_SUCCESS;
    } while (0);

    //in case of failure clear public key
    //
    if (ret != SGX_SUCCESS) {
        (void)memset_s(p_att_pub_key, sizeof(sgx_ec256_public_t), 0, sizeof(sgx_ec256_public_t));
    }

    CLEAR_FREE_MEM(p_ecc_state, ctx_size);
    CLEAR_FREE_MEM(public_key, point_size);
    sgx_ipp_secure_free_BN(bn_o, sizeof(sgx_ec256_private_t));
    sgx_ipp_secure_free_BN(bn_x, sizeof(sgx_ec256_private_t));
    sgx_ipp_secure_free_BN(bn_y, sizeof(sgx_ec256_private_t));

    return ret;
}

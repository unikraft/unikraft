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

#include <stdlib.h>
#include <string.h>
#include "ippcp.h"
#include "ipp_wrapper.h"


extern "C" void secure_free_rsa_pri_key(IppsRSAPrivateKeyState *pri_key)
{
    if (pri_key == NULL) {
        return;
    }

    IppsBigNumState* p_bn_mod = NULL;
    IppsBigNumState* p_bn_p = NULL;
    IppsBigNumState* p_bn_exp = NULL;
    IppsBigNumState* p_bn_q = NULL;
    int mod_bits = 0;
    int exp_bits = 0;
    int p_bits = 0;
    int q_bits = 0;
    int ctx_size = 0;

    do {
        //create new BNs.
        // p_bn_mod/p: used to store modulus data in case of type1 key, in case of type2 key it'll store prime number P data.
        // p_bn_exp/q: used to store exponent data in case of type1 key, in case of type2 key it'll store prime number Q data.
        //
        if (sgx_ipp_newBN(NULL, MAX_IPP_BN_LENGTH, &p_bn_mod) != ippStsNoErr) {
            break;
        }
        if (sgx_ipp_newBN(NULL, MAX_IPP_BN_LENGTH, &p_bn_exp) != ippStsNoErr) {
            break;
        }
        p_bn_p = p_bn_mod;
        p_bn_q = p_bn_exp;
        //check private key type. IPP does not provide direct API to do this, we use the below workaround
        //
        if (ippsRSA_GetPrivateKeyType1(p_bn_mod, p_bn_exp, (IppsRSAPrivateKeyState*)pri_key) == ippStsNoErr) {
            if (ippsExtGet_BN(0, &mod_bits, 0, p_bn_mod) != ippStsNoErr) {
                break;
            }
            if (ippsExtGet_BN(0, &exp_bits, 0, p_bn_exp) != ippStsNoErr) {
                break;
            }
            if (ippsRSA_GetSizePrivateKeyType1(mod_bits, exp_bits, &ctx_size) != ippStsNoErr) {
                break;
            }
            if (ippsRSA_InitPrivateKeyType1(mod_bits, exp_bits, (IppsRSAPrivateKeyState*)pri_key, ctx_size) != ippStsNoErr) {
                break;
            }
        }
        else {
            if (ippsRSA_GetPrivateKeyType2(p_bn_p, p_bn_q, NULL, NULL, NULL, (IppsRSAPrivateKeyState*)pri_key) != ippStsNoErr) {
                break;
            }
            if (ippsExtGet_BN(0, &p_bits, 0, p_bn_p) != ippStsNoErr) {
                break;
            }
            if (ippsExtGet_BN(0, &q_bits, 0, p_bn_q) != ippStsNoErr) {
                break;
            }
            if (ippsRSA_GetSizePrivateKeyType2(p_bits, q_bits, &ctx_size) != ippStsNoErr) {
                break;
            }
            if (ippsRSA_InitPrivateKeyType2(p_bits, q_bits, (IppsRSAPrivateKeyState*)pri_key, ctx_size) != ippStsNoErr) {
                break;
            }
        }
        memset_s(pri_key, ctx_size, 0, ctx_size);
    } while (0);

    free(pri_key);
    sgx_ipp_secure_free_BN(p_bn_mod, MAX_IPP_BN_LENGTH);
    sgx_ipp_secure_free_BN(p_bn_exp, MAX_IPP_BN_LENGTH);
    return;
}

extern "C" void secure_free_rsa_pub_key(int n_byte_size, int e_byte_size, IppsRSAPublicKeyState *pub_key)
{
    if (n_byte_size <= 0 || e_byte_size <= 0 || pub_key == NULL) {
        if (pub_key)
            free(pub_key);
        return;
    }
    int rsa_size = 0;
    if (ippsRSA_GetSizePublicKey(n_byte_size * 8, e_byte_size * 8, &rsa_size) != ippStsNoErr) {
        free(pub_key);
        return;
    }
    /* Clear the buffer before free. */
    memset_s(pub_key, rsa_size, 0, rsa_size);
    free(pub_key);
    return;
}

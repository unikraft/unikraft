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

#include "ssl_wrapper.h"
#include <sgx_trts.h>
#include <string.h>
#include "sgx_tcrypto.h"
#include "se_tcrypto_common.h"
#include "openssl/bn.h"

/************************************************************************************************/
/********************************The below APIs are used internally only*************************/
/************************************************************************************************/

typedef struct OctStr256 {
    unsigned char data[256 / 8];  ///< 256 bit data
} OctStr256;
typedef struct FpElemStr {
    OctStr256 data;  ///< 256 bit octet string
} FpElemStr;

#define PRIV_F_LOWER_BOUND      1LL
#define PRIV_F_EXTRA_RAND_BYTES 12
#define PRIV_F_RAND_SIZE        (PRIV_F_EXTRA_RAND_BYTES+sizeof(FpElemStr))

extern "C" sgx_status_t sgx_gen_epid_priv_f(void* f)
{
    // Parameter P in Epid2Params in big endian which is order(number of elements) 
    // of the ECC group used in EPID2 library
    //
    static unsigned char p_data[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD,
        0x46, 0xE5, 0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9E,
        0x0C, 0xDC, 0x65, 0xFB, 0x12, 0x99, 0x92, 0x1A,
        0xF6, 0x2D, 0x53, 0x6C, 0xD1, 0x0B, 0x50, 0x0D
    };

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    BIGNUM* f_BN = NULL;
    BIGNUM* p_BN = NULL;
    BIGNUM* r_BN = NULL;
    BIGNUM *h_BN = NULL;
    BIGNUM *d_BN = NULL;
    BN_CTX* tmp_ctx = NULL;

    // Buffer to hold random bits, it has 96 more bits than f or p
    //
    uint8_t f_temp_buf[PRIV_F_RAND_SIZE];
    uint64_t lower_bound = PRIV_F_LOWER_BOUND;
    uint64_t diff = 2 * lower_bound - 1;

    // First create the mod P which is in LITTLE ENDIAN
    //
    do {

        // random generate a number f with 96 bits extra data
        // to make sure the output result f%(p_data-(2*PRIV_F_LOWER_BOUND-1)) is uniform distributed
        // the extra bits should be at least 80 bits while ipps functions requires the bits to be time of 32 bits
        //
        if (sgx_read_rand(f_temp_buf, static_cast<uint32_t>(PRIV_F_RAND_SIZE)) != SGX_SUCCESS) {
            break;
        }

        // tmp ctx used in BNs calculations
        //
        tmp_ctx = BN_CTX_new();
        NULL_BREAK(tmp_ctx);

        // convert BIG ENDIAN p_data to BN
        //
        p_BN = BN_bin2bn(p_data, sizeof(FpElemStr), p_BN);
        NULL_BREAK(p_BN);

        // convert BIG ENDIAN low_bound to BN
        //
        h_BN = BN_bin2bn(reinterpret_cast<unsigned char *>(&lower_bound), sizeof(lower_bound), h_BN);
        NULL_BREAK(h_BN);

        // convert BIG ENDIAN diff (2 * lower_bound - 1) to BN
        //
        d_BN = BN_bin2bn(reinterpret_cast<unsigned char *>(&diff), sizeof(diff), d_BN);
        NULL_BREAK(d_BN);

        // r = p_data - (2*PRIV_F_LOWER_BOUND-1)
        //
        r_BN = BN_new();
        NULL_BREAK(r_BN);
        if (!BN_sub(r_BN, p_BN, d_BN)) {
            break;
        }

        // convert random buffer f_temp_buf to BN
        //
        f_BN = BN_bin2bn(reinterpret_cast<unsigned char *>(f_temp_buf), static_cast<uint32_t>(PRIV_F_RAND_SIZE), f_BN);
        NULL_BREAK(f_BN);

        // calculate p_BN = f (mod r_BN=(p_data - (2*PRIV_F_LOWER_BOUND-1)))
        //
        if (!BN_mod(p_BN, f_BN, r_BN, tmp_ctx)) {
            break;
        }

        // r_BN = f (mod p_data - (2*PRIV_F_LOWER_BOUND-1)) + PRIV_F_LOWER_BOUND
        //
        if (!BN_add(r_BN, p_BN, h_BN)) {
            break;
        }

        if (BN_num_bytes(r_BN) > (int)sizeof(FpElemStr)) {
            break;
        }
        // output the result into big endian buffer
        //
        if (!BN_bn2bin(r_BN, reinterpret_cast<unsigned char *>(f))) {
            break;
        }

        ret = SGX_SUCCESS;
    } while (0);

    // In case of error, clear output buffer befor return
    //
    if (ret != SGX_SUCCESS) {
        (void)memset_s(f, sizeof(FpElemStr), 0, sizeof(FpElemStr));
    }

    BN_CTX_free(tmp_ctx);
    BN_clear_free(f_BN);
    BN_clear_free(p_BN);
    BN_clear_free(r_BN);
    BN_clear_free(h_BN);
    BN_clear_free(d_BN);
    (void)memset_s(f_temp_buf, sizeof(f_temp_buf), 0, sizeof(f_temp_buf));

    return ret;
}

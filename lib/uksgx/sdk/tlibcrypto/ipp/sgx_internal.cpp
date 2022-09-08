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


#include "sgx_tcrypto.h"
#include "ippcp.h"
#include "string.h"
#include "sgx_trts.h"
#include "ipp_wrapper.h"

/************************************************************************************************/
/*******************************The below APIs are used internally only**************************/
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
    static uint8_t p_data[] = {//Parameter P in Epid2Params in big endian which is order(number of elements) of the ECC group used in EPID2 library
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xCD,
        0x46, 0xE5, 0xF2, 0x5E, 0xEE, 0x71, 0xA4, 0x9E,
        0x0C, 0xDC, 0x65, 0xFB, 0x12, 0x99, 0x92, 0x1A,
        0xF6, 0x2D, 0x53, 0x6C, 0xD1, 0x0B, 0x50, 0x0D
    };

    if (f == NULL)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    IppsBigNumState* f_BN = NULL;
    IppsBigNumState* p_BN = NULL;
    IppsBigNumState* r_BN = NULL;
    IppsBigNumState *h_BN = NULL;
    IppsBigNumState *d_BN = NULL;
    IppStatus ipp_status = ippStsNoErr;

    uint8_t f_temp_buf[PRIV_F_RAND_SIZE]; //buffer to hold random bits, it has 96 more bits than f or p
    uint64_t lower_bound = PRIV_F_LOWER_BOUND;
    uint64_t diff = 2 * lower_bound - 1;
    se_static_assert(sizeof(FpElemStr) % 4 == 0); /*sizeof FpElemStr should be multiple of 4*/
    se_static_assert(PRIV_F_RAND_SIZE % 4 == 0); /*the number of bytes of random number should be multiple of 4*/
                                                 //First create the mod P which is in little endian
    do {
        ipp_status = sgx_ipp_newBN(NULL, sizeof(FpElemStr), &p_BN);//initialize integer buffer
        ERROR_BREAK(ipp_status);
        ipp_status = ippsSetOctString_BN(p_data, sizeof(FpElemStr), p_BN);//Input data in Bigendian format
        ERROR_BREAK(ipp_status);
        ipp_status = sgx_ipp_newBN(NULL, sizeof(FpElemStr), &r_BN);//create buffer to hold temp and output result
        ERROR_BREAK(ipp_status);
        //initialize a lower bound
        ipp_status = sgx_ipp_newBN(reinterpret_cast<Ipp32u *>(&lower_bound), sizeof(lower_bound), &h_BN);
        ERROR_BREAK(ipp_status);
        ipp_status = sgx_ipp_newBN(reinterpret_cast<Ipp32u *>(&diff), sizeof(diff), &d_BN);//2*PRIV_F_LOWER_BOUND-1
        ERROR_BREAK(ipp_status);
        //random generate a number f with 96 bits extra data
        //   to make sure the output result f%(p_data-(2*PRIV_F_LOWER_BOUND-1)) is uniform distributed
        //   the extra bits should be at least 80 bits while ipps functions requires the bits to be time of 32 bits

        if (sgx_read_rand(f_temp_buf, static_cast<uint32_t>(PRIV_F_RAND_SIZE)) != SGX_SUCCESS) {
            break;
        }
        ipp_status = ippsSub_BN(p_BN, d_BN, r_BN);// r = p_data - (2*PRIV_F_LOWER_BOUND-1)
        ERROR_BREAK(ipp_status);
        ipp_status = sgx_ipp_newBN(reinterpret_cast<Ipp32u*>(f_temp_buf), static_cast<uint32_t>(PRIV_F_RAND_SIZE), &f_BN);//create big number by f
        ERROR_BREAK(ipp_status);
        ipp_status = ippsMod_BN(f_BN, r_BN, p_BN); //calculate p_BN = f (mod r_BN=(p_data - (2*PRIV_F_LOWER_BOUND-1)))
        ERROR_BREAK(ipp_status);
        ipp_status = ippsAdd_BN(p_BN, h_BN, r_BN); //r_BN = f (mod p_data - (2*PRIV_F_LOWER_BOUND-1)) + PRIV_F_LOWER_BOUND;
        ERROR_BREAK(ipp_status);
        //output the result and transform it into big endian
        ipp_status = ippsGetOctString_BN(reinterpret_cast<uint8_t *>(f), sizeof(FpElemStr), r_BN);
        ERROR_BREAK(ipp_status);
        ret = SGX_SUCCESS;
    } while (0);

    (void)memset_s(f_temp_buf, sizeof(f_temp_buf), 0, sizeof(f_temp_buf));
    sgx_ipp_secure_free_BN(h_BN, sizeof(lower_bound));//free big integer securely (The function will also memset_s the buffer)
    sgx_ipp_secure_free_BN(f_BN, static_cast<uint32_t>(PRIV_F_RAND_SIZE));
    sgx_ipp_secure_free_BN(p_BN, sizeof(FpElemStr));
    sgx_ipp_secure_free_BN(r_BN, sizeof(FpElemStr));
    sgx_ipp_secure_free_BN(d_BN, sizeof(diff));

    return ret;
}

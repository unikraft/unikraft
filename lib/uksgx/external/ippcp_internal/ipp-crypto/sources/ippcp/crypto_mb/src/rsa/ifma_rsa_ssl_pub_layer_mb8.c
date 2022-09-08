typedef int to_avoid_translation_unit_is_empty_warning;

#ifndef BN_OPENSSL_DISABLE

/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <openssl/bn.h>

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_cvt52.h>
#include <internal/rsa/ifma_rsa_arith.h>


/*
// public exponent e=65537 implied
*/

void ifma_ssl_rsa1K_pub_layer_mb8(const int8u* const from_pa[8],
                                        int8u* const to_pa[8],
                                  const BIGNUM* const n_pa[8])
{
   #define RSA_BITLEN (RSA_1K)
   #define LEN52      (NUMBER_OF_DIGITS(RSA_BITLEN, DIGIT_SIZE))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u inout_mb8[LEN52][8];
   /* MULTIPLE_OF_10 because of AMS5x52x79_diagonal_mb8() implementaion specific */
   __ALIGN64 int64u   n_mb8[MULTIPLE_OF(LEN52, 10)][8];
   /* allocate stack for red(undant) result and multiplier */
   __ALIGN64 int64u work_buffer[LEN52*2][8];

   /* convert modulus to ifma fmt */
   zero_mb8(n_mb8, MULTIPLE_OF(LEN52, 10));
   ifma_BN_to_mb8(n_mb8, n_pa, RSA_BITLEN);

   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, n_mb8[0]);

   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, n_mb8, RSA_BITLEN);

   /* convert input to ifma fmt */
   ifma_HexStr8_to_mb8(inout_mb8, from_pa, RSA_BITLEN);

   /* exponentiation */
   EXP52x20_pub65537_mb8(inout_mb8,
            (const int64u (*)[8])inout_mb8,
            (const int64u (*)[8])n_mb8,
            (const int64u (*)[8])rr_mb8,
            k0_mb8,
            (int64u(*)[8])work_buffer);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inout_mb8, RSA_BITLEN);

   #undef RSA_BITLEN
   #undef LEN52
}


void ifma_ssl_rsa2K_pub_layer_mb8(const int8u* const from_pa[8],
                                        int8u* const to_pa[8],
                                  const BIGNUM* const n_pa[8])
{
   #define RSA_BITLEN (RSA_2K)
   #define LEN52      (NUMBER_OF_DIGITS(RSA_BITLEN, DIGIT_SIZE))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u inout_mb8[LEN52][8];
   /* MULTIPLE_OF_10 because of AMS5x52x79_diagonal_mb8() implementaion specific */
   __ALIGN64 int64u   n_mb8[MULTIPLE_OF(LEN52, 10)][8];
   /* allocate stack for red(undant) result and multiplier */
   __ALIGN64 int64u work_buffer[LEN52*2][8];

   /* convert modulus to ifma fmt */
   zero_mb8(n_mb8, MULTIPLE_OF(LEN52, 10));
   ifma_BN_to_mb8(n_mb8, n_pa, RSA_BITLEN);

   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, n_mb8[0]);

   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, n_mb8, RSA_BITLEN);

   /* convert input to ifma fmt */
   ifma_HexStr8_to_mb8(inout_mb8, from_pa, RSA_BITLEN);

   /* exponentiation */
   EXP52x40_pub65537_mb8(inout_mb8,
            (const int64u(*)[8])inout_mb8,
            (const int64u(*)[8])n_mb8,
            (const int64u(*)[8])rr_mb8,
            k0_mb8,
            (int64u(*)[8])work_buffer);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inout_mb8, RSA_BITLEN);

   #undef RSA_BITLEN
   #undef LEN52
}


void ifma_ssl_rsa3K_pub_layer_mb8(const int8u* const from_pa[8],
                                        int8u* const to_pa[8],
                                  const BIGNUM* const n_pa[8])
{
   #define RSA_BITLEN (RSA_3K)
   #define LEN52      (NUMBER_OF_DIGITS(RSA_BITLEN, DIGIT_SIZE))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u inout_mb8[LEN52][8];
   /* MULTIPLE_OF_10 because of AMS5x52x79_diagonal_mb8() implementaion specific */
   __ALIGN64 int64u   n_mb8[MULTIPLE_OF(LEN52, 10)][8];
   /* allocate stack for red(undant) result and multiplier */
   __ALIGN64 int64u work_buffer[LEN52*2][8];

   /* convert modulus to ifma fmt */
   zero_mb8(n_mb8, MULTIPLE_OF(LEN52, 10));
   ifma_BN_to_mb8(n_mb8, n_pa, RSA_BITLEN);

   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, n_mb8[0]);

   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, n_mb8, RSA_BITLEN);

   /* convert input to ifma fmt */
   ifma_HexStr8_to_mb8(inout_mb8, from_pa, RSA_BITLEN);

   /* exponentiation */
   EXP52x60_pub65537_mb8(inout_mb8,
            (const int64u(*)[8])inout_mb8,
            (const int64u(*)[8])n_mb8,
            (const int64u(*)[8])rr_mb8,
            k0_mb8,
            (int64u(*)[8])work_buffer);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inout_mb8, RSA_BITLEN);

   #undef RSA_BITLEN
   #undef LEN52
}


void ifma_ssl_rsa4K_pub_layer_mb8(const int8u* const from_pa[8],
                                        int8u* const to_pa[8],
                                  const BIGNUM* const n_pa[8])
{
   #define RSA_BITLEN (RSA_4K)
   #define LEN52      (NUMBER_OF_DIGITS(RSA_BITLEN, DIGIT_SIZE))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u inout_mb8[LEN52][8];
   /* MULTIPLE_OF_10 because of AMS5x52x79_diagonal_mb8() implementaion specific */
   __ALIGN64 int64u   n_mb8[MULTIPLE_OF(LEN52, 10)][8];
   /* allocate stack for red(undant) result and multiplier */
   __ALIGN64 int64u work_buffer[LEN52*2+1][8];

   /* convert modulus to ifma fmt */
   zero_mb8(n_mb8, MULTIPLE_OF(LEN52, 10));
   ifma_BN_to_mb8(n_mb8, n_pa, RSA_BITLEN);

   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, n_mb8[0]);

   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, n_mb8, RSA_BITLEN);

   /* convert input to ifma fmt */
   ifma_HexStr8_to_mb8(inout_mb8, from_pa, RSA_BITLEN);

   /* exponentiation */
   EXP52x79_pub65537_mb8(inout_mb8,
            (const int64u(*)[8])inout_mb8,
            (const int64u(*)[8])n_mb8,
            (const int64u(*)[8])rr_mb8,
            k0_mb8,
            (int64u(*)[8])work_buffer);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inout_mb8, RSA_BITLEN);

   #undef RSA_BITLEN
   #undef LEN52
}

#endif /* BN_OPENSSL_DISABLE */

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

#define EXP_WIN_SIZE (5) //(4)
#define EXP_WIN_MASK ((1<<EXP_WIN_SIZE) -1)

/*
// y = x^d mod n (crt)
*/

void ifma_ssl_rsa1K_prv5_layer_mb8(const int8u* const from_pa[8],
                                         int8u* const to_pa[8],
                                   const BIGNUM* const  p_pa[8],
                                   const BIGNUM* const  q_pa[8],
                                   const BIGNUM* const dp_pa[8],
                                   const BIGNUM* const dq_pa[8],
                                   const BIGNUM* const iq_pa[8])
{
   #define RSA_BITLEN   (RSA_1K)
   #define FACTOR_BITLEN (RSA_BITLEN/2)
   #define LEN52      (NUMBER_OF_DIGITS(FACTOR_BITLEN, DIGIT_SIZE))
   #define LEN64      (NUMBER_OF_DIGITS(FACTOR_BITLEN, 64))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u   p_mb8[LEN52][8];
   __ALIGN64 int64u   q_mb8[LEN52][8];
   __ALIGN64 int64u   d_mb8[LEN64][8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u  xp_mb8[LEN52][8];
   __ALIGN64 int64u  xq_mb8[LEN52][8];
   __ALIGN64 int64u inp_mb8[LEN52*2][8];
   /* allocate stack for red(undant) result, multiplier, for exponent X and for pre-computed table of base powers */
   __ALIGN64 int64u work_buffer[LEN52*2 + 1 + (LEN64 + 1) + (1 << EXP_WIN_SIZE)*LEN52][8];

   /* convert input to ifma fmt */
   zero_mb8(inp_mb8, LEN52*2);
   ifma_HexStr8_to_mb8(inp_mb8, from_pa, RSA_BITLEN);

   /*
   // q exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(q_mb8, q_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, q_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, q_mb8, FACTOR_BITLEN);
   /* xq = x mod q */
   ifma_amred52x10_mb8(xq_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])q_mb8, k0_mb8);
   ifma_amm52x10_mb8((int64u*)xq_mb8, (int64u*)xq_mb8, (int64u*)rr_mb8, (int64u*)q_mb8, k0_mb8);
   ifma_modsub52x10_mb8(xq_mb8, (const int64u(*)[8])xq_mb8, (const int64u(*)[8])q_mb8, (const int64u(*)[8])q_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dq_pa, FACTOR_BITLEN);

   EXP52x10_mb8(xq_mb8,
      (const int64u(*)[8])xq_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])q_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // p exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(p_mb8, p_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, p_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, p_mb8, FACTOR_BITLEN);
   /* xp = x mod p */
   ifma_amred52x10_mb8(xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8, k0_mb8);
   ifma_amm52x10_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);
   ifma_modsub52x10_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dp_pa, FACTOR_BITLEN);

   EXP52x10_mb8(xp_mb8,
      (const int64u(*)[8])xp_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])p_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // crt recombination
   */

   /* xp = (xp-xq) mod p */
   ifma_modsub52x10_mb8(inp_mb8,(const int64u(*)[8])xq_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); /* for specific case p<q */
   ifma_modsub52x10_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8);

   /* xp = (xp*coef) mod p */
   ifma_BN_to_mb8(inp_mb8, iq_pa, FACTOR_BITLEN); /* coef */
   ifma_amm52x10_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);                     /* -> mont domain */
   ifma_amm52x10_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)inp_mb8, (int64u*)p_mb8, k0_mb8);                    /* mmul */
   ifma_modsub52x10_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8);/* correction */

   /* xp = (xp*q + xq) */
   zero_mb8(inp_mb8, LEN52*2);
   copy_mb8(inp_mb8, (const int64u(*)[8])xq_mb8, LEN52);
   ifma_addmul52x10_mb8(inp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])q_mb8);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inp_mb8, RSA_BITLEN);

   /* clear exponents, p, q */
   zero_mb8(d_mb8, LEN64);
   zero_mb8(q_mb8, LEN52);
   zero_mb8(p_mb8, LEN52);

   #undef RSA_BITLEN
   #undef FACTOR_BITLEN
   #undef LEN52
   #undef LEN64
}


void ifma_ssl_rsa2K_prv5_layer_mb8(const int8u* const from_pa[8],
                                         int8u* const to_pa[8],
                                   const BIGNUM* const  p_pa[8],
                                   const BIGNUM* const  q_pa[8],
                                   const BIGNUM* const dp_pa[8],
                                   const BIGNUM* const dq_pa[8],
                                   const BIGNUM* const iq_pa[8])
{
   #define RSA_BITLEN   (RSA_2K)
   #define FACTOR_BITLEN (RSA_BITLEN/2)
   #define LEN52      (NUMBER_OF_DIGITS(FACTOR_BITLEN, DIGIT_SIZE))
   #define LEN64      (NUMBER_OF_DIGITS(FACTOR_BITLEN, 64))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u   p_mb8[LEN52][8];
   __ALIGN64 int64u   q_mb8[LEN52][8];
   __ALIGN64 int64u   d_mb8[LEN64][8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u  xp_mb8[LEN52][8];
   __ALIGN64 int64u  xq_mb8[LEN52][8];
   __ALIGN64 int64u inp_mb8[LEN52*2][8];
   /* allocate stack for red(undant) result, multiplier, for exponent X and for pre-computed table of base powers */
   __ALIGN64 int64u work_buffer[LEN52*2 + 1 + (LEN64 + 1) + (1 << EXP_WIN_SIZE)*LEN52][8];

   /* convert input to ifma fmt */
   zero_mb8(inp_mb8, LEN52*2);
   ifma_HexStr8_to_mb8(inp_mb8, from_pa, RSA_BITLEN);

   /*
   // q exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(q_mb8, q_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, q_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, q_mb8, FACTOR_BITLEN);
   /* xq = x mod q */
   ifma_amred52x20_mb8(xq_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])q_mb8, k0_mb8);
   ifma_amm52x20_mb8((int64u*)xq_mb8, (int64u*)xq_mb8, (int64u*)rr_mb8, (int64u*)q_mb8, k0_mb8);
   ifma_modsub52x20_mb8(xq_mb8, (const int64u(*)[8])xq_mb8, (const int64u(*)[8])q_mb8, (const int64u(*)[8])q_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dq_pa, FACTOR_BITLEN);

   EXP52x20_mb8(xq_mb8,
      (const int64u(*)[8])xq_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])q_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // p exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(p_mb8, p_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, p_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, p_mb8, FACTOR_BITLEN);
   /* xp = x mod p */
   ifma_amred52x20_mb8(xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8, k0_mb8);
   ifma_amm52x20_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);
   ifma_modsub52x20_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dp_pa, FACTOR_BITLEN);

   EXP52x20_mb8(xp_mb8,
      (const int64u(*)[8])xp_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])p_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // crt recombination
   */

   /* xp = (xp-xq) mod p */
   ifma_modsub52x20_mb8(inp_mb8,(const int64u(*)[8])xq_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); /* for specific case p<q */
   ifma_modsub52x20_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8);

   /* xp = (xp*coef) mod p */
   ifma_BN_to_mb8(inp_mb8, iq_pa, FACTOR_BITLEN); /* coef */
   ifma_amm52x20_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);                     /* -> mont domain */
   ifma_amm52x20_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)inp_mb8, (int64u*)p_mb8, k0_mb8);                    /* mmul */
   ifma_modsub52x20_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8);/* correction */

   /* xp = (xp*q + xq) */
   zero_mb8(inp_mb8, LEN52*2);
   copy_mb8(inp_mb8, (const int64u(*)[8])xq_mb8, LEN52);
   ifma_addmul52x20_mb8(inp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])q_mb8);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inp_mb8, RSA_BITLEN);

   /* clear exponents, p, q */
   zero_mb8(d_mb8, LEN64);
   zero_mb8(q_mb8, LEN52);
   zero_mb8(p_mb8, LEN52);

   #undef RSA_BITLEN
   #undef FACTOR_BITLEN
   #undef LEN52
   #undef LEN64
}


void ifma_ssl_rsa3K_prv5_layer_mb8(const int8u* const from_pa[8],
                                         int8u* const to_pa[8],
                                   const BIGNUM* const  p_pa[8],
                                   const BIGNUM* const  q_pa[8],
                                   const BIGNUM* const dp_pa[8],
                                   const BIGNUM* const dq_pa[8],
                                   const BIGNUM* const iq_pa[8])
{
   #define RSA_BITLEN   (RSA_3K)
   #define FACTOR_BITLEN (RSA_BITLEN/2)
   #define LEN52      (NUMBER_OF_DIGITS(FACTOR_BITLEN, DIGIT_SIZE))
   #define LEN64      (NUMBER_OF_DIGITS(FACTOR_BITLEN, 64))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u   p_mb8[LEN52][8];
   __ALIGN64 int64u   q_mb8[LEN52][8];
   __ALIGN64 int64u   d_mb8[LEN64][8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u  xp_mb8[LEN52][8];
   __ALIGN64 int64u  xq_mb8[LEN52][8];
   __ALIGN64 int64u inp_mb8[LEN52 * 2][8];
   /* allocate stack for red(undant) result, multiplier, for exponent X and for pre-computed table of base powers */
   __ALIGN64 int64u work_buffer[LEN52*2 + 1 + (LEN64 + 1) + (1 << EXP_WIN_SIZE)*LEN52][8];

   /* convert input to ifma fmt */
   zero_mb8(inp_mb8, LEN52 * 2);
   ifma_HexStr8_to_mb8(inp_mb8, from_pa, RSA_BITLEN);

   /*
   // q exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(q_mb8, q_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, q_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, q_mb8, FACTOR_BITLEN);
   /* xq = x mod q */
   ifma_amred52x30_mb8(xq_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])q_mb8, k0_mb8);
   ifma_amm52x30_mb8((int64u*)xq_mb8, (int64u*)xq_mb8, (int64u*)rr_mb8, (int64u*)q_mb8, k0_mb8);
   ifma_modsub52x30_mb8(xq_mb8, (const int64u(*)[8])xq_mb8, (const int64u(*)[8])q_mb8, (const int64u(*)[8])q_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dq_pa, FACTOR_BITLEN);

   EXP52x30_mb8(xq_mb8,
      (const int64u(*)[8])xq_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])q_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // p exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(p_mb8, p_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, p_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, p_mb8, FACTOR_BITLEN);
   /* xp = x mod p */
   ifma_amred52x30_mb8(xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8, k0_mb8);
   ifma_amm52x30_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);
   ifma_modsub52x30_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dp_pa, FACTOR_BITLEN);

   EXP52x30_mb8(xp_mb8,
      (const int64u(*)[8])xp_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])p_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // crt recombination
   */

   /* xp = (xp-xq) mod p */
   ifma_modsub52x30_mb8(inp_mb8,(const int64u(*)[8])xq_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); /* for specific case p<q */
   ifma_modsub52x30_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8);

   /* xp = (xp*coef) mod p */
   ifma_BN_to_mb8(inp_mb8, iq_pa, FACTOR_BITLEN); /* coef */
   ifma_amm52x30_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);                     /* -> mont domain */
   ifma_amm52x30_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)inp_mb8, (int64u*)p_mb8, k0_mb8);                    /* mmul */
   ifma_modsub52x30_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8);/* correction */

   /* xp = (xp*q + xq) */
   zero_mb8(inp_mb8, LEN52 * 2);
   copy_mb8(inp_mb8, (const int64u(*)[8])xq_mb8, LEN52);
   ifma_addmul52x30_mb8(inp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])q_mb8);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inp_mb8, RSA_BITLEN);

   /* clear exponents, p, q */
   zero_mb8(d_mb8, LEN64);
   zero_mb8(q_mb8, LEN52);
   zero_mb8(p_mb8, LEN52);

   #undef RSA_BITLEN
   #undef FACTOR_BITLEN
   #undef LEN52
   #undef LEN64
}


void ifma_ssl_rsa4K_prv5_layer_mb8(const int8u* const from_pa[8],
                                         int8u* const to_pa[8],
                                   const BIGNUM* const  p_pa[8],
                                   const BIGNUM* const  q_pa[8],
                                   const BIGNUM* const dp_pa[8],
                                   const BIGNUM* const dq_pa[8],
                                   const BIGNUM* const iq_pa[8])
{
   #define RSA_BITLEN   (RSA_4K)
   #define FACTOR_BITLEN (RSA_BITLEN/2)
   #define LEN52      (NUMBER_OF_DIGITS(FACTOR_BITLEN, DIGIT_SIZE))
   #define LEN64      (NUMBER_OF_DIGITS(FACTOR_BITLEN, 64))

   /* allocate mb8 buffers */
   __ALIGN64 int64u  k0_mb8[8];
   __ALIGN64 int64u   p_mb8[LEN52][8];
   __ALIGN64 int64u   q_mb8[LEN52][8];
   __ALIGN64 int64u   d_mb8[LEN64][8];
   __ALIGN64 int64u  rr_mb8[LEN52][8];
   __ALIGN64 int64u  xp_mb8[LEN52][8];
   __ALIGN64 int64u  xq_mb8[LEN52][8];
   __ALIGN64 int64u inp_mb8[LEN52*2][8];
   /* allocate stack for red(undant) result, multiplier, for exponent X and for pre-computed table of base powers */
   __ALIGN64 int64u work_buffer[LEN52*2 + 1 + (LEN64 + 1) + (1 << EXP_WIN_SIZE)*LEN52][8];

   /* convert input to ifma fmt */
   zero_mb8(inp_mb8, LEN52 * 2);
   ifma_HexStr8_to_mb8(inp_mb8, from_pa, RSA_BITLEN);

   /*
   // q exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(q_mb8, q_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, q_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, q_mb8, FACTOR_BITLEN);
   /* xq = x mod q */
   ifma_amred52x40_mb8(xq_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])q_mb8, k0_mb8);
   ifma_amm52x40_mb8((int64u*)xq_mb8, (int64u*)xq_mb8, (int64u*)rr_mb8, (int64u*)q_mb8, k0_mb8);
   ifma_modsub52x40_mb8(xq_mb8, (const int64u(*)[8])xq_mb8, (const int64u(*)[8])q_mb8, (const int64u(*)[8])q_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dq_pa, FACTOR_BITLEN);

   EXP52x40_mb8(xq_mb8,
      (const int64u(*)[8])xq_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])q_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // p exponentiation
   */

   /* convert modulus to ifma fmt */
   ifma_BN_to_mb8(p_mb8, p_pa, FACTOR_BITLEN);
   /* compute k0[] */
   ifma_montFactor52_mb8(k0_mb8, p_mb8[0]);
   /* compute to_Montgomery domain converters */
   ifma_montRR52x_mb8(rr_mb8, p_mb8, FACTOR_BITLEN);
   /* xp = x mod p */
   ifma_amred52x40_mb8(xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8, k0_mb8);
   ifma_amm52x40_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);
   ifma_modsub52x40_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); // ??
   /* re-arrange exps to ifma */
   ifma_BN_transpose_copy(d_mb8, dp_pa, FACTOR_BITLEN);

   EXP52x40_mb8(xp_mb8,
      (const int64u(*)[8])xp_mb8,
      (const int64u(*)[8])d_mb8,
      (const int64u(*)[8])p_mb8,
      (const int64u(*)[8])rr_mb8,
      k0_mb8,
      (int64u(*)[8])work_buffer);

   /*
   // crt recombination
   */

   /* xp = (xp-xq) mod p */
   ifma_modsub52x40_mb8(inp_mb8,(const int64u(*)[8])xq_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8); /* for specific case p<q */
   ifma_modsub52x40_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])inp_mb8, (const int64u(*)[8])p_mb8);

   /* xp = (xp*coef) mod p */
   ifma_BN_to_mb8(inp_mb8, iq_pa, FACTOR_BITLEN); /* coef */
   ifma_amm52x40_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)rr_mb8, (int64u*)p_mb8, k0_mb8);                     /* -> mont domain */
   ifma_amm52x40_mb8((int64u*)xp_mb8, (int64u*)xp_mb8, (int64u*)inp_mb8, (int64u*)p_mb8, k0_mb8);                    /* mmul */
   ifma_modsub52x40_mb8(xp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])p_mb8, (const int64u(*)[8])p_mb8);/* correction */

   /* xp = (xp*q + xq) */
   zero_mb8(inp_mb8, LEN52 * 2);
   copy_mb8(inp_mb8, (const int64u(*)[8])xq_mb8, LEN52);
   ifma_addmul52x40_mb8(inp_mb8, (const int64u(*)[8])xp_mb8, (const int64u(*)[8])q_mb8);

   /* convert result from ifma fmt */
   ifma_mb8_to_HexStr8(to_pa, (const int64u(*)[8])inp_mb8, RSA_BITLEN);

   /* clear exponents, p, q */
   zero_mb8(d_mb8, LEN64);
   zero_mb8(q_mb8, LEN52);
   zero_mb8(p_mb8, LEN52);

   #undef RSA_BITLEN
   #undef FACTOR_BITLEN
   #undef LEN52
   #undef LEN64
}

#endif /* BN_OPENSSL_DISABLE */

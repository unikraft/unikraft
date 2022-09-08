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

#include "owncp.h"

#if(_IPP32E>=_IPP32E_K1)

#include "pcptool.h"

#include "pcpngmontexpstuff_avx512.h"
#include "ifma_math_avx512vl.h"
#include "ifma_rsa_arith.h"

#define BITSIZE_MODULUS (1024)
#define LEN52           (NUMBER_OF_DIGITS(BITSIZE_MODULUS,52))  // 20
#define LEN64           (NUMBER_OF_DIGITS(BITSIZE_MODULUS,64))  // 16

#define EXP_WIN_SIZE (5U)
#define EXP_WIN_MASK ((1U<<EXP_WIN_SIZE) - 1)

#define DAMM ifma256_amm52x20_dual
#define DAMS ifma256_ams52x20_dual

__INLINE void extract_multiplier_n(Ipp64u *red_Y,
                             const Ipp64u red_table[1U << EXP_WIN_SIZE][2][LEN52],
                                   int red_table_idx, int tbl_idx)
{
    U64 idx = set64(red_table_idx);
    U64 cur_idx = set64(0);

    U64 t0, t1, t2, t3, t4;
    t0 = t1 = t2 = t3 = t4 = get_zero64();

    for (int t = 0; t < (1 << EXP_WIN_SIZE); ++t, cur_idx = add64(cur_idx, set64(1))) {
        __mmask8 m = _mm256_cmp_epi64_mask(idx, cur_idx, _MM_CMPINT_EQ);

        t0 = _mm256_mask_xor_epi64(t0, m, t0, loadu64(&red_table[t][tbl_idx][4*0]));
        t1 = _mm256_mask_xor_epi64(t1, m, t1, loadu64(&red_table[t][tbl_idx][4*1]));
        t2 = _mm256_mask_xor_epi64(t2, m, t2, loadu64(&red_table[t][tbl_idx][4*2]));
        t3 = _mm256_mask_xor_epi64(t3, m, t3, loadu64(&red_table[t][tbl_idx][4*3]));
        t4 = _mm256_mask_xor_epi64(t4, m, t4, loadu64(&red_table[t][tbl_idx][4*4]));
    }

    /* Clear index */
    idx = xor64(idx, idx);

    storeu64(&red_Y[4*0], t0);
    storeu64(&red_Y[4*1], t1);
    storeu64(&red_Y[4*2], t2);
    storeu64(&red_Y[4*3], t3);
    storeu64(&red_Y[4*4], t4);
}


// Dual exponentiation in Montgomery domain.
IPP_OWN_DEFN (void, ifma256_exp52x20_dual, (Ipp64u out    [2][LEN52],
                                      const Ipp64u base   [2][LEN52],
                                      const Ipp64u *exp   [2], // 2x16
                                      const Ipp64u modulus[2][LEN52],
                                      const Ipp64u toMont [2][LEN52],
                                      const Ipp64u k0     [2]))
{
    /* allocate stack for red(undant) result Y and multiplier X */
    __ALIGN64 Ipp64u red_Y[2][LEN52];
    __ALIGN64 Ipp64u red_X[2][LEN52];

    /* allocate expanded exponent */
    __ALIGN64 Ipp64u expz[2][LEN64+1];

    /* pre-computed table of base powers */
    __ALIGN64 Ipp64u red_table[1U << EXP_WIN_SIZE][2][LEN52];

    int idx;

    ZEXPAND_BNU((Ipp64u*)red_Y, 0, 2 * LEN52);
    ZEXPAND_BNU((Ipp64u*)red_table, 0, 2 * LEN52 * (1 << EXP_WIN_SIZE));
    ZEXPAND_BNU((Ipp64u*)red_X, 0, 2 * LEN52);

    storeu64(&red_X[0][0], _mm256_setr_epi64x(1,0,0,0));
    storeu64(&red_X[1][0], _mm256_setr_epi64x(1,0,0,0));

    /*
    // compute table of powers base^i, i=0, ..., (2^EXP_WIN_SIZE) -1
    //    table[0] = mont(x^0) = mont(1)
    //    table[1] = mont(x^1) = mont(x)
    */
    DAMM(red_table[0], (const Ipp64u(*)[LEN52])red_X, toMont, modulus, k0);
    DAMM(red_table[1], base,  toMont, modulus, k0);

    for (idx = 1; idx < (1 << EXP_WIN_SIZE) / 2; idx++) {
        DAMS(red_table[2*idx+0], (const Ipp64u(*)[LEN52])red_table[1*idx], modulus, k0);
        DAMM(red_table[2*idx+1], (const Ipp64u(*)[LEN52])red_table[2*idx], (const Ipp64u(*)[LEN52])red_table[1], modulus, k0);
    }

    /* copy and expand exponents */
    ZEXPAND_COPY_BNU(expz[0], LEN64+1, exp[0], LEN64);
    ZEXPAND_COPY_BNU(expz[1], LEN64+1, exp[1], LEN64);

    /* exponentiation */
    {
        int rem = BITSIZE_MODULUS % EXP_WIN_SIZE;
        int delta = rem ? rem : EXP_WIN_SIZE;
        Ipp64u table_idx_mask = EXP_WIN_MASK;

        int exp_bit_no = BITSIZE_MODULUS - delta;
        int exp_chunk_no = exp_bit_no / 64;
        int exp_chunk_shift = exp_bit_no % 64;

        /* process 1-st exp window - just init result */
        Ipp64u red_table_idx_0 = expz[0][exp_chunk_no];
        Ipp64u red_table_idx_1 = expz[1][exp_chunk_no];
        red_table_idx_0 = red_table_idx_0 >> exp_chunk_shift;
        red_table_idx_1 = red_table_idx_1 >> exp_chunk_shift;

        extract_multiplier_n(red_Y[0], (const Ipp64u(*)[2][LEN52])red_table, (int)red_table_idx_0, 0);
        extract_multiplier_n(red_Y[1], (const Ipp64u(*)[2][LEN52])red_table, (int)red_table_idx_1, 1);

        /* process other exp windows */
        for (exp_bit_no -= EXP_WIN_SIZE; exp_bit_no >= 0; exp_bit_no -= EXP_WIN_SIZE) {
            /* extract pre-computed multiplier from the table */
            {
                Ipp64u T;
                exp_chunk_no = exp_bit_no / 64;
                exp_chunk_shift = exp_bit_no % 64;
                {
                    red_table_idx_0 = expz[0][exp_chunk_no];
                    T = expz[0][exp_chunk_no + 1];

                    red_table_idx_0 = red_table_idx_0 >> exp_chunk_shift;
                    T = exp_chunk_shift == 0 ? 0 : T << (64 - exp_chunk_shift);
                    red_table_idx_0 = (red_table_idx_0 ^ T) & table_idx_mask;

                    extract_multiplier_n(red_X[0], (const Ipp64u(*)[2][LEN52])red_table, (int)red_table_idx_0, 0);
                }
                {
                    red_table_idx_1 = expz[1][exp_chunk_no];
                    T = expz[1][exp_chunk_no + 1];

                    red_table_idx_1 = red_table_idx_1 >> exp_chunk_shift;
                    T = exp_chunk_shift == 0 ? 0 : T << (64 - exp_chunk_shift);
                    red_table_idx_1 = (red_table_idx_1 ^ T) & table_idx_mask;

                    extract_multiplier_n(red_X[1], (const Ipp64u(*)[2][LEN52])red_table, (int)red_table_idx_1, 1);
                }

            }

            /* series of squaring */
            DAMS(red_Y, (const Ipp64u(*)[LEN52])red_Y, modulus, k0);
            DAMS(red_Y, (const Ipp64u(*)[LEN52])red_Y, modulus, k0);
            DAMS(red_Y, (const Ipp64u(*)[LEN52])red_Y, modulus, k0);
            DAMS(red_Y, (const Ipp64u(*)[LEN52])red_Y, modulus, k0);
            DAMS(red_Y, (const Ipp64u(*)[LEN52])red_Y, modulus, k0);

            DAMM(red_Y, (const Ipp64u(*)[LEN52])red_Y, (const Ipp64u(*)[LEN52])red_X, modulus, k0);
        }
    }

    /* clear exponents */
    PurgeBlock((Ipp64u*)expz, 2*(LEN64+1)*(int)sizeof(Ipp64u));

    /* convert result back in regular 2^52 domain */
    ZEXPAND_BNU((Ipp64u*)red_X, 0, 2*LEN52);
    storeu64(&red_X[0][0], _mm256_setr_epi64x(1,0,0,0));
    storeu64(&red_X[1][0], _mm256_setr_epi64x(1,0,0,0));
    DAMM(out, (const Ipp64u(*)[LEN52])red_Y, (const Ipp64u(*)[LEN52])red_X, modulus, k0);

    PurgeBlock((Ipp64u*)red_Y, 2*LEN52*(int)sizeof(Ipp64u));
}

#endif

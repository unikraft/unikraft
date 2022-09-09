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

#include "pcpngmontexpstuff_avx512.h"

#include "ifma_norm52x.h"
#include "ifma_math_avx512vl.h"
#include "ifma_rsa_arith.h"

#define ADD104(rh, rl, ih, il) { \
   rl += il;                     \
   rh += ih;                     \
   rh += (rl<il)? 1 : 0;         \
}

#define SHRD52(rh, rl)  ((rl>>52U) | rh<<(64U-52U))

/*
 * Almost Montgomery Multiplication in 2^52-radix
 *
 *   Implements two independent interleaved multiplications.
 *   Data represented as 2 20-qwords arrays in 2^52-radix.
 *
 */
IPP_OWN_DEFN(void, ifma256_amm52x20_dual, (Ipp64u out[2][20],
                                     const Ipp64u a  [2][20],
                                     const Ipp64u b  [2][20],
                                     const Ipp64u m  [2][20],
                                     const Ipp64u k0 [2]))
{
    const Ipp64u *a_0 = a[0];
    const Ipp64u *b_0 = b[0];
    const Ipp64u *m_0 = m[0];
    Ipp64u m0_0   = m_0[0];
    Ipp64u a0_0   = a_0[0];
    Ipp64u acc0_0 = 0;
    U64 R0_0, R1_0, R2_0, R0_0h, R1_0h, R2_0h;
    R0_0 = R1_0 = R2_0 = R0_0h = R1_0h = R2_0h = get_zero64();

    const Ipp64u *a_1 = a[1];
    const Ipp64u *b_1 = b[1];
    const Ipp64u *m_1 = m[1];
    Ipp64u m0_1   = m_1[0];
    Ipp64u a0_1   = a_1[0];
    Ipp64u acc0_1 = 0;
    U64 R0_1, R1_1, R2_1, R0_1h, R1_1h, R2_1h;
    R0_1 = R1_1 = R2_1 = R0_1h = R1_1h = R2_1h = get_zero64();

    int i;
    for (i=0; i<20; i++) {
        {
            Ipp64u t0, t1, t2, yi;
            U64 Bi = set64((long long)b_0[i]);              /* broadcast(b[i]) */
            /* compute yi */
            t0 = _mulx_u64(a0_0, b_0[i], &t2);              /* (t2:t0) = acc0 + a[0]*b[i] */
            ADD104(t2, acc0_0, 0, t0)
            yi = (acc0_0 * k0[0])  & EXP_DIGIT_MASK_AVX512; /* yi = acc0*k0 */
            U64 Yi = set64((long long)yi);

            t0 = _mulx_u64(m0_0, yi, &t1);                  /* (t1:t0)   = m0*yi     */
            ADD104(t2, acc0_0, t1, t0)                      /* (t2:acc0) += (t1:t0)  */
            acc0_0 = SHRD52(t2, acc0_0);

            fma52x8lo_mem(R0_0, R0_0, Bi, a_0, 64*0)
            fma52x8lo_mem(R1_0, R1_0, Bi, a_0, 64*1)
            fma52x8lo_mem_len(R2_0, R2_0, Bi, a_0, 64*2, 4)
            fma52x8lo_mem(R0_0, R0_0, Yi, m_0, 64*0)
            fma52x8lo_mem(R1_0, R1_0, Yi, m_0, 64*1)
            fma52x8lo_mem_len(R2_0, R2_0, Yi, m_0, 64*2, 4)

            shift64_imm(R0_0, R0_0h, 1)
            shift64_imm(R0_0h, R1_0, 1)
            shift64_imm(R1_0, R1_0h, 1)
            shift64_imm(R1_0h, R2_0, 1)
            shift64_imm(R2_0, get_zero64(), 1)

            /* "shift" R */
            t0 = get64(R0_0, 0);
            acc0_0 += t0;

            /* U = A*Bi (hi) */
            fma52x8hi_mem(R0_0, R0_0, Bi, a_0, 64*0)
            fma52x8hi_mem(R1_0, R1_0, Bi, a_0, 64*1)
            fma52x8hi_mem_len(R2_0, R2_0, Bi, a_0, 64*2, 4)
            /* R += M*Yi (hi) */
            fma52x8hi_mem(R0_0, R0_0, Yi, m_0, 64*0)
            fma52x8hi_mem(R1_0, R1_0, Yi, m_0, 64*1)
            fma52x8hi_mem_len(R2_0, R2_0, Yi, m_0, 64*2, 4)
        }
        {
            Ipp64u t0, t1, t2, yi;
            U64 Bi = set64((long long)b_1[i]);              /* broadcast(b[i]) */
            /* compute yi */
            t0 = _mulx_u64(a0_1, b_1[i], &t2);              /* (t2:t0) = acc0 + a[0]*b[i] */
            ADD104(t2, acc0_1, 0, t0)
            yi = (acc0_1 * k0[1])  & EXP_DIGIT_MASK_AVX512; /* yi = acc0*k0 */
            U64 Yi = set64((long long)yi);

            t0 = _mulx_u64(m0_1, yi, &t1);                  /* (t1:t0)   = m0*yi     */
            ADD104(t2, acc0_1, t1, t0)                      /* (t2:acc0) += (t1:t0)  */
            acc0_1 = SHRD52(t2, acc0_1);

            fma52x8lo_mem(R0_1, R0_1, Bi, a_1, 64*0)
            fma52x8lo_mem(R1_1, R1_1, Bi, a_1, 64*1)
            fma52x8lo_mem_len(R2_1, R2_1, Bi, a_1, 64*2, 4)
            fma52x8lo_mem(R0_1, R0_1, Yi, m_1, 64*0)
            fma52x8lo_mem(R1_1, R1_1, Yi, m_1, 64*1)
            fma52x8lo_mem_len(R2_1, R2_1, Yi, m_1, 64*2, 4)

            shift64_imm(R0_1, R0_1h, 1)
            shift64_imm(R0_1h, R1_1, 1)
            shift64_imm(R1_1, R1_1h, 1)
            shift64_imm(R1_1h, R2_1, 1)
            shift64_imm(R2_1, get_zero64(), 1)

            /* "shift" R */
            t0 = get64(R0_1, 0);
            acc0_1 += t0;

            /* U = A*Bi (hi) */
            fma52x8hi_mem(R0_1, R0_1, Bi, a_1, 64*0)
            fma52x8hi_mem(R1_1, R1_1, Bi, a_1, 64*1)
            fma52x8hi_mem_len(R2_1, R2_1, Bi, a_1, 64*2, 4)
            /* R += M*Yi (hi) */
            fma52x8hi_mem(R0_1, R0_1, Yi, m_1, 64*0)
            fma52x8hi_mem(R1_1, R1_1, Yi, m_1, 64*1)
            fma52x8hi_mem_len(R2_1, R2_1, Yi, m_1, 64*2, 4)
        }
    }
    {
        /* Normalize and store idx=0 */
        /* Set R0.0 == acc0 */
        U64 Bi = set64((long long)acc0_0);
        R0_0 = blend64(R0_0, Bi, 1);
        NORMALIZE_52x20(R0_0, R1_0, R2_0)
        storeu64(out[0] + 0*4, R0_0);
        storeu64(out[0] + 1*4, R0_0h);
        storeu64(out[0] + 2*4, R1_0);
        storeu64(out[0] + 3*4, R1_0h);
        storeu64(out[0] + 4*4, R2_0);
    }
    {
        /* Normalize and store idx=1 */
        /* Set R0.1 == acc1 */
        U64 Bi = set64((long long)acc0_1);
        R0_1 = blend64(R0_1, Bi, 1);
        NORMALIZE_52x20(R0_1, R1_1, R2_1)
        storeu64(out[1] + 0*4, R0_1);
        storeu64(out[1] + 1*4, R0_1h);
        storeu64(out[1] + 2*4, R1_1);
        storeu64(out[1] + 3*4, R1_1h);
        storeu64(out[1] + 4*4, R2_1);
    }
}

IPP_OWN_DEFN(void, ifma256_ams52x20_dual, (Ipp64u out[2][20],
                                     const Ipp64u a  [2][20],
                                     const Ipp64u m  [2][20],
                                     const Ipp64u k0 [2]))
{
    ifma256_amm52x20_dual(out, a, a, m, k0);
}

#endif

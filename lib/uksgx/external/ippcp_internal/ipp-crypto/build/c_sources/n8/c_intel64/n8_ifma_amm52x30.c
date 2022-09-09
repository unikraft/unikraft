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
 *   Data represented as (30+2)-qwords arrays in 2^52-radix.
 *
 *   Note: 2 high qwords - zero padding.
 *
 */
IPP_OWN_DEFN(void, ifma256_amm52x30, (Ipp64u out[32],
                                const Ipp64u a  [32],
                                const Ipp64u b  [32],
                                const Ipp64u m  [32],
                                      Ipp64u k0))
{
   U64 R0 = get_zero64();
   U64 R1 = get_zero64();
   U64 R2 = get_zero64();
   U64 R3 = get_zero64();

   U64 R0h = get_zero64();
   U64 R1h = get_zero64();
   U64 R2h = get_zero64();
   U64 R3h = get_zero64();

   U64 Bi, Yi;

   Ipp64u m0 = m[0];
   Ipp64u a0 = a[0];
   Ipp64u acc0 = 0;

   int i;
   for (i=0; i<30; i++) {
      Ipp64u t0, t1, t2, yi;

      Bi = set64((long long)b[i]);               /* broadcast(b[i]) */
      /* compute yi */
      t0 = _mulx_u64(a0, b[i], &t2);             /* (t2:t0) = acc0 + a[0]*b[i] */
      ADD104(t2, acc0, 0, t0)
      yi = (acc0 * k0)  & EXP_DIGIT_MASK_AVX512; /* yi = acc0*k0 */
      Yi = set64((long long)yi);

      t0 = _mulx_u64(m0, yi, &t1);               /* (t1:t0)   = m0*yi     */
      ADD104(t2, acc0, t1, t0)                   /* (t2:acc0) += (t1:t0)  */
      acc0 = SHRD52(t2, acc0);

      fma52x8lo_mem(R0, R0, Bi, a, 64*0)
      fma52x8lo_mem(R1, R1, Bi, a, 64*1)
      fma52x8lo_mem(R2, R2, Bi, a, 64*2)
      fma52x8lo_mem(R3, R3, Bi, a, 64*3)

      fma52x8lo_mem(R0, R0, Yi, m, 64*0)
      fma52x8lo_mem(R1, R1, Yi, m, 64*1)
      fma52x8lo_mem(R2, R2, Yi, m, 64*2)
      fma52x8lo_mem(R3, R3, Yi, m, 64*3)

      shift64_imm(R0, R0h, 1)
      shift64_imm(R0h, R1, 1)
      shift64_imm(R1, R1h, 1)
      shift64_imm(R1h, R2, 1)
      shift64_imm(R2, R2h, 1)
      shift64_imm(R2h, R3, 1)
      shift64_imm(R3, R3h, 1)
      shift64_imm(R3h, get_zero64(), 1)

      /* "shift" R */
      t0 = get64(R0, 0);
      acc0 += t0;

      /* U = A*Bi (hi) */
      fma52x8hi_mem(R0, R0, Bi, a, 64*0)
      fma52x8hi_mem(R1, R1, Bi, a, 64*1)
      fma52x8hi_mem(R2, R2, Bi, a, 64*2)
      fma52x8hi_mem(R3, R3, Bi, a, 64*3)
      /* R += M*Yi (hi) */
      fma52x8hi_mem(R0, R0, Yi, m, 64*0)
      fma52x8hi_mem(R1, R1, Yi, m, 64*1)
      fma52x8hi_mem(R2, R2, Yi, m, 64*2)
      fma52x8hi_mem(R3, R3, Yi, m, 64*3)
   }

   /* Set R0[0] == acc0 */
   Bi = set64((long long)acc0);
   R0 = blend64(R0, Bi, 1);

   NORMALIZE_52x30(R0, R1, R2, R3)

   storeu64(out + 0*4, R0);
   storeu64(out + 1*4, R0h);
   storeu64(out + 2*4, R1);
   storeu64(out + 3*4, R1h);
   storeu64(out + 4*4, R2);
   storeu64(out + 5*4, R2h);
   storeu64(out + 6*4, R3);
   storeu64(out + 7*4, R3h);
}

IPP_OWN_DEFN(void, ifma256_ams52x30, (Ipp64u out[32],
                                const Ipp64u a  [32],
                                const Ipp64u m  [32],
                                      Ipp64u k0))
{
    ifma256_amm52x30(out, a, a, m, k0);
}

#endif

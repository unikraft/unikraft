/*******************************************************************************
* Copyright 2021-2021 Intel Corporation
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

#ifndef IFMA_ARITH_P25519_H
#define IFMA_ARITH_P25519_H

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>

/* underlying prime bitsize */
#define P25519_BITSIZE (255)

/* lengths of FF elements */
#define FE_LEN52  NUMBER_OF_DIGITS(P25519_BITSIZE, DIGIT_SIZE)
#define FE_LEN64  NUMBER_OF_DIGITS(P25519_BITSIZE, 64)

/* scalar field element (FE)*/
typedef int64u fe64[FE_LEN64];
typedef int64u fe52[FE_LEN52];

/* mb field element */
typedef U64 fe64_mb[FE_LEN64];
typedef U64 fe52_mb[FE_LEN52];

/* bitsize basepoint order L */
//#define L254_BITSIZE (254)
//#define L_LEN64  NUMBER_OF_DIGITS(L254_BITSIZE, 64)


/* set FE to zero */
__INLINE void fe52_0_mb(fe52_mb fe)
{
   fe[0] = fe[1] = fe[2] = fe[3] = fe[4] = get_zero64();
}
/* set FE to 1 */
__INLINE void fe52_1_mb(fe52_mb fe)
{
   fe[0] = set1(1LL);
   fe[1] = fe[2] = fe[3] = fe[4] = get_zero64();
}

/* copy FE */
__INLINE void fe52_copy_mb(fe52_mb r, const fe52_mb a)
{
   r[0] = a[0];
   r[1] = a[1];
   r[2] = a[2];
   r[3] = a[3];
   r[4] = a[4];
}

/* check if FE is zero */
__INLINE __mb_mask fe52_mb_is_zero(const fe52_mb a)
{
   U64 t = or64(or64(a[0], a[1]), or64(or64(a[2], a[3]), a[4]));
   return cmpeq64_mask(t, get_zero64());
}

/* move FE under mask (conditionally): r = k? a : b */
__INLINE void fe52_cmov1_mb(fe52_mb r, const fe52_mb b, __mb_mask k, const fe52 a)
{
   r[0] = mask_mov64(b[0], k, set1(a[0]));
   r[1] = mask_mov64(b[1], k, set1(a[1]));
   r[2] = mask_mov64(b[2], k, set1(a[2]));
   r[3] = mask_mov64(b[3], k, set1(a[3]));
   r[4] = mask_mov64(b[4], k, set1(a[4]));
}
__INLINE void fe52_cmov_mb(fe52_mb r, const fe52_mb b, __mb_mask k, const fe52_mb a)
{
   r[0] = mask_mov64(b[0], k, a[0]);
   r[1] = mask_mov64(b[1], k, a[1]);
   r[2] = mask_mov64(b[2], k, a[2]);
   r[3] = mask_mov64(b[3], k, a[3]);
   r[4] = mask_mov64(b[4], k, a[4]);
}

/* swap FE under mask (conditionally): r = k? a : b */
__INLINE void cswap_U64(U64* x, __mb_mask k, U64* y)
{
   *x = _mm512_mask_xor_epi64(*x, k, *x, *y);
   *y = _mm512_mask_xor_epi64(*y, k, *y, *x);
   *x = _mm512_mask_xor_epi64(*x, k, *x, *y);
}
__INLINE void fe52_cswap_mb(fe52_mb a, __mb_mask k, fe52_mb b)
{
   cswap_U64(&a[0], k, &b[0]);
   cswap_U64(&a[1], k, &b[1]);
   cswap_U64(&a[2], k, &b[2]);
   cswap_U64(&a[3], k, &b[3]);
   cswap_U64(&a[4], k, &b[4]);
}


void fe52_mb_add_mod25519(fe52_mb vr, const fe52_mb va, const fe52_mb vb);
void fe52_mb_sub_mod25519(fe52_mb vr, const fe52_mb va, const fe52_mb vb);
void fe52_mb_neg_mod25519(fe52_mb vr, const fe52_mb va);
void fe52_mb_mul_mod25519(fe52_mb vr, const fe52_mb va, const fe52_mb vb);
void fe52_mb_sqr_mod25519(fe52_mb vr, const fe52_mb va);
void fe52_mb_inv_mod25519(fe52_mb vr, const fe52_mb va);

#define fe52_add  fe52_mb_add_mod25519
#define fe52_sub  fe52_mb_sub_mod25519
#define fe52_neg  fe52_mb_neg_mod25519
#define fe52_mul  fe52_mb_mul_mod25519
#define fe52_sqr  fe52_mb_sqr_mod25519
#define fe52_inv  fe52_mb_inv_mod25519
#define fe52_p2n  fe52_mb_sqr_mod25519_times

#endif /* IFMA_ARITH_P25519_H */

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

#include <internal/ed25519/ifma_arith_p25519.h>

/*
// prime25519 = 2^255-19
// in the radix 2^52
// int64u prime25519[5] = {PRIME25519_LO, PRIME25519_MID, PRIME25519_MID, PRIME25519_MID, PRIME25519_HI}
*/
#define PRIME25519_LO  0x000FFFFFFFFFFFED
#define PRIME25519_MID 0x000FFFFFFFFFFFFF
#define PRIME25519_HI  0x00007FFFFFFFFFFF

__ALIGN64 static const int64u VPRIME25519_LO[sizeof(U64) / sizeof(int64u)] = {
   REP8_DECL(PRIME25519_LO)
};
__ALIGN64 static const int64u VPRIME25519_MID[sizeof(U64) / sizeof(int64u)] = {
   REP8_DECL(PRIME25519_MID)
};
__ALIGN64 static const int64u VPRIME25519_HI[sizeof(U64) / sizeof(int64u)] = {
   REP8_DECL(PRIME25519_HI)
};


/* normalization macros */
#define NORM_LSHIFTR(R, I, J) \
    R##J = add64(R##J, srli64(R##I, DIGIT_SIZE)); \
    R##I = and64_const(R##I, DIGIT_MASK);
#define NORM_ASHIFTR(R, I, J) \
    R##J = add64(R##J, srai64(R##I, DIGIT_SIZE)); \
    R##I = and64_const(R##I, DIGIT_MASK);


//__INLINE
void fe52_mb_add_mod25519(fe52_mb vr, const fe52_mb va, const fe52_mb vb)
{
   /* r = a+b */
   U64 r0 = add64(va[0], vb[0]);
   U64 r1 = add64(va[1], vb[1]);
   U64 r2 = add64(va[2], vb[2]);
   U64 r3 = add64(va[3], vb[3]);
   U64 r4 = add64(va[4], vb[4]);

   /* t = r-modulus (2^255-19) */
   U64 t0 = sub64(r0, loadu64(VPRIME25519_LO));
   U64 t1 = sub64(r1, loadu64(VPRIME25519_MID));
   U64 t2 = sub64(r2, loadu64(VPRIME25519_MID));
   U64 t3 = sub64(r3, loadu64(VPRIME25519_MID));
   U64 t4 = sub64(r4, loadu64(VPRIME25519_HI));

   /* normalize r0, r1, r2, r3, r4 */
   NORM_LSHIFTR(r, 0, 1)
   NORM_LSHIFTR(r, 1, 2)
   NORM_LSHIFTR(r, 2, 3)
   NORM_LSHIFTR(r, 3, 4)

   /* normalize t0, t1, t2, t3, t4 */
   NORM_ASHIFTR(t, 0, 1)
   NORM_ASHIFTR(t, 1, 2)
   NORM_ASHIFTR(t, 2, 3)
   NORM_ASHIFTR(t, 3, 4)

   /* condition mask t4<0? (-1) : 0 */
   __mb_mask cmask = cmp64_mask(t4, get_zero64(), _MM_CMPINT_LT);

   vr[0] = mask_mov64(t0, cmask, r0);
   vr[1] = mask_mov64(t1, cmask, r1);
   vr[2] = mask_mov64(t2, cmask, r2);
   vr[3] = mask_mov64(t3, cmask, r3);
   vr[4] = mask_mov64(t4, cmask, r4);
}

//__INLINE
void fe52_mb_sub_mod25519(fe52_mb vr, const fe52_mb va, const fe52_mb vb)
{
   /* r = a-b */
   U64 r0 = sub64(va[0], vb[0]);
   U64 r1 = sub64(va[1], vb[1]);
   U64 r2 = sub64(va[2], vb[2]);
   U64 r3 = sub64(va[3], vb[3]);
   U64 r4 = sub64(va[4], vb[4]);

   /* t = r+modulus (2^255-19) */
   U64 t0 = add64(r0, loadu64(VPRIME25519_LO));
   U64 t1 = add64(r1, loadu64(VPRIME25519_MID));
   U64 t2 = add64(r2, loadu64(VPRIME25519_MID));
   U64 t3 = add64(r3, loadu64(VPRIME25519_MID));
   U64 t4 = add64(r4, loadu64(VPRIME25519_HI));

   /* normalize r0, r1, r2, r3, r4 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)

   /* normalize t0, t1, t2, t3, t4 */
   NORM_ASHIFTR(t, 0, 1)
   NORM_ASHIFTR(t, 1, 2)
   NORM_ASHIFTR(t, 2, 3)
   NORM_ASHIFTR(t, 3, 4)

   /* condition mask r4<0? (-1) : 0 */
   __mb_mask cmask = cmp64_mask(r4, get_zero64(), _MM_CMPINT_LT);

   vr[0] = mask_mov64(r0, cmask, t0);
   vr[1] = mask_mov64(r1, cmask, t1);
   vr[2] = mask_mov64(r2, cmask, t2);
   vr[3] = mask_mov64(r3, cmask, t3);
   vr[4] = mask_mov64(r4, cmask, t4);
}

//__INLINE
void fe52_mb_neg_mod25519(fe52_mb vr, const fe52_mb va)
{
   __mb_mask non_zero = ~fe52_mb_is_zero(va);

   /* r = is_zero? a : p-a */
   U64 r0 = mask_sub64(va[0], non_zero, loadu64(VPRIME25519_LO),  va[0]);
   U64 r1 = mask_sub64(va[1], non_zero, loadu64(VPRIME25519_MID), va[1]);
   U64 r2 = mask_sub64(va[2], non_zero, loadu64(VPRIME25519_MID), va[2]);
   U64 r3 = mask_sub64(va[3], non_zero, loadu64(VPRIME25519_MID), va[3]);
   U64 r4 = mask_sub64(va[4], non_zero, loadu64(VPRIME25519_HI),  va[4]);

   /* normalize r0, r1, r2, r3, r4 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)

   vr[0] = r0;
   vr[1] = r1;
   vr[2] = r2;
   vr[3] = r3;
   vr[4] = r4;
}

/*
// multiplicative operations
*/

/* full multiplication macros */
#define ROUND_MUL_SRC(I, J, S_LO, R_LO, S_HI, R_HI) \
    R_LO = fma52lo(S_LO, va[I], vb[J]); \
    R_HI = fma52hi(S_HI, va[I], vb[J]);

#define ROUND_MUL(I, J, M0, M1) \
    ROUND_MUL_SRC(I, J, M0, M0, M1, M1)

/* reduction constants and macros */
#define MASK47 ((1ULL << (255 - 52 * 4)) - 1)

__ALIGN64 static const int64u    MASK47_[sizeof(U64) / sizeof(int64u)] = { REP8_DECL(MASK47) };
__ALIGN64 static const int64u MOD_2_255_[sizeof(U64) / sizeof(int64u)] = { REP8_DECL(19) };
__ALIGN64 static const int64u MOD_2_260_[sizeof(U64) / sizeof(int64u)] = { REP8_DECL(19*32) };

#define MASK_47   loadu64(MASK47_)
#define MOD_2_255 loadu64(MOD_2_255_)
#define MOD_2_260 loadu64(MOD_2_260_)

#define REDUCE_ROUND(R0, R1, R5) \
    r##R0 = fma52lo(r##R0, r##R5, MOD_2_260); \
    r##R1 = fma52lo( fma52hi(r##R1, r##R5, MOD_2_260), \
        srli64(r##R5, 52), MOD_2_260);


//__INLINE
void fe52_mb_mul_mod25519(fe52_mb vr, const fe52_mb va, const fe52_mb vb)
{
   U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

   /* full multiplication */
   ROUND_MUL(4, 4, r8, r9);
   ROUND_MUL(3, 0, r3, r4);
   ROUND_MUL(1, 2, r3, r4);
   ROUND_MUL(0, 3, r3, r4);
   ROUND_MUL(2, 1, r3, r4);
   ROUND_MUL(2, 2, r4, r5);
   ROUND_MUL(0, 4, r4, r5);
   ROUND_MUL(1, 3, r4, r5);
   ROUND_MUL(3, 1, r4, r5);
   ROUND_MUL(4, 0, r4, r5);

   ROUND_MUL(1, 4, r5, r6);
   ROUND_MUL(2, 3, r5, r6);
   ROUND_MUL(3, 2, r5, r6);
   ROUND_MUL(4, 1, r5, r6);
   ROUND_MUL(2, 4, r6, r7);
   ROUND_MUL(3, 3, r6, r7);
   ROUND_MUL(4, 2, r6, r7);

   ROUND_MUL(0, 0, r0, r1);
   ROUND_MUL(0, 1, r1, r2);
   ROUND_MUL(0, 2, r2, r3);
   ROUND_MUL(1, 0, r1, r2);
   ROUND_MUL(1, 1, r2, r3);
   ROUND_MUL(2, 0, r2, r3);
   ROUND_MUL(3, 4, r7, r8);
   ROUND_MUL(4, 3, r7, r8);

   /* reduce r4 upper bits */
   r4 = fma52lo(r4, r9, MOD_2_260);
   r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);
   r4 = and64(r4, MASK_47);
   /* rest of reduction */
   REDUCE_ROUND(0, 1, 5);
   REDUCE_ROUND(1, 2, 6);
   REDUCE_ROUND(2, 3, 7);
   REDUCE_ROUND(3, 4, 8);

   /* normalize result */
   NORM_LSHIFTR(r, 0, 1);
   NORM_LSHIFTR(r, 1, 2);
   NORM_LSHIFTR(r, 2, 3);
   NORM_LSHIFTR(r, 3, 4);

   vr[0] = r0;
   vr[1] = r1;
   vr[2] = r2;
   vr[3] = r3;
   vr[4] = r4;
}

/* SQR
c=0  (0,0)
c=1  (0,1)
c=2  (0,2)  (1,1)
c=3  (0,3)  (1,2)
c=4  (0,4)  (1,3)  (2,2)
c=5  (1,4)  (2,3)
c=6  (2,4)  (3,3)
c=7  (3,4)
c=8  (4,4)
*/
//__INLINE
void fe52_mb_sqr_mod25519(fe52_mb vr, const fe52_mb va)
{
   U64 *vb = (U64*)va;

   U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

   // Square
   ROUND_MUL(0, 1, r1, r2);
   ROUND_MUL(0, 2, r2, r3);
   ROUND_MUL(0, 3, r3, r4);
   ROUND_MUL(0, 4, r4, r5);
   ROUND_MUL(1, 4, r5, r6);
   ROUND_MUL(2, 4, r6, r7);
   ROUND_MUL(3, 4, r7, r8);

   ROUND_MUL(1, 2, r3, r4);
   ROUND_MUL(1, 3, r4, r5);
   ROUND_MUL(2, 3, r5, r6);

   r1 = add64(r1, r1);
   r2 = add64(r2, r2);
   r3 = add64(r3, r3);
   r4 = add64(r4, r4);
   r5 = add64(r5, r5);
   r6 = add64(r6, r6);
   r7 = add64(r7, r7);
   r8 = add64(r8, r8);

   ROUND_MUL(0, 0, r0, r1);
   ROUND_MUL(1, 1, r2, r3);
   ROUND_MUL(2, 2, r4, r5);
   ROUND_MUL(3, 3, r6, r7);
   ROUND_MUL(4, 4, r8, r9);

   /* reduce r4 upper bits */
   r4 = fma52lo(r4, r9, MOD_2_260);
   r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);
   r4 = and64(r4, MASK_47);
   /* rest of reduction */
   REDUCE_ROUND(0, 1, 5);
   REDUCE_ROUND(1, 2, 6);
   REDUCE_ROUND(2, 3, 7);
   REDUCE_ROUND(3, 4, 8);

   /* normalize result */
   NORM_LSHIFTR(r, 0, 1);
   NORM_LSHIFTR(r, 1, 2);
   NORM_LSHIFTR(r, 2, 3);
   NORM_LSHIFTR(r, 3, 4);

   vr[0] = r0;
   vr[1] = r1;
   vr[2] = r2;
   vr[3] = r3;
   vr[4] = r4;
}


/* variant of ROUND_MUL_SRC() and ROUND_MUL() macros */
#define ROUND_MUL_SRC_A(I, J, S_LO, R_LO, S_HI, R_HI) \
    R_LO = fma52lo(S_LO, a##I, a##J); \
    R_HI = fma52hi(S_HI, a##I, a##J);

#define ROUND_MUL_A(I, J, M0, M1) \
    ROUND_MUL_SRC_A(I, J, M0, M0, M1, M1)

void fe52_mb_sqr_mod25519_times(fe52_mb vr, const fe52_mb va, int count)
{
   U64 a0 = va[0];
   U64 a1 = va[1];
   U64 a2 = va[2];
   U64 a3 = va[3];
   U64 a4 = va[4];

   U64 r0, r1, r2, r3, r4_1, r4, r5, r6, r7, r8, r9;

   for(int i=0; i<count; ++i) {
      r0 = r1 = r2 = r3 = r4_1 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

      /* full square */
      ROUND_MUL_A(0, 1, r1, r2);
      ROUND_MUL_A(0, 2, r2, r3);
      ROUND_MUL_A(0, 3, r3, r4_1);
      ROUND_MUL_A(0, 4, r4_1, r5);
      ROUND_MUL_A(1, 4, r5, r6);
      ROUND_MUL_A(2, 4, r6, r7);
      ROUND_MUL_A(3, 4, r7, r8);

      ROUND_MUL_A(1, 2, r3, r4);
      ROUND_MUL_A(1, 3, r4, r5);
      ROUND_MUL_A(2, 3, r5, r6);

      r1 = add64(r1, r1);
      r2 = add64(r2, r2);
      r3 = add64(r3, r3);

      r4 = add64(r4, r4_1);
      r4 = add64(r4, r4);

      r5 = add64(r5, r5);
      r6 = add64(r6, r6);
      r7 = add64(r7, r7);
      r8 = add64(r8, r8);

      ROUND_MUL_A(0, 0, r0, r1);
      ROUND_MUL_A(1, 1, r2, r3);
      ROUND_MUL_A(2, 2, r4, r5);
      ROUND_MUL_A(3, 3, r6, r7);
      ROUND_MUL_A(4, 4, r8, r9);

      /* reduce r4 upper bits */
      r4 = fma52lo(r4, r9, MOD_2_260);
      r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);
      r4 = and64(r4, MASK_47);
      /* rest of reduction */
      REDUCE_ROUND(0, 1, 5);
      REDUCE_ROUND(1, 2, 6);
      REDUCE_ROUND(2, 3, 7);
      REDUCE_ROUND(3, 4, 8);

      /* normalize result */
      NORM_LSHIFTR(r, 0, 1);
      NORM_LSHIFTR(r, 1, 2);
      NORM_LSHIFTR(r, 2, 3);
      NORM_LSHIFTR(r, 3, 4);

      a0 = r0;
      a1 = r1;
      a2 = r2;
      a3 = r3;
      a4 = r4;
   }

   vr[0] = a0;
   vr[1] = a1;
   vr[2] = a2;
   vr[3] = a3;
   vr[4] = a4;
}


/*
   Compute 1/z = z^(2^255 - 19 - 2)
   considering the exponent as
   2^255 - 21 = (2^5) * (2^250 - 1) + 11.
*/
//__INLINE
void fe52_mb_inv_mod25519(fe52_mb r, const fe52_mb z)
{
   __ALIGN64 fe52_mb t0;
   __ALIGN64 fe52_mb t1;
   __ALIGN64 fe52_mb t2;
   __ALIGN64 fe52_mb t3;

   /* t0 = z ** 2 */
   fe52_sqr(t0, z);

   /* t1 = t0 ** (2 ** 2) = z ** 8 */
   fe52_sqr(t1, t0);
   fe52_sqr(t1, t1);

   /* t1 = z * t1 = z ** 9 */
   fe52_mul(t1, z, t1);
   /* t0 = t0 * t1 = z ** 11 -- stash t0 away for the end. */
   fe52_mul(t0, t0, t1);

   /* t2 = t0 ** 2 = z ** 22 */
   fe52_sqr(t2, t0);

   /* t1 = t1 * t2 = z ** (2 ** 5 - 1) */
   fe52_mul(t1, t1, t2);

   /* t2 = t1 ** (2 ** 5) = z ** ((2 ** 5) * (2 ** 5 - 1)) */
   fe52_p2n(t2, t1, 5);

   /* t1 = t1 * t2 = z ** ((2 ** 5 + 1) * (2 ** 5 - 1)) = z ** (2 ** 10 - 1) */
   fe52_mul(t1, t2, t1);

   /* Continuing similarly... */

   /* t2 = z ** (2 ** 20 - 1) */
   fe52_p2n(t2, t1, 10);

   fe52_mul(t2, t2, t1);

   /* t2 = z ** (2 ** 40 - 1) */
   fe52_p2n(t3, t2, 20);

   fe52_mul(t2, t3, t2);

   /* t2 = z ** (2 ** 10) * (2 ** 40 - 1) */
   fe52_p2n(t2, t2, 10);

   /* t1 = z ** (2 ** 50 - 1) */
   fe52_mul(t1, t2, t1);

   /* t2 = z ** (2 ** 100 - 1) */
   fe52_p2n(t2, t1, 50);

   fe52_mul(t2, t2, t1);

   /* t2 = z ** (2 ** 200 - 1) */
   fe52_p2n(t3, t2, 100);

   fe52_mul(t2, t3, t2);

   /* t2 = z ** ((2 ** 50) * (2 ** 200 - 1) */
   fe52_p2n(t2, t2, 50);

   /* t1 = z ** (2 ** 250 - 1) */
   fe52_mul(t1, t2, t1);

   /* t1 = z ** ((2 ** 5) * (2 ** 250 - 1)) */
   fe52_p2n(t1, t1, 5);

   /* Recall t0 = z ** 11; out = z ** (2 ** 255 - 21) */
   fe52_mul(r, t1, t0);
}

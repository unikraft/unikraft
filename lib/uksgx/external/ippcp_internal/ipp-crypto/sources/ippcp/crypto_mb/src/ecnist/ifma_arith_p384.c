/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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

#include <internal/ecnist/ifma_arith_p384.h>

/* Constants */

/*
// p384 = 2^384 - 2^128 - 2^96 + 2^32 - 1
// in 2^52 radix
*/
__ALIGN64 static const int64u p384_mb[P384_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x00000000ffffffff) },
   { REP8_DECL(0x000ff00000000000) },
   { REP8_DECL(0x000ffffffeffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x00000000000fffff) }
};

/* 2*p384 */
__ALIGN64 static const int64u p384_x2[P384_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x00000001fffffffe) },
   { REP8_DECL(0x000fe00000000000) },
   { REP8_DECL(0x000ffffffdffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x00000000001fffff) }
};

/*
// Note that
// k0 = -(1/p384 mod 2^DIGIT_SIZE)  equals 1
// The implementation takes this fact into account
*/

/* to Montgomery conversion constant
// rr = 2^((P384_LEN52*DIGIT_SIZE)*2) mod p384
*/
__ALIGN64 static const int64u p384_rr_mb[P384_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x000fe00000001000) },
   { REP8_DECL(0x0000000000ffffff) },
   { REP8_DECL(0x0000000000000020) },
   { REP8_DECL(0x0000fffffffe0000) },
   { REP8_DECL(0x0000000020000000) },
   { REP8_DECL(0x0000000000000100) },
   { REP8_DECL(0x0000000000000000) }
};


/*=====================================================================

 Specialized operations over p384:  sqr & mul

=====================================================================*/

#define ROUND_MUL(I, J, R_LO, R_HI) \
    R_LO = fma52lo(R_LO, va[I], vb[J]); \
    R_HI = fma52hi(R_HI, va[I], vb[J]);

#define P384_REDUCTION_ROUND(u, r0, r1, r2, r3, r4, r5, r6, r7, r8, modulus) \
{  \
   U64 t = sub64(get_zero64(), u); \
   U64 lo = and64(t, loadu64(VMASK52)); \
   U64 hi = add64(u, srai64(t,63)); \
   \
   r0 = fma52lo(r0, u, modulus[0]); \
   r1 = fma52lo(r1, u, modulus[1]); \
   r2 = fma52lo(r2, u, modulus[2]); \
   r3 = add64(r3, lo); /*r3 = fma52lo(r3, u, modulus[3]); */ \
   r4 = add64(r4, lo); /*r4 = fma52lo(r4, u, modulus[4]); */ \
   r5 = add64(r5, lo); /*r5 = fma52lo(r5, u, modulus[5]); */ \
   r6 = add64(r6, lo); /*r6 = fma52lo(r6, u, modulus[6]); */ \
   r7 = fma52lo(r7, u, modulus[7]); \
   \
      r1 = add64(r1, srli64(r0, DIGIT_SIZE)); /* carry propagation */ \
   \
   r1 = fma52hi(r1, u, modulus[0]); \
   r2 = fma52hi(r2, u, modulus[1]); \
   r3 = fma52hi(r3, u, modulus[2]); \
   r8 = fma52hi(r8, u, modulus[7]); \
      u = and64(add64(r1, slli64(r1, 32)), loadu64(VMASK52)); /* update u = r1*k, k = (2^32 +1) */ \
   r4 = add64(r4, hi); /*r4 = fma52hi(r4, u, modulus[3]); */ \
   r5 = add64(r5, hi); /*r5 = fma52hi(r5, u, modulus[4]); */ \
   r6 = add64(r6, hi); /*r6 = fma52hi(r6, u, modulus[5]); */ \
   r7 = add64(r7, hi); /*r7 = fma52hi(r7, u, modulus[6]); */ \
}

void MB_FUNC_NAME(ifma_amm52_p384_)(U64 r[], const U64 va[], const U64 vb[])
{
   U64 r0, r1, r2, r3, r4, r5, r6, r7;
   int itr;

   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = get_zero64();

   for(itr=0; itr < P384_LEN52; itr++) {
      U64 Yi, T, LO, HI;
      U64 Bi = loadu64(vb);
      vb++;

      fma52lo_mem(r0, r0, Bi, va, SIMD_BYTES * 0);
         Yi = and64(add64(r0, slli64(r0, 32)), loadu64(VMASK52)); /* k = (2^32 +1) */
         T = sub64(get_zero64(), Yi);
         LO = and64(T, loadu64(VMASK52));
         HI = add64(Yi, srai64(T,63));
      fma52lo_mem(r1, r1, Bi, va, SIMD_BYTES * 1);
      fma52lo_mem(r2, r2, Bi, va, SIMD_BYTES * 2);
      fma52lo_mem(r3, r3, Bi, va, SIMD_BYTES * 3);
      fma52lo_mem(r4, r4, Bi, va, SIMD_BYTES * 4);
      fma52lo_mem(r5, r5, Bi, va, SIMD_BYTES * 5);
      fma52lo_mem(r6, r6, Bi, va, SIMD_BYTES * 6);
      fma52lo_mem(r7, r7, Bi, va, SIMD_BYTES * 7);

      fma52lo_mem(r0, r0, Yi, p384_mb, SIMD_BYTES * 0);
      fma52lo_mem(r1, r1, Yi, p384_mb, SIMD_BYTES * 1);
         r1 = add64(r1, srli64(r0, DIGIT_SIZE));      /* carry propagation */
      fma52lo_mem(r2, r2, Yi, p384_mb, SIMD_BYTES * 2);
      r3 = add64(r3, LO);     /* fma52lo_mem(r3, r3, Yi, p384_mb, SIMD_BYTES * 3); */
      r4 = add64(r4, LO);     /* fma52lo_mem(r4, r4, Yi, p384_mb, SIMD_BYTES * 4); */
      r5 = add64(r5, LO);     /* fma52lo_mem(r5, r5, Yi, p384_mb, SIMD_BYTES * 5); */
      r6 = add64(r6, LO);     /* fma52lo_mem(r6, r6, Yi, p384_mb, SIMD_BYTES * 6); */
      fma52lo_mem(r7, r7, Yi, p384_mb, SIMD_BYTES * 7);

      fma52hi_mem(r0, r1, Bi, va, SIMD_BYTES * 0);
      fma52hi_mem(r1, r2, Bi, va, SIMD_BYTES * 1);
      fma52hi_mem(r2, r3, Bi, va, SIMD_BYTES * 2);
      fma52hi_mem(r3, r4, Bi, va, SIMD_BYTES * 3);
      fma52hi_mem(r4, r5, Bi, va, SIMD_BYTES * 4);
      fma52hi_mem(r5, r6, Bi, va, SIMD_BYTES * 5);
      fma52hi_mem(r6, r7, Bi, va, SIMD_BYTES * 6);
      fma52hi_mem(r7, get_zero64(), Bi, va, SIMD_BYTES * 7);

      fma52hi_mem(r0, r0, Yi, p384_mb, SIMD_BYTES * 0);
      fma52hi_mem(r1, r1, Yi, p384_mb, SIMD_BYTES * 1);
      fma52hi_mem(r2, r2, Yi, p384_mb, SIMD_BYTES * 2);
      r3 = add64(r3, HI);     /* fma52hi_mem(r3, r3, Yi, p384_mb, SIMD_BYTES * 3); */
      r4 = add64(r4, HI);     /* fma52hi_mem(r4, r4, Yi, p384_mb, SIMD_BYTES * 4); */
      r5 = add64(r5, HI);     /* fma52hi_mem(r5, r5, Yi, p384_mb, SIMD_BYTES * 5); */
      r6 = add64(r6, HI);     /* fma52hi_mem(r6, r6, Yi, p384_mb, SIMD_BYTES * 6); */
      fma52hi_mem(r7, r7, Yi, p384_mb, SIMD_BYTES * 7);
   }

   // normalization
   NORM_LSHIFTR(r, 0, 1)
   NORM_LSHIFTR(r, 1, 2)
   NORM_LSHIFTR(r, 2, 3)
   NORM_LSHIFTR(r, 3, 4)
   NORM_LSHIFTR(r, 4, 5)
   NORM_LSHIFTR(r, 5, 6)
   NORM_LSHIFTR(r, 6, 7)

   r[0] = r0;
   r[1] = r1;
   r[2] = r2;
   r[3] = r3;
   r[4] = r4;
   r[5] = r5;
   r[6] = r6;
   r[7] = r7;
}

void MB_FUNC_NAME(ifma_ams52_p384_)(U64 r[], const U64 va[])
{
   const U64* vb = va;
   U64* modulus = (U64*)p384_mb;

   U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 =
   r10 = r11 = r12 = r13 = r14 = r15 = get_zero64();

   // full square
   ROUND_MUL(0, 1, r1, r2)
   ROUND_MUL(0, 2, r2, r3)
   ROUND_MUL(0, 3, r3, r4)
   ROUND_MUL(0, 4, r4, r5)
   ROUND_MUL(0, 5, r5, r6)
   ROUND_MUL(0, 6, r6, r7)
   ROUND_MUL(0, 7, r7, r8)

   ROUND_MUL(1, 2, r3, r4)
   ROUND_MUL(1, 3, r4, r5)
   ROUND_MUL(1, 4, r5, r6)
   ROUND_MUL(1, 5, r6, r7)
   ROUND_MUL(1, 6, r7, r8)
   ROUND_MUL(1, 7, r8, r9)

   ROUND_MUL(2, 3, r5, r6)
   ROUND_MUL(2, 4, r6, r7)
   ROUND_MUL(2, 5, r7, r8)
   ROUND_MUL(2, 6, r8, r9)
   ROUND_MUL(2, 7, r9, r10)

   ROUND_MUL(3, 4, r7, r8)
   ROUND_MUL(3, 5, r8, r9)
   ROUND_MUL(3, 6, r9, r10)
   ROUND_MUL(3, 7, r10,r11)

   ROUND_MUL(4, 5, r9, r10)
   ROUND_MUL(4, 6, r10,r11)
   ROUND_MUL(4, 7, r11,r12)

   ROUND_MUL(5, 6, r11,r12)
   ROUND_MUL(5, 7, r12,r13)

   ROUND_MUL(6, 7, r13,r14)

   r1  = add64(r1,  r1);
   r2  = add64(r2,  r2);
   r3  = add64(r3,  r3);
   r4  = add64(r4,  r4);
   r5  = add64(r5,  r5);
   r6  = add64(r6,  r6);
   r7  = add64(r7,  r7);
   r8  = add64(r8,  r8);
   r9  = add64(r9,  r9);
   r10 = add64(r10, r10);
   r11 = add64(r11, r11);
   r12 = add64(r12, r12);
   r13 = add64(r13, r13);
   r14 = add64(r14, r14);

   U64 u;
   ROUND_MUL(0, 0, r0, r1)
   u = and64(add64(r0, slli64(r0, 32)), loadu64(VMASK52));
   ROUND_MUL(1, 1, r2, r3)
   ROUND_MUL(2, 2, r4, r5)
   ROUND_MUL(3, 3, r6, r7)
   ROUND_MUL(4, 4, r8, r9)
   ROUND_MUL(5, 5, r10, r11)
   ROUND_MUL(6, 6, r12, r13)
   ROUND_MUL(7, 7, r14, r15)

   // reduction
   P384_REDUCTION_ROUND(u, r0, r1, r2, r3, r4, r5, r6, r7, r8,  modulus);
   P384_REDUCTION_ROUND(u, r1, r2, r3, r4, r5, r6, r7, r8, r9,  modulus);
   P384_REDUCTION_ROUND(u, r2, r3, r4, r5, r6, r7, r8, r9, r10, modulus);
   P384_REDUCTION_ROUND(u, r3, r4, r5, r6, r7, r8, r9, r10,r11, modulus);
   P384_REDUCTION_ROUND(u, r4, r5, r6, r7, r8, r9, r10,r11,r12, modulus);
   P384_REDUCTION_ROUND(u, r5, r6, r7, r8, r9, r10,r11,r12,r13, modulus);
   P384_REDUCTION_ROUND(u, r6, r7, r8, r9, r10,r11,r12,r13,r14, modulus);
   P384_REDUCTION_ROUND(u, r7, r8, r9, r10,r11,r12,r13,r14,r15, modulus);

   // normalization
   NORM_LSHIFTR(r,  8, 9)
   NORM_LSHIFTR(r,  9, 10)
   NORM_LSHIFTR(r, 10, 11)
   NORM_LSHIFTR(r, 11, 12)
   NORM_LSHIFTR(r, 12, 13)
   NORM_LSHIFTR(r, 13, 14)
   NORM_LSHIFTR(r, 14, 15)

   r[0] = r8;
   r[1] = r9;
   r[2] = r10;
   r[3] = r11;
   r[4] = r12;
   r[5] = r13;
   r[6] = r14;
   r[7] = r15;
}

void MB_FUNC_NAME(ifma_tomont52_p384_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_p384_)(r, a, (U64*)p384_rr_mb);
}

void MB_FUNC_NAME(ifma_frommont52_p384_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_p384_)(r, a, (U64*)ones);
}

/*
// computes r = 1/z = z^(p384-2) mod p384
//       => r = z^(0xFFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFE FFFFFFFF00000000 00000000FFFFFFFD)
//
// note: z in in Montgomery domain (as soon mul() and sqr() below are amm-functions
//       r in Montgomery domain too
*/
#define fe52_sqr  MB_FUNC_NAME(ifma_ams52_p384_)
#define fe52_mul  MB_FUNC_NAME(ifma_amm52_p384_)

/* r = base^(2^n) */
__INLINE void fe52_sqr_pwr(U64 r[], const U64 base[], int n)
{
   if(r!=base)
      MB_FUNC_NAME(mov_FE384_)(r, base);
   for(; n>0; n--)
      fe52_sqr(r,r);
}

void MB_FUNC_NAME(ifma_aminv52_p384_)(U64 r[], const U64 z[])
{
   __ALIGN64 U64  u[P384_LEN52];
   __ALIGN64 U64  v[P384_LEN52];
   __ALIGN64 U64 zD[P384_LEN52];
   __ALIGN64 U64 zE[P384_LEN52];
   __ALIGN64 U64 zF[P384_LEN52];

   fe52_sqr(u, z);              /* u = z^2 */
   fe52_mul(v, u, z);           /* v = z^2 * z     = z^3  */
   fe52_sqr_pwr(zF, v, 2);      /* zF= (z^3)^(2^2) = z^12 */

   fe52_mul(zD, zF, z);         /* zD = z^12 * z    = z^xD */
   fe52_mul(zE, zF, u);         /* zE = z^12 * z^2  = z^xE */
   fe52_mul(zF, zF, v);         /* zF = z^12 * z^3  = z^xF */

   fe52_sqr_pwr(u, zF, 4);      /* u  = (z^xF)^(2^4)  = z^xF0 */
   fe52_mul(zD, u, zD);         /* zD = z^xF0 * z^xD  = z^xFD */
   fe52_mul(zE, u, zE);         /* zE = z^xF0 * z^xE  = z^xFE */
   fe52_mul(zF, u, zF);         /* zF = z^xF0 * z^xF  = z^xFF */

   fe52_sqr_pwr(u, zF, 8);      /* u = (z^xFF)^(2^8)    = z^xFF00 */
   fe52_mul(zD, u, zD);         /* zD = z^xFF00 * z^xFD = z^xFFFD */
   fe52_mul(zE, u, zE);         /* zE = z^xFF00 * z^xFE = z^xFFFE */
   fe52_mul(zF, u, zF);         /* zF = z^xFF00 * z^xFF = z^xFFFF */

   fe52_sqr_pwr(u, zF, 16);     /* u = (z^xFFFF)^(2^16)       = z^xFFFF0000 */
   fe52_mul(zD, u, zD);         /* zD = z^xFFFF0000 * z^xFFFD = z^xFFFFFFFD */
   fe52_mul(zE, u, zE);         /* zE = z^xFFFF0000 * z^xFFFE = z^xFFFFFFFE */
   fe52_mul(zF, u, zF);         /* zF = z^xFFFF0000 * z^xFFFF = z^xFFFFFFFF */

   fe52_sqr_pwr(u, zF, 32);     /* u = (z^xFFFFFFFF)^(2^32)               = z^xFFFFFFFF00000000 */
   fe52_mul(zE, u, zE);         /* zE = z^xFFFFFFFF00000000 * z^xFFFFFFFE = z^xFFFFFFFFFFFFFFFE */
   fe52_mul(zF, u, zF);         /* zF = z^xFFFFFFFF00000000 * z^xFFFFFFFF = z^xFFFFFFFFFFFFFFFF */

   /* v =  z^xFFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, zF, 64);
   fe52_mul(v, v, zF);
   /* v =  z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
          = z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);
   /* v =  z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFE
        = z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zE);
   /* v =  z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.0000000000000000 * z^xFFFFFFFF00000000
        = z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFF00000000 */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, u);
   /* r =  z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFF00000000.0000000000000000 * z^xFFFFFFFD
        = z^xFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFF00000000.00000000FFFFFFFD */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(r, v, zD);
}

/*=====================================================================

 Specialized single operations over p384:  add, sub, neg

=====================================================================*/
__INLINE __mb_mask MB_FUNC_NAME(lt_mbx_digit_)(const U64 a, const U64 b, const __mb_mask lt_mask)
{
   U64 d = mask_sub64(sub64(a, b), lt_mask, sub64(a, b), set1(1));
   return cmp64_mask(d, get_zero64(), _MM_CMPINT_LT);
}

void MB_FUNC_NAME(ifma_add52_p384_)(U64 R[], const U64 A[], const U64 B[])
{
   /* r = a + b */
   U64 r0 = add64(A[0], B[0]);
   U64 r1 = add64(A[1], B[1]);
   U64 r2 = add64(A[2], B[2]);
   U64 r3 = add64(A[3], B[3]);
   U64 r4 = add64(A[4], B[4]);
   U64 r5 = add64(A[5], B[5]);
   U64 r6 = add64(A[6], B[6]);
   U64 r7 = add64(A[7], B[7]);

   /* lt = {r0 - r7} < 2*p */
   __mb_mask
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r0, ((U64*)(p384_x2))[0], 0);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r1, ((U64*)(p384_x2))[1], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r2, ((U64*)(p384_x2))[2], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r3, ((U64*)(p384_x2))[3], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r4, ((U64*)(p384_x2))[4], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r5, ((U64*)(p384_x2))[5], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r6, ((U64*)(p384_x2))[6], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r7, ((U64*)(p384_x2))[7], lt);

   /* {r0 - r7} -= 2*p */
   r0 = mask_sub64(r0, ~lt, r0, ((U64*)(p384_x2))[0]);
   r1 = mask_sub64(r1, ~lt, r1, ((U64*)(p384_x2))[1]);
   r2 = mask_sub64(r2, ~lt, r2, ((U64*)(p384_x2))[2]);
   r3 = mask_sub64(r3, ~lt, r3, ((U64*)(p384_x2))[3]);
   r4 = mask_sub64(r4, ~lt, r4, ((U64*)(p384_x2))[4]);
   r5 = mask_sub64(r5, ~lt, r5, ((U64*)(p384_x2))[5]);
   r6 = mask_sub64(r6, ~lt, r6, ((U64*)(p384_x2))[6]);
   r7 = mask_sub64(r7, ~lt, r7, ((U64*)(p384_x2))[7]);

   /* normalize r0 - r7 */
   NORM_ASHIFTR(r, 0,1)
   NORM_ASHIFTR(r, 1,2)
   NORM_ASHIFTR(r, 2,3)
   NORM_ASHIFTR(r, 3,4)
   NORM_ASHIFTR(r, 4,5)
   NORM_ASHIFTR(r, 5,6)
   NORM_ASHIFTR(r, 6,7)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
   R[5] = r5;
   R[6] = r6;
   R[7] = r7;
}

void MB_FUNC_NAME(ifma_sub52_p384_)(U64 R[], const U64 A[], const U64 B[])
{
   /* r = a - b */
   U64 r0 = sub64(A[0], B[0]);
   U64 r1 = sub64(A[1], B[1]);
   U64 r2 = sub64(A[2], B[2]);
   U64 r3 = sub64(A[3], B[3]);
   U64 r4 = sub64(A[4], B[4]);
   U64 r5 = sub64(A[5], B[5]);
   U64 r6 = sub64(A[6], B[6]);
   U64 r7 = sub64(A[7], B[7]);

   /* lt = {r0 - r7} < 0 */
   __mb_mask
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r0, get_zero64(), 0);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r1, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r2, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r3, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r4, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r5, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r6, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r7, get_zero64(), lt);

   r0 = mask_add64(r0, lt, r0, ((U64*)(p384_x2))[0]);
   r1 = mask_add64(r1, lt, r1, ((U64*)(p384_x2))[1]);
   r2 = mask_add64(r2, lt, r2, ((U64*)(p384_x2))[2]);
   r3 = mask_add64(r3, lt, r3, ((U64*)(p384_x2))[3]);
   r4 = mask_add64(r4, lt, r4, ((U64*)(p384_x2))[4]);
   r5 = mask_add64(r5, lt, r5, ((U64*)(p384_x2))[5]);
   r6 = mask_add64(r6, lt, r6, ((U64*)(p384_x2))[6]);
   r7 = mask_add64(r7, lt, r7, ((U64*)(p384_x2))[7]);

   /* normalize r0 - r7 */
   NORM_ASHIFTR(r, 0,1)
   NORM_ASHIFTR(r, 1,2)
   NORM_ASHIFTR(r, 2,3)
   NORM_ASHIFTR(r, 3,4)
   NORM_ASHIFTR(r, 4,5)
   NORM_ASHIFTR(r, 5,6)
   NORM_ASHIFTR(r, 6,7)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
   R[5] = r5;
   R[6] = r6;
   R[7] = r7;
}

void MB_FUNC_NAME(ifma_neg52_p384_)(U64 R[], const U64 A[])
{
   __mb_mask nz_mask = ~MB_FUNC_NAME(is_zero_FE384_)(A);

   /* {r0 - r7} = 2*p - A */
   U64 r0 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[0], A[0] );
   U64 r1 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[1], A[1] );
   U64 r2 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[2], A[2] );
   U64 r3 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[3], A[3] );
   U64 r4 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[4], A[4] );
   U64 r5 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[5], A[5] );
   U64 r6 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[6], A[6] );
   U64 r7 = mask_sub64( A[0], nz_mask, ((U64*)(p384_x2))[7], A[7] );

   /* normalize r0 - r7 */
   NORM_ASHIFTR(r, 0,1)
   NORM_ASHIFTR(r, 1,2)
   NORM_ASHIFTR(r, 2,3)
   NORM_ASHIFTR(r, 3,4)
   NORM_ASHIFTR(r, 4,5)
   NORM_ASHIFTR(r, 5,6)
   NORM_ASHIFTR(r, 6,7)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
   R[5] = r5;
   R[6] = r6;
   R[7] = r7;
}

void MB_FUNC_NAME(ifma_double52_p384_)(U64 R[], const U64 A[])
{
   MB_FUNC_NAME(ifma_add52_p384_)(R, A, A);
}

void MB_FUNC_NAME(ifma_tripple52_p384_)(U64 R[], const U64 A[])
{
   U64 T[P384_LEN52];
   MB_FUNC_NAME(ifma_add52_p384_)(T, A, A);
   MB_FUNC_NAME(ifma_add52_p384_)(R, T, A);
}

void MB_FUNC_NAME(ifma_half52_p384_)(U64 R[], const U64 A[])
{
   U64 one = set1(1);
   U64 base = set1(DIGIT_BASE);

   /* res = a + is_odd(a)? p384 : 0 */
   U64 mask = sub64(get_zero64(), and64(A[0], one));
   U64 t0 = add64(A[0], and64(((U64*)p384_mb)[0], mask));
   U64 t1 = add64(A[1], and64(((U64*)p384_mb)[1], mask));
   U64 t2 = add64(A[2], and64(((U64*)p384_mb)[2], mask));
   U64 t3 = add64(A[3], and64(((U64*)p384_mb)[3], mask));
   U64 t4 = add64(A[4], and64(((U64*)p384_mb)[4], mask));
   U64 t5 = add64(A[5], and64(((U64*)p384_mb)[5], mask));
   U64 t6 = add64(A[6], and64(((U64*)p384_mb)[6], mask));
   U64 t7 = add64(A[7], and64(((U64*)p384_mb)[7], mask));

   /* t =>> 1 */
   mask = sub64(get_zero64(), and64(t1, one));
   t0 = add64(t0, and64(base, mask));
   t0 = srli64(t0, 1);

   mask = sub64(get_zero64(), and64(t2, one));
   t1 = add64(t1, and64(base, mask));
   t1 = srli64(t1, 1);

   mask = sub64(get_zero64(), and64(t3, one));
   t2 = add64(t2, and64(base, mask));
   t2 = srli64(t2, 1);

   mask = sub64(get_zero64(), and64(t4, one));
   t3 = add64(t3, and64(base, mask));
   t3 = srli64(t3, 1);

   mask = sub64(get_zero64(), and64(t5, one));
   t4 = add64(t4, and64(base, mask));
   t4 = srli64(t4, 1);

   mask = sub64(get_zero64(), and64(t6, one));
   t5 = add64(t5, and64(base, mask));
   t5 = srli64(t5, 1);

   mask = sub64(get_zero64(), and64(t7, one));
   t6 = add64(t6, and64(base, mask));
   t6 = srli64(t6, 1);

   t7 = srli64(t7, 1);

   /* normalize t0, t1, t2, t3, t4 */
   NORM_LSHIFTR(t, 0,1)
   NORM_LSHIFTR(t, 1,2)
   NORM_LSHIFTR(t, 2,3)
   NORM_LSHIFTR(t, 3,4)
   NORM_LSHIFTR(t, 4,5)
   NORM_LSHIFTR(t, 5,6)
   NORM_LSHIFTR(t, 6,7)

   R[0] = t0;
   R[1] = t1;
   R[2] = t2;
   R[3] = t3;
   R[4] = t4;
   R[5] = t5;
   R[6] = t6;
   R[7] = t7;
}

__mb_mask MB_FUNC_NAME(ifma_cmp_lt_p384_)(const U64 a[])
{
   return MB_FUNC_NAME(cmp_lt_FE384_)(a,(const U64 (*))p384_mb);
}

__mb_mask MB_FUNC_NAME(ifma_check_range_p384_)(const U64 A[])
{
   __mb_mask
   mask = MB_FUNC_NAME(is_zero_FE384_)(A);
   mask |= ~MB_FUNC_NAME(ifma_cmp_lt_p384_)(A);

   return mask;
}

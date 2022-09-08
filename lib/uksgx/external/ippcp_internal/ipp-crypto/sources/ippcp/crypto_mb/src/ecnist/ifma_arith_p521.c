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

#include <internal/ecnist/ifma_arith_p521.h>

/* Constants */

/*
// p521 = 2^521 - 1
// in 2^52 radix
*/
__ALIGN64 static const int64u p521_mb[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x0000000000000001) }
};

/* 2*p521 */
__ALIGN64 static const int64u p521_x2[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x000ffffffffffffe) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x0000000000000003) }
};

/*
// Note that
// k0 = -(1/p521 mod 2^DIGIT_SIZE)  equals 1
// The implementation takes this fact into account
*/

/* to Montgomery conversion constant
// rr = 2^((P521_LEN52*DIGIT_SIZE)*2) mod p521
*/
__ALIGN64 static const int64u p521_rr_mb[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0004000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) }
};


/*=====================================================================

 Specialized operations over p521:  sqr & mul

=====================================================================*/

#define ROUND_MUL(I, J, LO, HI) \
    LO = fma52lo(LO, va[I], vb[J]); \
    HI = fma52hi(HI, va[I], vb[J])

#define RED_P521_STEP(u, r0,r1, r10) { \
   U64 u = and64(r0, loadu64(VMASK52));     /* u = r0*k, k == 1 */ \
   r1 = add64(r1, srli64(r0, DIGIT_SIZE));  /* carry propagation */ \
   /* reduction */ \
   u = add64(u, u); \
   r10 = add64(r10, u); \
}

void MB_FUNC_NAME(ifma_amm52_p521_)(U64 r[], const U64 va[], const U64 vb[])
{
   U64 r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9;
   U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
   U64 r20, r21;

   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = 
   r10 = r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19=
   r20 = r21 = get_zero64();

   // full multiplication
   ROUND_MUL(0, 0, r0, r1);
   ROUND_MUL(1, 0, r1, r2);
   ROUND_MUL(2, 0, r2, r3);
   ROUND_MUL(3, 0, r3, r4);
   ROUND_MUL(4, 0, r4, r5);
   ROUND_MUL(5, 0, r5, r6);
   ROUND_MUL(6, 0, r6, r7);
   ROUND_MUL(7, 0, r7, r8);
   ROUND_MUL(8, 0, r8, r9);
   ROUND_MUL(9, 0, r9, r10);
   ROUND_MUL(10,0, r10,r11);

   ROUND_MUL(0, 1, r1, r2);
   ROUND_MUL(1, 1, r2, r3);
   ROUND_MUL(2, 1, r3, r4);
   ROUND_MUL(3, 1, r4, r5);
   ROUND_MUL(4, 1, r5, r6);
   ROUND_MUL(5, 1, r6, r7);
   ROUND_MUL(6, 1, r7, r8);
   ROUND_MUL(7, 1, r8, r9);
   ROUND_MUL(8, 1, r9, r10);
   ROUND_MUL(9, 1, r10,r11);
   ROUND_MUL(10,1, r11,r12);

   ROUND_MUL(0, 2, r2, r3);
   ROUND_MUL(1, 2, r3, r4);
   ROUND_MUL(2, 2, r4, r5);
   ROUND_MUL(3, 2, r5, r6);
   ROUND_MUL(4, 2, r6, r7);
   ROUND_MUL(5, 2, r7, r8);
   ROUND_MUL(6, 2, r8, r9);
   ROUND_MUL(7, 2, r9, r10);
   ROUND_MUL(8, 2, r10,r11);
   ROUND_MUL(9, 2, r11,r12);
   ROUND_MUL(10,2, r12,r13);

   ROUND_MUL(0, 3, r3, r4);
   ROUND_MUL(1, 3, r4, r5);
   ROUND_MUL(2, 3, r5, r6);
   ROUND_MUL(3, 3, r6, r7);
   ROUND_MUL(4, 3, r7, r8);
   ROUND_MUL(5, 3, r8, r9);
   ROUND_MUL(6, 3, r9, r10);
   ROUND_MUL(7, 3, r10,r11);
   ROUND_MUL(8, 3, r11,r12);
   ROUND_MUL(9, 3, r12,r13);
   ROUND_MUL(10,3, r13,r14);

   ROUND_MUL(0, 4, r4, r5);
   ROUND_MUL(1, 4, r5, r6);
   ROUND_MUL(2, 4, r6, r7);
   ROUND_MUL(3, 4, r7, r8);
   ROUND_MUL(4, 4, r8, r9);
   ROUND_MUL(5, 4, r9, r10);
   ROUND_MUL(6, 4, r10,r11);
   ROUND_MUL(7, 4, r11,r12);
   ROUND_MUL(8, 4, r12,r13);
   ROUND_MUL(9, 4, r13,r14);
   ROUND_MUL(10,4, r14,r15);

   ROUND_MUL(0, 5, r5, r6);
   ROUND_MUL(1, 5, r6, r7);
   ROUND_MUL(2, 5, r7, r8);
   ROUND_MUL(3, 5, r8, r9);
   ROUND_MUL(4, 5, r9, r10);
   ROUND_MUL(5, 5, r10,r11);
   ROUND_MUL(6, 5, r11,r12);
   ROUND_MUL(7, 5, r12,r13);
   ROUND_MUL(8, 5, r13,r14);
   ROUND_MUL(9, 5, r14,r15);
   ROUND_MUL(10,5, r15,r16);

   ROUND_MUL(0, 6, r6, r7);
   ROUND_MUL(1, 6, r7, r8);
   ROUND_MUL(2, 6, r8, r9);
   ROUND_MUL(3, 6, r9, r10);
   ROUND_MUL(4, 6, r10,r11);
   ROUND_MUL(5, 6, r11,r12);
   ROUND_MUL(6, 6, r12,r13);
   ROUND_MUL(7, 6, r13,r14);
   ROUND_MUL(8, 6, r14,r15);
   ROUND_MUL(9, 6, r15,r16);
   ROUND_MUL(10,6, r16,r17);

   ROUND_MUL(0, 7, r7, r8);
   ROUND_MUL(1, 7, r8, r9);
   ROUND_MUL(2, 7, r9, r10);
   ROUND_MUL(3, 7, r10,r11);
   ROUND_MUL(4, 7, r11,r12);
   ROUND_MUL(5, 7, r12,r13);
   ROUND_MUL(6, 7, r13,r14);
   ROUND_MUL(7, 7, r14,r15);
   ROUND_MUL(8, 7, r15,r16);
   ROUND_MUL(9, 7, r16,r17);
   ROUND_MUL(10,7, r17,r18);

   ROUND_MUL(0, 8, r8, r9);
   ROUND_MUL(1, 8, r9, r10);
   ROUND_MUL(2, 8, r10,r11);
   ROUND_MUL(3, 8, r11,r12);
   ROUND_MUL(4, 8, r12,r13);
   ROUND_MUL(5, 8, r13,r14);
   ROUND_MUL(6, 8, r14,r15);
   ROUND_MUL(7, 8, r15,r16);
   ROUND_MUL(8, 8, r16,r17);
   ROUND_MUL(9, 8, r17,r18);
   ROUND_MUL(10,8, r18,r19);

   ROUND_MUL(0, 9, r9, r10);
   ROUND_MUL(1, 9, r10,r11);
   ROUND_MUL(2, 9, r11,r12);
   ROUND_MUL(3, 9, r12,r13);
   ROUND_MUL(4, 9, r13,r14);
   ROUND_MUL(5, 9, r14,r15);
   ROUND_MUL(6, 9, r15,r16);
   ROUND_MUL(7, 9, r16,r17);
   ROUND_MUL(8, 9, r17,r18);
   ROUND_MUL(9, 9, r18,r19);
   ROUND_MUL(10,9, r19,r20);

   ROUND_MUL(0, 10, r10,r11);
   ROUND_MUL(1, 10, r11,r12);
   ROUND_MUL(2, 10, r12,r13);
   ROUND_MUL(3, 10, r13,r14);
   ROUND_MUL(4, 10, r14,r15);
   ROUND_MUL(5, 10, r15,r16);
   ROUND_MUL(6, 10, r16,r17);
   ROUND_MUL(7, 10, r17,r18);
   ROUND_MUL(8, 10, r18,r19);
   ROUND_MUL(9, 10, r19,r20);
   ROUND_MUL(10,10, r20,r21);

   // reduction p = 2^521 -1
   RED_P521_STEP(u0, r0, r1, r10);
   RED_P521_STEP(u1, r1, r2, r11);
   RED_P521_STEP(u2, r2, r3, r12);
   RED_P521_STEP(u3, r3, r4, r13);
   RED_P521_STEP(u4, r4, r5, r14);
   RED_P521_STEP(u5, r5, r6, r15);
   RED_P521_STEP(u6, r6, r7, r16);
   RED_P521_STEP(u7, r7, r8, r17);
   RED_P521_STEP(u8, r8, r9, r18);
   RED_P521_STEP(u9, r9, r10,r19);
   RED_P521_STEP(u10,r10,r11,r20);

   // normalization
   NORM_LSHIFTR(r, 11, 12)
   NORM_LSHIFTR(r, 12, 13)
   NORM_LSHIFTR(r, 13, 14)
   NORM_LSHIFTR(r, 14, 15)
   NORM_LSHIFTR(r, 15, 16)
   NORM_LSHIFTR(r, 16, 17)
   NORM_LSHIFTR(r, 17, 18)
   NORM_LSHIFTR(r, 18, 19)
   NORM_LSHIFTR(r, 19, 20)
   NORM_LSHIFTR(r, 20, 21)

   r[0] = r11;
   r[1] = r12;
   r[2] = r13;
   r[3] = r14;
   r[4] = r15;
   r[5] = r16;
   r[6] = r17;
   r[7] = r18;
   r[8] = r19;
   r[9] = r20;
   r[10]= r21;
}

void MB_FUNC_NAME(ifma_ams52_p521_)(U64 r[], const U64 va[])
{
   const U64* vb = va;

   U64 r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9;
   U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
   U64 r20, r21;

   r0  = r1  = r2  = r3  = r4  = r5  = r6  = r7  = r8  = r9  = r10 = 
   r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19 = r20 = r21 = get_zero64();

   // full square
   ROUND_MUL(0, 1, r1, r2);
   ROUND_MUL(0, 2, r2, r3);
   ROUND_MUL(0, 3, r3, r4);
   ROUND_MUL(0, 4, r4, r5);
   ROUND_MUL(0, 5, r5, r6);
   ROUND_MUL(0, 6, r6, r7);
   ROUND_MUL(0, 7, r7, r8);
   ROUND_MUL(0, 8, r8, r9);
   ROUND_MUL(0, 9, r9, r10);
   ROUND_MUL(0, 10,r10,r11);

   ROUND_MUL(1, 2, r3, r4);
   ROUND_MUL(1, 3, r4, r5);
   ROUND_MUL(1, 4, r5, r6);
   ROUND_MUL(1, 5, r6, r7);
   ROUND_MUL(1, 6, r7, r8);
   ROUND_MUL(1, 7, r8, r9);
   ROUND_MUL(1, 8, r9, r10);
   ROUND_MUL(1, 9, r10,r11);
   ROUND_MUL(1, 10,r11,r12);

   ROUND_MUL(2, 3, r5, r6);
   ROUND_MUL(2, 4, r6, r7);
   ROUND_MUL(2, 5, r7, r8);
   ROUND_MUL(2, 6, r8, r9);
   ROUND_MUL(2, 7, r9, r10);
   ROUND_MUL(2, 8, r10,r11);
   ROUND_MUL(2, 9, r11,r12);
   ROUND_MUL(2, 10,r12,r13);

   ROUND_MUL(3, 4, r7, r8);
   ROUND_MUL(3, 5, r8, r9);
   ROUND_MUL(3, 6, r9, r10);
   ROUND_MUL(3, 7, r10,r11);
   ROUND_MUL(3, 8, r11,r12);
   ROUND_MUL(3, 9, r12,r13);
   ROUND_MUL(3, 10,r13,r14);

   ROUND_MUL(4, 5, r9, r10);
   ROUND_MUL(4, 6, r10,r11);
   ROUND_MUL(4, 7, r11,r12);
   ROUND_MUL(4, 8, r12,r13);
   ROUND_MUL(4, 9, r13,r14);
   ROUND_MUL(4, 10,r14,r15);

   ROUND_MUL(5, 6, r11,r12);
   ROUND_MUL(5, 7, r12,r13);
   ROUND_MUL(5, 8, r13,r14);
   ROUND_MUL(5, 9, r14,r15);
   ROUND_MUL(5, 10,r15,r16);

   ROUND_MUL(6, 7, r13,r14);
   ROUND_MUL(6, 8, r14,r15);
   ROUND_MUL(6, 9, r15,r16);
   ROUND_MUL(6, 10,r16,r17);

   ROUND_MUL(7, 8, r15,r16);
   ROUND_MUL(7, 9, r16,r17);
   ROUND_MUL(7, 10,r17,r18);

   ROUND_MUL(8, 9, r17,r18);
   ROUND_MUL(8, 10,r18,r19);

   ROUND_MUL(9, 10,r19,r20);

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
   r15 = add64(r15, r15);
   r16 = add64(r16, r16);
   r17 = add64(r17, r17);
   r18 = add64(r18, r18);
   r19 = add64(r19, r19);
   r20 = add64(r20, r20);

   ROUND_MUL(0, 0, r0,  r1);
   ROUND_MUL(1, 1, r2,  r3);
   ROUND_MUL(2, 2, r4,  r5);
   ROUND_MUL(3, 3, r6,  r7);
   ROUND_MUL(4, 4, r8,  r9);
   ROUND_MUL(5, 5, r10, r11);
   ROUND_MUL(6, 6, r12, r13);
   ROUND_MUL(7, 7, r14, r15);
   ROUND_MUL(8, 8, r16, r17);
   ROUND_MUL(9, 9, r18, r19);
   ROUND_MUL(10,10,r20, r21);

   // reduction p = 2^521 -1
   RED_P521_STEP(u0, r0, r1, r10);
   RED_P521_STEP(u1, r1, r2, r11);
   RED_P521_STEP(u2, r2, r3, r12);
   RED_P521_STEP(u3, r3, r4, r13);
   RED_P521_STEP(u4, r4, r5, r14);
   RED_P521_STEP(u5, r5, r6, r15);
   RED_P521_STEP(u6, r6, r7, r16);
   RED_P521_STEP(u7, r7, r8, r17);
   RED_P521_STEP(u8, r8, r9, r18);
   RED_P521_STEP(u9, r9, r10,r19);
   RED_P521_STEP(u10,r10,r11,r20);

   // normalization
   NORM_LSHIFTR(r, 11, 12)
   NORM_LSHIFTR(r, 12, 13)
   NORM_LSHIFTR(r, 13, 14)
   NORM_LSHIFTR(r, 14, 15)
   NORM_LSHIFTR(r, 15, 16)
   NORM_LSHIFTR(r, 16, 17)
   NORM_LSHIFTR(r, 17, 18)
   NORM_LSHIFTR(r, 18, 19)
   NORM_LSHIFTR(r, 19, 20)
   NORM_LSHIFTR(r, 20, 21)

   r[0] = r11;
   r[1] = r12;
   r[2] = r13;
   r[3] = r14;
   r[4] = r15;
   r[5] = r16;
   r[6] = r17;
   r[7] = r18;
   r[8] = r19;
   r[9] = r20;
   r[10]= r21;
}

void MB_FUNC_NAME(ifma_tomont52_p521_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_p521_)(r, a, (U64*)p521_rr_mb);
}

void MB_FUNC_NAME(ifma_frommont52_p521_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_p521_)(r, a, (U64*)ones);
}

/*
// computes r = 1/z = z^(p521-2) mod p521
//       => r = z^(0x1FF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFD)
//
// note: z in in Montgomery domain (as soon mul() and sqr() below are amm-functions
//       r in Montgomery domain too
*/
#define fe52_sqr  MB_FUNC_NAME(ifma_ams52_p521_)
#define fe52_mul  MB_FUNC_NAME(ifma_amm52_p521_)

/* r = base^(2^n) */
__INLINE void fe52_sqr_pwr(U64 r[], const U64 base[], int n)
{
   if(r!=base)
      MB_FUNC_NAME(mov_FE521_)(r, base);
   for(; n>0; n--)
      fe52_sqr(r,r);
}

void MB_FUNC_NAME(ifma_aminv52_p521_)(U64 r[], const U64 z[])
{
   __ALIGN64 U64  u[P521_LEN52];
   __ALIGN64 U64  v[P521_LEN52];
   __ALIGN64 U64 zD[P521_LEN52];
   __ALIGN64 U64 zF[P521_LEN52];
   __ALIGN64 U64 z1FF[P521_LEN52];

   fe52_sqr(u, z);              /* u = z^2 */
   fe52_mul(v, u, z);           /* v = z^2 * z     = z^3  */
   fe52_sqr_pwr(zF, v, 2);      /* zF= (z^3)^(2^2) = z^12 */

   fe52_mul(zD, zF, z);         /* zD = z^12 * z    = z^xD */
   fe52_mul(zF, zF, v);         /* zF = z^12 * z^3  = z^xF */

   fe52_sqr_pwr(u, zF, 4);      /* u  = (z^xF)^(2^4)  = z^xF0 */
   fe52_mul(zD, u, zD);         /* zD = z^xF0 * z^xD  = z^xFD */
   fe52_mul(zF, u, zF);         /* zF = z^xF0 * z^xF  = z^xFF */

   fe52_sqr(z1FF, zF);          /* z1FF= (zF^2) = z^x1FE */
   fe52_mul(z1FF, z1FF, z);     /* z1FF *= z    = z^x1FF */

   fe52_sqr_pwr(u, zF, 8);      /* u = (z^xFF)^(2^8)    = z^xFF00 */
   fe52_mul(zD, u, zD);         /* zD = z^xFF00 * z^xFD = z^xFFFD */
   fe52_mul(zF, u, zF);         /* zF = z^xFF00 * z^xFF = z^xFFFF */

   fe52_sqr_pwr(u, zF, 16);     /* u = (z^xFFFF)^(2^16)       = z^xFFFF0000 */
   fe52_mul(zD, u, zD);         /* zD = z^xFFFF0000 * z^xFFFD = z^xFFFFFFFD */
   fe52_mul(zF, u, zF);         /* zF = z^xFFFF0000 * z^xFFFF = z^xFFFFFFFF */

   fe52_sqr_pwr(u, zF, 32);     /* u = (z^xFFFFFFFF)^(2^32)               = z^xFFFFFFFF00000000 */
   fe52_mul(zD, u, zD);         /* zD = z^xFFFFFFFF00000000 * z^xFFFFFFFD = z^xFFFFFFFFFFFFFFFD */
   fe52_mul(zF, u, zF);         /* zF = z^xFFFFFFFF00000000 * z^xFFFFFFFF = z^xFFFFFFFFFFFFFFFF */

   /* v = z^x1FF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, z1FF, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFF
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, zF);

   /* v = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.0000000000000000 * z^xFFFFFFFFFFFFFFFD
        = z^x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFE.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFD */
   fe52_sqr_pwr(v, v, 64);
   fe52_mul(r, v, zD);
}

/*=====================================================================

 Specialized single operations over p521:  add, sub, neg

=====================================================================*/
__INLINE __mb_mask MB_FUNC_NAME(lt_mbx_digit_)(const U64 a, const U64 b, const __mb_mask lt_mask)
{
   U64 d = mask_sub64(sub64(a, b), lt_mask, sub64(a, b), set1(1));
   return cmp64_mask(d, get_zero64(), _MM_CMPINT_LT);
}

void MB_FUNC_NAME(ifma_add52_p521_)(U64 R[], const U64 A[], const U64 B[])
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
   U64 r8 = add64(A[8], B[8]);
   U64 r9 = add64(A[9], B[9]);
   U64 r10= add64(A[10],B[10]);

   /* lt = {r0 - r10} < 2*p */
   __mb_mask
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r0, ((U64*)(p521_x2))[0], 0);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r1, ((U64*)(p521_x2))[1], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r2, ((U64*)(p521_x2))[2], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r3, ((U64*)(p521_x2))[3], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r4, ((U64*)(p521_x2))[4], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r5, ((U64*)(p521_x2))[5], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r6, ((U64*)(p521_x2))[6], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r7, ((U64*)(p521_x2))[7], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r8, ((U64*)(p521_x2))[8], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r9, ((U64*)(p521_x2))[9], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r10,((U64*)(p521_x2))[10],lt);

   /* {r0 - r10} -= 2*p */
   r0 = mask_sub64(r0, ~lt, r0, ((U64*)(p521_x2))[0]);
   r1 = mask_sub64(r1, ~lt, r1, ((U64*)(p521_x2))[1]);
   r2 = mask_sub64(r2, ~lt, r2, ((U64*)(p521_x2))[2]);
   r3 = mask_sub64(r3, ~lt, r3, ((U64*)(p521_x2))[3]);
   r4 = mask_sub64(r4, ~lt, r4, ((U64*)(p521_x2))[4]);
   r5 = mask_sub64(r5, ~lt, r5, ((U64*)(p521_x2))[5]);
   r6 = mask_sub64(r6, ~lt, r6, ((U64*)(p521_x2))[6]);
   r7 = mask_sub64(r7, ~lt, r7, ((U64*)(p521_x2))[7]);
   r8 = mask_sub64(r8, ~lt, r8, ((U64*)(p521_x2))[8]);
   r9 = mask_sub64(r9, ~lt, r9, ((U64*)(p521_x2))[9]);
   r10= mask_sub64(r10,~lt, r10,((U64*)(p521_x2))[10]);

   /* normalize r0 - r7 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)
   NORM_ASHIFTR(r, 4, 5)
   NORM_ASHIFTR(r, 5, 6)
   NORM_ASHIFTR(r, 6, 7)
   NORM_ASHIFTR(r, 7, 8)
   NORM_ASHIFTR(r, 8, 9)
   NORM_ASHIFTR(r, 9,10)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
   R[5] = r5;
   R[6] = r6;
   R[7] = r7;
   R[8] = r8;
   R[9] = r9;
   R[10]= r10;
}

void MB_FUNC_NAME(ifma_sub52_p521_)(U64 R[], const U64 A[], const U64 B[])
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
   U64 r8 = sub64(A[8], B[8]);
   U64 r9 = sub64(A[9], B[9]);
   U64 r10= sub64(A[10],B[10]);

   /* lt = {r0 - r10} < 0 */
   __mb_mask
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r0, get_zero64(), 0);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r1, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r2, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r3, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r4, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r5, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r6, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r7, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r8, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r9, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r10,get_zero64(), lt);

   r0 = mask_add64(r0, lt, r0, ((U64*)(p521_x2))[0]);
   r1 = mask_add64(r1, lt, r1, ((U64*)(p521_x2))[1]);
   r2 = mask_add64(r2, lt, r2, ((U64*)(p521_x2))[2]);
   r3 = mask_add64(r3, lt, r3, ((U64*)(p521_x2))[3]);
   r4 = mask_add64(r4, lt, r4, ((U64*)(p521_x2))[4]);
   r5 = mask_add64(r5, lt, r5, ((U64*)(p521_x2))[5]);
   r6 = mask_add64(r6, lt, r6, ((U64*)(p521_x2))[6]);
   r7 = mask_add64(r7, lt, r7, ((U64*)(p521_x2))[7]);
   r8 = mask_add64(r8, lt, r8, ((U64*)(p521_x2))[8]);
   r9 = mask_add64(r9, lt, r9, ((U64*)(p521_x2))[9]);
   r10= mask_add64(r10,lt, r10,((U64*)(p521_x2))[10]);

   /* normalize r0 - r7 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)
   NORM_ASHIFTR(r, 4, 5)
   NORM_ASHIFTR(r, 5, 6)
   NORM_ASHIFTR(r, 6, 7)
   NORM_ASHIFTR(r, 7, 8)
   NORM_ASHIFTR(r, 8, 9)
   NORM_ASHIFTR(r, 9, 10)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
   R[5] = r5;
   R[6] = r6;
   R[7] = r7;
   R[8] = r8;
   R[9] = r9;
   R[10]= r10;
}

void MB_FUNC_NAME(ifma_neg52_p521_)(U64 R[], const U64 A[])
{
   __mb_mask nz_mask = ~MB_FUNC_NAME(is_zero_FE521_)(A);

   /* {r0 - r10} = 2*p - A */
   U64 r0 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[0], A[0] );
   U64 r1 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[1], A[1] );
   U64 r2 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[2], A[2] );
   U64 r3 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[3], A[3] );
   U64 r4 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[4], A[4] );
   U64 r5 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[5], A[5] );
   U64 r6 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[6], A[6] );
   U64 r7 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[7], A[7] );
   U64 r8 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[8], A[8] );
   U64 r9 = mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[9], A[9] );
   U64 r10= mask_sub64( A[0], nz_mask, ((U64*)(p521_x2))[10],A[10]);

   /* normalize r0 - r10 */
   NORM_ASHIFTR(r, 0, 1)
   NORM_ASHIFTR(r, 1, 2)
   NORM_ASHIFTR(r, 2, 3)
   NORM_ASHIFTR(r, 3, 4)
   NORM_ASHIFTR(r, 4, 5)
   NORM_ASHIFTR(r, 5, 6)
   NORM_ASHIFTR(r, 6, 7)
   NORM_ASHIFTR(r, 7, 8)
   NORM_ASHIFTR(r, 8, 9)
   NORM_ASHIFTR(r, 9, 10)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
   R[5] = r5;
   R[6] = r6;
   R[7] = r7;
   R[8] = r8;
   R[9] = r9;
   R[10]= r10;
}

void MB_FUNC_NAME(ifma_double52_p521_)(U64 R[], const U64 A[])
{
   MB_FUNC_NAME(ifma_add52_p521_)(R, A, A);
}

void MB_FUNC_NAME(ifma_tripple52_p521_)(U64 R[], const U64 A[])
{
   U64 T[P521_LEN52];
   MB_FUNC_NAME(ifma_add52_p521_)(T, A, A);
   MB_FUNC_NAME(ifma_add52_p521_)(R, T, A);
}

void MB_FUNC_NAME(ifma_half52_p521_)(U64 R[], const U64 A[])
{
   U64 one = set1(1);
   U64 base = set1(DIGIT_BASE);

   /* res = a + is_odd(a)? p521 : 0 */
   U64 mask = sub64(get_zero64(), and64(A[0], one));
   U64 t0 = add64(A[0], and64(((U64*)p521_mb)[0], mask));
   U64 t1 = add64(A[1], and64(((U64*)p521_mb)[1], mask));
   U64 t2 = add64(A[2], and64(((U64*)p521_mb)[2], mask));
   U64 t3 = add64(A[3], and64(((U64*)p521_mb)[3], mask));
   U64 t4 = add64(A[4], and64(((U64*)p521_mb)[4], mask));
   U64 t5 = add64(A[5], and64(((U64*)p521_mb)[5], mask));
   U64 t6 = add64(A[6], and64(((U64*)p521_mb)[6], mask));
   U64 t7 = add64(A[7], and64(((U64*)p521_mb)[7], mask));
   U64 t8 = add64(A[8], and64(((U64*)p521_mb)[8], mask));
   U64 t9 = add64(A[9], and64(((U64*)p521_mb)[9], mask));
   U64 t10= add64(A[10],and64(((U64*)p521_mb)[10],mask));

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

   mask = sub64(get_zero64(), and64(t8, one));
   t7 = add64(t7, and64(base, mask));
   t7 = srli64(t7, 1);

   mask = sub64(get_zero64(), and64(t9, one));
   t8 = add64(t8, and64(base, mask));
   t8 = srli64(t8, 1);

   mask = sub64(get_zero64(), and64(t10, one));
   t9 = add64(t9, and64(base, mask));
   t9 = srli64(t9, 1);

   t10 = srli64(t10, 1);

   /* normalize t0, t1, t2, t3, t4 */
   NORM_LSHIFTR(t, 0, 1)
   NORM_LSHIFTR(t, 1, 2)
   NORM_LSHIFTR(t, 2, 3)
   NORM_LSHIFTR(t, 3, 4)
   NORM_LSHIFTR(t, 4, 5)
   NORM_LSHIFTR(t, 5, 6)
   NORM_LSHIFTR(t, 6, 7)
   NORM_LSHIFTR(t, 7, 8)
   NORM_LSHIFTR(t, 8, 9)
   NORM_LSHIFTR(t, 9,10)

   R[0] = t0;
   R[1] = t1;
   R[2] = t2;
   R[3] = t3;
   R[4] = t4;
   R[5] = t5;
   R[6] = t6;
   R[7] = t7;
   R[8] = t8;
   R[9] = t9;
   R[10]= t10;
}

__mb_mask MB_FUNC_NAME(ifma_cmp_lt_p521_)(const U64 a[])
{
   return MB_FUNC_NAME(cmp_lt_FE521_)(a,(const U64 (*))p521_mb);
}

__mb_mask MB_FUNC_NAME(ifma_check_range_p521_)(const U64 A[])
{
   __mb_mask
   mask = MB_FUNC_NAME(is_zero_FE521_)(A);
   mask |= ~MB_FUNC_NAME(ifma_cmp_lt_p521_)(A);

   return mask;
}

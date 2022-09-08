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
// EC NIST-P521 prime base point order
// in 2^52 radix
*/
__ALIGN64 static const int64u n521_mb[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x000fb71e91386409) },
   { REP8_DECL(0x000b8899c47aebb6) },
   { REP8_DECL(0x000709a5d03bb5c9) },
   { REP8_DECL(0x000966b7fcc0148f) },
   { REP8_DECL(0x000a51868783bf2f) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x0000000000000001) }
};

/* 2*n521 */
__ALIGN64 static const int64u n521_x2[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x000f6e3d2270c812) },
   { REP8_DECL(0x0007113388f5d76d) },
   { REP8_DECL(0x000e134ba0776b93) },
   { REP8_DECL(0x0002cd6ff980291e) },
   { REP8_DECL(0x0004a30d0f077e5f) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x0000000000000003) }
};
/* k0 = -( (1/n521 mod 2^DIGIT_SIZE) ) mod 2^DIGIT_SIZE */
__ALIGN64 static const int64u n521_k0_mb[sizeof(U64)/sizeof(int64u)] = {
   REP8_DECL(0x000f5ccd79a995c7)
};

/* to Montgomery conversion constant
// rr = 2^((P521_LEN52*DIGIT_SIZE)*2) mod n521
*/
__ALIGN64 static const int64u n521_rr_mb[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x0003b4d7a5b140ce) },
   { REP8_DECL(0x000cb0bf26c55bf9) },
   { REP8_DECL(0x00037e5396c67ee9) },
   { REP8_DECL(0x0002bd1c80cf7b13) },
   { REP8_DECL(0x00073cbe28f15e41) },
   { REP8_DECL(0x000dd6e23d82e49c) },
   { REP8_DECL(0x0003d142b7756e3e) },
   { REP8_DECL(0x00061a8e567bccff) },
   { REP8_DECL(0x00092d0d455bcc6d) },
   { REP8_DECL(0x000383d2d8e03d14) },
   { REP8_DECL(0x0000000000000000) }
};

/* ifma_tomont52_n521_(1) */
__ALIGN64 static const int64u n521_r_mb[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x0008000000000000) },
   { REP8_DECL(0x00082470b763cdfb) },
   { REP8_DECL(0x00023bb31dc28a24) },
   { REP8_DECL(0x00047b2d17e2251b) },
   { REP8_DECL(0x00034ca4019ff5b8) },
   { REP8_DECL(0x0002d73cbc3e2068) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) }
};


/*=====================================================================

 Specialized single operations over n384:  sqr & mul

=====================================================================*/

void MB_FUNC_NAME(ifma_amm52_n521_)(U64 r[], const U64 va[], const U64 vb[])
{
   U64 K = loadu64(n521_k0_mb);

   U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10;
   int itr;

   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = r10 = get_zero64();

   for(itr=0; itr < P521_LEN52; itr++) {
      U64 Yi, T, LO, HI;
      U64 Bi = loadu64(vb);
      vb++;

      fma52lo_mem(r0, r0, Bi, va, SIMD_BYTES * 0);
      fma52lo_mem(r1, r1, Bi, va, SIMD_BYTES * 1);
      fma52lo_mem(r2, r2, Bi, va, SIMD_BYTES * 2);
      fma52lo_mem(r3, r3, Bi, va, SIMD_BYTES * 3);
      fma52lo_mem(r4, r4, Bi, va, SIMD_BYTES * 4);
      fma52lo_mem(r5, r5, Bi, va, SIMD_BYTES * 5);
      fma52lo_mem(r6, r6, Bi, va, SIMD_BYTES * 6);
      fma52lo_mem(r7, r7, Bi, va, SIMD_BYTES * 7);
      fma52lo_mem(r8, r8, Bi, va, SIMD_BYTES * 8);
      fma52lo_mem(r9, r9, Bi, va, SIMD_BYTES * 9);
      fma52lo_mem(r10,r10,Bi, va, SIMD_BYTES * 10);

         Yi = fma52lo(get_zero64(), r0, K);
         T = sub64(get_zero64(), Yi);
         LO = and64(T, loadu64(VMASK52));
         HI = add64(Yi, srai64(T,63));

      fma52lo_mem(r0, r0, Yi, n521_mb, SIMD_BYTES * 0);
      fma52lo_mem(r1, r1, Yi, n521_mb, SIMD_BYTES * 1);
      fma52lo_mem(r2, r2, Yi, n521_mb, SIMD_BYTES * 2);
      fma52lo_mem(r3, r3, Yi, n521_mb, SIMD_BYTES * 3);
      fma52lo_mem(r4, r4, Yi, n521_mb, SIMD_BYTES * 4);
         r1 = add64(r1, srli64(r0, DIGIT_SIZE));      /* carry propagation */
      r5 = add64(r5, LO);     /* fma52lo_mem(r5, r5, Yi, n521_mb, SIMD_BYTES * 5); */
      r6 = add64(r6, LO);     /* fma52lo_mem(r6, r6, Yi, n521_mb, SIMD_BYTES * 6); */
      r7 = add64(r7, LO);     /* fma52lo_mem(r7, r7, Yi, n521_mb, SIMD_BYTES * 7); */
      r8 = add64(r8, LO);     /* fma52lo_mem(r8, r8, Yi, n521_mb, SIMD_BYTES * 8); */
      r9 = add64(r9, LO);     /* fma52lo_mem(r9, r9, Yi, n521_mb, SIMD_BYTES * 9); */
      r10= add64(r10,Yi);     /* fma52lo_mem(r10,r10,Yi, n521_mb, SIMD_BYTES * 10); */

      fma52hi_mem(r0, r1, Bi, va, SIMD_BYTES * 0);
      fma52hi_mem(r1, r2, Bi, va, SIMD_BYTES * 1);
      fma52hi_mem(r2, r3, Bi, va, SIMD_BYTES * 2);
      fma52hi_mem(r3, r4, Bi, va, SIMD_BYTES * 3);
      fma52hi_mem(r4, r5, Bi, va, SIMD_BYTES * 4);
      fma52hi_mem(r5, r6, Bi, va, SIMD_BYTES * 5);
      fma52hi_mem(r6, r7, Bi, va, SIMD_BYTES * 6);
      fma52hi_mem(r7, r8, Bi, va, SIMD_BYTES * 7);
      fma52hi_mem(r8, r9, Bi, va, SIMD_BYTES * 8);
      fma52hi_mem(r9, r10,Bi, va, SIMD_BYTES * 9);
      fma52hi_mem(r10,get_zero64(), Bi, va, SIMD_BYTES * 10);

      fma52hi_mem(r0, r0, Yi, n521_mb, SIMD_BYTES * 0);
      fma52hi_mem(r1, r1, Yi, n521_mb, SIMD_BYTES * 1);
      fma52hi_mem(r2, r2, Yi, n521_mb, SIMD_BYTES * 2);
      fma52hi_mem(r3, r3, Yi, n521_mb, SIMD_BYTES * 3);
      fma52hi_mem(r4, r4, Yi, n521_mb, SIMD_BYTES * 4);
      r5 = add64(r5, HI);     /* fma52hi_mem(r5, r5, Yi, n521_mb, SIMD_BYTES * 5); */
      r6 = add64(r6, HI);     /* fma52hi_mem(r6, r6, Yi, n521_mb, SIMD_BYTES * 6); */
      r7 = add64(r7, HI);     /* fma52hi_mem(r7, r7, Yi, n521_mb, SIMD_BYTES * 7); */
      r8 = add64(r8, HI);     /* fma52hi_mem(r8, r8, Yi, n521_mb, SIMD_BYTES * 8); */
      r9 = add64(r9, HI);     /* fma52hi_mem(r9, r9, Yi, n521_mb, SIMD_BYTES * 9); */
                              /* 0 == hi(Yi, n521_mb[10] */;
   }

   // normalization
   NORM_LSHIFTR(r, 0, 1)
   NORM_LSHIFTR(r, 1, 2)
   NORM_LSHIFTR(r, 2, 3)
   NORM_LSHIFTR(r, 3, 4)
   NORM_LSHIFTR(r, 4, 5)
   NORM_LSHIFTR(r, 5, 6)
   NORM_LSHIFTR(r, 6, 7)
   NORM_LSHIFTR(r, 7, 8)
   NORM_LSHIFTR(r, 8, 9)
   NORM_LSHIFTR(r, 9, 10)

   r[0] = r0;
   r[1] = r1;
   r[2] = r2;
   r[3] = r3;
   r[4] = r4;
   r[5] = r5;
   r[6] = r6;
   r[7] = r7;
   r[8] = r8;
   r[9] = r9;
   r[10]= r10;
}

#define ROUND_MUL(I, J, R_LO, R_HI) \
    R_LO = fma52lo(R_LO, va[I], vb[J]); \
    R_HI = fma52hi(R_HI, va[I], vb[J]);

#define N521_REDUCTION_ROUND(u, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, modulus, k) \
{  \
   U64 t = sub64(get_zero64(), u); \
   U64 lo = and64(t, loadu64(VMASK52)); \
   U64 hi = add64(u, srai64(t,63)); \
   \
   r0 = fma52lo(r0, u, modulus[0]); \
   r1 = fma52lo(r1, u, modulus[1]); \
   r2 = fma52lo(r2, u, modulus[2]); \
   r3 = fma52lo(r3, u, modulus[3]); \
   r4 = fma52lo(r4, u, modulus[4]); \
      r1 = add64(r1, srli64(r0, DIGIT_SIZE)); /* carry propagation */ \
   r5 = add64(r5, lo); \
   r6 = add64(r6, lo); \
   r7 = add64(r7, lo); \
   r8 = add64(r8, lo); \
   r9 = add64(r9, lo); \
   r10 = add64(r10, u); \
   \
   \
   r1 = fma52hi(r1, u, modulus[0]); \
   r2 = fma52hi(r2, u, modulus[1]); \
   r3 = fma52hi(r3, u, modulus[2]); \
   r4 = fma52hi(r4, u, modulus[3]); \
   r5 = fma52hi(r5, u, modulus[4]); \
      u = fma52lo(get_zero64(), r1, k); /* update u = r1*k */ \
   r6 = add64(r6, hi); \
   r7 = add64(r7, hi); \
   r8 = add64(r8, hi); \
   r9 = add64(r9, hi); \
   r10= add64(r10,hi); \
}

void MB_FUNC_NAME(ifma_ams52_n521_)(U64 r[], const U64 va[])
{
   U64 K = loadu64(n521_k0_mb);

   const U64* vb = va;
   U64* modulus = (U64*)n521_mb;

   U64 r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9;
   U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
   U64 r20, r21;
   U64 u;

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
   u = fma52lo(get_zero64(), r0, K);
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

   // reduction n521
   N521_REDUCTION_ROUND(u, r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9,  r10, modulus, K);
   N521_REDUCTION_ROUND(u, r1,  r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9,  r10, r11, modulus, K);
   N521_REDUCTION_ROUND(u, r2,  r3,  r4,  r5,  r6,  r7,  r8,  r9,  r10, r11, r12, modulus, K);
   N521_REDUCTION_ROUND(u, r3,  r4,  r5,  r6,  r7,  r8,  r9,  r10, r11, r12, r13, modulus, K);
   N521_REDUCTION_ROUND(u, r4,  r5,  r6,  r7,  r8,  r9,  r10, r11, r12, r13, r14, modulus, K);
   N521_REDUCTION_ROUND(u, r5,  r6,  r7,  r8,  r9,  r10, r11, r12, r13, r14, r15, modulus, K);
   N521_REDUCTION_ROUND(u, r6,  r7,  r8,  r9,  r10, r11, r12, r13, r14, r15, r16, modulus, K);
   N521_REDUCTION_ROUND(u, r7,  r8,  r9,  r10, r11, r12, r13, r14, r15, r16, r17, modulus, K);
   N521_REDUCTION_ROUND(u, r8,  r9,  r10, r11, r12, r13, r14, r15, r16, r17, r18, modulus, K);
   N521_REDUCTION_ROUND(u, r9,  r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, modulus, K);
   N521_REDUCTION_ROUND(u, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, modulus, K);


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

void MB_FUNC_NAME(ifma_tomont52_n521_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_n521_)(r, a, (U64*)n521_rr_mb);
}

void MB_FUNC_NAME(ifma_frommont52_n521_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_n521_)(r, a, (U64*)ones);
}

/*
// computes r = 1/z = z^(n384-2) mod n384
//
// note: z in in Montgomery domain (as soon mul() and sqr() below are amm-functions
//       r in Montgomery domain too
*/
#define fe52_sqr    MB_FUNC_NAME(ifma_ams52_n521_)
#define fe52_mul    MB_FUNC_NAME(ifma_amm52_n521_)

/* r = base^(2^n) */
__INLINE void fe52_sqr_pwr(U64 r[], const U64 base[], int n)
{
   if(r!=base) {
      fe52_sqr(r,base);
      n--;
   }
   for(; n>0; n--)
      fe52_sqr(r,r);
}

void MB_FUNC_NAME(ifma_aminv52_n521_)(U64 r[], const U64 z[])
{
   int i;

   // pwr_z_Tbl[i][] = z^i, i=0,..,15
   __ALIGN64 U64 pwr_z_Tbl[16][P521_LEN52];
   __ALIGN64 U64  lexp[P521_LEN52];

   MB_FUNC_NAME(mov_FE521_)(pwr_z_Tbl[0], (U64*)n521_r_mb);
   MB_FUNC_NAME(mov_FE521_)(pwr_z_Tbl[1], z);

   for(i=2; i<16; i+=2) {
      fe52_sqr(pwr_z_Tbl[i], pwr_z_Tbl[i/2]);
      fe52_mul(pwr_z_Tbl[i+1], pwr_z_Tbl[i], z);
   }

   // pwr = (n521-2) in big endian
   int8u pwr[] = "\x1\xFF"
                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFA"
                 "\x51\x86\x87\x83\xBF\x2F\x96\x6B"
                 "\x7F\xCC\x01\x48\xF7\x09\xA5\xD0"
                 "\x3B\xB5\xC9\xB8\x89\x9C\x47\xAE"
                 "\xBB\x6F\xB7\x1E\x91\x38\x64\x07";

   /*
   // process 25 low bytes of the exponent: :FA 51 86 ... 64 07"
   */
   /* init result */
   MB_FUNC_NAME(mov_FE521_)(lexp, (U64*)n521_r_mb);

   for(i=33; i<sizeof(pwr)-1; i++) {
      int v = pwr[i];
      int hi = (v>>4) &0xF;
      int lo = v & 0xF;

      fe52_sqr(lexp, lexp);
      fe52_sqr(lexp, lexp);
      fe52_sqr(lexp, lexp);
      fe52_sqr(lexp, lexp);
      if(hi)
         fe52_mul(lexp, lexp, pwr_z_Tbl[hi]);
      fe52_sqr(lexp, lexp);
      fe52_sqr(lexp, lexp);
      fe52_sqr(lexp, lexp);
      fe52_sqr(lexp, lexp);
      if(lo)
         fe52_mul(lexp, lexp, pwr_z_Tbl[lo]);
   }

   /*
   // process high part of the exponent: "0x1 0xffffffffffffffff 0xffffffffffffffff 0xffffffffffffffff 0xffffffffffffffff"
   */
   __ALIGN64 U64  u[P521_LEN52];
   __ALIGN64 U64  v[P521_LEN52];

   MB_FUNC_NAME(mov_FE521_)(u, pwr_z_Tbl[15]);  /* u = z^0xF */

   fe52_sqr_pwr(v, u, 4);     /* v= (z^0xF)^(2^4) = z^(0xF0) */
   fe52_mul(u, v, u);         /* u = z^0xF0 * z^(0xF) = z^(0xFF) */

   fe52_sqr_pwr(v, u, 8);     /* v= (z^0xFF)^(2^8) = z^(0xFF00) */
   fe52_mul(u, v, u);         /* u = z^0xFF00 * z^(0xFF) = z^(0xFFFF) */

   fe52_sqr_pwr(v, u, 16);    /* v= (z^0xFFFF)^(2^16) = z^(0xFFFF0000) */
   fe52_mul(u, v, u);         /* u = z^0xFFFF0000 * z^(0xFFFF) = z^(0xFFFFFFFF) */

   fe52_sqr_pwr(v, u, 32);    /* v= (z^0xFFFFFFFF)^(2^32) = z^(0xFFFFFFFF00000000) */
   fe52_mul(u, v, u);         /* u = z^0xFFFFFFFF00000000 * z^(0xFFFFFFFF) = z^(0xFFFFFFFFFFFFFFFF) */

   fe52_sqr_pwr(v, z, 64);
   fe52_mul(v, v, u);         /* v = z^(0x1.FFFFFFFFFFFFFFFF) */

   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, u);         /* v = z^(0x1.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF) */

   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, u);         /* v = z^(0x1.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF) */

   fe52_sqr_pwr(v, v, 64);
   fe52_mul(v, v, u);         /* v = z^(0x1.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF) */

   /* combine low and high results */
   fe52_sqr_pwr(v, v, 64*4+8);/* u = z^(0x1.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.00.0000000000000000.0000000000000000.0000000000000000.0000000000000000) */
   fe52_mul(r, v, lexp);      /* r = z^(0x1FF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFF.FFFFFFFFFFFFFFFA.51868783BF2F966B.7FCC0148F709A5D0.3BB5C9B8899C47AE.BB6FB71E91386407) */
}


/*=====================================================================

 Specialized single operations over n521  add, sub, neg

=====================================================================*/
__INLINE __mb_mask MB_FUNC_NAME(lt_mbx_digit_)(const U64 a, const U64 b, const __mb_mask lt_mask)
{
   U64 d = mask_sub64(sub64(a, b), lt_mask, sub64(a, b), set1(1));
   return cmp64_mask(d, get_zero64(), _MM_CMPINT_LT);
}

void MB_FUNC_NAME(ifma_add52_n521_)(U64 R[], const U64 A[], const U64 B[])
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

   /* lt = {r0 - r10} < 2*n */
   __mb_mask
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r0, ((U64*)(n521_x2))[0], 0);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r1, ((U64*)(n521_x2))[1], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r2, ((U64*)(n521_x2))[2], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r3, ((U64*)(n521_x2))[3], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r4, ((U64*)(n521_x2))[4], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r5, ((U64*)(n521_x2))[5], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r6, ((U64*)(n521_x2))[6], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r7, ((U64*)(n521_x2))[7], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r8, ((U64*)(n521_x2))[8], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r9, ((U64*)(n521_x2))[9], lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)( r10,((U64*)(n521_x2))[10],lt);

   /* {r0 - r10} -= 2*p */
   r0 = mask_sub64(r0, ~lt, r0, ((U64*)(n521_x2))[0]);
   r1 = mask_sub64(r1, ~lt, r1, ((U64*)(n521_x2))[1]);
   r2 = mask_sub64(r2, ~lt, r2, ((U64*)(n521_x2))[2]);
   r3 = mask_sub64(r3, ~lt, r3, ((U64*)(n521_x2))[3]);
   r4 = mask_sub64(r4, ~lt, r4, ((U64*)(n521_x2))[4]);
   r5 = mask_sub64(r5, ~lt, r5, ((U64*)(n521_x2))[5]);
   r6 = mask_sub64(r6, ~lt, r6, ((U64*)(n521_x2))[6]);
   r7 = mask_sub64(r7, ~lt, r7, ((U64*)(n521_x2))[7]);
   r8 = mask_sub64(r8, ~lt, r8, ((U64*)(n521_x2))[8]);
   r9 = mask_sub64(r9, ~lt, r9, ((U64*)(n521_x2))[9]);
   r10= mask_sub64(r10,~lt, r10,((U64*)(n521_x2))[10]);

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

void MB_FUNC_NAME(ifma_sub52_n521_)(U64 R[], const U64 A[], const U64 B[])
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

   r0 = mask_add64(r0, lt, r0, ((U64*)(n521_x2))[0]);
   r1 = mask_add64(r1, lt, r1, ((U64*)(n521_x2))[1]);
   r2 = mask_add64(r2, lt, r2, ((U64*)(n521_x2))[2]);
   r3 = mask_add64(r3, lt, r3, ((U64*)(n521_x2))[3]);
   r4 = mask_add64(r4, lt, r4, ((U64*)(n521_x2))[4]);
   r5 = mask_add64(r5, lt, r5, ((U64*)(n521_x2))[5]);
   r6 = mask_add64(r6, lt, r6, ((U64*)(n521_x2))[6]);
   r7 = mask_add64(r7, lt, r7, ((U64*)(n521_x2))[7]);
   r8 = mask_add64(r8, lt, r8, ((U64*)(n521_x2))[8]);
   r9 = mask_add64(r9, lt, r9, ((U64*)(n521_x2))[9]);
   r10= mask_add64(r10,lt, r10,((U64*)(n521_x2))[10]);

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

void MB_FUNC_NAME(ifma_neg52_n521_)(U64 R[], const U64 A[])
{
   __mb_mask nz_mask = ~MB_FUNC_NAME(is_zero_FE521_)(A);

   /* {r0 - r10} = 2*p - A */
   U64 r0 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[0], A[0] );
   U64 r1 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[1], A[1] );
   U64 r2 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[2], A[2] );
   U64 r3 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[3], A[3] );
   U64 r4 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[4], A[4] );
   U64 r5 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[5], A[5] );
   U64 r6 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[6], A[6] );
   U64 r7 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[7], A[7] );
   U64 r8 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[8], A[8] );
   U64 r9 = mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[9], A[9] );
   U64 r10= mask_sub64( A[0], nz_mask, ((U64*)(n521_x2))[10],A[10]);

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

// Disable optimization for VS17
#if defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", off )
#endif

/* r = (a>=n521)? a-n521 : a */
void MB_FUNC_NAME(ifma_fastred52_pn521_)(U64 R[], const U64 A[])
{
   /* r = a - b */
   U64 r0 = sub64(A[0], ((U64*)(n521_mb))[0]);
   U64 r1 = sub64(A[1], ((U64*)(n521_mb))[1]);
   U64 r2 = sub64(A[2], ((U64*)(n521_mb))[2]);
   U64 r3 = sub64(A[3], ((U64*)(n521_mb))[3]);
   U64 r4 = sub64(A[4], ((U64*)(n521_mb))[4]);
   U64 r5 = sub64(A[5], ((U64*)(n521_mb))[5]);
   U64 r6 = sub64(A[6], ((U64*)(n521_mb))[6]);
   U64 r7 = sub64(A[7], ((U64*)(n521_mb))[7]);
   U64 r8 = sub64(A[8], ((U64*)(n521_mb))[8]);
   U64 r9 = sub64(A[9], ((U64*)(n521_mb))[9]);
   U64 r10= sub64(A[10],((U64*)(n521_mb))[10]);

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

   r0 = mask_mov64(A[0], ~lt, r0);
   r1 = mask_mov64(A[1], ~lt, r1);
   r2 = mask_mov64(A[2], ~lt, r2);
   r3 = mask_mov64(A[3], ~lt, r3);
   r4 = mask_mov64(A[4], ~lt, r4);
   r5 = mask_mov64(A[5], ~lt, r5);
   r6 = mask_mov64(A[6], ~lt, r6);
   r7 = mask_mov64(A[7], ~lt, r7);
   r8 = mask_mov64(A[8], ~lt, r8);
   r9 = mask_mov64(A[9], ~lt, r9);
   r10= mask_mov64(A[10],~lt, r10);

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

#if defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__INTEL_COMPILER)
    #pragma optimize( "", on )
#endif

__mb_mask MB_FUNC_NAME(ifma_cmp_lt_n521_)(const U64 a[])
{
   return MB_FUNC_NAME(cmp_lt_FE521_)(a,(const U64 (*))n521_mb);
}

__mb_mask MB_FUNC_NAME(ifma_check_range_n521_)(const U64 A[])
{
   __mb_mask
   mask = MB_FUNC_NAME(is_zero_FE521_)(A);
   mask |= ~MB_FUNC_NAME(ifma_cmp_lt_n521_)(A);

   return mask;
}
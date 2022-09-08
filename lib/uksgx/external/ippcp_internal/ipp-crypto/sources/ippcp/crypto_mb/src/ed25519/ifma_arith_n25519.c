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

#include <crypto_mb/status.h>

#include <internal/common/ifma_math.h>
#include <internal/ed25519/ifma_arith_n25519.h>

/* Constants */
//#define N25519_BITSIZE  (253)
//#define NE_LEN52        NUMBER_OF_DIGITS(N25519_BITSIZE, DIGIT_SIZE)

/*
// ED25519 prime base point order
// n = 2^252+27742317777372353535851937790883648493 = 0x1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED
// in 2^52 radix
*/
__ALIGN64 static const int64u ed25519n_mb[NE_LEN52][sizeof(U64) / sizeof(int64u)] = {
   { REP8_DECL(0x0002631A5CF5D3ED) },
   { REP8_DECL(0x000DEA2F79CD6581) },
   { REP8_DECL(0x000000000014DEF9) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000100000000000) }
};

/* mu = floor( 2^2*DIGIT_SIZE/n)  = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEB2106215D086329A7ED9CE5A30A2C131B39 */
__ALIGN64 static const int64u ed25519mu_mb[NE_LEN52+1][sizeof(U64) / sizeof(int64u)] = {
   { REP8_DECL(0x0005A30A2C131B39) },
   { REP8_DECL(0x000086329A7ED9CE) },
   { REP8_DECL(0x000FFFEB2106215D) },
   { REP8_DECL(0x000FFFFFFFFFFFFF) },
   { REP8_DECL(0x000FFFFFFFFFFFFF) },
   { REP8_DECL(0x00000000000000FF) }
};

#define ROUND_MUL(R_LO, R_HI, A_I, B_J) \
    (R_LO) = fma52lo((R_LO), (A_I), (B_J)); \
    (R_HI) = fma52hi((R_HI), (A_I), (B_J));

__ALIGN64 static const int64u VBASE52[sizeof(U64)/sizeof(int64u)] = {
   REP8_DECL(DIGIT_BASE)
};

__ALIGN64 static const int64u VMASK52[sizeof(U64)/sizeof(int64u)] = {
   REP8_DECL(DIGIT_MASK)
};


#define NORM_LSHIFTR(LO, HI) \
    (HI) = add64((HI), srli64((LO), DIGIT_SIZE)); \
    (LO) = and64((LO), loadu64(VMASK52));


void ifma52_sub_with_borrow(U64 r[], const U64 x[], const U64 y[])
{
   U64 base = loadu64(VBASE52);
   U64 one = set1(1LL);

   U64 subtrahend = get_zero64();
   __mmask8 lt = 0;

   subtrahend = _mm512_mask_add_epi64(y[0], lt, y[0], one);
   lt = _mm512_cmp_epu64_mask(x[0], subtrahend, _MM_CMPINT_LT);
   r[0] = _mm512_sub_epi64(x[0], subtrahend);
   r[0] = _mm512_mask_add_epi64(r[0], lt, r[0], base);

   subtrahend = _mm512_mask_add_epi64(y[1], lt, y[1], one);
   lt = _mm512_cmp_epu64_mask(x[1], subtrahend, _MM_CMPINT_LT);
   r[1] = _mm512_sub_epi64(x[1], subtrahend);
   r[1] = _mm512_mask_add_epi64(r[1], lt, r[1], base);

   subtrahend = _mm512_mask_add_epi64(y[2], lt, y[2], one);
   lt = _mm512_cmp_epu64_mask(x[2], subtrahend, _MM_CMPINT_LT);
   r[2] = _mm512_sub_epi64(x[2], subtrahend);
   r[2] = _mm512_mask_add_epi64(r[2], lt, r[2], base);

   subtrahend = _mm512_mask_add_epi64(y[3], lt, y[3], one);
   lt = _mm512_cmp_epu64_mask(x[3], subtrahend, _MM_CMPINT_LT);
   r[3] = _mm512_sub_epi64(x[3], subtrahend);
   r[3] = _mm512_mask_add_epi64(r[3], lt, r[3], base);

   subtrahend = _mm512_mask_add_epi64(y[4], lt, y[4], one);
   lt = _mm512_cmp_epu64_mask(x[4], subtrahend, _MM_CMPINT_LT);
   r[4] = _mm512_sub_epi64(x[4], subtrahend);
   r[4] = _mm512_mask_add_epi64(r[4], lt, r[4], base);

   /* the latest step is not necessary, bcause Barett algorithms guarantee that
   // 0<= (r = r1-r2) <3*modulus
   // r[0..4] is enough to keep 3*modulus
   */
   #if 0
   subtrahend = _mm512_mask_add_epi64(y[5], lt, y[5], one);
   lt = _mm512_cmp_epu64_mask(x[5], subtrahend, _MM_CMPINT_LT);
   r[5] = _mm512_sub_epi64(x[5], subtrahend);
   r[5] = _mm512_mask_add_epi64(r[5], lt, r[4], base);
   #endif
}

// r = x<ed25519n? x : x-ed25519n 
void ifma52_sub_ed25519n(U64 r[], const U64 x[])
{
   U64* n = (U64*)ed25519n_mb;
   U64 base = loadu64(VBASE52);
   U64 one = set1(1LL);

   U64 subtrahend = get_zero64();
   U64 t[NE_LEN52];
   __mmask8 lt = 0;

   subtrahend = _mm512_mask_add_epi64(n[0], lt, n[0], one);
   lt = _mm512_cmp_epu64_mask(x[0], subtrahend, _MM_CMPINT_LT);
   t[0] = _mm512_sub_epi64(x[0], subtrahend);
   t[0] = _mm512_mask_add_epi64(t[0], lt, t[0], base);

   subtrahend = _mm512_mask_add_epi64(n[1], lt, n[1], one);
   lt = _mm512_cmp_epu64_mask(x[1], subtrahend, _MM_CMPINT_LT);
   t[1] = _mm512_sub_epi64(x[1], subtrahend);
   t[1] = _mm512_mask_add_epi64(t[1], lt, t[1], base);

   subtrahend = _mm512_mask_add_epi64(n[2], lt, n[2], one);
   lt = _mm512_cmp_epu64_mask(x[2], subtrahend, _MM_CMPINT_LT);
   t[2] = _mm512_sub_epi64(x[2], subtrahend);
   t[2] = _mm512_mask_add_epi64(t[2], lt, t[2], base);

   subtrahend = _mm512_mask_add_epi64(n[3], lt, n[3], one);
   lt = _mm512_cmp_epu64_mask(x[3], subtrahend, _MM_CMPINT_LT);
   t[3] = _mm512_sub_epi64(x[3], subtrahend);
   t[3] = _mm512_mask_add_epi64(t[3], lt, t[3], base);

   subtrahend = _mm512_mask_add_epi64(n[4], lt, n[3], one);
   lt = _mm512_cmp_epu64_mask(x[4], subtrahend, _MM_CMPINT_LT);
   t[4] = _mm512_sub_epi64(x[4], subtrahend);
   t[4] = _mm512_mask_add_epi64(t[4], lt, t[4], base);

   /* keep x if x was smaller than n */
   r[0] = _mm512_mask_blend_epi64(lt, t[0], x[0]);
   r[1] = _mm512_mask_blend_epi64(lt, t[1], x[1]);
   r[2] = _mm512_mask_blend_epi64(lt, t[2], x[2]);
   r[3] = _mm512_mask_blend_epi64(lt, t[3], x[3]);
   r[4] = _mm512_mask_blend_epi64(lt, t[4], x[4]);
}


/*
// Barrett reduction: r = x mod n
//    where x and n are (2*k) and (k) digits numbers represented in radix b
//       b = 2^DIGIT_SIZE, k = ceil(253, DIGIT_SIZE)
//
//  input: x, bitsizeof(x) = 512, k=10
// output: r, bitsizeof(r) = 253, k=5
// pre-computed mu = floor(b^(2*k)/n)
//
// 1. q1 = floor(x/b^(k-1))
// 2. q2 = q1*mu
// 3. q3 = floor(q2/b^(k+1)) = q2 >>DITIT_SIZE*(K+1)
//
// 4. r1 = x mod b^(k+1)
// 5. r2 = q3*n mod b^(k+1)
// 6. r = r1-r2, if r<0 then r+=b^(k+1)
// 7. if r>n then r -=n  -- at most 2 final reductions
//       r>n then r -=n
*/
void ifma52_ed25519n_reduce(U64 r[NE_LEN52], const U64 x[NE_LEN52*2])
{
   /* 1. q1 = x/b^(k-1) -- just (k+1) most significant digits, len(q1) = 2*k - (k-1) = k+1 */
   const U64* q1 = x + (NE_LEN52-1);

   /*
   // 2. q2 = q1*mu, len(q2) = (k+1) + (k+1) = 2*k+2
   // 3. q3 = floor(q2 / b^(k+1)), len(q3) = 2*k+2 - (k+1) = k+1
   // => no necessary do compute whole (q1*mu) product, (k+1) most significant digits are enough
   //
   // code below represents full multiplication
   // from which discarded (commented out) computation of the least significant digits
   */
   U64* mu = (U64*)ed25519mu_mb;
   U64 /*q3_0, q3_1, q3_2, q3_3, q3_4, */ q3_5, q3_6, q3_7, q3_8, q3_9, q3_10, q3_11;
       /*q3_0= q3_1= q3_2= q3_3= q3_4= */ q3_5 = q3_6 = q3_7 = q3_8 = q3_9 = q3_10 = q3_11 = get_zero64();

   //ROUND_MUL(q3_0, q3_1, q1[0], mu[0]);

   //ROUND_MUL(q3_1, q3_2, q1[0], mu[1]);
   //ROUND_MUL(q3_1, q3_2, q1[1], mu[0]);

   //ROUND_MUL(q3_2, q3_3, q1[0], mu[2]);
   //ROUND_MUL(q3_2, q3_3, q1[1], mu[1]);
   //ROUND_MUL(q3_2, q3_3, q1[2], mu[0]);

   //ROUND_MUL(q3_3, q3_4, q1[0], mu[3]);
   //ROUND_MUL(q3_3, q3_4, q1[1], mu[2]);
   //ROUND_MUL(q3_3, q3_4, q1[2], mu[1]);
   //ROUND_MUL(q3_3, q3_4, q1[3], mu[0]);

   //ROUND_MUL(q3_4, q3_5, q1[0], mu[4]);
   //ROUND_MUL(q3_4, q3_5, q1[1], mu[3]);
   //ROUND_MUL(q3_4, q3_5, q1[2], mu[2]);
   //ROUND_MUL(q3_4, q3_5, q1[3], mu[1]);
   //ROUND_MUL(q3_4, q3_5, q1[4], mu[0]);
   q3_5 = fma52hi(q3_5, q1[0], mu[4]);
   q3_5 = fma52hi(q3_5, q1[1], mu[3]);
   q3_5 = fma52hi(q3_5, q1[2], mu[2]);
   q3_5 = fma52hi(q3_5, q1[3], mu[1]);
   q3_5 = fma52hi(q3_5, q1[4], mu[0]);

   ROUND_MUL(q3_5, q3_6, q1[0], mu[5]);
   ROUND_MUL(q3_5, q3_6, q1[1], mu[4]);
   ROUND_MUL(q3_5, q3_6, q1[2], mu[3]);
   ROUND_MUL(q3_5, q3_6, q1[3], mu[2]);
   ROUND_MUL(q3_5, q3_6, q1[4], mu[1]);
   ROUND_MUL(q3_5, q3_6, q1[5], mu[0]);

   ROUND_MUL(q3_6, q3_7, q1[1], mu[5]);
   ROUND_MUL(q3_6, q3_7, q1[2], mu[4]);
   ROUND_MUL(q3_6, q3_7, q1[3], mu[3]);
   ROUND_MUL(q3_6, q3_7, q1[4], mu[2]);
   ROUND_MUL(q3_6, q3_7, q1[5], mu[1]);

   ROUND_MUL(q3_7, q3_8, q1[2], mu[5]);
   ROUND_MUL(q3_7, q3_8, q1[3], mu[4]);
   ROUND_MUL(q3_7, q3_8, q1[4], mu[3]);
   ROUND_MUL(q3_7, q3_8, q1[5], mu[2]);

   ROUND_MUL(q3_8, q3_9, q1[3], mu[5]);
   ROUND_MUL(q3_8, q3_9, q1[4], mu[4]);
   ROUND_MUL(q3_8, q3_9, q1[5], mu[3]);

   ROUND_MUL(q3_9, q3_10, q1[4], mu[5]);
   ROUND_MUL(q3_9, q3_10, q1[5], mu[4]);

   /* note, that the lates (2*k+2) digit will always be 0 (count bits presentation of x and mu) */
   ROUND_MUL(q3_10, q3_11, q1[5], mu[5]);

   /* normalization,  q3 = {q3_11, q3_10, q3_9, q3_8, q3_7, q3_6}, note q3_11 is zero */
   NORM_LSHIFTR(q3_5, q3_6);
   NORM_LSHIFTR(q3_6, q3_7);
   NORM_LSHIFTR(q3_7, q3_8);
   NORM_LSHIFTR(q3_8, q3_9);
   NORM_LSHIFTR(q3_9, q3_10);
   NORM_LSHIFTR(q3_10, q3_11);

   /* 4. r1 = x mod b^(k+1) is just (k+1) least significant digits of x */
   const U64* r1 = x;

   /* 5. r2 = (q3*n) mod b^(k+1)
   // => no necessary do compute whole (q3*n) product, (k+1) least significant digits are enough
   //
   // code below represents full multiplication
   // from which discarded (commented out) computation of the most significant digits
   */
   U64* n = (U64*)ed25519n_mb;
   U64 r2_0,  r2_1,  r2_2,  r2_3,  r2_4,  r2_5;
       r2_0 = r2_1 = r2_2 = r2_3 = r2_4 = r2_5 = q3_11 = get_zero64();

   ROUND_MUL(r2_0, r2_1, q3_6, n[0]);

   ROUND_MUL(r2_1, r2_2, q3_6, n[1]);
   ROUND_MUL(r2_1, r2_2, q3_7, n[0]);

   ROUND_MUL(r2_2, r2_3, q3_6, n[2]);
   ROUND_MUL(r2_2, r2_3, q3_7, n[1]);
   ROUND_MUL(r2_2, r2_3, q3_8, n[0]);

   ROUND_MUL(r2_3, r2_4, q3_6, n[3]);
   ROUND_MUL(r2_3, r2_4, q3_7, n[2]);
   ROUND_MUL(r2_3, r2_4, q3_8, n[1]);
   ROUND_MUL(r2_3, r2_4, q3_9, n[0]);

   ROUND_MUL(r2_4, r2_5, q3_6, n[4]);
   ROUND_MUL(r2_4, r2_5, q3_7, n[3]);
   ROUND_MUL(r2_4, r2_5, q3_8, n[2]);
   ROUND_MUL(r2_4, r2_5, q3_9, n[1]);
   ROUND_MUL(r2_4, r2_5,q3_10, n[0]);

   r2_5 = fma52lo(r2_5, q3_7, n[4]);
   r2_5 = fma52lo(r2_5, q3_8, n[3]);
   r2_5 = fma52lo(r2_5, q3_9, n[2]);
   r2_5 = fma52lo(r2_5,q3_10, n[1]);
   r2_5 = fma52lo(r2_5,q3_11, n[0]);

   /* r2[6], r2[7], ... amd other most significal digits of full multiplication are discarded */
   //r2[6] = fma52hi(r2[5], q3_7, n[4]);
   //r2[6] = fma52hi(r2[5], q3_8, n[3]);
   //r2[6] = fma52hi(r2[5], q3_9, n[2]);
   //r2[6] = fma52hi(r2[5],q3_10, n[1]);
   //r2[6] = fma52hi(r2[5],q3_11, n[0]);

   /* normalization */
   NORM_LSHIFTR(r2_0, r2_1);
   NORM_LSHIFTR(r2_1, r2_2);
   NORM_LSHIFTR(r2_2, r2_3);
   NORM_LSHIFTR(r2_3, r2_4);
   NORM_LSHIFTR(r2_4, r2_5);
   NORM_LSHIFTR(r2_5, q3_11);
   /*  note q3_11 is out of mod b^(k+1) */

   U64 r2[NE_LEN52+1];
   r2[0] = r2_0;
   r2[1] = r2_1;
   r2[2] = r2_2;
   r2[3] = r2_3;
   r2[4] = r2_4;
   r2[5] = r2_5;

   /* 6. r = r1-r2 */
   ifma52_sub_with_borrow(r, r1, r2);

   /* 7. reduce modulo twice */
   ifma52_sub_ed25519n(r, r);
   ifma52_sub_ed25519n(r, r);
}

// r = a*b+c %n
void ifma52_ed25519n_madd(U64 r[NE_LEN52], const U64 a[NE_LEN52], const U64 b[NE_LEN52], const U64 c[NE_LEN52])
{
   U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

   r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

   /* (a*b) */
   ROUND_MUL(r0, r1, a[0], b[0]);
   ROUND_MUL(r1, r2, a[1], b[0]);
   ROUND_MUL(r2, r3, a[2], b[0]);
   ROUND_MUL(r3, r4, a[3], b[0]);
   ROUND_MUL(r4, r5, a[4], b[0]);

   ROUND_MUL(r1, r2, a[0], b[1]);
   ROUND_MUL(r2, r3, a[1], b[1]);
   ROUND_MUL(r3, r4, a[2], b[1]);
   ROUND_MUL(r4, r5, a[3], b[1]);
   ROUND_MUL(r5, r6, a[4], b[1]);

   ROUND_MUL(r2, r3, a[0], b[2]);
   ROUND_MUL(r3, r4, a[1], b[2]);
   ROUND_MUL(r4, r5, a[2], b[2]);
   ROUND_MUL(r5, r6, a[3], b[2]);
   ROUND_MUL(r6, r7, a[4], b[2]);

   ROUND_MUL(r3, r4, a[0], b[3]);
   ROUND_MUL(r4, r5, a[1], b[3]);
   ROUND_MUL(r5, r6, a[2], b[3]);
   ROUND_MUL(r6, r7, a[3], b[3]);
   ROUND_MUL(r7, r8, a[4], b[3]);

   ROUND_MUL(r4, r5, a[0], b[4]);
   ROUND_MUL(r5, r6, a[1], b[4]);
   ROUND_MUL(r6, r7, a[2], b[4]);
   ROUND_MUL(r7, r8, a[3], b[4]);
   ROUND_MUL(r8, r9, a[4], b[4]);

   /* (a*b) + c */
   r0 = add64(r0, c[0]);
   r1 = add64(r1, c[1]);
   r2 = add64(r2, c[2]);
   r3 = add64(r3, c[3]);
   r4 = add64(r4, c[4]);

   /* normalize result */
   NORM_LSHIFTR(r0, r1);
   NORM_LSHIFTR(r1, r2);
   NORM_LSHIFTR(r2, r3);
   NORM_LSHIFTR(r3, r4);
   NORM_LSHIFTR(r4, r5);
   NORM_LSHIFTR(r5, r6);
   NORM_LSHIFTR(r6, r7);
   NORM_LSHIFTR(r7, r8);
   NORM_LSHIFTR(r8, r9);

   U64 t[NE_LEN52*2];
   t[0] = r0;
   t[1] = r1;
   t[2] = r2;
   t[3] = r3;
   t[4] = r4;
   t[5] = r5;
   t[6] = r6;
   t[7] = r7;
   t[8] = r8;
   t[9] = r9;
   ifma52_ed25519n_reduce(r, t);
}

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

#include <internal/ecnist/ifma_arith_p256.h>


/*=====================================================================

 General 256-bit operations - sqr & mul

=====================================================================*/

void MB_FUNC_NAME(ifma_amm52x5_)(U64 R[], const U64 inpA[], const U64 inpB[], const U64 inpM[], const int64u* k0_mb)
{
  U64 res00, res01, res02, res03, res04;
  U64 K = loadu64(k0_mb); /* k0[] */
  int itr;

  res00 = res01 = res02 = res03 = res04 = get_zero64();

  for (itr = 0; itr < P256_LEN52; itr++) {
    U64 Yi;
    U64 Bi = loadu64(inpB);
    inpB++;

    res00 = fma52lo(res00, Bi, inpA[0]);
    res01 = fma52lo(res01, Bi, inpA[1]);
    res02 = fma52lo(res02, Bi, inpA[2]);
    res03 = fma52lo(res03, Bi, inpA[3]);
    res04 = fma52lo(res04, Bi, inpA[4]);

    Yi = fma52lo(get_zero64(), res00, K);

    res00 = fma52lo(res00, Yi, inpM[0]);
    res01 = fma52lo(res01, Yi, inpM[1]);
    res02 = fma52lo(res02, Yi, inpM[2]);
    res03 = fma52lo(res03, Yi, inpM[3]);
    res04 = fma52lo(res04, Yi, inpM[4]);

    res00 = srli64(res00, DIGIT_SIZE);
    res01 = add64 (res01, res00);

    res00 = fma52hi(res01, Bi, inpA[0]);
    res01 = fma52hi(res02, Bi, inpA[1]);
    res02 = fma52hi(res03, Bi, inpA[2]);
    res03 = fma52hi(res04, Bi, inpA[3]);
    res04 = fma52hi(get_zero64(), Bi, inpA[4]);

    res00 = fma52hi(res00, Yi, inpM[0]);
    res01 = fma52hi(res01, Yi, inpM[1]);
    res02 = fma52hi(res02, Yi, inpM[2]);
    res03 = fma52hi(res03, Yi, inpM[3]);
    res04 = fma52hi(res04, Yi, inpM[4]);
  }

  // normalization
  NORM_LSHIFTR(res0, 0, 1)
  NORM_LSHIFTR(res0, 1, 2)
  NORM_LSHIFTR(res0, 2, 3)
  NORM_LSHIFTR(res0, 3, 4)

#if 0
  // t = res -modulus and normalize
  U64 t0 = sub64(res00, inpM[0]);
  U64 t1 = sub64(res01, inpM[1]);
  U64 t2 = sub64(res02, inpM[2]);
  U64 t3 = sub64(res03, inpM[3]);
  U64 t4 = sub64(res04, inpM[4]);
  NORM_ASHIFTR(t, 0,1)
  NORM_ASHIFTR(t, 1,2)
  NORM_ASHIFTR(t, 2,3)
  NORM_ASHIFTR(t, 3,4)

   /* condition mov R[] = (t4>=0)? t : res */
  __mb_mask cmask = cmp64_mask(t4, get_zero64(), _MM_CMPINT_GE);
   R[0] = cmov_U64(res00, t0, cmask);
   R[1] = cmov_U64(res01, t1, cmask);
   R[2] = cmov_U64(res02, t2, cmask);
   R[3] = cmov_U64(res03, t3, cmask);
   R[4] = cmov_U64(res04, t4, cmask);
#endif
    R[0] = res00;
    R[1] = res01;
    R[2] = res02;
    R[3] = res03;
    R[4] = res04;
}

void MB_FUNC_NAME(ifma_ams52x5_)(U64 r[], const U64 a[], const U64 m[], const int64u* k0_mb)
{
  U64 k = loadu64((U64*)k0_mb);
  U64 res0, res1, res2, res3, res4, res5, res6, res7, res8, res9;
  res0 = res1 = res2 = res3 = res4 = res5 = res6 = res7 = res8 = res9 = get_zero64();

  // Calculate full square
  res1 = fma52lo(res1, a[0], a[1]);	// Sum(1)
  res2 = fma52hi(res2, a[0], a[1]);	// Sum(1)
  res2 = fma52lo(res2, a[0], a[2]);	// Sum(2)
  res3 = fma52hi(res3, a[0], a[2]);	// Sum(2)
  res3 = fma52lo(res3, a[1], a[2]);	// Sum(3)
  res4 = fma52hi(res4, a[1], a[2]);	// Sum(3)
  res3 = fma52lo(res3, a[0], a[3]);	// Sum(3)
  res4 = fma52hi(res4, a[0], a[3]);	// Sum(3)
  res4 = fma52lo(res4, a[1], a[3]);	// Sum(4)
  res5 = fma52hi(res5, a[1], a[3]);	// Sum(4)
  res5 = fma52lo(res5, a[2], a[3]);	// Sum(5)
  res6 = fma52hi(res6, a[2], a[3]);	// Sum(5)
  res4 = fma52lo(res4, a[0], a[4]);	// Sum(4)
  res5 = fma52hi(res5, a[0], a[4]);	// Sum(4)
  res5 = fma52lo(res5, a[1], a[4]);	// Sum(5)
  res6 = fma52hi(res6, a[1], a[4]);	// Sum(5)
  res6 = fma52lo(res6, a[2], a[4]);	// Sum(6)
  res7 = fma52hi(res7, a[2], a[4]);	// Sum(6)
  res7 = fma52lo(res7, a[3], a[4]);	// Sum(7)
  res8 = fma52hi(res8, a[3], a[4]);	// Sum(7)
  {
    res0 = add64(res0, res0);	// Double(0)
    res1 = add64(res1, res1);	// Double(1)
    res2 = add64(res2, res2);	// Double(2)
    res3 = add64(res3, res3);	// Double(3)
    res4 = add64(res4, res4);	// Double(4)
    res5 = add64(res5, res5);	// Double(5)
    res6 = add64(res6, res6);	// Double(6)
    res7 = add64(res7, res7);	// Double(7)
    res8 = add64(res8, res8);	// Double(8)
    {
      res0 = fma52lo(res0, a[0], a[0]);	// Add sqr(0)
      res1 = fma52hi(res1, a[0], a[0]);	// Add sqr(0)
      res2 = fma52lo(res2, a[1], a[1]);	// Add sqr(2)
      res3 = fma52hi(res3, a[1], a[1]);	// Add sqr(2)
      res4 = fma52lo(res4, a[2], a[2]);	// Add sqr(4)
      res5 = fma52hi(res5, a[2], a[2]);	// Add sqr(4)
      res6 = fma52lo(res6, a[3], a[3]);	// Add sqr(6)
      res7 = fma52hi(res7, a[3], a[3]);	// Add sqr(6)
      res8 = fma52lo(res8, a[4], a[4]);	// Add sqr(8)
      res9 = fma52hi(res9, a[4], a[4]);	// Add sqr(8)
    }
  }

  // Reduction
  U64 u0 = mul52lo(res0,k);

  // Create u0
  res0 = fma52lo(res0, u0, m[0]);
  res1 = fma52hi(res1, u0, m[0]);
  res1 = fma52lo(res1, u0, m[1]);
  res2 = fma52hi(res2, u0, m[1]);
  res1 = add64(res1, srli64(res0, DIGIT_SIZE));
  U64 u1 = mul52lo(res1,k);
  res2 = fma52lo(res2, u0, m[2]);
  res3 = fma52hi(res3, u0, m[2]);
  res3 = fma52lo(res3, u0, m[3]);
  res4 = fma52hi(res4, u0, m[3]);
  res4 = fma52lo(res4, u0, m[4]);
  res5 = fma52hi(res5, u0, m[4]);

  // Create u1
  res1 = fma52lo(res1, u1, m[0]);
  res2 = fma52hi(res2, u1, m[0]);
  res2 = fma52lo(res2, u1, m[1]);
  res3 = fma52hi(res3, u1, m[1]);
  res2 = add64(res2, srli64(res1, DIGIT_SIZE));
  U64 u2 = mul52lo(res2,k);
  res3 = fma52lo(res3, u1, m[2]);
  res4 = fma52hi(res4, u1, m[2]);
  res4 = fma52lo(res4, u1, m[3]);
  res5 = fma52hi(res5, u1, m[3]);
  res5 = fma52lo(res5, u1, m[4]);
  res6 = fma52hi(res6, u1, m[4]);

  // Create u2
  res2 = fma52lo(res2, u2, m[0]);
  res3 = fma52hi(res3, u2, m[0]);
  res3 = fma52lo(res3, u2, m[1]);
  res4 = fma52hi(res4, u2, m[1]);
  res3 = add64(res3, srli64(res2, DIGIT_SIZE));
  U64 u3 = mul52lo(res3,k);
  res4 = fma52lo(res4, u2, m[2]);
  res5 = fma52hi(res5, u2, m[2]);
  res5 = fma52lo(res5, u2, m[3]);
  res6 = fma52hi(res6, u2, m[3]);
  res6 = fma52lo(res6, u2, m[4]);
  res7 = fma52hi(res7, u2, m[4]);

  // Create u3
  res3 = fma52lo(res3, u3, m[0]);
  res4 = fma52hi(res4, u3, m[0]);
  res4 = fma52lo(res4, u3, m[1]);
  res5 = fma52hi(res5, u3, m[1]);
  res4 = add64(res4, srli64(res3, DIGIT_SIZE));
  U64 u4 = mul52lo(res4,k);
  res5 = fma52lo(res5, u3, m[2]);
  res6 = fma52hi(res6, u3, m[2]);
  res6 = fma52lo(res6, u3, m[3]);
  res7 = fma52hi(res7, u3, m[3]);
  res7 = fma52lo(res7, u3, m[4]);
  res8 = fma52hi(res8, u3, m[4]);

  // Create u4
  res4 = fma52lo(res4, u4, m[0]);
  res5 = fma52hi(res5, u4, m[0]);
  res5 = fma52lo(res5, u4, m[1]);
  res6 = fma52hi(res6, u4, m[1]);
  res5 = add64(res5, srli64(res4, DIGIT_SIZE));
  res6 = fma52lo(res6, u4, m[2]);
  res7 = fma52hi(res7, u4, m[2]);
  res7 = fma52lo(res7, u4, m[3]);
  res8 = fma52hi(res8, u4, m[3]);
  res8 = fma52lo(res8, u4, m[4]);
  res9 = fma52hi(res9, u4, m[4]);

  // normalization
  NORM_LSHIFTR(res, 5, 6)
  NORM_LSHIFTR(res, 6, 7)
  NORM_LSHIFTR(res, 7, 8)
  NORM_LSHIFTR(res, 8, 9)
#if 0
  // {res0-4) = (res5-9) -modulus and normalize
  res0 = sub64(res5, m[0]);
  res1 = sub64(res6, m[1]);
  res2 = sub64(res7, m[2]);
  res3 = sub64(res8, m[3]);
  res4 = sub64(res9, m[4]);
  NORM_ASHIFTR(res, 0,1)
  NORM_ASHIFTR(res, 1,2)
  NORM_ASHIFTR(res, 2,3)
  NORM_ASHIFTR(res, 3,4)

   /* condition mov r[] = (res4>=0)? res(5 - 9) : res(0 - 4) */
  __mb_mask cmask = cmp64_mask(res4, get_zero64(), _MM_CMPINT_GE);
   r[0] = cmov_U64(res5, res0, cmask);
   r[1] = cmov_U64(res6, res1, cmask);
   r[2] = cmov_U64(res7, res2, cmask);
   r[3] = cmov_U64(res8, res3, cmask);
   r[4] = cmov_U64(res9, res4, cmask);
#endif

   r[0] = res5;
   r[1] = res6;
   r[2] = res7;
   r[3] = res8;
   r[4] = res9;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* R = (A+B) mod M */
void MB_FUNC_NAME(ifma_add52x5_)(U64 R[], const U64 A[], const U64 B[], const U64 M[])
{
   /* r = a + b */
   U64 r0 = add64(A[0], B[0]);
   U64 r1 = add64(A[1], B[1]);
   U64 r2 = add64(A[2], B[2]);
   U64 r3 = add64(A[3], B[3]);
   U64 r4 = add64(A[4], B[4]);

   /* t = r - M */
   U64 t0 = sub64(r0, M[0]);
   U64 t1 = sub64(r1, M[1]);
   U64 t2 = sub64(r2, M[2]);
   U64 t3 = sub64(r3, M[3]);
   U64 t4 = sub64(r4, M[4]);

   /* normalize r0, r1, r2, r3, r4 */
   NORM_LSHIFTR(r, 0,1)
   NORM_LSHIFTR(r, 1,2)
   NORM_LSHIFTR(r, 2,3)
   NORM_LSHIFTR(r, 3,4)

   /* normalize t0, t1, t2, t3, t4 */
   NORM_ASHIFTR(t, 0,1)
   NORM_ASHIFTR(t, 1,2)
   NORM_ASHIFTR(t, 2,3)
   NORM_ASHIFTR(t, 3,4)

   /* condition mask t4<0? (-1) : 0 */
   __mb_mask cmask = cmp64_mask(t4, get_zero64(), _MM_CMPINT_LT);

   R[0] = cmov_U64(t0, r0, cmask);
   R[1] = cmov_U64(t1, r1, cmask);
   R[2] = cmov_U64(t2, r2, cmask);
   R[3] = cmov_U64(t3, r3, cmask);
   R[4] = cmov_U64(t4, r4, cmask);
}


/* R = (A-B) mod M */
void MB_FUNC_NAME(ifma_sub52x5_)(U64 R[], const U64 A[], const U64 B[], const U64 M[])
{
   /* r = a-b */
   U64 r0 = sub64(A[0], B[0]);
   U64 r1 = sub64(A[1], B[1]);
   U64 r2 = sub64(A[2], B[2]);
   U64 r3 = sub64(A[3], B[3]);
   U64 r4 = sub64(A[4], B[4]);

   /* t = r + M */
   U64 t0 = add64(r0, M[0]);
   U64 t1 = add64(r1, M[1]);
   U64 t2 = add64(r2, M[2]);
   U64 t3 = add64(r3, M[3]);
   U64 t4 = add64(r4, M[4]);

   /* normalize r0, r1, r2, r3, r4 */
   NORM_ASHIFTR(r, 0,1)
   NORM_ASHIFTR(r, 1,2)
   NORM_ASHIFTR(r, 2,3)
   NORM_ASHIFTR(r, 3,4)

   /* normalize t0, t1, t2, t3, t4 */
   NORM_ASHIFTR(t, 0,1)
   NORM_ASHIFTR(t, 1,2)
   NORM_ASHIFTR(t, 2,3)
   NORM_ASHIFTR(t, 3,4)

   /* condition mask t4<0? (-1) : 0 */
   __mb_mask cmask = cmp64_mask(r4, get_zero64(), _MM_CMPINT_LT);

   R[0] = cmov_U64(r0, t0, cmask);
   R[1] = cmov_U64(r1, t1, cmask);
   R[2] = cmov_U64(r2, t2, cmask);
   R[3] = cmov_U64(r3, t3, cmask);
   R[4] = cmov_U64(r4, t4, cmask);
}

/* R = (-A) mod M */
void MB_FUNC_NAME(ifma_neg52x5_)(U64 R[], const U64 A[], const U64 M[])
{
   /*  mask = a[]!=0? 1 : 0 */
   U64 t = _mm512_or_epi64(A[0], A[1]);
   t = _mm512_or_epi64(t, A[2]);
   t = _mm512_or_epi64(t, A[3]);
   t = _mm512_or_epi64(t, A[4]);
   __mb_mask mask = cmp64_mask(t, get_zero64(), _MM_CMPINT_NE);

   /* r = M - A */
   U64 r0 = _mm512_maskz_sub_epi64(mask, M[0], A[0]);
   U64 r1 = _mm512_maskz_sub_epi64(mask, M[1], A[1]);
   U64 r2 = _mm512_maskz_sub_epi64(mask, M[2], A[2]);
   U64 r3 = _mm512_maskz_sub_epi64(mask, M[3], A[3]);
   U64 r4 = _mm512_maskz_sub_epi64(mask, M[4], A[4]);

   /* normalize r0, r1, r2, r3, r4 */
   NORM_ASHIFTR(r, 0,1)
   NORM_ASHIFTR(r, 1,2)
   NORM_ASHIFTR(r, 2,3)
   NORM_ASHIFTR(r, 3,4)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
}

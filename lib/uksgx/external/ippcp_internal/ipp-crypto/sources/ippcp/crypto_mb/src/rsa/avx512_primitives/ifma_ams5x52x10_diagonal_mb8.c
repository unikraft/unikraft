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

#include <internal/common/ifma_math.h>

#ifdef __GNUC__
#define ASM(a) __asm__(a);
#else
#define ASM(a)
#endif

void AMS5x52x10_diagonal_mb8(int64u *out_mb, const int64u *inpA_mb,
                             const int64u *inpM_mb, const int64u *k0_mb) {
  U64 res0, res1, res2, res3, res4, res5, res6, res7, res8, res9, res10, res11,
      res12, res13, res14, res15, res16, res17, res18, res19;
  U64 k;
  U64 *a = (U64 *)inpA_mb;
  U64 *m = (U64 *)inpM_mb;
  U64 *r = (U64 *)out_mb;
  int iter;
  const int iters = 5;

  k = loadu64((U64 *)k0_mb);
  for (iter = 0; iter < iters; ++iter) {
    res0 = res1 = res2 = res3 = res4 = res5 = res6 = res7 = res8 = res9 =
        res10 = res11 = res12 = res13 = res14 = res15 = res16 = res17 = res18 =
            res19 = get_zero64();
    // Calculate full square
    res1 = fma52lo(res1, a[0], a[1]);   // Sum(1)
    res2 = fma52hi(res2, a[0], a[1]);   // Sum(1)
    res2 = fma52lo(res2, a[0], a[2]);   // Sum(2)
    res3 = fma52hi(res3, a[0], a[2]);   // Sum(2)
    res3 = fma52lo(res3, a[1], a[2]);   // Sum(3)
    res4 = fma52hi(res4, a[1], a[2]);   // Sum(3)
    res3 = fma52lo(res3, a[0], a[3]);   // Sum(3)
    res4 = fma52hi(res4, a[0], a[3]);   // Sum(3)
    res4 = fma52lo(res4, a[1], a[3]);   // Sum(4)
    res5 = fma52hi(res5, a[1], a[3]);   // Sum(4)
    res5 = fma52lo(res5, a[2], a[3]);   // Sum(5)
    res6 = fma52hi(res6, a[2], a[3]);   // Sum(5)
    res4 = fma52lo(res4, a[0], a[4]);   // Sum(4)
    res5 = fma52hi(res5, a[0], a[4]);   // Sum(4)
    res5 = fma52lo(res5, a[1], a[4]);   // Sum(5)
    res6 = fma52hi(res6, a[1], a[4]);   // Sum(5)
    res6 = fma52lo(res6, a[2], a[4]);   // Sum(6)
    res7 = fma52hi(res7, a[2], a[4]);   // Sum(6)
    res7 = fma52lo(res7, a[3], a[4]);   // Sum(7)
    res8 = fma52hi(res8, a[3], a[4]);   // Sum(7)
    res5 = fma52lo(res5, a[0], a[5]);   // Sum(5)
    res6 = fma52hi(res6, a[0], a[5]);   // Sum(5)
    res6 = fma52lo(res6, a[1], a[5]);   // Sum(6)
    res7 = fma52hi(res7, a[1], a[5]);   // Sum(6)
    res7 = fma52lo(res7, a[2], a[5]);   // Sum(7)
    res8 = fma52hi(res8, a[2], a[5]);   // Sum(7)
    res8 = fma52lo(res8, a[3], a[5]);   // Sum(8)
    res9 = fma52hi(res9, a[3], a[5]);   // Sum(8)
    res9 = fma52lo(res9, a[4], a[5]);   // Sum(9)
    res10 = fma52hi(res10, a[4], a[5]); // Sum(9)
    res6 = fma52lo(res6, a[0], a[6]);   // Sum(6)
    res7 = fma52hi(res7, a[0], a[6]);   // Sum(6)
    res7 = fma52lo(res7, a[1], a[6]);   // Sum(7)
    res8 = fma52hi(res8, a[1], a[6]);   // Sum(7)
    res8 = fma52lo(res8, a[2], a[6]);   // Sum(8)
    res9 = fma52hi(res9, a[2], a[6]);   // Sum(8)
    res9 = fma52lo(res9, a[3], a[6]);   // Sum(9)
    res10 = fma52hi(res10, a[3], a[6]); // Sum(9)
    res10 = fma52lo(res10, a[4], a[6]); // Sum(10)
    res11 = fma52hi(res11, a[4], a[6]); // Sum(10)
    res11 = fma52lo(res11, a[5], a[6]); // Sum(11)
    res12 = fma52hi(res12, a[5], a[6]); // Sum(11)
    res7 = fma52lo(res7, a[0], a[7]);   // Sum(7)
    res8 = fma52hi(res8, a[0], a[7]);   // Sum(7)
    res8 = fma52lo(res8, a[1], a[7]);   // Sum(8)
    res9 = fma52hi(res9, a[1], a[7]);   // Sum(8)
    res9 = fma52lo(res9, a[2], a[7]);   // Sum(9)
    res10 = fma52hi(res10, a[2], a[7]); // Sum(9)
    res10 = fma52lo(res10, a[3], a[7]); // Sum(10)
    res11 = fma52hi(res11, a[3], a[7]); // Sum(10)
    res11 = fma52lo(res11, a[4], a[7]); // Sum(11)
    res12 = fma52hi(res12, a[4], a[7]); // Sum(11)
    res8 = fma52lo(res8, a[0], a[8]);   // Sum(8)
    res9 = fma52hi(res9, a[0], a[8]);   // Sum(8)
    res9 = fma52lo(res9, a[1], a[8]);   // Sum(9)
    res10 = fma52hi(res10, a[1], a[8]); // Sum(9)
    res10 = fma52lo(res10, a[2], a[8]); // Sum(10)
    res11 = fma52hi(res11, a[2], a[8]); // Sum(10)
    res11 = fma52lo(res11, a[3], a[8]); // Sum(11)
    res12 = fma52hi(res12, a[3], a[8]); // Sum(11)
    res9 = fma52lo(res9, a[0], a[9]);   // Sum(9)
    res10 = fma52hi(res10, a[0], a[9]); // Sum(9)
    res10 = fma52lo(res10, a[1], a[9]); // Sum(10)
    res11 = fma52hi(res11, a[1], a[9]); // Sum(10)
    res11 = fma52lo(res11, a[2], a[9]); // Sum(11)
    res12 = fma52hi(res12, a[2], a[9]); // Sum(11)
    res0 = add64(res0, res0);           // Double(0)
    res1 = add64(res1, res1);           // Double(1)
    res2 = add64(res2, res2);           // Double(2)
    res3 = add64(res3, res3);           // Double(3)
    res4 = add64(res4, res4);           // Double(4)
    res5 = add64(res5, res5);           // Double(5)
    res6 = add64(res6, res6);           // Double(6)
    res7 = add64(res7, res7);           // Double(7)
    res8 = add64(res8, res8);           // Double(8)
    res9 = add64(res9, res9);           // Double(9)
    res10 = add64(res10, res10);        // Double(10)
    res11 = add64(res11, res11);        // Double(11)
    res0 = fma52lo(res0, a[0], a[0]);   // Add sqr(0)
    res1 = fma52hi(res1, a[0], a[0]);   // Add sqr(0)
    res2 = fma52lo(res2, a[1], a[1]);   // Add sqr(2)
    res3 = fma52hi(res3, a[1], a[1]);   // Add sqr(2)
    res4 = fma52lo(res4, a[2], a[2]);   // Add sqr(4)
    res5 = fma52hi(res5, a[2], a[2]);   // Add sqr(4)
    res6 = fma52lo(res6, a[3], a[3]);   // Add sqr(6)
    res7 = fma52hi(res7, a[3], a[3]);   // Add sqr(6)
    res8 = fma52lo(res8, a[4], a[4]);   // Add sqr(8)
    res9 = fma52hi(res9, a[4], a[4]);   // Add sqr(8)
    res10 = fma52lo(res10, a[5], a[5]); // Add sqr(10)
    res11 = fma52hi(res11, a[5], a[5]); // Add sqr(10)
    res12 = fma52lo(res12, a[5], a[7]); // Sum(12)
    res13 = fma52hi(res13, a[5], a[7]); // Sum(12)
    res13 = fma52lo(res13, a[6], a[7]); // Sum(13)
    res14 = fma52hi(res14, a[6], a[7]); // Sum(13)
    res12 = fma52lo(res12, a[4], a[8]); // Sum(12)
    res13 = fma52hi(res13, a[4], a[8]); // Sum(12)
    res13 = fma52lo(res13, a[5], a[8]); // Sum(13)
    res14 = fma52hi(res14, a[5], a[8]); // Sum(13)
    res14 = fma52lo(res14, a[6], a[8]); // Sum(14)
    res15 = fma52hi(res15, a[6], a[8]); // Sum(14)
    res15 = fma52lo(res15, a[7], a[8]); // Sum(15)
    res16 = fma52hi(res16, a[7], a[8]); // Sum(15)
    res12 = fma52lo(res12, a[3], a[9]); // Sum(12)
    res13 = fma52hi(res13, a[3], a[9]); // Sum(12)
    res13 = fma52lo(res13, a[4], a[9]); // Sum(13)
    res14 = fma52hi(res14, a[4], a[9]); // Sum(13)
    res14 = fma52lo(res14, a[5], a[9]); // Sum(14)
    res15 = fma52hi(res15, a[5], a[9]); // Sum(14)
    res15 = fma52lo(res15, a[6], a[9]); // Sum(15)
    res16 = fma52hi(res16, a[6], a[9]); // Sum(15)
    res16 = fma52lo(res16, a[7], a[9]); // Sum(16)
    res17 = fma52hi(res17, a[7], a[9]); // Sum(16)
    res17 = fma52lo(res17, a[8], a[9]); // Sum(17)
    res18 = fma52hi(res18, a[8], a[9]); // Sum(17)
    res12 = add64(res12, res12);        // Double(12)
    res13 = add64(res13, res13);        // Double(13)
    res14 = add64(res14, res14);        // Double(14)
    res15 = add64(res15, res15);        // Double(15)
    res16 = add64(res16, res16);        // Double(16)
    res17 = add64(res17, res17);        // Double(17)
    res18 = add64(res18, res18);        // Double(18)
    res12 = fma52lo(res12, a[6], a[6]); // Add sqr(12)
    res13 = fma52hi(res13, a[6], a[6]); // Add sqr(12)
    res14 = fma52lo(res14, a[7], a[7]); // Add sqr(14)
    res15 = fma52hi(res15, a[7], a[7]); // Add sqr(14)
    res16 = fma52lo(res16, a[8], a[8]); // Add sqr(16)
    res17 = fma52hi(res17, a[8], a[8]); // Add sqr(16)
    res18 = fma52lo(res18, a[9], a[9]); // Add sqr(18)
    res19 = fma52hi(res19, a[9], a[9]); // Add sqr(18)

    // Generate u_i
    U64 u0 = mul52lo(res0, k);
    ASM("jmp l0\nl0:\n");

    // Create u0
    fma52lo_mem(res0, res0, u0, m, SIMD_BYTES * 0);
    fma52hi_mem(res1, res1, u0, m, SIMD_BYTES * 0);
    res1 = fma52lo(res1, u0, m[1]);
    res2 = fma52hi(res2, u0, m[1]);
    res1 = add64(res1, srli64(res0, DIGIT_SIZE));
    U64 u1 = mul52lo(res1, k);
    fma52lo_mem(res2, res2, u0, m, SIMD_BYTES * 2);
    fma52hi_mem(res3, res3, u0, m, SIMD_BYTES * 2);
    res3 = fma52lo(res3, u0, m[3]);
    res4 = fma52hi(res4, u0, m[3]);
    fma52lo_mem(res4, res4, u0, m, SIMD_BYTES * 4);
    fma52hi_mem(res5, res5, u0, m, SIMD_BYTES * 4);
    res5 = fma52lo(res5, u0, m[5]);
    res6 = fma52hi(res6, u0, m[5]);
    fma52lo_mem(res6, res6, u0, m, SIMD_BYTES * 6);
    fma52hi_mem(res7, res7, u0, m, SIMD_BYTES * 6);
    res7 = fma52lo(res7, u0, m[7]);
    res8 = fma52hi(res8, u0, m[7]);
    fma52lo_mem(res8, res8, u0, m, SIMD_BYTES * 8);
    fma52hi_mem(res9, res9, u0, m, SIMD_BYTES * 8);
    res9 = fma52lo(res9, u0, m[9]);
    res10 = fma52hi(res10, u0, m[9]);

    // Create u1
    fma52lo_mem(res1, res1, u1, m, SIMD_BYTES * 0);
    fma52hi_mem(res2, res2, u1, m, SIMD_BYTES * 0);
    res2 = fma52lo(res2, u1, m[1]);
    res3 = fma52hi(res3, u1, m[1]);
    res2 = add64(res2, srli64(res1, DIGIT_SIZE));
    U64 u2 = mul52lo(res2, k);
    fma52lo_mem(res3, res3, u1, m, SIMD_BYTES * 2);
    fma52hi_mem(res4, res4, u1, m, SIMD_BYTES * 2);
    res4 = fma52lo(res4, u1, m[3]);
    res5 = fma52hi(res5, u1, m[3]);
    fma52lo_mem(res5, res5, u1, m, SIMD_BYTES * 4);
    fma52hi_mem(res6, res6, u1, m, SIMD_BYTES * 4);
    res6 = fma52lo(res6, u1, m[5]);
    res7 = fma52hi(res7, u1, m[5]);
    fma52lo_mem(res7, res7, u1, m, SIMD_BYTES * 6);
    fma52hi_mem(res8, res8, u1, m, SIMD_BYTES * 6);
    res8 = fma52lo(res8, u1, m[7]);
    res9 = fma52hi(res9, u1, m[7]);
    fma52lo_mem(res9, res9, u1, m, SIMD_BYTES * 8);
    fma52hi_mem(res10, res10, u1, m, SIMD_BYTES * 8);
    res10 = fma52lo(res10, u1, m[9]);
    res11 = fma52hi(res11, u1, m[9]);
    ASM("jmp l2\nl2:\n");

    // Create u2
    fma52lo_mem(res2, res2, u2, m, SIMD_BYTES * 0);
    fma52hi_mem(res3, res3, u2, m, SIMD_BYTES * 0);
    res3 = fma52lo(res3, u2, m[1]);
    res4 = fma52hi(res4, u2, m[1]);
    res3 = add64(res3, srli64(res2, DIGIT_SIZE));
    U64 u3 = mul52lo(res3, k);
    fma52lo_mem(res4, res4, u2, m, SIMD_BYTES * 2);
    fma52hi_mem(res5, res5, u2, m, SIMD_BYTES * 2);
    res5 = fma52lo(res5, u2, m[3]);
    res6 = fma52hi(res6, u2, m[3]);
    fma52lo_mem(res6, res6, u2, m, SIMD_BYTES * 4);
    fma52hi_mem(res7, res7, u2, m, SIMD_BYTES * 4);
    res7 = fma52lo(res7, u2, m[5]);
    res8 = fma52hi(res8, u2, m[5]);
    fma52lo_mem(res8, res8, u2, m, SIMD_BYTES * 6);
    fma52hi_mem(res9, res9, u2, m, SIMD_BYTES * 6);
    res9 = fma52lo(res9, u2, m[7]);
    res10 = fma52hi(res10, u2, m[7]);
    fma52lo_mem(res10, res10, u2, m, SIMD_BYTES * 8);
    fma52hi_mem(res11, res11, u2, m, SIMD_BYTES * 8);
    res11 = fma52lo(res11, u2, m[9]);
    res12 = fma52hi(res12, u2, m[9]);

    // Create u3
    fma52lo_mem(res3, res3, u3, m, SIMD_BYTES * 0);
    fma52hi_mem(res4, res4, u3, m, SIMD_BYTES * 0);
    res4 = fma52lo(res4, u3, m[1]);
    res5 = fma52hi(res5, u3, m[1]);
    res4 = add64(res4, srli64(res3, DIGIT_SIZE));
    U64 u4 = mul52lo(res4, k);
    fma52lo_mem(res5, res5, u3, m, SIMD_BYTES * 2);
    fma52hi_mem(res6, res6, u3, m, SIMD_BYTES * 2);
    res6 = fma52lo(res6, u3, m[3]);
    res7 = fma52hi(res7, u3, m[3]);
    fma52lo_mem(res7, res7, u3, m, SIMD_BYTES * 4);
    fma52hi_mem(res8, res8, u3, m, SIMD_BYTES * 4);
    res8 = fma52lo(res8, u3, m[5]);
    res9 = fma52hi(res9, u3, m[5]);
    fma52lo_mem(res9, res9, u3, m, SIMD_BYTES * 6);
    fma52hi_mem(res10, res10, u3, m, SIMD_BYTES * 6);
    res10 = fma52lo(res10, u3, m[7]);
    res11 = fma52hi(res11, u3, m[7]);
    fma52lo_mem(res11, res11, u3, m, SIMD_BYTES * 8);
    fma52hi_mem(res12, res12, u3, m, SIMD_BYTES * 8);
    res12 = fma52lo(res12, u3, m[9]);
    res13 = fma52hi(res13, u3, m[9]);
    ASM("jmp l4\nl4:\n");

    // Create u4
    fma52lo_mem(res4, res4, u4, m, SIMD_BYTES * 0);
    fma52hi_mem(res5, res5, u4, m, SIMD_BYTES * 0);
    res5 = fma52lo(res5, u4, m[1]);
    res6 = fma52hi(res6, u4, m[1]);
    res5 = add64(res5, srli64(res4, DIGIT_SIZE));
    U64 u5 = mul52lo(res5, k);
    fma52lo_mem(res6, res6, u4, m, SIMD_BYTES * 2);
    fma52hi_mem(res7, res7, u4, m, SIMD_BYTES * 2);
    res7 = fma52lo(res7, u4, m[3]);
    res8 = fma52hi(res8, u4, m[3]);
    fma52lo_mem(res8, res8, u4, m, SIMD_BYTES * 4);
    fma52hi_mem(res9, res9, u4, m, SIMD_BYTES * 4);
    res9 = fma52lo(res9, u4, m[5]);
    res10 = fma52hi(res10, u4, m[5]);
    fma52lo_mem(res10, res10, u4, m, SIMD_BYTES * 6);
    fma52hi_mem(res11, res11, u4, m, SIMD_BYTES * 6);
    res11 = fma52lo(res11, u4, m[7]);
    res12 = fma52hi(res12, u4, m[7]);
    fma52lo_mem(res12, res12, u4, m, SIMD_BYTES * 8);
    fma52hi_mem(res13, res13, u4, m, SIMD_BYTES * 8);
    res13 = fma52lo(res13, u4, m[9]);
    res14 = fma52hi(res14, u4, m[9]);

    // Create u5
    fma52lo_mem(res5, res5, u5, m, SIMD_BYTES * 0);
    fma52hi_mem(res6, res6, u5, m, SIMD_BYTES * 0);
    res6 = fma52lo(res6, u5, m[1]);
    res7 = fma52hi(res7, u5, m[1]);
    res6 = add64(res6, srli64(res5, DIGIT_SIZE));
    U64 u6 = mul52lo(res6, k);
    fma52lo_mem(res7, res7, u5, m, SIMD_BYTES * 2);
    fma52hi_mem(res8, res8, u5, m, SIMD_BYTES * 2);
    res8 = fma52lo(res8, u5, m[3]);
    res9 = fma52hi(res9, u5, m[3]);
    fma52lo_mem(res9, res9, u5, m, SIMD_BYTES * 4);
    fma52hi_mem(res10, res10, u5, m, SIMD_BYTES * 4);
    res10 = fma52lo(res10, u5, m[5]);
    res11 = fma52hi(res11, u5, m[5]);
    fma52lo_mem(res11, res11, u5, m, SIMD_BYTES * 6);
    fma52hi_mem(res12, res12, u5, m, SIMD_BYTES * 6);
    res12 = fma52lo(res12, u5, m[7]);
    res13 = fma52hi(res13, u5, m[7]);
    fma52lo_mem(res13, res13, u5, m, SIMD_BYTES * 8);
    fma52hi_mem(res14, res14, u5, m, SIMD_BYTES * 8);
    res14 = fma52lo(res14, u5, m[9]);
    res15 = fma52hi(res15, u5, m[9]);
    ASM("jmp l6\nl6:\n");

    // Create u6
    fma52lo_mem(res6, res6, u6, m, SIMD_BYTES * 0);
    fma52hi_mem(res7, res7, u6, m, SIMD_BYTES * 0);
    res7 = fma52lo(res7, u6, m[1]);
    res8 = fma52hi(res8, u6, m[1]);
    res7 = add64(res7, srli64(res6, DIGIT_SIZE));
    U64 u7 = mul52lo(res7, k);
    fma52lo_mem(res8, res8, u6, m, SIMD_BYTES * 2);
    fma52hi_mem(res9, res9, u6, m, SIMD_BYTES * 2);
    res9 = fma52lo(res9, u6, m[3]);
    res10 = fma52hi(res10, u6, m[3]);
    fma52lo_mem(res10, res10, u6, m, SIMD_BYTES * 4);
    fma52hi_mem(res11, res11, u6, m, SIMD_BYTES * 4);
    res11 = fma52lo(res11, u6, m[5]);
    res12 = fma52hi(res12, u6, m[5]);
    fma52lo_mem(res12, res12, u6, m, SIMD_BYTES * 6);
    fma52hi_mem(res13, res13, u6, m, SIMD_BYTES * 6);
    res13 = fma52lo(res13, u6, m[7]);
    res14 = fma52hi(res14, u6, m[7]);
    fma52lo_mem(res14, res14, u6, m, SIMD_BYTES * 8);
    fma52hi_mem(res15, res15, u6, m, SIMD_BYTES * 8);
    res15 = fma52lo(res15, u6, m[9]);
    res16 = fma52hi(res16, u6, m[9]);

    // Create u7
    fma52lo_mem(res7, res7, u7, m, SIMD_BYTES * 0);
    fma52hi_mem(res8, res8, u7, m, SIMD_BYTES * 0);
    res8 = fma52lo(res8, u7, m[1]);
    res9 = fma52hi(res9, u7, m[1]);
    res8 = add64(res8, srli64(res7, DIGIT_SIZE));
    U64 u8 = mul52lo(res8, k);
    fma52lo_mem(res9, res9, u7, m, SIMD_BYTES * 2);
    fma52hi_mem(res10, res10, u7, m, SIMD_BYTES * 2);
    res10 = fma52lo(res10, u7, m[3]);
    res11 = fma52hi(res11, u7, m[3]);
    fma52lo_mem(res11, res11, u7, m, SIMD_BYTES * 4);
    fma52hi_mem(res12, res12, u7, m, SIMD_BYTES * 4);
    res12 = fma52lo(res12, u7, m[5]);
    res13 = fma52hi(res13, u7, m[5]);
    fma52lo_mem(res13, res13, u7, m, SIMD_BYTES * 6);
    fma52hi_mem(res14, res14, u7, m, SIMD_BYTES * 6);
    res14 = fma52lo(res14, u7, m[7]);
    res15 = fma52hi(res15, u7, m[7]);
    fma52lo_mem(res15, res15, u7, m, SIMD_BYTES * 8);
    fma52hi_mem(res16, res16, u7, m, SIMD_BYTES * 8);
    res16 = fma52lo(res16, u7, m[9]);
    res17 = fma52hi(res17, u7, m[9]);
    ASM("jmp l8\nl8:\n");

    // Create u8
    fma52lo_mem(res8, res8, u8, m, SIMD_BYTES * 0);
    fma52hi_mem(res9, res9, u8, m, SIMD_BYTES * 0);
    res9 = fma52lo(res9, u8, m[1]);
    res10 = fma52hi(res10, u8, m[1]);
    res9 = add64(res9, srli64(res8, DIGIT_SIZE));
    U64 u9 = mul52lo(res9, k);
    fma52lo_mem(res10, res10, u8, m, SIMD_BYTES * 2);
    fma52hi_mem(res11, res11, u8, m, SIMD_BYTES * 2);
    res11 = fma52lo(res11, u8, m[3]);
    res12 = fma52hi(res12, u8, m[3]);
    fma52lo_mem(res12, res12, u8, m, SIMD_BYTES * 4);
    fma52hi_mem(res13, res13, u8, m, SIMD_BYTES * 4);
    res13 = fma52lo(res13, u8, m[5]);
    res14 = fma52hi(res14, u8, m[5]);
    fma52lo_mem(res14, res14, u8, m, SIMD_BYTES * 6);
    fma52hi_mem(res15, res15, u8, m, SIMD_BYTES * 6);
    res15 = fma52lo(res15, u8, m[7]);
    res16 = fma52hi(res16, u8, m[7]);
    fma52lo_mem(res16, res16, u8, m, SIMD_BYTES * 8);
    fma52hi_mem(res17, res17, u8, m, SIMD_BYTES * 8);
    res17 = fma52lo(res17, u8, m[9]);
    res18 = fma52hi(res18, u8, m[9]);

    // Create u9
    fma52lo_mem(res9, res9, u9, m, SIMD_BYTES * 0);
    fma52hi_mem(res10, res10, u9, m, SIMD_BYTES * 0);
    res10 = fma52lo(res10, u9, m[1]);
    res11 = fma52hi(res11, u9, m[1]);
    res10 = add64(res10, srli64(res9, DIGIT_SIZE));
    fma52lo_mem(res11, res11, u9, m, SIMD_BYTES * 2);
    fma52hi_mem(res12, res12, u9, m, SIMD_BYTES * 2);
    res12 = fma52lo(res12, u9, m[3]);
    res13 = fma52hi(res13, u9, m[3]);
    fma52lo_mem(res13, res13, u9, m, SIMD_BYTES * 4);
    fma52hi_mem(res14, res14, u9, m, SIMD_BYTES * 4);
    res14 = fma52lo(res14, u9, m[5]);
    res15 = fma52hi(res15, u9, m[5]);
    fma52lo_mem(res15, res15, u9, m, SIMD_BYTES * 6);
    fma52hi_mem(res16, res16, u9, m, SIMD_BYTES * 6);
    res16 = fma52lo(res16, u9, m[7]);
    res17 = fma52hi(res17, u9, m[7]);
    fma52lo_mem(res17, res17, u9, m, SIMD_BYTES * 8);
    fma52hi_mem(res18, res18, u9, m, SIMD_BYTES * 8);
    res18 = fma52lo(res18, u9, m[9]);
    res19 = fma52hi(res19, u9, m[9]);

    // Normalization
    r[0] = res10;
    res11 = add64(res11, srli64(res10, DIGIT_SIZE));
    r[1] = res11;
    res12 = add64(res12, srli64(res11, DIGIT_SIZE));
    r[2] = res12;
    res13 = add64(res13, srli64(res12, DIGIT_SIZE));
    r[3] = res13;
    res14 = add64(res14, srli64(res13, DIGIT_SIZE));
    r[4] = res14;
    res15 = add64(res15, srli64(res14, DIGIT_SIZE));
    r[5] = res15;
    res16 = add64(res16, srli64(res15, DIGIT_SIZE));
    r[6] = res16;
    res17 = add64(res17, srli64(res16, DIGIT_SIZE));
    r[7] = res17;
    res18 = add64(res18, srli64(res17, DIGIT_SIZE));
    r[8] = res18;
    res19 = add64(res19, srli64(res18, DIGIT_SIZE));
    r[9] = res19;
    a = (U64 *)out_mb;
  }
}

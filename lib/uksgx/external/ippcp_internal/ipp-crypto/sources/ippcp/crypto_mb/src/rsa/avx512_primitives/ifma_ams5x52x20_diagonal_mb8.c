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

void AMS5x52x20_diagonal_mb8(int64u *out_mb, const int64u *inpA_mb,
                             const int64u *inpM_mb, const int64u *k0_mb) {
  U64 res0, res1, res2, res3, res4, res5, res6, res7, res8, res9, res10, res11,
      res12, res13, res14, res15, res16, res17, res18, res19, res20, res21,
      res22, res23, res24, res25, res26, res27, res28, res29, res30, res31,
      res32, res33, res34, res35, res36, res37, res38, res39;
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
            res19 = res20 = res21 = res22 = res23 = res24 = res25 = res26 =
                res27 = res28 = res29 = res30 = res31 = res32 = res33 = res34 =
                    res35 = res36 = res37 = res38 = res39 = get_zero64();
    // Calculate full square
    res1 = fma52lo(res1, a[0], a[1]);     // Sum(1)
    res2 = fma52hi(res2, a[0], a[1]);     // Sum(1)
    res2 = fma52lo(res2, a[0], a[2]);     // Sum(2)
    res3 = fma52hi(res3, a[0], a[2]);     // Sum(2)
    res3 = fma52lo(res3, a[1], a[2]);     // Sum(3)
    res4 = fma52hi(res4, a[1], a[2]);     // Sum(3)
    res3 = fma52lo(res3, a[0], a[3]);     // Sum(3)
    res4 = fma52hi(res4, a[0], a[3]);     // Sum(3)
    res4 = fma52lo(res4, a[1], a[3]);     // Sum(4)
    res5 = fma52hi(res5, a[1], a[3]);     // Sum(4)
    res5 = fma52lo(res5, a[2], a[3]);     // Sum(5)
    res6 = fma52hi(res6, a[2], a[3]);     // Sum(5)
    res4 = fma52lo(res4, a[0], a[4]);     // Sum(4)
    res5 = fma52hi(res5, a[0], a[4]);     // Sum(4)
    res5 = fma52lo(res5, a[1], a[4]);     // Sum(5)
    res6 = fma52hi(res6, a[1], a[4]);     // Sum(5)
    res6 = fma52lo(res6, a[2], a[4]);     // Sum(6)
    res7 = fma52hi(res7, a[2], a[4]);     // Sum(6)
    res7 = fma52lo(res7, a[3], a[4]);     // Sum(7)
    res8 = fma52hi(res8, a[3], a[4]);     // Sum(7)
    res5 = fma52lo(res5, a[0], a[5]);     // Sum(5)
    res6 = fma52hi(res6, a[0], a[5]);     // Sum(5)
    res6 = fma52lo(res6, a[1], a[5]);     // Sum(6)
    res7 = fma52hi(res7, a[1], a[5]);     // Sum(6)
    res7 = fma52lo(res7, a[2], a[5]);     // Sum(7)
    res8 = fma52hi(res8, a[2], a[5]);     // Sum(7)
    res8 = fma52lo(res8, a[3], a[5]);     // Sum(8)
    res9 = fma52hi(res9, a[3], a[5]);     // Sum(8)
    res9 = fma52lo(res9, a[4], a[5]);     // Sum(9)
    res10 = fma52hi(res10, a[4], a[5]);   // Sum(9)
    res6 = fma52lo(res6, a[0], a[6]);     // Sum(6)
    res7 = fma52hi(res7, a[0], a[6]);     // Sum(6)
    res7 = fma52lo(res7, a[1], a[6]);     // Sum(7)
    res8 = fma52hi(res8, a[1], a[6]);     // Sum(7)
    res8 = fma52lo(res8, a[2], a[6]);     // Sum(8)
    res9 = fma52hi(res9, a[2], a[6]);     // Sum(8)
    res9 = fma52lo(res9, a[3], a[6]);     // Sum(9)
    res10 = fma52hi(res10, a[3], a[6]);   // Sum(9)
    res10 = fma52lo(res10, a[4], a[6]);   // Sum(10)
    res11 = fma52hi(res11, a[4], a[6]);   // Sum(10)
    res11 = fma52lo(res11, a[5], a[6]);   // Sum(11)
    res12 = fma52hi(res12, a[5], a[6]);   // Sum(11)
    res7 = fma52lo(res7, a[0], a[7]);     // Sum(7)
    res8 = fma52hi(res8, a[0], a[7]);     // Sum(7)
    res8 = fma52lo(res8, a[1], a[7]);     // Sum(8)
    res9 = fma52hi(res9, a[1], a[7]);     // Sum(8)
    res9 = fma52lo(res9, a[2], a[7]);     // Sum(9)
    res10 = fma52hi(res10, a[2], a[7]);   // Sum(9)
    res10 = fma52lo(res10, a[3], a[7]);   // Sum(10)
    res11 = fma52hi(res11, a[3], a[7]);   // Sum(10)
    res11 = fma52lo(res11, a[4], a[7]);   // Sum(11)
    res12 = fma52hi(res12, a[4], a[7]);   // Sum(11)
    res8 = fma52lo(res8, a[0], a[8]);     // Sum(8)
    res9 = fma52hi(res9, a[0], a[8]);     // Sum(8)
    res9 = fma52lo(res9, a[1], a[8]);     // Sum(9)
    res10 = fma52hi(res10, a[1], a[8]);   // Sum(9)
    res10 = fma52lo(res10, a[2], a[8]);   // Sum(10)
    res11 = fma52hi(res11, a[2], a[8]);   // Sum(10)
    res11 = fma52lo(res11, a[3], a[8]);   // Sum(11)
    res12 = fma52hi(res12, a[3], a[8]);   // Sum(11)
    res9 = fma52lo(res9, a[0], a[9]);     // Sum(9)
    res10 = fma52hi(res10, a[0], a[9]);   // Sum(9)
    res10 = fma52lo(res10, a[1], a[9]);   // Sum(10)
    res11 = fma52hi(res11, a[1], a[9]);   // Sum(10)
    res11 = fma52lo(res11, a[2], a[9]);   // Sum(11)
    res12 = fma52hi(res12, a[2], a[9]);   // Sum(11)
    res10 = fma52lo(res10, a[0], a[10]);  // Sum(10)
    res11 = fma52hi(res11, a[0], a[10]);  // Sum(10)
    res11 = fma52lo(res11, a[1], a[10]);  // Sum(11)
    res12 = fma52hi(res12, a[1], a[10]);  // Sum(11)
    res11 = fma52lo(res11, a[0], a[11]);  // Sum(11)
    res12 = fma52hi(res12, a[0], a[11]);  // Sum(11)
    res0 = add64(res0, res0);             // Double(0)
    res1 = add64(res1, res1);             // Double(1)
    res2 = add64(res2, res2);             // Double(2)
    res3 = add64(res3, res3);             // Double(3)
    res4 = add64(res4, res4);             // Double(4)
    res5 = add64(res5, res5);             // Double(5)
    res6 = add64(res6, res6);             // Double(6)
    res7 = add64(res7, res7);             // Double(7)
    res8 = add64(res8, res8);             // Double(8)
    res9 = add64(res9, res9);             // Double(9)
    res10 = add64(res10, res10);          // Double(10)
    res11 = add64(res11, res11);          // Double(11)
    res0 = fma52lo(res0, a[0], a[0]);     // Add sqr(0)
    res1 = fma52hi(res1, a[0], a[0]);     // Add sqr(0)
    res2 = fma52lo(res2, a[1], a[1]);     // Add sqr(2)
    res3 = fma52hi(res3, a[1], a[1]);     // Add sqr(2)
    res4 = fma52lo(res4, a[2], a[2]);     // Add sqr(4)
    res5 = fma52hi(res5, a[2], a[2]);     // Add sqr(4)
    res6 = fma52lo(res6, a[3], a[3]);     // Add sqr(6)
    res7 = fma52hi(res7, a[3], a[3]);     // Add sqr(6)
    res8 = fma52lo(res8, a[4], a[4]);     // Add sqr(8)
    res9 = fma52hi(res9, a[4], a[4]);     // Add sqr(8)
    res10 = fma52lo(res10, a[5], a[5]);   // Add sqr(10)
    res11 = fma52hi(res11, a[5], a[5]);   // Add sqr(10)
    res12 = fma52lo(res12, a[5], a[7]);   // Sum(12)
    res13 = fma52hi(res13, a[5], a[7]);   // Sum(12)
    res13 = fma52lo(res13, a[6], a[7]);   // Sum(13)
    res14 = fma52hi(res14, a[6], a[7]);   // Sum(13)
    res12 = fma52lo(res12, a[4], a[8]);   // Sum(12)
    res13 = fma52hi(res13, a[4], a[8]);   // Sum(12)
    res13 = fma52lo(res13, a[5], a[8]);   // Sum(13)
    res14 = fma52hi(res14, a[5], a[8]);   // Sum(13)
    res14 = fma52lo(res14, a[6], a[8]);   // Sum(14)
    res15 = fma52hi(res15, a[6], a[8]);   // Sum(14)
    res15 = fma52lo(res15, a[7], a[8]);   // Sum(15)
    res16 = fma52hi(res16, a[7], a[8]);   // Sum(15)
    res12 = fma52lo(res12, a[3], a[9]);   // Sum(12)
    res13 = fma52hi(res13, a[3], a[9]);   // Sum(12)
    res13 = fma52lo(res13, a[4], a[9]);   // Sum(13)
    res14 = fma52hi(res14, a[4], a[9]);   // Sum(13)
    res14 = fma52lo(res14, a[5], a[9]);   // Sum(14)
    res15 = fma52hi(res15, a[5], a[9]);   // Sum(14)
    res15 = fma52lo(res15, a[6], a[9]);   // Sum(15)
    res16 = fma52hi(res16, a[6], a[9]);   // Sum(15)
    res16 = fma52lo(res16, a[7], a[9]);   // Sum(16)
    res17 = fma52hi(res17, a[7], a[9]);   // Sum(16)
    res17 = fma52lo(res17, a[8], a[9]);   // Sum(17)
    res18 = fma52hi(res18, a[8], a[9]);   // Sum(17)
    res12 = fma52lo(res12, a[2], a[10]);  // Sum(12)
    res13 = fma52hi(res13, a[2], a[10]);  // Sum(12)
    res13 = fma52lo(res13, a[3], a[10]);  // Sum(13)
    res14 = fma52hi(res14, a[3], a[10]);  // Sum(13)
    res14 = fma52lo(res14, a[4], a[10]);  // Sum(14)
    res15 = fma52hi(res15, a[4], a[10]);  // Sum(14)
    res15 = fma52lo(res15, a[5], a[10]);  // Sum(15)
    res16 = fma52hi(res16, a[5], a[10]);  // Sum(15)
    res16 = fma52lo(res16, a[6], a[10]);  // Sum(16)
    res17 = fma52hi(res17, a[6], a[10]);  // Sum(16)
    res17 = fma52lo(res17, a[7], a[10]);  // Sum(17)
    res18 = fma52hi(res18, a[7], a[10]);  // Sum(17)
    res18 = fma52lo(res18, a[8], a[10]);  // Sum(18)
    res19 = fma52hi(res19, a[8], a[10]);  // Sum(18)
    res19 = fma52lo(res19, a[9], a[10]);  // Sum(19)
    res20 = fma52hi(res20, a[9], a[10]);  // Sum(19)
    res12 = fma52lo(res12, a[1], a[11]);  // Sum(12)
    res13 = fma52hi(res13, a[1], a[11]);  // Sum(12)
    res13 = fma52lo(res13, a[2], a[11]);  // Sum(13)
    res14 = fma52hi(res14, a[2], a[11]);  // Sum(13)
    res14 = fma52lo(res14, a[3], a[11]);  // Sum(14)
    res15 = fma52hi(res15, a[3], a[11]);  // Sum(14)
    res15 = fma52lo(res15, a[4], a[11]);  // Sum(15)
    res16 = fma52hi(res16, a[4], a[11]);  // Sum(15)
    res16 = fma52lo(res16, a[5], a[11]);  // Sum(16)
    res17 = fma52hi(res17, a[5], a[11]);  // Sum(16)
    res17 = fma52lo(res17, a[6], a[11]);  // Sum(17)
    res18 = fma52hi(res18, a[6], a[11]);  // Sum(17)
    res18 = fma52lo(res18, a[7], a[11]);  // Sum(18)
    res19 = fma52hi(res19, a[7], a[11]);  // Sum(18)
    res19 = fma52lo(res19, a[8], a[11]);  // Sum(19)
    res20 = fma52hi(res20, a[8], a[11]);  // Sum(19)
    res20 = fma52lo(res20, a[9], a[11]);  // Sum(20)
    res21 = fma52hi(res21, a[9], a[11]);  // Sum(20)
    res21 = fma52lo(res21, a[10], a[11]); // Sum(21)
    res22 = fma52hi(res22, a[10], a[11]); // Sum(21)
    res12 = fma52lo(res12, a[0], a[12]);  // Sum(12)
    res13 = fma52hi(res13, a[0], a[12]);  // Sum(12)
    res13 = fma52lo(res13, a[1], a[12]);  // Sum(13)
    res14 = fma52hi(res14, a[1], a[12]);  // Sum(13)
    res14 = fma52lo(res14, a[2], a[12]);  // Sum(14)
    res15 = fma52hi(res15, a[2], a[12]);  // Sum(14)
    res15 = fma52lo(res15, a[3], a[12]);  // Sum(15)
    res16 = fma52hi(res16, a[3], a[12]);  // Sum(15)
    res16 = fma52lo(res16, a[4], a[12]);  // Sum(16)
    res17 = fma52hi(res17, a[4], a[12]);  // Sum(16)
    res17 = fma52lo(res17, a[5], a[12]);  // Sum(17)
    res18 = fma52hi(res18, a[5], a[12]);  // Sum(17)
    res18 = fma52lo(res18, a[6], a[12]);  // Sum(18)
    res19 = fma52hi(res19, a[6], a[12]);  // Sum(18)
    res19 = fma52lo(res19, a[7], a[12]);  // Sum(19)
    res20 = fma52hi(res20, a[7], a[12]);  // Sum(19)
    res20 = fma52lo(res20, a[8], a[12]);  // Sum(20)
    res21 = fma52hi(res21, a[8], a[12]);  // Sum(20)
    res21 = fma52lo(res21, a[9], a[12]);  // Sum(21)
    res22 = fma52hi(res22, a[9], a[12]);  // Sum(21)
    res22 = fma52lo(res22, a[10], a[12]); // Sum(22)
    res23 = fma52hi(res23, a[10], a[12]); // Sum(22)
    res23 = fma52lo(res23, a[11], a[12]); // Sum(23)
    res24 = fma52hi(res24, a[11], a[12]); // Sum(23)
    res13 = fma52lo(res13, a[0], a[13]);  // Sum(13)
    res14 = fma52hi(res14, a[0], a[13]);  // Sum(13)
    res14 = fma52lo(res14, a[1], a[13]);  // Sum(14)
    res15 = fma52hi(res15, a[1], a[13]);  // Sum(14)
    res15 = fma52lo(res15, a[2], a[13]);  // Sum(15)
    res16 = fma52hi(res16, a[2], a[13]);  // Sum(15)
    res16 = fma52lo(res16, a[3], a[13]);  // Sum(16)
    res17 = fma52hi(res17, a[3], a[13]);  // Sum(16)
    res17 = fma52lo(res17, a[4], a[13]);  // Sum(17)
    res18 = fma52hi(res18, a[4], a[13]);  // Sum(17)
    res18 = fma52lo(res18, a[5], a[13]);  // Sum(18)
    res19 = fma52hi(res19, a[5], a[13]);  // Sum(18)
    res19 = fma52lo(res19, a[6], a[13]);  // Sum(19)
    res20 = fma52hi(res20, a[6], a[13]);  // Sum(19)
    res20 = fma52lo(res20, a[7], a[13]);  // Sum(20)
    res21 = fma52hi(res21, a[7], a[13]);  // Sum(20)
    res21 = fma52lo(res21, a[8], a[13]);  // Sum(21)
    res22 = fma52hi(res22, a[8], a[13]);  // Sum(21)
    res22 = fma52lo(res22, a[9], a[13]);  // Sum(22)
    res23 = fma52hi(res23, a[9], a[13]);  // Sum(22)
    res23 = fma52lo(res23, a[10], a[13]); // Sum(23)
    res24 = fma52hi(res24, a[10], a[13]); // Sum(23)
    res14 = fma52lo(res14, a[0], a[14]);  // Sum(14)
    res15 = fma52hi(res15, a[0], a[14]);  // Sum(14)
    res15 = fma52lo(res15, a[1], a[14]);  // Sum(15)
    res16 = fma52hi(res16, a[1], a[14]);  // Sum(15)
    res16 = fma52lo(res16, a[2], a[14]);  // Sum(16)
    res17 = fma52hi(res17, a[2], a[14]);  // Sum(16)
    res17 = fma52lo(res17, a[3], a[14]);  // Sum(17)
    res18 = fma52hi(res18, a[3], a[14]);  // Sum(17)
    res18 = fma52lo(res18, a[4], a[14]);  // Sum(18)
    res19 = fma52hi(res19, a[4], a[14]);  // Sum(18)
    res19 = fma52lo(res19, a[5], a[14]);  // Sum(19)
    res20 = fma52hi(res20, a[5], a[14]);  // Sum(19)
    res20 = fma52lo(res20, a[6], a[14]);  // Sum(20)
    res21 = fma52hi(res21, a[6], a[14]);  // Sum(20)
    res21 = fma52lo(res21, a[7], a[14]);  // Sum(21)
    res22 = fma52hi(res22, a[7], a[14]);  // Sum(21)
    res22 = fma52lo(res22, a[8], a[14]);  // Sum(22)
    res23 = fma52hi(res23, a[8], a[14]);  // Sum(22)
    res23 = fma52lo(res23, a[9], a[14]);  // Sum(23)
    res24 = fma52hi(res24, a[9], a[14]);  // Sum(23)
    res15 = fma52lo(res15, a[0], a[15]);  // Sum(15)
    res16 = fma52hi(res16, a[0], a[15]);  // Sum(15)
    res16 = fma52lo(res16, a[1], a[15]);  // Sum(16)
    res17 = fma52hi(res17, a[1], a[15]);  // Sum(16)
    res17 = fma52lo(res17, a[2], a[15]);  // Sum(17)
    res18 = fma52hi(res18, a[2], a[15]);  // Sum(17)
    res18 = fma52lo(res18, a[3], a[15]);  // Sum(18)
    res19 = fma52hi(res19, a[3], a[15]);  // Sum(18)
    res19 = fma52lo(res19, a[4], a[15]);  // Sum(19)
    res20 = fma52hi(res20, a[4], a[15]);  // Sum(19)
    res20 = fma52lo(res20, a[5], a[15]);  // Sum(20)
    res21 = fma52hi(res21, a[5], a[15]);  // Sum(20)
    res21 = fma52lo(res21, a[6], a[15]);  // Sum(21)
    res22 = fma52hi(res22, a[6], a[15]);  // Sum(21)
    res22 = fma52lo(res22, a[7], a[15]);  // Sum(22)
    res23 = fma52hi(res23, a[7], a[15]);  // Sum(22)
    res23 = fma52lo(res23, a[8], a[15]);  // Sum(23)
    res24 = fma52hi(res24, a[8], a[15]);  // Sum(23)
    res16 = fma52lo(res16, a[0], a[16]);  // Sum(16)
    res17 = fma52hi(res17, a[0], a[16]);  // Sum(16)
    res17 = fma52lo(res17, a[1], a[16]);  // Sum(17)
    res18 = fma52hi(res18, a[1], a[16]);  // Sum(17)
    res18 = fma52lo(res18, a[2], a[16]);  // Sum(18)
    res19 = fma52hi(res19, a[2], a[16]);  // Sum(18)
    res19 = fma52lo(res19, a[3], a[16]);  // Sum(19)
    res20 = fma52hi(res20, a[3], a[16]);  // Sum(19)
    res20 = fma52lo(res20, a[4], a[16]);  // Sum(20)
    res21 = fma52hi(res21, a[4], a[16]);  // Sum(20)
    res21 = fma52lo(res21, a[5], a[16]);  // Sum(21)
    res22 = fma52hi(res22, a[5], a[16]);  // Sum(21)
    res22 = fma52lo(res22, a[6], a[16]);  // Sum(22)
    res23 = fma52hi(res23, a[6], a[16]);  // Sum(22)
    res23 = fma52lo(res23, a[7], a[16]);  // Sum(23)
    res24 = fma52hi(res24, a[7], a[16]);  // Sum(23)
    res17 = fma52lo(res17, a[0], a[17]);  // Sum(17)
    res18 = fma52hi(res18, a[0], a[17]);  // Sum(17)
    res18 = fma52lo(res18, a[1], a[17]);  // Sum(18)
    res19 = fma52hi(res19, a[1], a[17]);  // Sum(18)
    res19 = fma52lo(res19, a[2], a[17]);  // Sum(19)
    res20 = fma52hi(res20, a[2], a[17]);  // Sum(19)
    res20 = fma52lo(res20, a[3], a[17]);  // Sum(20)
    res21 = fma52hi(res21, a[3], a[17]);  // Sum(20)
    res21 = fma52lo(res21, a[4], a[17]);  // Sum(21)
    res22 = fma52hi(res22, a[4], a[17]);  // Sum(21)
    res22 = fma52lo(res22, a[5], a[17]);  // Sum(22)
    res23 = fma52hi(res23, a[5], a[17]);  // Sum(22)
    res23 = fma52lo(res23, a[6], a[17]);  // Sum(23)
    res24 = fma52hi(res24, a[6], a[17]);  // Sum(23)
    res18 = fma52lo(res18, a[0], a[18]);  // Sum(18)
    res19 = fma52hi(res19, a[0], a[18]);  // Sum(18)
    res19 = fma52lo(res19, a[1], a[18]);  // Sum(19)
    res20 = fma52hi(res20, a[1], a[18]);  // Sum(19)
    res20 = fma52lo(res20, a[2], a[18]);  // Sum(20)
    res21 = fma52hi(res21, a[2], a[18]);  // Sum(20)
    res21 = fma52lo(res21, a[3], a[18]);  // Sum(21)
    res22 = fma52hi(res22, a[3], a[18]);  // Sum(21)
    res22 = fma52lo(res22, a[4], a[18]);  // Sum(22)
    res23 = fma52hi(res23, a[4], a[18]);  // Sum(22)
    res23 = fma52lo(res23, a[5], a[18]);  // Sum(23)
    res24 = fma52hi(res24, a[5], a[18]);  // Sum(23)
    res19 = fma52lo(res19, a[0], a[19]);  // Sum(19)
    res20 = fma52hi(res20, a[0], a[19]);  // Sum(19)
    res20 = fma52lo(res20, a[1], a[19]);  // Sum(20)
    res21 = fma52hi(res21, a[1], a[19]);  // Sum(20)
    res21 = fma52lo(res21, a[2], a[19]);  // Sum(21)
    res22 = fma52hi(res22, a[2], a[19]);  // Sum(21)
    res22 = fma52lo(res22, a[3], a[19]);  // Sum(22)
    res23 = fma52hi(res23, a[3], a[19]);  // Sum(22)
    res23 = fma52lo(res23, a[4], a[19]);  // Sum(23)
    res24 = fma52hi(res24, a[4], a[19]);  // Sum(23)
    res12 = add64(res12, res12);          // Double(12)
    res13 = add64(res13, res13);          // Double(13)
    res14 = add64(res14, res14);          // Double(14)
    res15 = add64(res15, res15);          // Double(15)
    res16 = add64(res16, res16);          // Double(16)
    res17 = add64(res17, res17);          // Double(17)
    res18 = add64(res18, res18);          // Double(18)
    res19 = add64(res19, res19);          // Double(19)
    res20 = add64(res20, res20);          // Double(20)
    res21 = add64(res21, res21);          // Double(21)
    res22 = add64(res22, res22);          // Double(22)
    res23 = add64(res23, res23);          // Double(23)
    res12 = fma52lo(res12, a[6], a[6]);   // Add sqr(12)
    res13 = fma52hi(res13, a[6], a[6]);   // Add sqr(12)
    res14 = fma52lo(res14, a[7], a[7]);   // Add sqr(14)
    res15 = fma52hi(res15, a[7], a[7]);   // Add sqr(14)
    res16 = fma52lo(res16, a[8], a[8]);   // Add sqr(16)
    res17 = fma52hi(res17, a[8], a[8]);   // Add sqr(16)
    res18 = fma52lo(res18, a[9], a[9]);   // Add sqr(18)
    res19 = fma52hi(res19, a[9], a[9]);   // Add sqr(18)
    res20 = fma52lo(res20, a[10], a[10]); // Add sqr(20)
    res21 = fma52hi(res21, a[10], a[10]); // Add sqr(20)
    res22 = fma52lo(res22, a[11], a[11]); // Add sqr(22)
    res23 = fma52hi(res23, a[11], a[11]); // Add sqr(22)
    res24 = fma52lo(res24, a[11], a[13]); // Sum(24)
    res25 = fma52hi(res25, a[11], a[13]); // Sum(24)
    res25 = fma52lo(res25, a[12], a[13]); // Sum(25)
    res26 = fma52hi(res26, a[12], a[13]); // Sum(25)
    res24 = fma52lo(res24, a[10], a[14]); // Sum(24)
    res25 = fma52hi(res25, a[10], a[14]); // Sum(24)
    res25 = fma52lo(res25, a[11], a[14]); // Sum(25)
    res26 = fma52hi(res26, a[11], a[14]); // Sum(25)
    res26 = fma52lo(res26, a[12], a[14]); // Sum(26)
    res27 = fma52hi(res27, a[12], a[14]); // Sum(26)
    res27 = fma52lo(res27, a[13], a[14]); // Sum(27)
    res28 = fma52hi(res28, a[13], a[14]); // Sum(27)
    res24 = fma52lo(res24, a[9], a[15]);  // Sum(24)
    res25 = fma52hi(res25, a[9], a[15]);  // Sum(24)
    res25 = fma52lo(res25, a[10], a[15]); // Sum(25)
    res26 = fma52hi(res26, a[10], a[15]); // Sum(25)
    res26 = fma52lo(res26, a[11], a[15]); // Sum(26)
    res27 = fma52hi(res27, a[11], a[15]); // Sum(26)
    res27 = fma52lo(res27, a[12], a[15]); // Sum(27)
    res28 = fma52hi(res28, a[12], a[15]); // Sum(27)
    res28 = fma52lo(res28, a[13], a[15]); // Sum(28)
    res29 = fma52hi(res29, a[13], a[15]); // Sum(28)
    res29 = fma52lo(res29, a[14], a[15]); // Sum(29)
    res30 = fma52hi(res30, a[14], a[15]); // Sum(29)
    res24 = fma52lo(res24, a[8], a[16]);  // Sum(24)
    res25 = fma52hi(res25, a[8], a[16]);  // Sum(24)
    res25 = fma52lo(res25, a[9], a[16]);  // Sum(25)
    res26 = fma52hi(res26, a[9], a[16]);  // Sum(25)
    res26 = fma52lo(res26, a[10], a[16]); // Sum(26)
    res27 = fma52hi(res27, a[10], a[16]); // Sum(26)
    res27 = fma52lo(res27, a[11], a[16]); // Sum(27)
    res28 = fma52hi(res28, a[11], a[16]); // Sum(27)
    res28 = fma52lo(res28, a[12], a[16]); // Sum(28)
    res29 = fma52hi(res29, a[12], a[16]); // Sum(28)
    res29 = fma52lo(res29, a[13], a[16]); // Sum(29)
    res30 = fma52hi(res30, a[13], a[16]); // Sum(29)
    res30 = fma52lo(res30, a[14], a[16]); // Sum(30)
    res31 = fma52hi(res31, a[14], a[16]); // Sum(30)
    res31 = fma52lo(res31, a[15], a[16]); // Sum(31)
    res32 = fma52hi(res32, a[15], a[16]); // Sum(31)
    res24 = fma52lo(res24, a[7], a[17]);  // Sum(24)
    res25 = fma52hi(res25, a[7], a[17]);  // Sum(24)
    res25 = fma52lo(res25, a[8], a[17]);  // Sum(25)
    res26 = fma52hi(res26, a[8], a[17]);  // Sum(25)
    res26 = fma52lo(res26, a[9], a[17]);  // Sum(26)
    res27 = fma52hi(res27, a[9], a[17]);  // Sum(26)
    res27 = fma52lo(res27, a[10], a[17]); // Sum(27)
    res28 = fma52hi(res28, a[10], a[17]); // Sum(27)
    res28 = fma52lo(res28, a[11], a[17]); // Sum(28)
    res29 = fma52hi(res29, a[11], a[17]); // Sum(28)
    res29 = fma52lo(res29, a[12], a[17]); // Sum(29)
    res30 = fma52hi(res30, a[12], a[17]); // Sum(29)
    res30 = fma52lo(res30, a[13], a[17]); // Sum(30)
    res31 = fma52hi(res31, a[13], a[17]); // Sum(30)
    res31 = fma52lo(res31, a[14], a[17]); // Sum(31)
    res32 = fma52hi(res32, a[14], a[17]); // Sum(31)
    res32 = fma52lo(res32, a[15], a[17]); // Sum(32)
    res33 = fma52hi(res33, a[15], a[17]); // Sum(32)
    res33 = fma52lo(res33, a[16], a[17]); // Sum(33)
    res34 = fma52hi(res34, a[16], a[17]); // Sum(33)
    res24 = fma52lo(res24, a[6], a[18]);  // Sum(24)
    res25 = fma52hi(res25, a[6], a[18]);  // Sum(24)
    res25 = fma52lo(res25, a[7], a[18]);  // Sum(25)
    res26 = fma52hi(res26, a[7], a[18]);  // Sum(25)
    res26 = fma52lo(res26, a[8], a[18]);  // Sum(26)
    res27 = fma52hi(res27, a[8], a[18]);  // Sum(26)
    res27 = fma52lo(res27, a[9], a[18]);  // Sum(27)
    res28 = fma52hi(res28, a[9], a[18]);  // Sum(27)
    res28 = fma52lo(res28, a[10], a[18]); // Sum(28)
    res29 = fma52hi(res29, a[10], a[18]); // Sum(28)
    res29 = fma52lo(res29, a[11], a[18]); // Sum(29)
    res30 = fma52hi(res30, a[11], a[18]); // Sum(29)
    res30 = fma52lo(res30, a[12], a[18]); // Sum(30)
    res31 = fma52hi(res31, a[12], a[18]); // Sum(30)
    res31 = fma52lo(res31, a[13], a[18]); // Sum(31)
    res32 = fma52hi(res32, a[13], a[18]); // Sum(31)
    res32 = fma52lo(res32, a[14], a[18]); // Sum(32)
    res33 = fma52hi(res33, a[14], a[18]); // Sum(32)
    res33 = fma52lo(res33, a[15], a[18]); // Sum(33)
    res34 = fma52hi(res34, a[15], a[18]); // Sum(33)
    res34 = fma52lo(res34, a[16], a[18]); // Sum(34)
    res35 = fma52hi(res35, a[16], a[18]); // Sum(34)
    res35 = fma52lo(res35, a[17], a[18]); // Sum(35)
    res36 = fma52hi(res36, a[17], a[18]); // Sum(35)
    res24 = fma52lo(res24, a[5], a[19]);  // Sum(24)
    res25 = fma52hi(res25, a[5], a[19]);  // Sum(24)
    res25 = fma52lo(res25, a[6], a[19]);  // Sum(25)
    res26 = fma52hi(res26, a[6], a[19]);  // Sum(25)
    res26 = fma52lo(res26, a[7], a[19]);  // Sum(26)
    res27 = fma52hi(res27, a[7], a[19]);  // Sum(26)
    res27 = fma52lo(res27, a[8], a[19]);  // Sum(27)
    res28 = fma52hi(res28, a[8], a[19]);  // Sum(27)
    res28 = fma52lo(res28, a[9], a[19]);  // Sum(28)
    res29 = fma52hi(res29, a[9], a[19]);  // Sum(28)
    res29 = fma52lo(res29, a[10], a[19]); // Sum(29)
    res30 = fma52hi(res30, a[10], a[19]); // Sum(29)
    res30 = fma52lo(res30, a[11], a[19]); // Sum(30)
    res31 = fma52hi(res31, a[11], a[19]); // Sum(30)
    res31 = fma52lo(res31, a[12], a[19]); // Sum(31)
    res32 = fma52hi(res32, a[12], a[19]); // Sum(31)
    res32 = fma52lo(res32, a[13], a[19]); // Sum(32)
    res33 = fma52hi(res33, a[13], a[19]); // Sum(32)
    res33 = fma52lo(res33, a[14], a[19]); // Sum(33)
    res34 = fma52hi(res34, a[14], a[19]); // Sum(33)
    res34 = fma52lo(res34, a[15], a[19]); // Sum(34)
    res35 = fma52hi(res35, a[15], a[19]); // Sum(34)
    res35 = fma52lo(res35, a[16], a[19]); // Sum(35)
    res36 = fma52hi(res36, a[16], a[19]); // Sum(35)
    res24 = add64(res24, res24);          // Double(24)
    res25 = add64(res25, res25);          // Double(25)
    res26 = add64(res26, res26);          // Double(26)
    res27 = add64(res27, res27);          // Double(27)
    res28 = add64(res28, res28);          // Double(28)
    res29 = add64(res29, res29);          // Double(29)
    res30 = add64(res30, res30);          // Double(30)
    res31 = add64(res31, res31);          // Double(31)
    res32 = add64(res32, res32);          // Double(32)
    res33 = add64(res33, res33);          // Double(33)
    res34 = add64(res34, res34);          // Double(34)
    res35 = add64(res35, res35);          // Double(35)
    res24 = fma52lo(res24, a[12], a[12]); // Add sqr(24)
    res25 = fma52hi(res25, a[12], a[12]); // Add sqr(24)
    res26 = fma52lo(res26, a[13], a[13]); // Add sqr(26)
    res27 = fma52hi(res27, a[13], a[13]); // Add sqr(26)
    res28 = fma52lo(res28, a[14], a[14]); // Add sqr(28)
    res29 = fma52hi(res29, a[14], a[14]); // Add sqr(28)
    res30 = fma52lo(res30, a[15], a[15]); // Add sqr(30)
    res31 = fma52hi(res31, a[15], a[15]); // Add sqr(30)
    res32 = fma52lo(res32, a[16], a[16]); // Add sqr(32)
    res33 = fma52hi(res33, a[16], a[16]); // Add sqr(32)
    res34 = fma52lo(res34, a[17], a[17]); // Add sqr(34)
    res35 = fma52hi(res35, a[17], a[17]); // Add sqr(34)
    res36 = fma52lo(res36, a[17], a[19]); // Sum(36)
    res37 = fma52hi(res37, a[17], a[19]); // Sum(36)
    res37 = fma52lo(res37, a[18], a[19]); // Sum(37)
    res38 = fma52hi(res38, a[18], a[19]); // Sum(37)
    res36 = add64(res36, res36);          // Double(36)
    res37 = add64(res37, res37);          // Double(37)
    res38 = add64(res38, res38);          // Double(38)
    res36 = fma52lo(res36, a[18], a[18]); // Add sqr(36)
    res37 = fma52hi(res37, a[18], a[18]); // Add sqr(36)
    res38 = fma52lo(res38, a[19], a[19]); // Add sqr(38)
    res39 = fma52hi(res39, a[19], a[19]); // Add sqr(38)

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
    fma52lo_mem(res10, res10, u0, m, SIMD_BYTES * 10);
    fma52hi_mem(res11, res11, u0, m, SIMD_BYTES * 10);
    res11 = fma52lo(res11, u0, m[11]);
    res12 = fma52hi(res12, u0, m[11]);
    fma52lo_mem(res12, res12, u0, m, SIMD_BYTES * 12);
    fma52hi_mem(res13, res13, u0, m, SIMD_BYTES * 12);
    res13 = fma52lo(res13, u0, m[13]);
    res14 = fma52hi(res14, u0, m[13]);
    fma52lo_mem(res14, res14, u0, m, SIMD_BYTES * 14);
    fma52hi_mem(res15, res15, u0, m, SIMD_BYTES * 14);
    res15 = fma52lo(res15, u0, m[15]);
    res16 = fma52hi(res16, u0, m[15]);
    fma52lo_mem(res16, res16, u0, m, SIMD_BYTES * 16);
    fma52hi_mem(res17, res17, u0, m, SIMD_BYTES * 16);
    res17 = fma52lo(res17, u0, m[17]);
    res18 = fma52hi(res18, u0, m[17]);
    fma52lo_mem(res18, res18, u0, m, SIMD_BYTES * 18);
    fma52hi_mem(res19, res19, u0, m, SIMD_BYTES * 18);
    res19 = fma52lo(res19, u0, m[19]);
    res20 = fma52hi(res20, u0, m[19]);

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
    fma52lo_mem(res11, res11, u1, m, SIMD_BYTES * 10);
    fma52hi_mem(res12, res12, u1, m, SIMD_BYTES * 10);
    res12 = fma52lo(res12, u1, m[11]);
    res13 = fma52hi(res13, u1, m[11]);
    fma52lo_mem(res13, res13, u1, m, SIMD_BYTES * 12);
    fma52hi_mem(res14, res14, u1, m, SIMD_BYTES * 12);
    res14 = fma52lo(res14, u1, m[13]);
    res15 = fma52hi(res15, u1, m[13]);
    fma52lo_mem(res15, res15, u1, m, SIMD_BYTES * 14);
    fma52hi_mem(res16, res16, u1, m, SIMD_BYTES * 14);
    res16 = fma52lo(res16, u1, m[15]);
    res17 = fma52hi(res17, u1, m[15]);
    fma52lo_mem(res17, res17, u1, m, SIMD_BYTES * 16);
    fma52hi_mem(res18, res18, u1, m, SIMD_BYTES * 16);
    res18 = fma52lo(res18, u1, m[17]);
    res19 = fma52hi(res19, u1, m[17]);
    fma52lo_mem(res19, res19, u1, m, SIMD_BYTES * 18);
    fma52hi_mem(res20, res20, u1, m, SIMD_BYTES * 18);
    res20 = fma52lo(res20, u1, m[19]);
    res21 = fma52hi(res21, u1, m[19]);
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
    fma52lo_mem(res12, res12, u2, m, SIMD_BYTES * 10);
    fma52hi_mem(res13, res13, u2, m, SIMD_BYTES * 10);
    res13 = fma52lo(res13, u2, m[11]);
    res14 = fma52hi(res14, u2, m[11]);
    fma52lo_mem(res14, res14, u2, m, SIMD_BYTES * 12);
    fma52hi_mem(res15, res15, u2, m, SIMD_BYTES * 12);
    res15 = fma52lo(res15, u2, m[13]);
    res16 = fma52hi(res16, u2, m[13]);
    fma52lo_mem(res16, res16, u2, m, SIMD_BYTES * 14);
    fma52hi_mem(res17, res17, u2, m, SIMD_BYTES * 14);
    res17 = fma52lo(res17, u2, m[15]);
    res18 = fma52hi(res18, u2, m[15]);
    fma52lo_mem(res18, res18, u2, m, SIMD_BYTES * 16);
    fma52hi_mem(res19, res19, u2, m, SIMD_BYTES * 16);
    res19 = fma52lo(res19, u2, m[17]);
    res20 = fma52hi(res20, u2, m[17]);
    fma52lo_mem(res20, res20, u2, m, SIMD_BYTES * 18);
    fma52hi_mem(res21, res21, u2, m, SIMD_BYTES * 18);
    res21 = fma52lo(res21, u2, m[19]);
    res22 = fma52hi(res22, u2, m[19]);

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
    fma52lo_mem(res13, res13, u3, m, SIMD_BYTES * 10);
    fma52hi_mem(res14, res14, u3, m, SIMD_BYTES * 10);
    res14 = fma52lo(res14, u3, m[11]);
    res15 = fma52hi(res15, u3, m[11]);
    fma52lo_mem(res15, res15, u3, m, SIMD_BYTES * 12);
    fma52hi_mem(res16, res16, u3, m, SIMD_BYTES * 12);
    res16 = fma52lo(res16, u3, m[13]);
    res17 = fma52hi(res17, u3, m[13]);
    fma52lo_mem(res17, res17, u3, m, SIMD_BYTES * 14);
    fma52hi_mem(res18, res18, u3, m, SIMD_BYTES * 14);
    res18 = fma52lo(res18, u3, m[15]);
    res19 = fma52hi(res19, u3, m[15]);
    fma52lo_mem(res19, res19, u3, m, SIMD_BYTES * 16);
    fma52hi_mem(res20, res20, u3, m, SIMD_BYTES * 16);
    res20 = fma52lo(res20, u3, m[17]);
    res21 = fma52hi(res21, u3, m[17]);
    fma52lo_mem(res21, res21, u3, m, SIMD_BYTES * 18);
    fma52hi_mem(res22, res22, u3, m, SIMD_BYTES * 18);
    res22 = fma52lo(res22, u3, m[19]);
    res23 = fma52hi(res23, u3, m[19]);
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
    fma52lo_mem(res14, res14, u4, m, SIMD_BYTES * 10);
    fma52hi_mem(res15, res15, u4, m, SIMD_BYTES * 10);
    res15 = fma52lo(res15, u4, m[11]);
    res16 = fma52hi(res16, u4, m[11]);
    fma52lo_mem(res16, res16, u4, m, SIMD_BYTES * 12);
    fma52hi_mem(res17, res17, u4, m, SIMD_BYTES * 12);
    res17 = fma52lo(res17, u4, m[13]);
    res18 = fma52hi(res18, u4, m[13]);
    fma52lo_mem(res18, res18, u4, m, SIMD_BYTES * 14);
    fma52hi_mem(res19, res19, u4, m, SIMD_BYTES * 14);
    res19 = fma52lo(res19, u4, m[15]);
    res20 = fma52hi(res20, u4, m[15]);
    fma52lo_mem(res20, res20, u4, m, SIMD_BYTES * 16);
    fma52hi_mem(res21, res21, u4, m, SIMD_BYTES * 16);
    res21 = fma52lo(res21, u4, m[17]);
    res22 = fma52hi(res22, u4, m[17]);
    fma52lo_mem(res22, res22, u4, m, SIMD_BYTES * 18);
    fma52hi_mem(res23, res23, u4, m, SIMD_BYTES * 18);
    res23 = fma52lo(res23, u4, m[19]);
    res24 = fma52hi(res24, u4, m[19]);

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
    fma52lo_mem(res15, res15, u5, m, SIMD_BYTES * 10);
    fma52hi_mem(res16, res16, u5, m, SIMD_BYTES * 10);
    res16 = fma52lo(res16, u5, m[11]);
    res17 = fma52hi(res17, u5, m[11]);
    fma52lo_mem(res17, res17, u5, m, SIMD_BYTES * 12);
    fma52hi_mem(res18, res18, u5, m, SIMD_BYTES * 12);
    res18 = fma52lo(res18, u5, m[13]);
    res19 = fma52hi(res19, u5, m[13]);
    fma52lo_mem(res19, res19, u5, m, SIMD_BYTES * 14);
    fma52hi_mem(res20, res20, u5, m, SIMD_BYTES * 14);
    res20 = fma52lo(res20, u5, m[15]);
    res21 = fma52hi(res21, u5, m[15]);
    fma52lo_mem(res21, res21, u5, m, SIMD_BYTES * 16);
    fma52hi_mem(res22, res22, u5, m, SIMD_BYTES * 16);
    res22 = fma52lo(res22, u5, m[17]);
    res23 = fma52hi(res23, u5, m[17]);
    fma52lo_mem(res23, res23, u5, m, SIMD_BYTES * 18);
    fma52hi_mem(res24, res24, u5, m, SIMD_BYTES * 18);
    res24 = fma52lo(res24, u5, m[19]);
    res25 = fma52hi(res25, u5, m[19]);
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
    fma52lo_mem(res16, res16, u6, m, SIMD_BYTES * 10);
    fma52hi_mem(res17, res17, u6, m, SIMD_BYTES * 10);
    res17 = fma52lo(res17, u6, m[11]);
    res18 = fma52hi(res18, u6, m[11]);
    fma52lo_mem(res18, res18, u6, m, SIMD_BYTES * 12);
    fma52hi_mem(res19, res19, u6, m, SIMD_BYTES * 12);
    res19 = fma52lo(res19, u6, m[13]);
    res20 = fma52hi(res20, u6, m[13]);
    fma52lo_mem(res20, res20, u6, m, SIMD_BYTES * 14);
    fma52hi_mem(res21, res21, u6, m, SIMD_BYTES * 14);
    res21 = fma52lo(res21, u6, m[15]);
    res22 = fma52hi(res22, u6, m[15]);
    fma52lo_mem(res22, res22, u6, m, SIMD_BYTES * 16);
    fma52hi_mem(res23, res23, u6, m, SIMD_BYTES * 16);
    res23 = fma52lo(res23, u6, m[17]);
    res24 = fma52hi(res24, u6, m[17]);
    fma52lo_mem(res24, res24, u6, m, SIMD_BYTES * 18);
    fma52hi_mem(res25, res25, u6, m, SIMD_BYTES * 18);
    res25 = fma52lo(res25, u6, m[19]);
    res26 = fma52hi(res26, u6, m[19]);

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
    fma52lo_mem(res17, res17, u7, m, SIMD_BYTES * 10);
    fma52hi_mem(res18, res18, u7, m, SIMD_BYTES * 10);
    res18 = fma52lo(res18, u7, m[11]);
    res19 = fma52hi(res19, u7, m[11]);
    fma52lo_mem(res19, res19, u7, m, SIMD_BYTES * 12);
    fma52hi_mem(res20, res20, u7, m, SIMD_BYTES * 12);
    res20 = fma52lo(res20, u7, m[13]);
    res21 = fma52hi(res21, u7, m[13]);
    fma52lo_mem(res21, res21, u7, m, SIMD_BYTES * 14);
    fma52hi_mem(res22, res22, u7, m, SIMD_BYTES * 14);
    res22 = fma52lo(res22, u7, m[15]);
    res23 = fma52hi(res23, u7, m[15]);
    fma52lo_mem(res23, res23, u7, m, SIMD_BYTES * 16);
    fma52hi_mem(res24, res24, u7, m, SIMD_BYTES * 16);
    res24 = fma52lo(res24, u7, m[17]);
    res25 = fma52hi(res25, u7, m[17]);
    fma52lo_mem(res25, res25, u7, m, SIMD_BYTES * 18);
    fma52hi_mem(res26, res26, u7, m, SIMD_BYTES * 18);
    res26 = fma52lo(res26, u7, m[19]);
    res27 = fma52hi(res27, u7, m[19]);
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
    fma52lo_mem(res18, res18, u8, m, SIMD_BYTES * 10);
    fma52hi_mem(res19, res19, u8, m, SIMD_BYTES * 10);
    res19 = fma52lo(res19, u8, m[11]);
    res20 = fma52hi(res20, u8, m[11]);
    fma52lo_mem(res20, res20, u8, m, SIMD_BYTES * 12);
    fma52hi_mem(res21, res21, u8, m, SIMD_BYTES * 12);
    res21 = fma52lo(res21, u8, m[13]);
    res22 = fma52hi(res22, u8, m[13]);
    fma52lo_mem(res22, res22, u8, m, SIMD_BYTES * 14);
    fma52hi_mem(res23, res23, u8, m, SIMD_BYTES * 14);
    res23 = fma52lo(res23, u8, m[15]);
    res24 = fma52hi(res24, u8, m[15]);
    fma52lo_mem(res24, res24, u8, m, SIMD_BYTES * 16);
    fma52hi_mem(res25, res25, u8, m, SIMD_BYTES * 16);
    res25 = fma52lo(res25, u8, m[17]);
    res26 = fma52hi(res26, u8, m[17]);
    fma52lo_mem(res26, res26, u8, m, SIMD_BYTES * 18);
    fma52hi_mem(res27, res27, u8, m, SIMD_BYTES * 18);
    res27 = fma52lo(res27, u8, m[19]);
    res28 = fma52hi(res28, u8, m[19]);

    // Create u9
    fma52lo_mem(res9, res9, u9, m, SIMD_BYTES * 0);
    fma52hi_mem(res10, res10, u9, m, SIMD_BYTES * 0);
    res10 = fma52lo(res10, u9, m[1]);
    res11 = fma52hi(res11, u9, m[1]);
    res10 = add64(res10, srli64(res9, DIGIT_SIZE));
    U64 u10 = mul52lo(res10, k);
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
    fma52lo_mem(res19, res19, u9, m, SIMD_BYTES * 10);
    fma52hi_mem(res20, res20, u9, m, SIMD_BYTES * 10);
    res20 = fma52lo(res20, u9, m[11]);
    res21 = fma52hi(res21, u9, m[11]);
    fma52lo_mem(res21, res21, u9, m, SIMD_BYTES * 12);
    fma52hi_mem(res22, res22, u9, m, SIMD_BYTES * 12);
    res22 = fma52lo(res22, u9, m[13]);
    res23 = fma52hi(res23, u9, m[13]);
    fma52lo_mem(res23, res23, u9, m, SIMD_BYTES * 14);
    fma52hi_mem(res24, res24, u9, m, SIMD_BYTES * 14);
    res24 = fma52lo(res24, u9, m[15]);
    res25 = fma52hi(res25, u9, m[15]);
    fma52lo_mem(res25, res25, u9, m, SIMD_BYTES * 16);
    fma52hi_mem(res26, res26, u9, m, SIMD_BYTES * 16);
    res26 = fma52lo(res26, u9, m[17]);
    res27 = fma52hi(res27, u9, m[17]);
    fma52lo_mem(res27, res27, u9, m, SIMD_BYTES * 18);
    fma52hi_mem(res28, res28, u9, m, SIMD_BYTES * 18);
    res28 = fma52lo(res28, u9, m[19]);
    res29 = fma52hi(res29, u9, m[19]);
    ASM("jmp l10\nl10:\n");

    // Create u10
    fma52lo_mem(res10, res10, u10, m, SIMD_BYTES * 0);
    fma52hi_mem(res11, res11, u10, m, SIMD_BYTES * 0);
    res11 = fma52lo(res11, u10, m[1]);
    res12 = fma52hi(res12, u10, m[1]);
    res11 = add64(res11, srli64(res10, DIGIT_SIZE));
    U64 u11 = mul52lo(res11, k);
    fma52lo_mem(res12, res12, u10, m, SIMD_BYTES * 2);
    fma52hi_mem(res13, res13, u10, m, SIMD_BYTES * 2);
    res13 = fma52lo(res13, u10, m[3]);
    res14 = fma52hi(res14, u10, m[3]);
    fma52lo_mem(res14, res14, u10, m, SIMD_BYTES * 4);
    fma52hi_mem(res15, res15, u10, m, SIMD_BYTES * 4);
    res15 = fma52lo(res15, u10, m[5]);
    res16 = fma52hi(res16, u10, m[5]);
    fma52lo_mem(res16, res16, u10, m, SIMD_BYTES * 6);
    fma52hi_mem(res17, res17, u10, m, SIMD_BYTES * 6);
    res17 = fma52lo(res17, u10, m[7]);
    res18 = fma52hi(res18, u10, m[7]);
    fma52lo_mem(res18, res18, u10, m, SIMD_BYTES * 8);
    fma52hi_mem(res19, res19, u10, m, SIMD_BYTES * 8);
    res19 = fma52lo(res19, u10, m[9]);
    res20 = fma52hi(res20, u10, m[9]);
    fma52lo_mem(res20, res20, u10, m, SIMD_BYTES * 10);
    fma52hi_mem(res21, res21, u10, m, SIMD_BYTES * 10);
    res21 = fma52lo(res21, u10, m[11]);
    res22 = fma52hi(res22, u10, m[11]);
    fma52lo_mem(res22, res22, u10, m, SIMD_BYTES * 12);
    fma52hi_mem(res23, res23, u10, m, SIMD_BYTES * 12);
    res23 = fma52lo(res23, u10, m[13]);
    res24 = fma52hi(res24, u10, m[13]);
    fma52lo_mem(res24, res24, u10, m, SIMD_BYTES * 14);
    fma52hi_mem(res25, res25, u10, m, SIMD_BYTES * 14);
    res25 = fma52lo(res25, u10, m[15]);
    res26 = fma52hi(res26, u10, m[15]);
    fma52lo_mem(res26, res26, u10, m, SIMD_BYTES * 16);
    fma52hi_mem(res27, res27, u10, m, SIMD_BYTES * 16);
    res27 = fma52lo(res27, u10, m[17]);
    res28 = fma52hi(res28, u10, m[17]);
    fma52lo_mem(res28, res28, u10, m, SIMD_BYTES * 18);
    fma52hi_mem(res29, res29, u10, m, SIMD_BYTES * 18);
    res29 = fma52lo(res29, u10, m[19]);
    res30 = fma52hi(res30, u10, m[19]);

    // Create u11
    fma52lo_mem(res11, res11, u11, m, SIMD_BYTES * 0);
    fma52hi_mem(res12, res12, u11, m, SIMD_BYTES * 0);
    res12 = fma52lo(res12, u11, m[1]);
    res13 = fma52hi(res13, u11, m[1]);
    res12 = add64(res12, srli64(res11, DIGIT_SIZE));
    U64 u12 = mul52lo(res12, k);
    fma52lo_mem(res13, res13, u11, m, SIMD_BYTES * 2);
    fma52hi_mem(res14, res14, u11, m, SIMD_BYTES * 2);
    res14 = fma52lo(res14, u11, m[3]);
    res15 = fma52hi(res15, u11, m[3]);
    fma52lo_mem(res15, res15, u11, m, SIMD_BYTES * 4);
    fma52hi_mem(res16, res16, u11, m, SIMD_BYTES * 4);
    res16 = fma52lo(res16, u11, m[5]);
    res17 = fma52hi(res17, u11, m[5]);
    fma52lo_mem(res17, res17, u11, m, SIMD_BYTES * 6);
    fma52hi_mem(res18, res18, u11, m, SIMD_BYTES * 6);
    res18 = fma52lo(res18, u11, m[7]);
    res19 = fma52hi(res19, u11, m[7]);
    fma52lo_mem(res19, res19, u11, m, SIMD_BYTES * 8);
    fma52hi_mem(res20, res20, u11, m, SIMD_BYTES * 8);
    res20 = fma52lo(res20, u11, m[9]);
    res21 = fma52hi(res21, u11, m[9]);
    fma52lo_mem(res21, res21, u11, m, SIMD_BYTES * 10);
    fma52hi_mem(res22, res22, u11, m, SIMD_BYTES * 10);
    res22 = fma52lo(res22, u11, m[11]);
    res23 = fma52hi(res23, u11, m[11]);
    fma52lo_mem(res23, res23, u11, m, SIMD_BYTES * 12);
    fma52hi_mem(res24, res24, u11, m, SIMD_BYTES * 12);
    res24 = fma52lo(res24, u11, m[13]);
    res25 = fma52hi(res25, u11, m[13]);
    fma52lo_mem(res25, res25, u11, m, SIMD_BYTES * 14);
    fma52hi_mem(res26, res26, u11, m, SIMD_BYTES * 14);
    res26 = fma52lo(res26, u11, m[15]);
    res27 = fma52hi(res27, u11, m[15]);
    fma52lo_mem(res27, res27, u11, m, SIMD_BYTES * 16);
    fma52hi_mem(res28, res28, u11, m, SIMD_BYTES * 16);
    res28 = fma52lo(res28, u11, m[17]);
    res29 = fma52hi(res29, u11, m[17]);
    fma52lo_mem(res29, res29, u11, m, SIMD_BYTES * 18);
    fma52hi_mem(res30, res30, u11, m, SIMD_BYTES * 18);
    res30 = fma52lo(res30, u11, m[19]);
    res31 = fma52hi(res31, u11, m[19]);
    ASM("jmp l12\nl12:\n");

    // Create u12
    fma52lo_mem(res12, res12, u12, m, SIMD_BYTES * 0);
    fma52hi_mem(res13, res13, u12, m, SIMD_BYTES * 0);
    res13 = fma52lo(res13, u12, m[1]);
    res14 = fma52hi(res14, u12, m[1]);
    res13 = add64(res13, srli64(res12, DIGIT_SIZE));
    U64 u13 = mul52lo(res13, k);
    fma52lo_mem(res14, res14, u12, m, SIMD_BYTES * 2);
    fma52hi_mem(res15, res15, u12, m, SIMD_BYTES * 2);
    res15 = fma52lo(res15, u12, m[3]);
    res16 = fma52hi(res16, u12, m[3]);
    fma52lo_mem(res16, res16, u12, m, SIMD_BYTES * 4);
    fma52hi_mem(res17, res17, u12, m, SIMD_BYTES * 4);
    res17 = fma52lo(res17, u12, m[5]);
    res18 = fma52hi(res18, u12, m[5]);
    fma52lo_mem(res18, res18, u12, m, SIMD_BYTES * 6);
    fma52hi_mem(res19, res19, u12, m, SIMD_BYTES * 6);
    res19 = fma52lo(res19, u12, m[7]);
    res20 = fma52hi(res20, u12, m[7]);
    fma52lo_mem(res20, res20, u12, m, SIMD_BYTES * 8);
    fma52hi_mem(res21, res21, u12, m, SIMD_BYTES * 8);
    res21 = fma52lo(res21, u12, m[9]);
    res22 = fma52hi(res22, u12, m[9]);
    fma52lo_mem(res22, res22, u12, m, SIMD_BYTES * 10);
    fma52hi_mem(res23, res23, u12, m, SIMD_BYTES * 10);
    res23 = fma52lo(res23, u12, m[11]);
    res24 = fma52hi(res24, u12, m[11]);
    fma52lo_mem(res24, res24, u12, m, SIMD_BYTES * 12);
    fma52hi_mem(res25, res25, u12, m, SIMD_BYTES * 12);
    res25 = fma52lo(res25, u12, m[13]);
    res26 = fma52hi(res26, u12, m[13]);
    fma52lo_mem(res26, res26, u12, m, SIMD_BYTES * 14);
    fma52hi_mem(res27, res27, u12, m, SIMD_BYTES * 14);
    res27 = fma52lo(res27, u12, m[15]);
    res28 = fma52hi(res28, u12, m[15]);
    fma52lo_mem(res28, res28, u12, m, SIMD_BYTES * 16);
    fma52hi_mem(res29, res29, u12, m, SIMD_BYTES * 16);
    res29 = fma52lo(res29, u12, m[17]);
    res30 = fma52hi(res30, u12, m[17]);
    fma52lo_mem(res30, res30, u12, m, SIMD_BYTES * 18);
    fma52hi_mem(res31, res31, u12, m, SIMD_BYTES * 18);
    res31 = fma52lo(res31, u12, m[19]);
    res32 = fma52hi(res32, u12, m[19]);

    // Create u13
    fma52lo_mem(res13, res13, u13, m, SIMD_BYTES * 0);
    fma52hi_mem(res14, res14, u13, m, SIMD_BYTES * 0);
    res14 = fma52lo(res14, u13, m[1]);
    res15 = fma52hi(res15, u13, m[1]);
    res14 = add64(res14, srli64(res13, DIGIT_SIZE));
    U64 u14 = mul52lo(res14, k);
    fma52lo_mem(res15, res15, u13, m, SIMD_BYTES * 2);
    fma52hi_mem(res16, res16, u13, m, SIMD_BYTES * 2);
    res16 = fma52lo(res16, u13, m[3]);
    res17 = fma52hi(res17, u13, m[3]);
    fma52lo_mem(res17, res17, u13, m, SIMD_BYTES * 4);
    fma52hi_mem(res18, res18, u13, m, SIMD_BYTES * 4);
    res18 = fma52lo(res18, u13, m[5]);
    res19 = fma52hi(res19, u13, m[5]);
    fma52lo_mem(res19, res19, u13, m, SIMD_BYTES * 6);
    fma52hi_mem(res20, res20, u13, m, SIMD_BYTES * 6);
    res20 = fma52lo(res20, u13, m[7]);
    res21 = fma52hi(res21, u13, m[7]);
    fma52lo_mem(res21, res21, u13, m, SIMD_BYTES * 8);
    fma52hi_mem(res22, res22, u13, m, SIMD_BYTES * 8);
    res22 = fma52lo(res22, u13, m[9]);
    res23 = fma52hi(res23, u13, m[9]);
    fma52lo_mem(res23, res23, u13, m, SIMD_BYTES * 10);
    fma52hi_mem(res24, res24, u13, m, SIMD_BYTES * 10);
    res24 = fma52lo(res24, u13, m[11]);
    res25 = fma52hi(res25, u13, m[11]);
    fma52lo_mem(res25, res25, u13, m, SIMD_BYTES * 12);
    fma52hi_mem(res26, res26, u13, m, SIMD_BYTES * 12);
    res26 = fma52lo(res26, u13, m[13]);
    res27 = fma52hi(res27, u13, m[13]);
    fma52lo_mem(res27, res27, u13, m, SIMD_BYTES * 14);
    fma52hi_mem(res28, res28, u13, m, SIMD_BYTES * 14);
    res28 = fma52lo(res28, u13, m[15]);
    res29 = fma52hi(res29, u13, m[15]);
    fma52lo_mem(res29, res29, u13, m, SIMD_BYTES * 16);
    fma52hi_mem(res30, res30, u13, m, SIMD_BYTES * 16);
    res30 = fma52lo(res30, u13, m[17]);
    res31 = fma52hi(res31, u13, m[17]);
    fma52lo_mem(res31, res31, u13, m, SIMD_BYTES * 18);
    fma52hi_mem(res32, res32, u13, m, SIMD_BYTES * 18);
    res32 = fma52lo(res32, u13, m[19]);
    res33 = fma52hi(res33, u13, m[19]);
    ASM("jmp l14\nl14:\n");

    // Create u14
    fma52lo_mem(res14, res14, u14, m, SIMD_BYTES * 0);
    fma52hi_mem(res15, res15, u14, m, SIMD_BYTES * 0);
    res15 = fma52lo(res15, u14, m[1]);
    res16 = fma52hi(res16, u14, m[1]);
    res15 = add64(res15, srli64(res14, DIGIT_SIZE));
    U64 u15 = mul52lo(res15, k);
    fma52lo_mem(res16, res16, u14, m, SIMD_BYTES * 2);
    fma52hi_mem(res17, res17, u14, m, SIMD_BYTES * 2);
    res17 = fma52lo(res17, u14, m[3]);
    res18 = fma52hi(res18, u14, m[3]);
    fma52lo_mem(res18, res18, u14, m, SIMD_BYTES * 4);
    fma52hi_mem(res19, res19, u14, m, SIMD_BYTES * 4);
    res19 = fma52lo(res19, u14, m[5]);
    res20 = fma52hi(res20, u14, m[5]);
    fma52lo_mem(res20, res20, u14, m, SIMD_BYTES * 6);
    fma52hi_mem(res21, res21, u14, m, SIMD_BYTES * 6);
    res21 = fma52lo(res21, u14, m[7]);
    res22 = fma52hi(res22, u14, m[7]);
    fma52lo_mem(res22, res22, u14, m, SIMD_BYTES * 8);
    fma52hi_mem(res23, res23, u14, m, SIMD_BYTES * 8);
    res23 = fma52lo(res23, u14, m[9]);
    res24 = fma52hi(res24, u14, m[9]);
    fma52lo_mem(res24, res24, u14, m, SIMD_BYTES * 10);
    fma52hi_mem(res25, res25, u14, m, SIMD_BYTES * 10);
    res25 = fma52lo(res25, u14, m[11]);
    res26 = fma52hi(res26, u14, m[11]);
    fma52lo_mem(res26, res26, u14, m, SIMD_BYTES * 12);
    fma52hi_mem(res27, res27, u14, m, SIMD_BYTES * 12);
    res27 = fma52lo(res27, u14, m[13]);
    res28 = fma52hi(res28, u14, m[13]);
    fma52lo_mem(res28, res28, u14, m, SIMD_BYTES * 14);
    fma52hi_mem(res29, res29, u14, m, SIMD_BYTES * 14);
    res29 = fma52lo(res29, u14, m[15]);
    res30 = fma52hi(res30, u14, m[15]);
    fma52lo_mem(res30, res30, u14, m, SIMD_BYTES * 16);
    fma52hi_mem(res31, res31, u14, m, SIMD_BYTES * 16);
    res31 = fma52lo(res31, u14, m[17]);
    res32 = fma52hi(res32, u14, m[17]);
    fma52lo_mem(res32, res32, u14, m, SIMD_BYTES * 18);
    fma52hi_mem(res33, res33, u14, m, SIMD_BYTES * 18);
    res33 = fma52lo(res33, u14, m[19]);
    res34 = fma52hi(res34, u14, m[19]);

    // Create u15
    fma52lo_mem(res15, res15, u15, m, SIMD_BYTES * 0);
    fma52hi_mem(res16, res16, u15, m, SIMD_BYTES * 0);
    res16 = fma52lo(res16, u15, m[1]);
    res17 = fma52hi(res17, u15, m[1]);
    res16 = add64(res16, srli64(res15, DIGIT_SIZE));
    U64 u16 = mul52lo(res16, k);
    fma52lo_mem(res17, res17, u15, m, SIMD_BYTES * 2);
    fma52hi_mem(res18, res18, u15, m, SIMD_BYTES * 2);
    res18 = fma52lo(res18, u15, m[3]);
    res19 = fma52hi(res19, u15, m[3]);
    fma52lo_mem(res19, res19, u15, m, SIMD_BYTES * 4);
    fma52hi_mem(res20, res20, u15, m, SIMD_BYTES * 4);
    res20 = fma52lo(res20, u15, m[5]);
    res21 = fma52hi(res21, u15, m[5]);
    fma52lo_mem(res21, res21, u15, m, SIMD_BYTES * 6);
    fma52hi_mem(res22, res22, u15, m, SIMD_BYTES * 6);
    res22 = fma52lo(res22, u15, m[7]);
    res23 = fma52hi(res23, u15, m[7]);
    fma52lo_mem(res23, res23, u15, m, SIMD_BYTES * 8);
    fma52hi_mem(res24, res24, u15, m, SIMD_BYTES * 8);
    res24 = fma52lo(res24, u15, m[9]);
    res25 = fma52hi(res25, u15, m[9]);
    fma52lo_mem(res25, res25, u15, m, SIMD_BYTES * 10);
    fma52hi_mem(res26, res26, u15, m, SIMD_BYTES * 10);
    res26 = fma52lo(res26, u15, m[11]);
    res27 = fma52hi(res27, u15, m[11]);
    fma52lo_mem(res27, res27, u15, m, SIMD_BYTES * 12);
    fma52hi_mem(res28, res28, u15, m, SIMD_BYTES * 12);
    res28 = fma52lo(res28, u15, m[13]);
    res29 = fma52hi(res29, u15, m[13]);
    fma52lo_mem(res29, res29, u15, m, SIMD_BYTES * 14);
    fma52hi_mem(res30, res30, u15, m, SIMD_BYTES * 14);
    res30 = fma52lo(res30, u15, m[15]);
    res31 = fma52hi(res31, u15, m[15]);
    fma52lo_mem(res31, res31, u15, m, SIMD_BYTES * 16);
    fma52hi_mem(res32, res32, u15, m, SIMD_BYTES * 16);
    res32 = fma52lo(res32, u15, m[17]);
    res33 = fma52hi(res33, u15, m[17]);
    fma52lo_mem(res33, res33, u15, m, SIMD_BYTES * 18);
    fma52hi_mem(res34, res34, u15, m, SIMD_BYTES * 18);
    res34 = fma52lo(res34, u15, m[19]);
    res35 = fma52hi(res35, u15, m[19]);
    ASM("jmp l16\nl16:\n");

    // Create u16
    fma52lo_mem(res16, res16, u16, m, SIMD_BYTES * 0);
    fma52hi_mem(res17, res17, u16, m, SIMD_BYTES * 0);
    res17 = fma52lo(res17, u16, m[1]);
    res18 = fma52hi(res18, u16, m[1]);
    res17 = add64(res17, srli64(res16, DIGIT_SIZE));
    U64 u17 = mul52lo(res17, k);
    fma52lo_mem(res18, res18, u16, m, SIMD_BYTES * 2);
    fma52hi_mem(res19, res19, u16, m, SIMD_BYTES * 2);
    res19 = fma52lo(res19, u16, m[3]);
    res20 = fma52hi(res20, u16, m[3]);
    fma52lo_mem(res20, res20, u16, m, SIMD_BYTES * 4);
    fma52hi_mem(res21, res21, u16, m, SIMD_BYTES * 4);
    res21 = fma52lo(res21, u16, m[5]);
    res22 = fma52hi(res22, u16, m[5]);
    fma52lo_mem(res22, res22, u16, m, SIMD_BYTES * 6);
    fma52hi_mem(res23, res23, u16, m, SIMD_BYTES * 6);
    res23 = fma52lo(res23, u16, m[7]);
    res24 = fma52hi(res24, u16, m[7]);
    fma52lo_mem(res24, res24, u16, m, SIMD_BYTES * 8);
    fma52hi_mem(res25, res25, u16, m, SIMD_BYTES * 8);
    res25 = fma52lo(res25, u16, m[9]);
    res26 = fma52hi(res26, u16, m[9]);
    fma52lo_mem(res26, res26, u16, m, SIMD_BYTES * 10);
    fma52hi_mem(res27, res27, u16, m, SIMD_BYTES * 10);
    res27 = fma52lo(res27, u16, m[11]);
    res28 = fma52hi(res28, u16, m[11]);
    fma52lo_mem(res28, res28, u16, m, SIMD_BYTES * 12);
    fma52hi_mem(res29, res29, u16, m, SIMD_BYTES * 12);
    res29 = fma52lo(res29, u16, m[13]);
    res30 = fma52hi(res30, u16, m[13]);
    fma52lo_mem(res30, res30, u16, m, SIMD_BYTES * 14);
    fma52hi_mem(res31, res31, u16, m, SIMD_BYTES * 14);
    res31 = fma52lo(res31, u16, m[15]);
    res32 = fma52hi(res32, u16, m[15]);
    fma52lo_mem(res32, res32, u16, m, SIMD_BYTES * 16);
    fma52hi_mem(res33, res33, u16, m, SIMD_BYTES * 16);
    res33 = fma52lo(res33, u16, m[17]);
    res34 = fma52hi(res34, u16, m[17]);
    fma52lo_mem(res34, res34, u16, m, SIMD_BYTES * 18);
    fma52hi_mem(res35, res35, u16, m, SIMD_BYTES * 18);
    res35 = fma52lo(res35, u16, m[19]);
    res36 = fma52hi(res36, u16, m[19]);

    // Create u17
    fma52lo_mem(res17, res17, u17, m, SIMD_BYTES * 0);
    fma52hi_mem(res18, res18, u17, m, SIMD_BYTES * 0);
    res18 = fma52lo(res18, u17, m[1]);
    res19 = fma52hi(res19, u17, m[1]);
    res18 = add64(res18, srli64(res17, DIGIT_SIZE));
    U64 u18 = mul52lo(res18, k);
    fma52lo_mem(res19, res19, u17, m, SIMD_BYTES * 2);
    fma52hi_mem(res20, res20, u17, m, SIMD_BYTES * 2);
    res20 = fma52lo(res20, u17, m[3]);
    res21 = fma52hi(res21, u17, m[3]);
    fma52lo_mem(res21, res21, u17, m, SIMD_BYTES * 4);
    fma52hi_mem(res22, res22, u17, m, SIMD_BYTES * 4);
    res22 = fma52lo(res22, u17, m[5]);
    res23 = fma52hi(res23, u17, m[5]);
    fma52lo_mem(res23, res23, u17, m, SIMD_BYTES * 6);
    fma52hi_mem(res24, res24, u17, m, SIMD_BYTES * 6);
    res24 = fma52lo(res24, u17, m[7]);
    res25 = fma52hi(res25, u17, m[7]);
    fma52lo_mem(res25, res25, u17, m, SIMD_BYTES * 8);
    fma52hi_mem(res26, res26, u17, m, SIMD_BYTES * 8);
    res26 = fma52lo(res26, u17, m[9]);
    res27 = fma52hi(res27, u17, m[9]);
    fma52lo_mem(res27, res27, u17, m, SIMD_BYTES * 10);
    fma52hi_mem(res28, res28, u17, m, SIMD_BYTES * 10);
    res28 = fma52lo(res28, u17, m[11]);
    res29 = fma52hi(res29, u17, m[11]);
    fma52lo_mem(res29, res29, u17, m, SIMD_BYTES * 12);
    fma52hi_mem(res30, res30, u17, m, SIMD_BYTES * 12);
    res30 = fma52lo(res30, u17, m[13]);
    res31 = fma52hi(res31, u17, m[13]);
    fma52lo_mem(res31, res31, u17, m, SIMD_BYTES * 14);
    fma52hi_mem(res32, res32, u17, m, SIMD_BYTES * 14);
    res32 = fma52lo(res32, u17, m[15]);
    res33 = fma52hi(res33, u17, m[15]);
    fma52lo_mem(res33, res33, u17, m, SIMD_BYTES * 16);
    fma52hi_mem(res34, res34, u17, m, SIMD_BYTES * 16);
    res34 = fma52lo(res34, u17, m[17]);
    res35 = fma52hi(res35, u17, m[17]);
    fma52lo_mem(res35, res35, u17, m, SIMD_BYTES * 18);
    fma52hi_mem(res36, res36, u17, m, SIMD_BYTES * 18);
    res36 = fma52lo(res36, u17, m[19]);
    res37 = fma52hi(res37, u17, m[19]);
    ASM("jmp l18\nl18:\n");

    // Create u18
    fma52lo_mem(res18, res18, u18, m, SIMD_BYTES * 0);
    fma52hi_mem(res19, res19, u18, m, SIMD_BYTES * 0);
    res19 = fma52lo(res19, u18, m[1]);
    res20 = fma52hi(res20, u18, m[1]);
    res19 = add64(res19, srli64(res18, DIGIT_SIZE));
    U64 u19 = mul52lo(res19, k);
    fma52lo_mem(res20, res20, u18, m, SIMD_BYTES * 2);
    fma52hi_mem(res21, res21, u18, m, SIMD_BYTES * 2);
    res21 = fma52lo(res21, u18, m[3]);
    res22 = fma52hi(res22, u18, m[3]);
    fma52lo_mem(res22, res22, u18, m, SIMD_BYTES * 4);
    fma52hi_mem(res23, res23, u18, m, SIMD_BYTES * 4);
    res23 = fma52lo(res23, u18, m[5]);
    res24 = fma52hi(res24, u18, m[5]);
    fma52lo_mem(res24, res24, u18, m, SIMD_BYTES * 6);
    fma52hi_mem(res25, res25, u18, m, SIMD_BYTES * 6);
    res25 = fma52lo(res25, u18, m[7]);
    res26 = fma52hi(res26, u18, m[7]);
    fma52lo_mem(res26, res26, u18, m, SIMD_BYTES * 8);
    fma52hi_mem(res27, res27, u18, m, SIMD_BYTES * 8);
    res27 = fma52lo(res27, u18, m[9]);
    res28 = fma52hi(res28, u18, m[9]);
    fma52lo_mem(res28, res28, u18, m, SIMD_BYTES * 10);
    fma52hi_mem(res29, res29, u18, m, SIMD_BYTES * 10);
    res29 = fma52lo(res29, u18, m[11]);
    res30 = fma52hi(res30, u18, m[11]);
    fma52lo_mem(res30, res30, u18, m, SIMD_BYTES * 12);
    fma52hi_mem(res31, res31, u18, m, SIMD_BYTES * 12);
    res31 = fma52lo(res31, u18, m[13]);
    res32 = fma52hi(res32, u18, m[13]);
    fma52lo_mem(res32, res32, u18, m, SIMD_BYTES * 14);
    fma52hi_mem(res33, res33, u18, m, SIMD_BYTES * 14);
    res33 = fma52lo(res33, u18, m[15]);
    res34 = fma52hi(res34, u18, m[15]);
    fma52lo_mem(res34, res34, u18, m, SIMD_BYTES * 16);
    fma52hi_mem(res35, res35, u18, m, SIMD_BYTES * 16);
    res35 = fma52lo(res35, u18, m[17]);
    res36 = fma52hi(res36, u18, m[17]);
    fma52lo_mem(res36, res36, u18, m, SIMD_BYTES * 18);
    fma52hi_mem(res37, res37, u18, m, SIMD_BYTES * 18);
    res37 = fma52lo(res37, u18, m[19]);
    res38 = fma52hi(res38, u18, m[19]);

    // Create u19
    fma52lo_mem(res19, res19, u19, m, SIMD_BYTES * 0);
    fma52hi_mem(res20, res20, u19, m, SIMD_BYTES * 0);
    res20 = fma52lo(res20, u19, m[1]);
    res21 = fma52hi(res21, u19, m[1]);
    res20 = add64(res20, srli64(res19, DIGIT_SIZE));
    fma52lo_mem(res21, res21, u19, m, SIMD_BYTES * 2);
    fma52hi_mem(res22, res22, u19, m, SIMD_BYTES * 2);
    res22 = fma52lo(res22, u19, m[3]);
    res23 = fma52hi(res23, u19, m[3]);
    fma52lo_mem(res23, res23, u19, m, SIMD_BYTES * 4);
    fma52hi_mem(res24, res24, u19, m, SIMD_BYTES * 4);
    res24 = fma52lo(res24, u19, m[5]);
    res25 = fma52hi(res25, u19, m[5]);
    fma52lo_mem(res25, res25, u19, m, SIMD_BYTES * 6);
    fma52hi_mem(res26, res26, u19, m, SIMD_BYTES * 6);
    res26 = fma52lo(res26, u19, m[7]);
    res27 = fma52hi(res27, u19, m[7]);
    fma52lo_mem(res27, res27, u19, m, SIMD_BYTES * 8);
    fma52hi_mem(res28, res28, u19, m, SIMD_BYTES * 8);
    res28 = fma52lo(res28, u19, m[9]);
    res29 = fma52hi(res29, u19, m[9]);
    fma52lo_mem(res29, res29, u19, m, SIMD_BYTES * 10);
    fma52hi_mem(res30, res30, u19, m, SIMD_BYTES * 10);
    res30 = fma52lo(res30, u19, m[11]);
    res31 = fma52hi(res31, u19, m[11]);
    fma52lo_mem(res31, res31, u19, m, SIMD_BYTES * 12);
    fma52hi_mem(res32, res32, u19, m, SIMD_BYTES * 12);
    res32 = fma52lo(res32, u19, m[13]);
    res33 = fma52hi(res33, u19, m[13]);
    fma52lo_mem(res33, res33, u19, m, SIMD_BYTES * 14);
    fma52hi_mem(res34, res34, u19, m, SIMD_BYTES * 14);
    res34 = fma52lo(res34, u19, m[15]);
    res35 = fma52hi(res35, u19, m[15]);
    fma52lo_mem(res35, res35, u19, m, SIMD_BYTES * 16);
    fma52hi_mem(res36, res36, u19, m, SIMD_BYTES * 16);
    res36 = fma52lo(res36, u19, m[17]);
    res37 = fma52hi(res37, u19, m[17]);
    fma52lo_mem(res37, res37, u19, m, SIMD_BYTES * 18);
    fma52hi_mem(res38, res38, u19, m, SIMD_BYTES * 18);
    res38 = fma52lo(res38, u19, m[19]);
    res39 = fma52hi(res39, u19, m[19]);

    // Normalization
    r[0] = res20;
    res21 = add64(res21, srli64(res20, DIGIT_SIZE));
    r[1] = res21;
    res22 = add64(res22, srli64(res21, DIGIT_SIZE));
    r[2] = res22;
    res23 = add64(res23, srli64(res22, DIGIT_SIZE));
    r[3] = res23;
    res24 = add64(res24, srli64(res23, DIGIT_SIZE));
    r[4] = res24;
    res25 = add64(res25, srli64(res24, DIGIT_SIZE));
    r[5] = res25;
    res26 = add64(res26, srli64(res25, DIGIT_SIZE));
    r[6] = res26;
    res27 = add64(res27, srli64(res26, DIGIT_SIZE));
    r[7] = res27;
    res28 = add64(res28, srli64(res27, DIGIT_SIZE));
    r[8] = res28;
    res29 = add64(res29, srli64(res28, DIGIT_SIZE));
    r[9] = res29;
    res30 = add64(res30, srli64(res29, DIGIT_SIZE));
    r[10] = res30;
    res31 = add64(res31, srli64(res30, DIGIT_SIZE));
    r[11] = res31;
    res32 = add64(res32, srli64(res31, DIGIT_SIZE));
    r[12] = res32;
    res33 = add64(res33, srli64(res32, DIGIT_SIZE));
    r[13] = res33;
    res34 = add64(res34, srli64(res33, DIGIT_SIZE));
    r[14] = res34;
    res35 = add64(res35, srli64(res34, DIGIT_SIZE));
    r[15] = res35;
    res36 = add64(res36, srli64(res35, DIGIT_SIZE));
    r[16] = res36;
    res37 = add64(res37, srli64(res36, DIGIT_SIZE));
    r[17] = res37;
    res38 = add64(res38, srli64(res37, DIGIT_SIZE));
    r[18] = res38;
    res39 = add64(res39, srli64(res38, DIGIT_SIZE));
    r[19] = res39;
    a = (U64 *)out_mb;
  }
}

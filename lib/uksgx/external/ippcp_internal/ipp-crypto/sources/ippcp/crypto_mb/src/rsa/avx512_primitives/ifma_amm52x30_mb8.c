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

void ifma_amm52x30_mb8(int64u *out_mb, const int64u *inpA_mb,
                       const int64u *inpB_mb, const int64u *inpM_mb,
                       const int64u *k0_mb) {
  U64 res00, res01, res02, res03, res04, res05, res06, res07, res08, res09,
      res10, res11, res12, res13, res14, res15, res16, res17, res18, res19,
      res20, res21, res22, res23, res24, res25, res26, res27, res28, res29;
  U64 K = loadu64(k0_mb);
  int itr;
  res00 = res01 = res02 = res03 = res04 = res05 = res06 = res07 = res08 =
      res09 = res10 = res11 = res12 = res13 = res14 = res15 = res16 = res17 =
          res18 = res19 = res20 = res21 = res22 = res23 = res24 = res25 =
              res26 = res27 = res28 = res29 = get_zero64();

  for (itr = 0; itr < 30; itr++) {
    U64 Yi;
    U64 Bi = loadu64(inpB_mb);
    inpB_mb += MB_WIDTH;
    fma52lo_mem(res00, res00, Bi, inpA_mb, SIMD_BYTES * 0);
    fma52lo_mem(res01, res01, Bi, inpA_mb, SIMD_BYTES * 1);
    fma52lo_mem(res02, res02, Bi, inpA_mb, SIMD_BYTES * 2);
    fma52lo_mem(res03, res03, Bi, inpA_mb, SIMD_BYTES * 3);
    fma52lo_mem(res04, res04, Bi, inpA_mb, SIMD_BYTES * 4);
    fma52lo_mem(res05, res05, Bi, inpA_mb, SIMD_BYTES * 5);
    fma52lo_mem(res06, res06, Bi, inpA_mb, SIMD_BYTES * 6);
    fma52lo_mem(res07, res07, Bi, inpA_mb, SIMD_BYTES * 7);
    fma52lo_mem(res08, res08, Bi, inpA_mb, SIMD_BYTES * 8);
    fma52lo_mem(res09, res09, Bi, inpA_mb, SIMD_BYTES * 9);
    fma52lo_mem(res10, res10, Bi, inpA_mb, SIMD_BYTES * 10);
    fma52lo_mem(res11, res11, Bi, inpA_mb, SIMD_BYTES * 11);
    fma52lo_mem(res12, res12, Bi, inpA_mb, SIMD_BYTES * 12);
    fma52lo_mem(res13, res13, Bi, inpA_mb, SIMD_BYTES * 13);
    fma52lo_mem(res14, res14, Bi, inpA_mb, SIMD_BYTES * 14);
    fma52lo_mem(res15, res15, Bi, inpA_mb, SIMD_BYTES * 15);
    fma52lo_mem(res16, res16, Bi, inpA_mb, SIMD_BYTES * 16);
    fma52lo_mem(res17, res17, Bi, inpA_mb, SIMD_BYTES * 17);
    fma52lo_mem(res18, res18, Bi, inpA_mb, SIMD_BYTES * 18);
    fma52lo_mem(res19, res19, Bi, inpA_mb, SIMD_BYTES * 19);
    fma52lo_mem(res20, res20, Bi, inpA_mb, SIMD_BYTES * 20);
    fma52lo_mem(res21, res21, Bi, inpA_mb, SIMD_BYTES * 21);
    fma52lo_mem(res22, res22, Bi, inpA_mb, SIMD_BYTES * 22);
    fma52lo_mem(res23, res23, Bi, inpA_mb, SIMD_BYTES * 23);
    fma52lo_mem(res24, res24, Bi, inpA_mb, SIMD_BYTES * 24);
    fma52lo_mem(res25, res25, Bi, inpA_mb, SIMD_BYTES * 25);
    fma52lo_mem(res26, res26, Bi, inpA_mb, SIMD_BYTES * 26);
    fma52lo_mem(res27, res27, Bi, inpA_mb, SIMD_BYTES * 27);
    fma52lo_mem(res28, res28, Bi, inpA_mb, SIMD_BYTES * 28);
    fma52lo_mem(res29, res29, Bi, inpA_mb, SIMD_BYTES * 29);
    Yi = fma52lo(get_zero64(), res00, K);
    fma52lo_mem(res00, res00, Yi, inpM_mb, SIMD_BYTES * 0);
    fma52lo_mem(res01, res01, Yi, inpM_mb, SIMD_BYTES * 1);
    fma52lo_mem(res02, res02, Yi, inpM_mb, SIMD_BYTES * 2);
    fma52lo_mem(res03, res03, Yi, inpM_mb, SIMD_BYTES * 3);
    fma52lo_mem(res04, res04, Yi, inpM_mb, SIMD_BYTES * 4);
    fma52lo_mem(res05, res05, Yi, inpM_mb, SIMD_BYTES * 5);
    fma52lo_mem(res06, res06, Yi, inpM_mb, SIMD_BYTES * 6);
    fma52lo_mem(res07, res07, Yi, inpM_mb, SIMD_BYTES * 7);
    fma52lo_mem(res08, res08, Yi, inpM_mb, SIMD_BYTES * 8);
    fma52lo_mem(res09, res09, Yi, inpM_mb, SIMD_BYTES * 9);
    fma52lo_mem(res10, res10, Yi, inpM_mb, SIMD_BYTES * 10);
    fma52lo_mem(res11, res11, Yi, inpM_mb, SIMD_BYTES * 11);
    fma52lo_mem(res12, res12, Yi, inpM_mb, SIMD_BYTES * 12);
    fma52lo_mem(res13, res13, Yi, inpM_mb, SIMD_BYTES * 13);
    fma52lo_mem(res14, res14, Yi, inpM_mb, SIMD_BYTES * 14);
    fma52lo_mem(res15, res15, Yi, inpM_mb, SIMD_BYTES * 15);
    fma52lo_mem(res16, res16, Yi, inpM_mb, SIMD_BYTES * 16);
    fma52lo_mem(res17, res17, Yi, inpM_mb, SIMD_BYTES * 17);
    fma52lo_mem(res18, res18, Yi, inpM_mb, SIMD_BYTES * 18);
    fma52lo_mem(res19, res19, Yi, inpM_mb, SIMD_BYTES * 19);
    fma52lo_mem(res20, res20, Yi, inpM_mb, SIMD_BYTES * 20);
    fma52lo_mem(res21, res21, Yi, inpM_mb, SIMD_BYTES * 21);
    fma52lo_mem(res22, res22, Yi, inpM_mb, SIMD_BYTES * 22);
    fma52lo_mem(res23, res23, Yi, inpM_mb, SIMD_BYTES * 23);
    fma52lo_mem(res24, res24, Yi, inpM_mb, SIMD_BYTES * 24);
    fma52lo_mem(res25, res25, Yi, inpM_mb, SIMD_BYTES * 25);
    fma52lo_mem(res26, res26, Yi, inpM_mb, SIMD_BYTES * 26);
    fma52lo_mem(res27, res27, Yi, inpM_mb, SIMD_BYTES * 27);
    fma52lo_mem(res28, res28, Yi, inpM_mb, SIMD_BYTES * 28);
    fma52lo_mem(res29, res29, Yi, inpM_mb, SIMD_BYTES * 29);
    res00 = srli64(res00, DIGIT_SIZE);
    res01 = add64(res01, res00);
    fma52hi_mem(res00, res01, Bi, inpA_mb, SIMD_BYTES * 0);
    fma52hi_mem(res01, res02, Bi, inpA_mb, SIMD_BYTES * 1);
    fma52hi_mem(res02, res03, Bi, inpA_mb, SIMD_BYTES * 2);
    fma52hi_mem(res03, res04, Bi, inpA_mb, SIMD_BYTES * 3);
    fma52hi_mem(res04, res05, Bi, inpA_mb, SIMD_BYTES * 4);
    fma52hi_mem(res05, res06, Bi, inpA_mb, SIMD_BYTES * 5);
    fma52hi_mem(res06, res07, Bi, inpA_mb, SIMD_BYTES * 6);
    fma52hi_mem(res07, res08, Bi, inpA_mb, SIMD_BYTES * 7);
    fma52hi_mem(res08, res09, Bi, inpA_mb, SIMD_BYTES * 8);
    fma52hi_mem(res09, res10, Bi, inpA_mb, SIMD_BYTES * 9);
    fma52hi_mem(res10, res11, Bi, inpA_mb, SIMD_BYTES * 10);
    fma52hi_mem(res11, res12, Bi, inpA_mb, SIMD_BYTES * 11);
    fma52hi_mem(res12, res13, Bi, inpA_mb, SIMD_BYTES * 12);
    fma52hi_mem(res13, res14, Bi, inpA_mb, SIMD_BYTES * 13);
    fma52hi_mem(res14, res15, Bi, inpA_mb, SIMD_BYTES * 14);
    fma52hi_mem(res15, res16, Bi, inpA_mb, SIMD_BYTES * 15);
    fma52hi_mem(res16, res17, Bi, inpA_mb, SIMD_BYTES * 16);
    fma52hi_mem(res17, res18, Bi, inpA_mb, SIMD_BYTES * 17);
    fma52hi_mem(res18, res19, Bi, inpA_mb, SIMD_BYTES * 18);
    fma52hi_mem(res19, res20, Bi, inpA_mb, SIMD_BYTES * 19);
    fma52hi_mem(res20, res21, Bi, inpA_mb, SIMD_BYTES * 20);
    fma52hi_mem(res21, res22, Bi, inpA_mb, SIMD_BYTES * 21);
    fma52hi_mem(res22, res23, Bi, inpA_mb, SIMD_BYTES * 22);
    fma52hi_mem(res23, res24, Bi, inpA_mb, SIMD_BYTES * 23);
    fma52hi_mem(res24, res25, Bi, inpA_mb, SIMD_BYTES * 24);
    fma52hi_mem(res25, res26, Bi, inpA_mb, SIMD_BYTES * 25);
    fma52hi_mem(res26, res27, Bi, inpA_mb, SIMD_BYTES * 26);
    fma52hi_mem(res27, res28, Bi, inpA_mb, SIMD_BYTES * 27);
    fma52hi_mem(res28, res29, Bi, inpA_mb, SIMD_BYTES * 28);
    fma52hi_mem(res29, get_zero64(), Bi, inpA_mb, SIMD_BYTES * 29);
    fma52hi_mem(res00, res00, Yi, inpM_mb, SIMD_BYTES * 0);
    fma52hi_mem(res01, res01, Yi, inpM_mb, SIMD_BYTES * 1);
    fma52hi_mem(res02, res02, Yi, inpM_mb, SIMD_BYTES * 2);
    fma52hi_mem(res03, res03, Yi, inpM_mb, SIMD_BYTES * 3);
    fma52hi_mem(res04, res04, Yi, inpM_mb, SIMD_BYTES * 4);
    fma52hi_mem(res05, res05, Yi, inpM_mb, SIMD_BYTES * 5);
    fma52hi_mem(res06, res06, Yi, inpM_mb, SIMD_BYTES * 6);
    fma52hi_mem(res07, res07, Yi, inpM_mb, SIMD_BYTES * 7);
    fma52hi_mem(res08, res08, Yi, inpM_mb, SIMD_BYTES * 8);
    fma52hi_mem(res09, res09, Yi, inpM_mb, SIMD_BYTES * 9);
    fma52hi_mem(res10, res10, Yi, inpM_mb, SIMD_BYTES * 10);
    fma52hi_mem(res11, res11, Yi, inpM_mb, SIMD_BYTES * 11);
    fma52hi_mem(res12, res12, Yi, inpM_mb, SIMD_BYTES * 12);
    fma52hi_mem(res13, res13, Yi, inpM_mb, SIMD_BYTES * 13);
    fma52hi_mem(res14, res14, Yi, inpM_mb, SIMD_BYTES * 14);
    fma52hi_mem(res15, res15, Yi, inpM_mb, SIMD_BYTES * 15);
    fma52hi_mem(res16, res16, Yi, inpM_mb, SIMD_BYTES * 16);
    fma52hi_mem(res17, res17, Yi, inpM_mb, SIMD_BYTES * 17);
    fma52hi_mem(res18, res18, Yi, inpM_mb, SIMD_BYTES * 18);
    fma52hi_mem(res19, res19, Yi, inpM_mb, SIMD_BYTES * 19);
    fma52hi_mem(res20, res20, Yi, inpM_mb, SIMD_BYTES * 20);
    fma52hi_mem(res21, res21, Yi, inpM_mb, SIMD_BYTES * 21);
    fma52hi_mem(res22, res22, Yi, inpM_mb, SIMD_BYTES * 22);
    fma52hi_mem(res23, res23, Yi, inpM_mb, SIMD_BYTES * 23);
    fma52hi_mem(res24, res24, Yi, inpM_mb, SIMD_BYTES * 24);
    fma52hi_mem(res25, res25, Yi, inpM_mb, SIMD_BYTES * 25);
    fma52hi_mem(res26, res26, Yi, inpM_mb, SIMD_BYTES * 26);
    fma52hi_mem(res27, res27, Yi, inpM_mb, SIMD_BYTES * 27);
    fma52hi_mem(res28, res28, Yi, inpM_mb, SIMD_BYTES * 28);
    fma52hi_mem(res29, res29, Yi, inpM_mb, SIMD_BYTES * 29);
  }
  // Normalization
  {
    U64 T = get_zero64();
    U64 MASK = set64(DIGIT_MASK);
    T = srli64(res00, DIGIT_SIZE);
    res00 = and64(res00, MASK);
    storeu64(out_mb + MB_WIDTH * 0, res00);
    res01 = add64(res01, T);
    T = srli64(res01, DIGIT_SIZE);
    res01 = and64(res01, MASK);
    storeu64(out_mb + MB_WIDTH * 1, res01);
    res02 = add64(res02, T);
    T = srli64(res02, DIGIT_SIZE);
    res02 = and64(res02, MASK);
    storeu64(out_mb + MB_WIDTH * 2, res02);
    res03 = add64(res03, T);
    T = srli64(res03, DIGIT_SIZE);
    res03 = and64(res03, MASK);
    storeu64(out_mb + MB_WIDTH * 3, res03);
    res04 = add64(res04, T);
    T = srli64(res04, DIGIT_SIZE);
    res04 = and64(res04, MASK);
    storeu64(out_mb + MB_WIDTH * 4, res04);
    res05 = add64(res05, T);
    T = srli64(res05, DIGIT_SIZE);
    res05 = and64(res05, MASK);
    storeu64(out_mb + MB_WIDTH * 5, res05);
    res06 = add64(res06, T);
    T = srli64(res06, DIGIT_SIZE);
    res06 = and64(res06, MASK);
    storeu64(out_mb + MB_WIDTH * 6, res06);
    res07 = add64(res07, T);
    T = srli64(res07, DIGIT_SIZE);
    res07 = and64(res07, MASK);
    storeu64(out_mb + MB_WIDTH * 7, res07);
    res08 = add64(res08, T);
    T = srli64(res08, DIGIT_SIZE);
    res08 = and64(res08, MASK);
    storeu64(out_mb + MB_WIDTH * 8, res08);
    res09 = add64(res09, T);
    T = srli64(res09, DIGIT_SIZE);
    res09 = and64(res09, MASK);
    storeu64(out_mb + MB_WIDTH * 9, res09);
    res10 = add64(res10, T);
    T = srli64(res10, DIGIT_SIZE);
    res10 = and64(res10, MASK);
    storeu64(out_mb + MB_WIDTH * 10, res10);
    res11 = add64(res11, T);
    T = srli64(res11, DIGIT_SIZE);
    res11 = and64(res11, MASK);
    storeu64(out_mb + MB_WIDTH * 11, res11);
    res12 = add64(res12, T);
    T = srli64(res12, DIGIT_SIZE);
    res12 = and64(res12, MASK);
    storeu64(out_mb + MB_WIDTH * 12, res12);
    res13 = add64(res13, T);
    T = srli64(res13, DIGIT_SIZE);
    res13 = and64(res13, MASK);
    storeu64(out_mb + MB_WIDTH * 13, res13);
    res14 = add64(res14, T);
    T = srli64(res14, DIGIT_SIZE);
    res14 = and64(res14, MASK);
    storeu64(out_mb + MB_WIDTH * 14, res14);
    res15 = add64(res15, T);
    T = srli64(res15, DIGIT_SIZE);
    res15 = and64(res15, MASK);
    storeu64(out_mb + MB_WIDTH * 15, res15);
    res16 = add64(res16, T);
    T = srli64(res16, DIGIT_SIZE);
    res16 = and64(res16, MASK);
    storeu64(out_mb + MB_WIDTH * 16, res16);
    res17 = add64(res17, T);
    T = srli64(res17, DIGIT_SIZE);
    res17 = and64(res17, MASK);
    storeu64(out_mb + MB_WIDTH * 17, res17);
    res18 = add64(res18, T);
    T = srli64(res18, DIGIT_SIZE);
    res18 = and64(res18, MASK);
    storeu64(out_mb + MB_WIDTH * 18, res18);
    res19 = add64(res19, T);
    T = srli64(res19, DIGIT_SIZE);
    res19 = and64(res19, MASK);
    storeu64(out_mb + MB_WIDTH * 19, res19);
    res20 = add64(res20, T);
    T = srli64(res20, DIGIT_SIZE);
    res20 = and64(res20, MASK);
    storeu64(out_mb + MB_WIDTH * 20, res20);
    res21 = add64(res21, T);
    T = srli64(res21, DIGIT_SIZE);
    res21 = and64(res21, MASK);
    storeu64(out_mb + MB_WIDTH * 21, res21);
    res22 = add64(res22, T);
    T = srli64(res22, DIGIT_SIZE);
    res22 = and64(res22, MASK);
    storeu64(out_mb + MB_WIDTH * 22, res22);
    res23 = add64(res23, T);
    T = srli64(res23, DIGIT_SIZE);
    res23 = and64(res23, MASK);
    storeu64(out_mb + MB_WIDTH * 23, res23);
    res24 = add64(res24, T);
    T = srli64(res24, DIGIT_SIZE);
    res24 = and64(res24, MASK);
    storeu64(out_mb + MB_WIDTH * 24, res24);
    res25 = add64(res25, T);
    T = srli64(res25, DIGIT_SIZE);
    res25 = and64(res25, MASK);
    storeu64(out_mb + MB_WIDTH * 25, res25);
    res26 = add64(res26, T);
    T = srli64(res26, DIGIT_SIZE);
    res26 = and64(res26, MASK);
    storeu64(out_mb + MB_WIDTH * 26, res26);
    res27 = add64(res27, T);
    T = srli64(res27, DIGIT_SIZE);
    res27 = and64(res27, MASK);
    storeu64(out_mb + MB_WIDTH * 27, res27);
    res28 = add64(res28, T);
    T = srli64(res28, DIGIT_SIZE);
    res28 = and64(res28, MASK);
    storeu64(out_mb + MB_WIDTH * 28, res28);
    res29 = add64(res29, T);
    res29 = and64(res29, MASK);
    storeu64(out_mb + MB_WIDTH * 29, res29);
  }
}

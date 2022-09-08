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
#include <internal/rsa/ifma_rsa_arith.h>

void ifma_extract_amm52x20_mb8(int64u *out_mb8, const int64u *inpA_mb8,
                               int64u MulTbl[][redLen][8], const int64u Idx[8],
                               const int64u *inpM_mb8, const int64u *k0_mb8) {
  U64 res00, res01, res02, res03, res04, res05, res06, res07, res08, res09,
      res10, res11, res12, res13, res14, res15, res16, res17, res18, res19;
  U64 mulB00, mulB01, mulB02, mulB03, mulB04, mulB05, mulB06, mulB07, mulB08,
      mulB09, mulB10, mulB11, mulB12, mulB13, mulB14, mulB15, mulB16, mulB17,
      mulB18, mulB19;
  U64 K = loadu64(k0_mb8); /* k0[] */
  __mmask8 k;
  __ALIGN64 U64 inpB_mb8[20];
  int itr;
  res00 = res01 = res02 = res03 = res04 = res05 = res06 = res07 = res08 =
      res09 = res10 = res11 = res12 = res13 = res14 = res15 = res16 = res17 =
          res18 = res19 = get_zero64();

  U64 idx_target = loadu64((U64 *)Idx);
  k = cmpeq64_mask(set64(1), idx_target);
  mulB00 = loadu64(MulTbl[0][0]);
  mulB01 = loadu64(MulTbl[0][1]);
  mulB02 = loadu64(MulTbl[0][2]);
  mulB03 = loadu64(MulTbl[0][3]);
  mulB04 = loadu64(MulTbl[0][4]);
  mulB05 = loadu64(MulTbl[0][5]);
  mulB06 = loadu64(MulTbl[0][6]);
  mulB07 = loadu64(MulTbl[0][7]);
  mulB08 = loadu64(MulTbl[0][8]);
  mulB09 = loadu64(MulTbl[0][9]);
  mulB10 = loadu64(MulTbl[0][10]);
  mulB11 = loadu64(MulTbl[0][11]);
  mulB12 = loadu64(MulTbl[0][12]);
  mulB13 = loadu64(MulTbl[0][13]);
  mulB14 = loadu64(MulTbl[0][14]);
  mulB15 = loadu64(MulTbl[0][15]);
  mulB16 = loadu64(MulTbl[0][16]);
  mulB17 = loadu64(MulTbl[0][17]);
  mulB18 = loadu64(MulTbl[0][18]);
  mulB19 = loadu64(MulTbl[0][19]);
  for (itr = 1; itr < (1 << 5); ++itr) {
    U64 idx_curr = set64(itr + 1);
    __mmask8 k_new = cmpeq64_mask(idx_curr, idx_target);
    mulB00 = select64(k, mulB00, (U64 *) MulTbl[itr][0]);
    mulB01 = select64(k, mulB01, (U64 *) MulTbl[itr][1]);
    mulB02 = select64(k, mulB02, (U64 *) MulTbl[itr][2]);
    mulB03 = select64(k, mulB03, (U64 *) MulTbl[itr][3]);
    mulB04 = select64(k, mulB04, (U64 *) MulTbl[itr][4]);
    mulB05 = select64(k, mulB05, (U64 *) MulTbl[itr][5]);
    mulB06 = select64(k, mulB06, (U64 *) MulTbl[itr][6]);
    mulB07 = select64(k, mulB07, (U64 *) MulTbl[itr][7]);
    mulB08 = select64(k, mulB08, (U64 *) MulTbl[itr][8]);
    mulB09 = select64(k, mulB09, (U64 *) MulTbl[itr][9]);
    mulB10 = select64(k, mulB10, (U64 *) MulTbl[itr][10]);
    mulB11 = select64(k, mulB11, (U64 *) MulTbl[itr][11]);
    mulB12 = select64(k, mulB12, (U64 *) MulTbl[itr][12]);
    mulB13 = select64(k, mulB13, (U64 *) MulTbl[itr][13]);
    mulB14 = select64(k, mulB14, (U64 *) MulTbl[itr][14]);
    mulB15 = select64(k, mulB15, (U64 *) MulTbl[itr][15]);
    mulB16 = select64(k, mulB16, (U64 *) MulTbl[itr][16]);
    mulB17 = select64(k, mulB17, (U64 *) MulTbl[itr][17]);
    mulB18 = select64(k, mulB18, (U64 *) MulTbl[itr][18]);
    mulB19 = select64(k, mulB19, (U64 *) MulTbl[itr][19]);
    k = k_new;
  }
  inpB_mb8[0] = mulB00;
  inpB_mb8[1] = mulB01;
  inpB_mb8[2] = mulB02;
  inpB_mb8[3] = mulB03;
  inpB_mb8[4] = mulB04;
  inpB_mb8[5] = mulB05;
  inpB_mb8[6] = mulB06;
  inpB_mb8[7] = mulB07;
  inpB_mb8[8] = mulB08;
  inpB_mb8[9] = mulB09;
  inpB_mb8[10] = mulB10;
  inpB_mb8[11] = mulB11;
  inpB_mb8[12] = mulB12;
  inpB_mb8[13] = mulB13;
  inpB_mb8[14] = mulB14;
  inpB_mb8[15] = mulB15;
  inpB_mb8[16] = mulB16;
  inpB_mb8[17] = mulB17;
  inpB_mb8[18] = mulB18;
  inpB_mb8[19] = mulB19;

  for (itr = 0; itr < 20; itr++) {
    U64 Yi;
    U64 Bi = inpB_mb8[itr];
    fma52lo_mem(res00, res00, Bi, inpA_mb8, 64 * 0);
    fma52lo_mem(res01, res01, Bi, inpA_mb8, 64 * 1);
    fma52lo_mem(res02, res02, Bi, inpA_mb8, 64 * 2);
    fma52lo_mem(res03, res03, Bi, inpA_mb8, 64 * 3);
    fma52lo_mem(res04, res04, Bi, inpA_mb8, 64 * 4);
    fma52lo_mem(res05, res05, Bi, inpA_mb8, 64 * 5);
    fma52lo_mem(res06, res06, Bi, inpA_mb8, 64 * 6);
    fma52lo_mem(res07, res07, Bi, inpA_mb8, 64 * 7);
    fma52lo_mem(res08, res08, Bi, inpA_mb8, 64 * 8);
    fma52lo_mem(res09, res09, Bi, inpA_mb8, 64 * 9);
    fma52lo_mem(res10, res10, Bi, inpA_mb8, 64 * 10);
    fma52lo_mem(res11, res11, Bi, inpA_mb8, 64 * 11);
    fma52lo_mem(res12, res12, Bi, inpA_mb8, 64 * 12);
    fma52lo_mem(res13, res13, Bi, inpA_mb8, 64 * 13);
    fma52lo_mem(res14, res14, Bi, inpA_mb8, 64 * 14);
    fma52lo_mem(res15, res15, Bi, inpA_mb8, 64 * 15);
    fma52lo_mem(res16, res16, Bi, inpA_mb8, 64 * 16);
    fma52lo_mem(res17, res17, Bi, inpA_mb8, 64 * 17);
    fma52lo_mem(res18, res18, Bi, inpA_mb8, 64 * 18);
    fma52lo_mem(res19, res19, Bi, inpA_mb8, 64 * 19);
    Yi = fma52lo(get_zero64(), res00, K);
    fma52lo_mem(res00, res00, Yi, inpM_mb8, 64 * 0);
    fma52lo_mem(res01, res01, Yi, inpM_mb8, 64 * 1);
    fma52lo_mem(res02, res02, Yi, inpM_mb8, 64 * 2);
    fma52lo_mem(res03, res03, Yi, inpM_mb8, 64 * 3);
    fma52lo_mem(res04, res04, Yi, inpM_mb8, 64 * 4);
    fma52lo_mem(res05, res05, Yi, inpM_mb8, 64 * 5);
    fma52lo_mem(res06, res06, Yi, inpM_mb8, 64 * 6);
    fma52lo_mem(res07, res07, Yi, inpM_mb8, 64 * 7);
    fma52lo_mem(res08, res08, Yi, inpM_mb8, 64 * 8);
    fma52lo_mem(res09, res09, Yi, inpM_mb8, 64 * 9);
    fma52lo_mem(res10, res10, Yi, inpM_mb8, 64 * 10);
    fma52lo_mem(res11, res11, Yi, inpM_mb8, 64 * 11);
    fma52lo_mem(res12, res12, Yi, inpM_mb8, 64 * 12);
    fma52lo_mem(res13, res13, Yi, inpM_mb8, 64 * 13);
    fma52lo_mem(res14, res14, Yi, inpM_mb8, 64 * 14);
    fma52lo_mem(res15, res15, Yi, inpM_mb8, 64 * 15);
    fma52lo_mem(res16, res16, Yi, inpM_mb8, 64 * 16);
    fma52lo_mem(res17, res17, Yi, inpM_mb8, 64 * 17);
    fma52lo_mem(res18, res18, Yi, inpM_mb8, 64 * 18);
    fma52lo_mem(res19, res19, Yi, inpM_mb8, 64 * 19);
    res00 = srli64(res00, DIGIT_SIZE);
    res01 = add64(res01, res00);
    fma52hi_mem(res00, res01, Bi, inpA_mb8, 64 * 0);
    fma52hi_mem(res01, res02, Bi, inpA_mb8, 64 * 1);
    fma52hi_mem(res02, res03, Bi, inpA_mb8, 64 * 2);
    fma52hi_mem(res03, res04, Bi, inpA_mb8, 64 * 3);
    fma52hi_mem(res04, res05, Bi, inpA_mb8, 64 * 4);
    fma52hi_mem(res05, res06, Bi, inpA_mb8, 64 * 5);
    fma52hi_mem(res06, res07, Bi, inpA_mb8, 64 * 6);
    fma52hi_mem(res07, res08, Bi, inpA_mb8, 64 * 7);
    fma52hi_mem(res08, res09, Bi, inpA_mb8, 64 * 8);
    fma52hi_mem(res09, res10, Bi, inpA_mb8, 64 * 9);
    fma52hi_mem(res10, res11, Bi, inpA_mb8, 64 * 10);
    fma52hi_mem(res11, res12, Bi, inpA_mb8, 64 * 11);
    fma52hi_mem(res12, res13, Bi, inpA_mb8, 64 * 12);
    fma52hi_mem(res13, res14, Bi, inpA_mb8, 64 * 13);
    fma52hi_mem(res14, res15, Bi, inpA_mb8, 64 * 14);
    fma52hi_mem(res15, res16, Bi, inpA_mb8, 64 * 15);
    fma52hi_mem(res16, res17, Bi, inpA_mb8, 64 * 16);
    fma52hi_mem(res17, res18, Bi, inpA_mb8, 64 * 17);
    fma52hi_mem(res18, res19, Bi, inpA_mb8, 64 * 18);
    fma52hi_mem(res19, get_zero64(), Bi, inpA_mb8, 64 * 19);
    fma52hi_mem(res00, res00, Yi, inpM_mb8, 64 * 0);
    fma52hi_mem(res01, res01, Yi, inpM_mb8, 64 * 1);
    fma52hi_mem(res02, res02, Yi, inpM_mb8, 64 * 2);
    fma52hi_mem(res03, res03, Yi, inpM_mb8, 64 * 3);
    fma52hi_mem(res04, res04, Yi, inpM_mb8, 64 * 4);
    fma52hi_mem(res05, res05, Yi, inpM_mb8, 64 * 5);
    fma52hi_mem(res06, res06, Yi, inpM_mb8, 64 * 6);
    fma52hi_mem(res07, res07, Yi, inpM_mb8, 64 * 7);
    fma52hi_mem(res08, res08, Yi, inpM_mb8, 64 * 8);
    fma52hi_mem(res09, res09, Yi, inpM_mb8, 64 * 9);
    fma52hi_mem(res10, res10, Yi, inpM_mb8, 64 * 10);
    fma52hi_mem(res11, res11, Yi, inpM_mb8, 64 * 11);
    fma52hi_mem(res12, res12, Yi, inpM_mb8, 64 * 12);
    fma52hi_mem(res13, res13, Yi, inpM_mb8, 64 * 13);
    fma52hi_mem(res14, res14, Yi, inpM_mb8, 64 * 14);
    fma52hi_mem(res15, res15, Yi, inpM_mb8, 64 * 15);
    fma52hi_mem(res16, res16, Yi, inpM_mb8, 64 * 16);
    fma52hi_mem(res17, res17, Yi, inpM_mb8, 64 * 17);
    fma52hi_mem(res18, res18, Yi, inpM_mb8, 64 * 18);
    fma52hi_mem(res19, res19, Yi, inpM_mb8, 64 * 19);
  }
  // Normalization
  {
    U64 T = get_zero64();
    //U64 MASK = set64(DIGIT_MASK);
    T = srli64(res00, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 0, res00);
    res01 = add64(res01, T);
    T = srli64(res01, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 1, res01);
    res02 = add64(res02, T);
    T = srli64(res02, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 2, res02);
    res03 = add64(res03, T);
    T = srli64(res03, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 3, res03);
    res04 = add64(res04, T);
    T = srli64(res04, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 4, res04);
    res05 = add64(res05, T);
    T = srli64(res05, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 5, res05);
    res06 = add64(res06, T);
    T = srli64(res06, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 6, res06);
    res07 = add64(res07, T);
    T = srli64(res07, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 7, res07);
    res08 = add64(res08, T);
    T = srli64(res08, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 8, res08);
    res09 = add64(res09, T);
    T = srli64(res09, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 9, res09);
    res10 = add64(res10, T);
    T = srli64(res10, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 10, res10);
    res11 = add64(res11, T);
    T = srli64(res11, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 11, res11);
    res12 = add64(res12, T);
    T = srli64(res12, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 12, res12);
    res13 = add64(res13, T);
    T = srli64(res13, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 13, res13);
    res14 = add64(res14, T);
    T = srli64(res14, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 14, res14);
    res15 = add64(res15, T);
    T = srli64(res15, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 15, res15);
    res16 = add64(res16, T);
    T = srli64(res16, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 16, res16);
    res17 = add64(res17, T);
    T = srli64(res17, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 17, res17);
    res18 = add64(res18, T);
    T = srli64(res18, DIGIT_SIZE);
    storeu64(out_mb8 + 8 * 18, res18);
    res19 = add64(res19, T);
    storeu64(out_mb8 + 8 * 19, res19);
  }
}

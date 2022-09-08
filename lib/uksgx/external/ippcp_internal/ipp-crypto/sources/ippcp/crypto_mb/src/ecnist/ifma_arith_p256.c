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

/* Constants */
#define LEN52    NUMBER_OF_DIGITS(256,DIGIT_SIZE)

/*
// prime256 = 2^256 - 2^224 + 2^192 + 2^96 -1
// in 2^52 radix
*/
__ALIGN64 static const int64u p256_mb[LEN52][8] = {
   {0xfffffffffffff, 0xfffffffffffff, 0xfffffffffffff, 0xfffffffffffff, 0xfffffffffffff, 0xfffffffffffff, 0xfffffffffffff, 0xfffffffffffff},
   {0x00fffffffffff, 0x00fffffffffff, 0x00fffffffffff, 0x00fffffffffff, 0x00fffffffffff, 0x00fffffffffff, 0x00fffffffffff, 0x00fffffffffff}, 
   {0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000}, 
   {0x0001000000000, 0x0001000000000, 0x0001000000000, 0x0001000000000, 0x0001000000000, 0x0001000000000, 0x0001000000000, 0x0001000000000}, 
   {0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000} 
};

__ALIGN64 static const int64u p256x2_mb[LEN52][8] = {
   {0xffffffffffffe, 0xffffffffffffe, 0xffffffffffffe, 0xffffffffffffe, 0xffffffffffffe, 0xffffffffffffe, 0xffffffffffffe, 0xffffffffffffe},
   {0x01fffffffffff, 0x01fffffffffff, 0x01fffffffffff, 0x01fffffffffff, 0x01fffffffffff, 0x01fffffffffff, 0x01fffffffffff, 0x01fffffffffff}, 
   {0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000, 0x0000000000000}, 
   {0x0002000000000, 0x0002000000000, 0x0002000000000, 0x0002000000000, 0x0002000000000, 0x0002000000000, 0x0002000000000, 0x0002000000000}, 
   {0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000} 
};

/*
// Note that
// k0 = -(1/p256 mod 2^DIGIT_SIZE) equals 1
// The implementation takes this fact into account
*/

/* to Montgomery conversion constant
// rr = 2^((5*DIGIT_SIZE)*2) mod p256
*/
__ALIGN64 static const int64u p256_rr_mb[5][8] = {
{0x0000000000000300,0x0000000000000300,0x0000000000000300,0x0000000000000300,0x0000000000000300,0x0000000000000300,0x0000000000000300,0x0000000000000300},
{0x000ffffffff00000,0x000ffffffff00000,0x000ffffffff00000,0x000ffffffff00000,0x000ffffffff00000,0x000ffffffff00000,0x000ffffffff00000,0x000ffffffff00000},
{0x000ffffefffffffb,0x000ffffefffffffb,0x000ffffefffffffb,0x000ffffefffffffb,0x000ffffefffffffb,0x000ffffefffffffb,0x000ffffefffffffb,0x000ffffefffffffb},
{0x000fdfffffffffff,0x000fdfffffffffff,0x000fdfffffffffff,0x000fdfffffffffff,0x000fdfffffffffff,0x000fdfffffffffff,0x000fdfffffffffff,0x000fdfffffffffff},
{0x0000000004ffffff,0x0000000004ffffff,0x0000000004ffffff,0x0000000004ffffff,0x0000000004ffffff,0x0000000004ffffff,0x0000000004ffffff,0x0000000004ffffff}
};



/* other constants */
__ALIGN64 static const int64u VDIGIT_MASK_[8] =
        {DIGIT_MASK, DIGIT_MASK, DIGIT_MASK, DIGIT_MASK,
         DIGIT_MASK, DIGIT_MASK, DIGIT_MASK, DIGIT_MASK};
#define VDIGIT_MASK loadu64(VDIGIT_MASK_)

#define P256_PRIME_TOP_ 0xFFFFFFFF0000LL
__ALIGN64 static const int64u VP256_PRIME_TOP_[8] =
        {P256_PRIME_TOP_, P256_PRIME_TOP_, P256_PRIME_TOP_, P256_PRIME_TOP_,
         P256_PRIME_TOP_, P256_PRIME_TOP_, P256_PRIME_TOP_, P256_PRIME_TOP_};
#define VP256_PRIME_TOP loadu64(VP256_PRIME_TOP_)


/* Round operations */

#define ROUND_MUL_SRC(I, J, S_LO, R_LO, S_HI, R_HI) \
    R_LO = fma52lo(S_LO, va[I], vb[J]); \
    R_HI = fma52hi(S_HI, va[I], vb[J]);

#define ROUND_MUL(I, J, M0, M1) \
    ROUND_MUL_SRC(I, J, M0, M0, M1, M1)

/* Reduction round for p256 prime */

// Note that k == 1 for p256 curve
#ifdef IFMA_RED

#define MUL_ADD_P256(u, res0, res1, res2, res3, res4, res5) \
  { \
    U64 u = and64_const(res0, DIGIT_MASK); /* k == 1 */ \
    res0 = fma52lo(res0, u, set64((1ULL<<52) - 1)); \
    res1 = fma52hi(res1, u, set64((1ULL<<52) - 1)); \
    res1 = add64(res1, srli64(res0, 52)); \
    res1 = fma52lo(res1, u, set64((1ULL<<44) - 1)); \
    res2 = fma52hi(res2, u, set64((1ULL<<44) - 1)); \
    res3 = fma52lo(res3, u, set64(1ULL<<36)); \
    res4 = fma52hi(res4, u, set64(1ULL<<36)); \
    res4 = fma52lo(res4, u, set64(0xFFFFFFFF0000LL)); \
    res5 = fma52hi(res5, u, set64(0xFFFFFFFF0000LL)); \
  }

#else // IFMA_RED

#define MUL_ADD_P256(u, res0, res1, res2, res3, res4, res5) \
  { \
    /* a * ( 2^96 - 1 ) = -a + a * 2^96 = -a + a * 2^{52+44} */ \
    U64 u = and64(res0, VDIGIT_MASK); /* k == 1 */ \
    /*res0 = sub64(res0, u);*/ /* Zero out low 52 bits */ \
    res1 = add64(res1, srli64(res0, DIGIT_SIZE)); /* Carry propagation */ \
    res1 = add64(res1, and64(slli64(u, 44), VDIGIT_MASK)); \
    res2 = add64(res2, srli64(u, DIGIT_SIZE - 44)); \
    /* ( a * 2^{36} ) * 2^{52*3} */ \
    res3 = add64(res3, and64(slli64(u, 36), VDIGIT_MASK)); \
    res4 = add64(res4, srli64(u, DIGIT_SIZE - 36)); \
    /* ( a * (2^{48} - 1) - a * (2^{16} - 1) ) * 2^{52*4} = */ \
    /* ( a * 2^{48} - a * 2^{16} ) * 2^{52*4} */ \
    res4 = fma52lo(res4, u, VP256_PRIME_TOP); \
    res5 = fma52hi(res5, u, VP256_PRIME_TOP); \
  }

#endif // IFMA_RED

/*=====================================================================

 Specialized single and dual operations in p256 - sqr & mul

=====================================================================*/

void MB_FUNC_NAME(ifma_ams52_p256_)(U64 r[], const U64 va[])
{
  const U64* vb = va;

  U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
  r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

  // Calculate full square
  ROUND_MUL(0, 1, r1, r2)
  ROUND_MUL(0, 2, r2, r3)
  ROUND_MUL(0, 3, r3, r4)
  ROUND_MUL(0, 4, r4, r5)
  ROUND_MUL(1, 4, r5, r6)
  ROUND_MUL(2, 4, r6, r7)
  ROUND_MUL(3, 4, r7, r8)

  ROUND_MUL(1, 2, r3, r4)
  ROUND_MUL(1, 3, r4, r5)
  ROUND_MUL(2, 3, r5, r6)

  r1 = add64(r1, r1);
  r2 = add64(r2, r2);
  r3 = add64(r3, r3);
  r4 = add64(r4, r4);
  r5 = add64(r5, r5);
  r6 = add64(r6, r6);
  r7 = add64(r7, r7);
  r8 = add64(r8, r8);

  ROUND_MUL(0, 0, r0, r1)
  ROUND_MUL(1, 1, r2, r3)
  ROUND_MUL(2, 2, r4, r5)
  ROUND_MUL(3, 3, r6, r7)
  ROUND_MUL(4, 4, r8, r9)

  // Reduction
  MUL_ADD_P256(u0, r0, r1, r2, r3, r4, r5);
  MUL_ADD_P256(u1, r1, r2, r3, r4, r5, r6);
  MUL_ADD_P256(u2, r2, r3, r4, r5, r6, r7);
  MUL_ADD_P256(u3, r3, r4, r5, r6, r7, r8);
  MUL_ADD_P256(u4, r4, r5, r6, r7, r8, r9);

  // normalization
  NORM_LSHIFTR(r, 5, 6)
  NORM_LSHIFTR(r, 6, 7)
  NORM_LSHIFTR(r, 7, 8)
  NORM_LSHIFTR(r, 8, 9)

   r[0] = r5;
   r[1] = r6;
   r[2] = r7;
   r[3] = r8;
   r[4] = r9;
}

void MB_FUNC_NAME(ifma_amm52_p256_)(U64 r[], const U64 va[], const U64 vb[])
{
  U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
  r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

  ROUND_MUL(4, 4, r8, r9)
  ROUND_MUL(3, 0, r3, r4)
  ROUND_MUL(1, 2, r3, r4)
  ROUND_MUL(0, 3, r3, r4)
  ROUND_MUL(2, 1, r3, r4)
  ROUND_MUL(2, 2, r4, r5)
  ROUND_MUL(0, 4, r4, r5)
  ROUND_MUL(1, 3, r4, r5)
  ROUND_MUL(3, 1, r4, r5)
  ROUND_MUL(4, 0, r4, r5)
  ROUND_MUL(1, 4, r5, r6)
  ROUND_MUL(2, 3, r5, r6)
  ROUND_MUL(3, 2, r5, r6)
  ROUND_MUL(4, 1, r5, r6)
  ROUND_MUL(2, 4, r6, r7)
  ROUND_MUL(3, 3, r6, r7)
  ROUND_MUL(4, 2, r6, r7)
  ROUND_MUL(0, 0, r0, r1)
  ROUND_MUL(0, 1, r1, r2)
  ROUND_MUL(0, 2, r2, r3)
  ROUND_MUL(1, 0, r1, r2)
  ROUND_MUL(1, 1, r2, r3)
  ROUND_MUL(2, 0, r2, r3)
  ROUND_MUL(3, 4, r7, r8)
  ROUND_MUL(4, 3, r7, r8)

  // Reduction
  MUL_ADD_P256(u0, r0, r1, r2, r3, r4, r5);
  MUL_ADD_P256(u1, r1, r2, r3, r4, r5, r6);
  MUL_ADD_P256(u2, r2, r3, r4, r5, r6, r7);
  MUL_ADD_P256(u3, r3, r4, r5, r6, r7, r8);
  MUL_ADD_P256(u4, r4, r5, r6, r7, r8, r9);

  // normalization
  NORM_LSHIFTR(r, 5, 6)
  NORM_LSHIFTR(r, 6, 7)
  NORM_LSHIFTR(r, 7, 8)
  NORM_LSHIFTR(r, 8, 9)

   r[0] = r5;
   r[1] = r6;
   r[2] = r7;
   r[3] = r8;
   r[4] = r9;
}

void MB_FUNC_NAME(ifma_ams52_p256_dual_)(U64 r0[], U64 r1[],
                                 const U64 inp0[], const U64 inp1[])
{
  U64 *va; // = (U64 *)inp0;
  U64 *vb; // = (U64 *)inp0;

  U64 r00, r01, r02, r03, r04, r05, r06, r07, r08, r09;
  U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
  r00 = r01 = r02 = r03 = r04 = r05 = r06 = r07 = r08 = r09 = get_zero64();
  r10 = r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19 = get_zero64();

  /// Calculate full square
  va = vb = (U64 *)inp0;
  // Multiplication
  ROUND_MUL(0, 1, r01, r02)
  ROUND_MUL(0, 2, r02, r03)
  ROUND_MUL(0, 3, r03, r04)
  ROUND_MUL(0, 4, r04, r05)
  ROUND_MUL(1, 4, r05, r06)
  ROUND_MUL(2, 4, r06, r07)
  ROUND_MUL(3, 4, r07, r08)
  ROUND_MUL(1, 2, r03, r04)
  ROUND_MUL(1, 3, r04, r05)
  ROUND_MUL(2, 3, r05, r06)
  // Doubling
  r01 = add64(r01, r01);
  r02 = add64(r02, r02);
  r03 = add64(r03, r03);
  r04 = add64(r04, r04);
  r05 = add64(r05, r05);
  r06 = add64(r06, r06);
  r07 = add64(r07, r07);
  r08 = add64(r08, r08);
  // Adding square
  ROUND_MUL(0, 0, r00, r01)
  ROUND_MUL(1, 1, r02, r03)
  ROUND_MUL(2, 2, r04, r05)
  ROUND_MUL(3, 3, r06, r07)
  ROUND_MUL(4, 4, r08, r09)

  /// Calculate full square
  va = vb = (U64 *)inp1;
  // Multiplication
  ROUND_MUL(0, 1, r11, r12)
  ROUND_MUL(0, 2, r12, r13)
  ROUND_MUL(0, 3, r13, r14)
  ROUND_MUL(0, 4, r14, r15)
  ROUND_MUL(1, 4, r15, r16)
  ROUND_MUL(2, 4, r16, r17)
  ROUND_MUL(3, 4, r17, r18)
  ROUND_MUL(1, 2, r13, r14)
  ROUND_MUL(1, 3, r14, r15)
  ROUND_MUL(2, 3, r15, r16)
  // Doubling
  r11 = add64(r11, r11);
  r12 = add64(r12, r12);
  r13 = add64(r13, r13);
  r14 = add64(r14, r14);
  r15 = add64(r15, r15);
  r16 = add64(r16, r16);
  r17 = add64(r17, r17);
  r18 = add64(r18, r18);
  // Adding square
  ROUND_MUL(0, 0, r10, r11)
  ROUND_MUL(1, 1, r12, r13)
  ROUND_MUL(2, 2, r14, r15)
  ROUND_MUL(3, 3, r16, r17)
  ROUND_MUL(4, 4, r18, r19)

  // Reduction res0
  MUL_ADD_P256(u0, r00, r01, r02, r03, r04, r05);
  MUL_ADD_P256(u1, r01, r02, r03, r04, r05, r06);
  MUL_ADD_P256(u2, r02, r03, r04, r05, r06, r07);
  MUL_ADD_P256(u3, r03, r04, r05, r06, r07, r08);
  MUL_ADD_P256(u4, r04, r05, r06, r07, r08, r09);

  // Reduction res1
  MUL_ADD_P256(u0, r10, r11, r12, r13, r14, r15);
  MUL_ADD_P256(u1, r11, r12, r13, r14, r15, r16);
  MUL_ADD_P256(u2, r12, r13, r14, r15, r16, r17);
  MUL_ADD_P256(u3, r13, r14, r15, r16, r17, r18);
  MUL_ADD_P256(u4, r14, r15, r16, r17, r18, r19);

  // normalization res0
  NORM_LSHIFTR(r0, 5, 6)
  NORM_LSHIFTR(r0, 6, 7)
  NORM_LSHIFTR(r0, 7, 8)
  NORM_LSHIFTR(r0, 8, 9)

   r0[0] = r05;
   r0[1] = r06;
   r0[2] = r07;
   r0[3] = r08;
   r0[4] = r09;

  // normalization res1
  NORM_LSHIFTR(r1, 5, 6)
  NORM_LSHIFTR(r1, 6, 7)
  NORM_LSHIFTR(r1, 7, 8)
  NORM_LSHIFTR(r1, 8, 9)

   r1[0] = r15;
   r1[1] = r16;
   r1[2] = r17;
   r1[3] = r18;
   r1[4] = r19;
}

void MB_FUNC_NAME(ifma_amm52_p256_dual_)(U64 r0[], U64 r1[],
                                   const U64 inp0A[], const U64 inp0B[],
                                   const U64 inp1A[], const U64 inp1B[])
{
  U64 *va, *vb;

  U64 r00, r01, r02, r03, r04, r05, r06, r07, r08, r09;
  U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
  r00 = r01 = r02 = r03 = r04 = r05 = r06 = r07 = r08 = r09 = get_zero64();
  r10 = r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19 = get_zero64();

  // 5x5 multiplication
  va = (U64 *)inp0A;
  vb = (U64 *)inp0B;
  ROUND_MUL(4, 4, r08, r09)
  ROUND_MUL(3, 0, r03, r04)
  ROUND_MUL(1, 2, r03, r04)
  ROUND_MUL(0, 3, r03, r04)
  ROUND_MUL(2, 1, r03, r04)
  ROUND_MUL(2, 2, r04, r05)
  ROUND_MUL(0, 4, r04, r05)
  ROUND_MUL(1, 3, r04, r05)
  ROUND_MUL(3, 1, r04, r05)
  ROUND_MUL(4, 0, r04, r05)
  ROUND_MUL(1, 4, r05, r06)
  ROUND_MUL(2, 3, r05, r06)
  ROUND_MUL(3, 2, r05, r06)
  ROUND_MUL(4, 1, r05, r06)
  ROUND_MUL(2, 4, r06, r07)
  ROUND_MUL(3, 3, r06, r07)
  ROUND_MUL(4, 2, r06, r07)
  ROUND_MUL(0, 0, r00, r01)
  ROUND_MUL(0, 1, r01, r02)
  ROUND_MUL(0, 2, r02, r03)
  ROUND_MUL(1, 0, r01, r02)
  ROUND_MUL(1, 1, r02, r03)
  ROUND_MUL(2, 0, r02, r03)
  ROUND_MUL(3, 4, r07, r08)
  ROUND_MUL(4, 3, r07, r08)

  // 5x5 multiplication
  va = (U64 *)inp1A;
  vb = (U64 *)inp1B;
  ROUND_MUL(4, 4, r18, r19)
  ROUND_MUL(3, 0, r13, r14)
  ROUND_MUL(1, 2, r13, r14)
  ROUND_MUL(0, 3, r13, r14)
  ROUND_MUL(2, 1, r13, r14)
  ROUND_MUL(2, 2, r14, r15)
  ROUND_MUL(0, 4, r14, r15)
  ROUND_MUL(1, 3, r14, r15)
  ROUND_MUL(3, 1, r14, r15)
  ROUND_MUL(4, 0, r14, r15)
  ROUND_MUL(1, 4, r15, r16)
  ROUND_MUL(2, 3, r15, r16)
  ROUND_MUL(3, 2, r15, r16)
  ROUND_MUL(4, 1, r15, r16)
  ROUND_MUL(2, 4, r16, r17)
  ROUND_MUL(3, 3, r16, r17)
  ROUND_MUL(4, 2, r16, r17)
  ROUND_MUL(0, 0, r10, r11)
  ROUND_MUL(0, 1, r11, r12)
  ROUND_MUL(0, 2, r12, r13)
  ROUND_MUL(1, 0, r11, r12)
  ROUND_MUL(1, 1, r12, r13)
  ROUND_MUL(2, 0, r12, r13)
  ROUND_MUL(3, 4, r17, r18)
  ROUND_MUL(4, 3, r17, r18)

  // Reduction for input 0
  MUL_ADD_P256(u0, r00, r01, r02, r03, r04, r05)
  MUL_ADD_P256(u1, r01, r02, r03, r04, r05, r06)
  MUL_ADD_P256(u2, r02, r03, r04, r05, r06, r07)
  MUL_ADD_P256(u3, r03, r04, r05, r06, r07, r08)
  MUL_ADD_P256(u4, r04, r05, r06, r07, r08, r09)

  // Reduction for input 1
  MUL_ADD_P256(u0, r10, r11, r12, r13, r14, r15)
  MUL_ADD_P256(u1, r11, r12, r13, r14, r15, r16)
  MUL_ADD_P256(u2, r12, r13, r14, r15, r16, r17)
  MUL_ADD_P256(u3, r13, r14, r15, r16, r17, r18)
  MUL_ADD_P256(u4, r14, r15, r16, r17, r18, r19)

  // normalization res0
  NORM_LSHIFTR(r0, 5, 6)
  NORM_LSHIFTR(r0, 6, 7)
  NORM_LSHIFTR(r0, 7, 8)
  NORM_LSHIFTR(r0, 8, 9)

   r0[0] = r05;
   r0[1] = r06;
   r0[2] = r07;
   r0[3] = r08;
   r0[4] = r09;

  // normalization res1
  NORM_LSHIFTR(r1, 5, 6)
  NORM_LSHIFTR(r1, 6, 7)
  NORM_LSHIFTR(r1, 7, 8)
  NORM_LSHIFTR(r1, 8, 9)

   r1[0] = r15;
   r1[1] = r16;
   r1[2] = r17;
   r1[3] = r18;
   r1[4] = r19;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LEN52    NUMBER_OF_DIGITS(256,DIGIT_SIZE)


void MB_FUNC_NAME(ifma_tomont52_p256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_p256_)(r, a, (U64*)p256_rr_mb);
}

void MB_FUNC_NAME(ifma_frommont52_p256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_p256_)(r, a, (U64*)ones);
}

/*
// computes r = 1/z = z^(p256-2) mod p256
//
// note: z in in Montgomery domain (as soon mul() and sqr() below are amm-functions
//       r in Montgomery domain too
*/
#define sqr_p256    MB_FUNC_NAME(ifma_ams52_p256_)
#define mul_p256    MB_FUNC_NAME(ifma_amm52_p256_)

void MB_FUNC_NAME(ifma_aminv52_p256_)(U64 r[], const U64 z[])
{
   __ALIGN64 U64 tmp1[LEN52];
   __ALIGN64 U64 tmp2[LEN52];

   /* each eI holds z^(2^I-1) */
   __ALIGN64 U64 e2[LEN52];
   __ALIGN64 U64 e4[LEN52];
   __ALIGN64 U64 e8[LEN52];
   __ALIGN64 U64 e16[LEN52];
   __ALIGN64 U64 e32[LEN52];
   __ALIGN64 U64 e64[LEN52];

   int i;

   /* tmp1 = z^(2^1) */
   sqr_p256(tmp1, z);
   /* e2 = tmp1 = tmp1 * z = z^(2^2 - 2^0) */
   mul_p256(tmp1, tmp1, z);
   MB_FUNC_NAME(mov_FE256_)(e2, tmp1);

   /* tmp1 = tmp1^2 = z^(2^2 - 2^0)*2 = z^(2^3 - 2^1) */
   sqr_p256(tmp1, tmp1);
   /* tmp1 = tmp1^2 = z^(2^3 - 2^1)*2 = z^(2^4 - 2^2) */
   sqr_p256(tmp1, tmp1);
   /* e4 = tmp1 = tmp1*e2 = z^(2^4 - 2^2) * z^(2^2 - 2^0) = z^(2^4 - 2^2 + 2^2 - 2^0) = z^(2^4 - 2^0)*/
   mul_p256(tmp1, tmp1, e2);
   MB_FUNC_NAME(mov_FE256_)(e4, tmp1);

   /* tmp1 = tmp1^2 = z^(2^4 - 2^0)*2 = z^(2^5 - 2^1) */
   sqr_p256(tmp1, tmp1);
   /* tmp1 = tmp1^2 = z^(2^5 - 2^1)*2 = z^(2^6 - 2^2) */
   sqr_p256(tmp1, tmp1);
   /* tmp1 = tmp1^2 = z^(2^6 - 2^2)*2 = z^(2^7 - 2^3) */
   sqr_p256(tmp1, tmp1);
   /* tmp1 = tmp1^2 = z^(2^7 - 2^3)*2 = z^(2^8 - 2^4) */
   sqr_p256(tmp1, tmp1);
   /* e8 = tmp1 = tmp1*e4 = z^(2^8 - 2^4) * z^(2^4 - 2^0) = z^(2^8 - 2^4 + 2^4 - 2^0) = z^(2^8 - 2^0)*/
   mul_p256(tmp1, tmp1, e4);
   MB_FUNC_NAME(mov_FE256_)(e8, tmp1);

   /* tmp1 = tmp1^(2^8) = z^(2^8 - 2^0)*2^8 = z^(2^16 - 2^8) */
   for(i=0; i<8;i++) sqr_p256(tmp1, tmp1);
   /* e16 = tmp1 = tmp1*e8 = z^(2^16 - 2^8) * z^(2^8 - 2^0) = z^(2^16 - 2^8 + 2^8 - 2^0) = z^(2^16 - 2^0)*/
   mul_p256(tmp1, tmp1, e8);
   MB_FUNC_NAME(mov_FE256_)(e16, tmp1);

   /* tmp1 = tmp1^(2^16) = z^(2^16 - 2^0)*2^16 = z^(2^32 - 2^16) */
   for(i=0; i<16; i++) sqr_p256(tmp1, tmp1);
   /* e32 = tmp1 = tmp1*e16 = z^(2^32 - 2^16) * z^(2^16 - 2^0) = z^(2^32 - 2^16 + 2^16 - 2^0) = z^(2^32 - 2^0)*/
   mul_p256(tmp1, tmp1, e16);
   MB_FUNC_NAME(mov_FE256_)(e32, tmp1);

   /* e64 = tmp1 = tmp1^(2^32) = z^(2^32 - 2^0)*2^32 = z^(2^64 - 2^32) */
   for(i=0; i<32; i++) sqr_p256(tmp1, tmp1);
   MB_FUNC_NAME(mov_FE256_)(e64, tmp1);
   /* tmp1 = tmp1*z = z^(2^64 - 2^32) * z = z^(2^64 - 2^32 + 2^0)*/
   mul_p256(tmp1, tmp1, z);

   /* tmp1 = tmp1^(2^192) = z^(2^64 - 2^32 + 2^0)*2^192 = z^(2^256 - 2^224 + 2^192) */
   for(i=0; i<192; i++) sqr_p256(tmp1, tmp1);

   /* tmp2 = e64*e32 = z^(2^64 - 2^32) * z^(2^32 - 2^0) = z^(2^64 - 2^32 + 2^32 - 2^0) = z^(2^64 - 2^0)*/
   mul_p256(tmp2, e64, e32);

   /* tmp2 = tmp2^(2^16) = z^(2^64 - 2^0)*2^16 = z^(2^80 - 2^16) */
   for(i=0; i<16; i++) sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2*e16 = z^(2^80 - 2^16) * z^(2^16 - 2^0) = z^(2^80 - 2^16 + 2^16 - 2^0) = z^(2^80 - 2^0)*/
   mul_p256(tmp2, tmp2, e16);

   /* tmp2 = tmp2^(2^8) = z^(2^80 - 2^0)*2^8 = z^(2^88 - 2^8) */
   for(i=0; i<8; i++) sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2*e8 = z^(2^88 - 2^8) * z^(2^8 - 2^0) = z^(2^88 - 2^8 + 2^8 - 2^0) = z^(2^88 - 2^0)*/
   mul_p256(tmp2, tmp2, e8);

   /* tmp2 = tmp2^(2^4) = z^(2^88 - 2^0)*2^4 = z^(2^92 - 2^4) */
   for(i=0; i<4; i++) sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2*e4 = z^(2^92 - 2^4) * z^(2^4 - 2^0) = z^(2^92 - 2^4 + 2^4 - 2^0) = z^(2^92 - 2^0)*/
   mul_p256(tmp2, tmp2, e4);

   /* tmp2 = tmp2^2 = z^(2^92 - 2^0)*2^1 = z^(2^93 - 2^1) */
   sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2^2 = z^(2^93 - 2^1)*2^1 = z^(2^94 - 2^2) */
   sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2*e2 = z^(2^94 - 2^2) * z^(2^2 - 2^0) = z^(2^94 - 2^2 + 2^2 - 2^0) = z^(2^94 - 2^0)*/
   mul_p256(tmp2, tmp2, e2);
   /* tmp2 = tmp2^2 = z^(2^94 - 2^0)*2^1 = z^(2^95 - 2^1) */
   sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2^2 = z^(2^95 - 2^1)*2^1 = z^(2^96 - 2^2) */
   sqr_p256(tmp2, tmp2);
   /* tmp2 = tmp2*z = z^(2^96 - 2^2) * z = z^(2^96 - 2^2 + 1) = z^(2^96 - 3) */
   mul_p256(tmp2, tmp2, z);

   /* r = tmp1*tmp2 = z^(2^256 - 2^224 + 2^192) * z^(2^96 - 3) = z^(2^256 - 2^224 + 2^192 + 2^96 - 3) */
   mul_p256(r, tmp1, tmp2);
}


/*=====================================================================

 Specialized single operations in n256 - add, sub & neg

=====================================================================*/

void MB_FUNC_NAME(ifma_add52_p256_)(U64 r[], const U64 a[], const U64 b[])
{
   MB_FUNC_NAME(ifma_add52x5_)(r, a, b, (U64*)p256x2_mb);
}

void MB_FUNC_NAME(ifma_sub52_p256_)(U64 r[], const U64 a[], const U64 b[])
{
   MB_FUNC_NAME(ifma_sub52x5_)(r, a, b, (U64*)p256x2_mb);
}

void MB_FUNC_NAME(ifma_neg52_p256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_neg52x5_)(r, a, (U64*)p256x2_mb);
}

void MB_FUNC_NAME(ifma_double52_p256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_add52x5_)(r, a, a, (U64*)p256x2_mb);
}

void MB_FUNC_NAME(ifma_tripple52_p256_)(U64 r[], const U64 a[])
{
   U64 t[LEN52];
   MB_FUNC_NAME(ifma_add52x5_)(t, a, a, (U64*)p256x2_mb);
   MB_FUNC_NAME(ifma_add52x5_)(r, t, a, (U64*)p256x2_mb);
}

void MB_FUNC_NAME(ifma_half52_p256_)(U64 r[], const U64 a[])
{
   U64 one = set1(1);
   U64 base = set1(DIGIT_BASE);

   /* res = a + is_odd(a)? p256 : 0 */
   U64 mask = sub64(get_zero64(), and64(a[0], one));
   U64 t0 = add64(a[0], and64(((U64*)p256_mb)[0], mask));
   U64 t1 = add64(a[1], and64(((U64*)p256_mb)[1], mask));
   U64 t2 = add64(a[2], and64(((U64*)p256_mb)[2], mask));
   U64 t3 = add64(a[3], and64(((U64*)p256_mb)[3], mask));
   U64 t4 = add64(a[4], and64(((U64*)p256_mb)[4], mask));

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

   t4 = srli64(t4, 1);

   /* normalize t0, t1, t2, t3, t4 */
   NORM_LSHIFTR(t, 0,1)
   NORM_LSHIFTR(t, 1,2)
   NORM_LSHIFTR(t, 2,3)
   NORM_LSHIFTR(t, 3,4)

   r[0] = t0;
   r[1] = t1;
   r[2] = t2;
   r[3] = t3;
   r[4] = t4;
}

__mb_mask MB_FUNC_NAME(ifma_cmp_lt_p256_)(const U64 a[])
{
   return MB_FUNC_NAME(cmp_lt_FE256_)(a,(const U64 (*))p256_mb);
}

__mb_mask MB_FUNC_NAME(ifma_check_range_p256_)(const U64 A[])
{
   __mb_mask
   mask = MB_FUNC_NAME(is_zero_FE256_)(A);
   mask |= ~MB_FUNC_NAME(ifma_cmp_lt_p256_)(A);

   return mask;
}


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

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>
#include <internal/common/ifma_cvt52.h>
#include <internal/rsa/ifma_rsa_arith.h>

#ifndef __GNUC__
#pragma warning(disable:4013)
#endif

#define MASK47 ((1ULL << (255 - 52 * 4)) - 1)

__ALIGN64 static const int64u MASK47_[8] = {MASK47, MASK47, MASK47, MASK47,
                                     MASK47, MASK47, MASK47, MASK47};
__ALIGN64 static const int64u MOD_2_255_[8] = {19, 19, 19, 19, 19, 19, 19, 19};
__ALIGN64 static const int64u MOD_2_260_[8] = {19*32, 19*32, 19*32, 19*32,
                                        19*32, 19*32, 19*32, 19*32};
#define MASK_47 loadu64(MASK47_)
#define MOD_2_255 loadu64(MOD_2_255_)
#define MOD_2_260 loadu64(MOD_2_260_)

#define ROUND_MUL_SRC(I, J, S_LO, R_LO, S_HI, R_HI) \
    R_LO = fma52lo(S_LO, va[I], vb[J]); \
    R_HI = fma52hi(S_HI, va[I], vb[J]);

#define ROUND_MUL(I, J, M0, M1) \
    ROUND_MUL_SRC(I, J, M0, M0, M1, M1)

#define REDUCE_ROUND(R0, R1, R5) \
    r##R0 = fma52lo(r##R0, r##R5, MOD_2_260); \
    r##R1 = fma52lo( fma52hi(r##R1, r##R5, MOD_2_260), \
        srli64(r##R5, 52), MOD_2_260);

#define NORM(I, J) \
    r##J = add64(r##J, srli64(r##I, 52)); \
    r##I = and64_const(r##I, (1ULL << 52) - 1);

////////////////////////////////////////////////////////////

__INLINE void ed25519_mul(U64 out[], const U64 a[], const U64 b[]) {
    U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

    U64 *va = (U64*) a;
    U64 *vb = (U64*) b;
    U64 *vr = (U64*) out;

    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

    // Full multiplication
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

    r4 = fma52lo(r4, r9, MOD_2_260);
    r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);
    r4 = and64(r4, MASK_47);

    REDUCE_ROUND(0, 1, 5);
    REDUCE_ROUND(1, 2, 6);
    REDUCE_ROUND(2, 3, 7);
    REDUCE_ROUND(3, 4, 8);

    // Normalize result
    NORM(0,1)
    NORM(1,2)
    NORM(2,3)
    NORM(3,4)

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

/* SQR
c=0  (0,0)  
c=1  (0,1)  
c=2  (0,2)  (1,1)  
c=3  (0,3)  (1,2)  
c=4  (0,4)  (1,3)  (2,2)  
c=5  (1,4)  (2,3)  
c=6  (2,4)  (3,3)  
c=7  (3,4)  
c=8  (4,4)
*/

__INLINE void ed25519_sqr(U64 out[], const U64 a[]) {
    U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

    U64 *va = (U64*) a;
    U64 *vb = (U64*) a;
    U64 *vr = (U64*) out;

    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

    // Square
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

    // Reduce r4 upper bits
    r4 = fma52lo(r4, r9, MOD_2_260);
    r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);
    r4 = and64(r4, MASK_47);

    REDUCE_ROUND(0, 1, 5);
    REDUCE_ROUND(1, 2, 6);
    REDUCE_ROUND(2, 3, 7);
    REDUCE_ROUND(3, 4, 8);

    // Normalize result
    NORM(0,1)
    NORM(1,2)
    NORM(2,3)
    NORM(3,4)

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

#define ROUND_MUL_SRC_A(I, J, S_LO, R_LO, S_HI, R_HI) \
    R_LO = fma52lo(S_LO, a##I, a##J); \
    R_HI = fma52hi(S_HI, a##I, a##J);

#define ROUND_MUL_A(I, J, M0, M1) \
    ROUND_MUL_SRC_A(I, J, M0, M0, M1, M1)

#define NORM(I, J) \
    r##J = add64(r##J, srli64(r##I, 52)); \
    r##I = and64_const(r##I, (1ULL << 52) - 1);


void MB_FUNC_NAME(ed25519_sqr_latency_)(U64 out[], const U64 a[], int count) {
    U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
    U64 a0, a1, a2, a3, a4;
    U64 r4_1;
    int i;

    U64 *vr = (U64*) out;

    a0 = a[0];
    a1 = a[1];
    a2 = a[2];
    a3 = a[3];
    a4 = a[4];
    for (i = 0; i < count; ++i)
    {
        r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();
        r4_1 = get_zero64();

        // Square
        ROUND_MUL_A(0, 1, r1, r2)
        ROUND_MUL_A(0, 2, r2, r3)
        ROUND_MUL_A(0, 3, r3, r4_1)
        ROUND_MUL_A(0, 4, r4_1, r5)
        ROUND_MUL_A(1, 4, r5, r6)
        ROUND_MUL_A(2, 4, r6, r7)
        ROUND_MUL_A(3, 4, r7, r8)

        ROUND_MUL_A(1, 2, r3, r4)
        ROUND_MUL_A(1, 3, r4, r5)
        ROUND_MUL_A(2, 3, r5, r6)

        r1 = add64(r1, r1);
        r2 = add64(r2, r2);
        r3 = add64(r3, r3);

        r4 = add64(r4, r4_1);
        r4 = add64(r4, r4);

        r5 = add64(r5, r5);
        r6 = add64(r6, r6);
        r7 = add64(r7, r7);
        r8 = add64(r8, r8);

        ROUND_MUL_A(0, 0, r0, r1)
        ROUND_MUL_A(1, 1, r2, r3)
        ROUND_MUL_A(2, 2, r4, r5)
        ROUND_MUL_A(3, 3, r6, r7)
        ROUND_MUL_A(4, 4, r8, r9)

        r4 = fma52lo(r4, r9, MOD_2_260);
        r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);
        r4 = and64(r4, MASK_47);

        REDUCE_ROUND(0, 1, 5);
        REDUCE_ROUND(1, 2, 6);
        REDUCE_ROUND(2, 3, 7);
        REDUCE_ROUND(3, 4, 8);

        // Normalize result
        NORM(0,1)
        NORM(1,2)
        NORM(2,3)
        NORM(3,4)

        a0 = r0;
        a1 = r1;
        a2 = r2;
        a3 = r3;
        a4 = r4;
    }

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

#define MASK_R4 ((1ULL << (255 - 52 * 4)) - 1)
static const int64u VMASK_R4[8] = {MASK_R4, MASK_R4, MASK_R4, MASK_R4,
    MASK_R4, MASK_R4, MASK_R4, MASK_R4};

#define MASK52 ((1ULL << 52) - 1)
static const int64u VMASK52[8] = {MASK52, MASK52, MASK52, MASK52,
    MASK52, MASK52, MASK52, MASK52};

#define REDUCE_ROUND_(R, R0, R1, R5) \
    R##R0 = fma52lo(R##R0, R##R5, MOD_2_260); \
    R##R1 = fma52lo( fma52hi(R##R1, R##R5, MOD_2_260), \
        srli64(R##R5, 52), MOD_2_260);

#define NORM_(R, I, J) \
    R##J = add64(R##J, srli64(R##I, 52)); \
    R##I = and64(R##I, loadu64(VMASK52));

#define REDUCE_R4_N_R9(R)                                         \
    R##4 = fma52lo(R##4, R##9, MOD_2_260);                        \
    R##0 = fma52lo(R##0, srli64(R##4, 47), MOD_2_255);            \
    R##4 = and64(R##4, loadu64(VMASK_R4));

__INLINE void ed25519_mul_dual(U64 out0[], U64 out1[],
                const U64 a0[], const U64 b0[],
                const U64 a1[], const U64 b1[]) {

    U64 r00, r01, r02, r03, r04, r05, r06, r07, r08, r09;
    U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;

    U64 *vr0 = (U64*) out0;
    U64 *vr1 = (U64*) out1;

    r00 = r01 = r02 = r03 = r04 = r05 = r06 = r07 = r08 = r09 = get_zero64();
    r10 = r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19 = get_zero64();

    // Full multiplication
    U64 *va = (U64*) a0;
    U64 *vb = (U64*) b0;
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

    va = (U64*) a1;
    vb = (U64*) b1;
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

    REDUCE_R4_N_R9(r0)
    REDUCE_R4_N_R9(r1)

    REDUCE_ROUND_(r0, 0, 1, 5);
    REDUCE_ROUND_(r0, 1, 2, 6);
    REDUCE_ROUND_(r0, 2, 3, 7);
    REDUCE_ROUND_(r0, 3, 4, 8);

    REDUCE_ROUND_(r1, 0, 1, 5);
    REDUCE_ROUND_(r1, 1, 2, 6);
    REDUCE_ROUND_(r1, 2, 3, 7);
    REDUCE_ROUND_(r1, 3, 4, 8);

    // Normalize result
    NORM_(r0, 0,1)
    NORM_(r0, 1,2)
    NORM_(r0, 2,3)
    NORM_(r0, 3,4)

    NORM_(r1, 0,1)
    NORM_(r1, 1,2)
    NORM_(r1, 2,3)
    NORM_(r1, 3,4)

    storeu64(&vr0[0], r00);
    storeu64(&vr0[1], r01);
    storeu64(&vr0[2], r02);
    storeu64(&vr0[3], r03);
    storeu64(&vr0[4], r04);

    storeu64(&vr1[0], r10);
    storeu64(&vr1[1], r11);
    storeu64(&vr1[2], r12);
    storeu64(&vr1[3], r13);
    storeu64(&vr1[4], r14);
}

__INLINE void ed25519_sqr_dual(U64 out0[], U64 out1[],
                const U64 a0[], const U64 a1[]) {

    U64 r00, r01, r02, r03, r04, r05, r06, r07, r08, r09;
    U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;

    U64 *vr0 = (U64*) out0;
    U64 *vr1 = (U64*) out1;

    r00 = r01 = r02 = r03 = r04 = r05 = r06 = r07 = r08 = r09 = get_zero64();
    r10 = r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19 = get_zero64();

    // Square
    U64 *va = (U64*) a0;
    U64 *vb = (U64*) a0;
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

    r01 = add64(r01, r01);
    r02 = add64(r02, r02);
    r03 = add64(r03, r03);
    r04 = add64(r04, r04);
    r05 = add64(r05, r05);
    r06 = add64(r06, r06);
    r07 = add64(r07, r07);
    r08 = add64(r08, r08);

    ROUND_MUL(0, 0, r00, r01)
    ROUND_MUL(1, 1, r02, r03)
    ROUND_MUL(2, 2, r04, r05)
    ROUND_MUL(3, 3, r06, r07)
    ROUND_MUL(4, 4, r08, r09)

    va = (U64*) a1;
    vb = (U64*) a1;
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

    r11 = add64(r11, r11);
    r12 = add64(r12, r12);
    r13 = add64(r13, r13);
    r14 = add64(r14, r14);
    r15 = add64(r15, r15);
    r16 = add64(r16, r16);
    r17 = add64(r17, r17);
    r18 = add64(r18, r18);

    ROUND_MUL(0, 0, r10, r11)
    ROUND_MUL(1, 1, r12, r13)
    ROUND_MUL(2, 2, r14, r15)
    ROUND_MUL(3, 3, r16, r17)
    ROUND_MUL(4, 4, r18, r19)

    REDUCE_R4_N_R9(r0)
    REDUCE_R4_N_R9(r1)

    REDUCE_ROUND_(r0, 0, 1, 5);
    REDUCE_ROUND_(r0, 1, 2, 6);
    REDUCE_ROUND_(r0, 2, 3, 7);
    REDUCE_ROUND_(r0, 3, 4, 8);

    REDUCE_ROUND_(r1, 0, 1, 5);
    REDUCE_ROUND_(r1, 1, 2, 6);
    REDUCE_ROUND_(r1, 2, 3, 7);
    REDUCE_ROUND_(r1, 3, 4, 8);

    // Normalize result
    NORM_(r0, 0,1)
    NORM_(r0, 1,2)
    NORM_(r0, 2,3)
    NORM_(r0, 3,4)

    NORM_(r1, 0,1)
    NORM_(r1, 1,2)
    NORM_(r1, 2,3)
    NORM_(r1, 3,4)

    storeu64(&vr0[0], r00);
    storeu64(&vr0[1], r01);
    storeu64(&vr0[2], r02);
    storeu64(&vr0[3], r03);
    storeu64(&vr0[4], r04);

    storeu64(&vr1[0], r10);
    storeu64(&vr1[1], r11);
    storeu64(&vr1[2], r12);
    storeu64(&vr1[3], r13);
    storeu64(&vr1[4], r14);
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

__INLINE void fe52mb8_set(U64 out[], int64u value)
{
    storeu64(&out[0], set64((long long)value));
    storeu64(&out[1], get_zero64());
    storeu64(&out[2], get_zero64());
    storeu64(&out[3], get_zero64());
    storeu64(&out[4], get_zero64());
}
__INLINE void fe52mb8_copy(U64 out[], const U64 in[])
{
    storeu64(&out[0], loadu64(&in[0]));
    storeu64(&out[1], loadu64(&in[1]));
    storeu64(&out[2], loadu64(&in[2]));
    storeu64(&out[3], loadu64(&in[3]));
    storeu64(&out[4], loadu64(&in[4]));
}

// Clang warning -Wunused-function
#if(0)
__INLINE void fe52mb8_mul_mod25519(U64 vr[], const U64 va[], const U64 vb[])
{
    U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

    // Full multiplication
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

    //REDUCE_ROUND(4, 5, 9);
    r4 = fma52lo(r4, r9, MOD_2_260); //r9 always contributes 0 to r5 (if input normalized?)
    REDUCE_ROUND(3, 4, 8);
    REDUCE_ROUND(2, 3, 7);
    REDUCE_ROUND(1, 2, 6);
    REDUCE_ROUND(0, 1, 5);

    // Reduce r4 upper bits
    r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);

    // Trim top r4 bits that were already reduced above
    r4 = and64(r4, MASK_47);

    // Normalize result
    NORM(0,1)
    NORM(1,2)
    NORM(2,3)
    NORM(3,4)

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

__INLINE void fe52mb8_sqr_mod25519(U64 out[], const U64 a[])
{
   fe52mb8_mul_mod25519(out, a, a);
}
#endif

__INLINE void fe52mb8_mul121666_mod25519(U64 vr[], const U64 va[])
{
    U64 multiplier = set64(121666);

    U64 r0, r1, r2, r3, r4, r5;
    r0 = r1 = r2 = r3 = r4 = r5 = get_zero64();

    // multiply
    r0 = fma52lo(r0, va[0], multiplier);
    r1 = fma52lo(r1, va[1], multiplier);
    r2 = fma52lo(r2, va[2], multiplier);
    r3 = fma52lo(r3, va[3], multiplier);
    r4 = fma52lo(r4, va[4], multiplier);

    r5 = fma52hi(r5, va[4], multiplier);
    r1 = fma52hi(r1, va[0], multiplier);
    r2 = fma52hi(r2, va[1], multiplier);
    r3 = fma52hi(r3, va[2], multiplier);
    r4 = fma52hi(r4, va[3], multiplier);

    // reduce
    REDUCE_ROUND(0, 1, 5);
    r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);

    // trim top r4 bits that were already reduced above
    r4 = and64(r4, MASK_47);

    // normalize
    NORM(0,1)
    NORM(1,2)
    NORM(2,3)
    NORM(3,4)

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

#define PRIME25519_LO  0x000FFFFFFFFFFFED
#define PRIME25519_MID 0x000FFFFFFFFFFFFF
#define PRIME25519_HI  0x00007FFFFFFFFFFF

// __ALIGN64 static const int64u prime25519[5] = {
//   PRIME25519_LO, PRIME25519_MID, PRIME25519_MID, PRIME25519_MID, PRIME25519_HI};

__ALIGN64 static const int64u VPRIME25519_LO[8] = 
    { PRIME25519_LO, PRIME25519_LO, PRIME25519_LO, PRIME25519_LO, 
      PRIME25519_LO, PRIME25519_LO, PRIME25519_LO, PRIME25519_LO };

__ALIGN64 static const int64u VPRIME25519_MID[8] = 
    { PRIME25519_MID, PRIME25519_MID, PRIME25519_MID, PRIME25519_MID, 
      PRIME25519_MID, PRIME25519_MID, PRIME25519_MID, PRIME25519_MID };

__ALIGN64 static const int64u VPRIME25519_HI[8] = 
    { PRIME25519_HI, PRIME25519_HI, PRIME25519_HI, PRIME25519_HI, 
      PRIME25519_HI, PRIME25519_HI, PRIME25519_HI, PRIME25519_HI };


__INLINE U64 cmov_U64(U64 a, U64 b, __mb_mask kmask)
{  return mask_mov64 (a, kmask, b); }

#define NORM_ASHIFTR(R, I, J) \
    R##J = add64(R##J, srai64(R##I, DIGIT_SIZE)); \
    R##I = and64(R##I, loadu64(VMASK52));

#define NORM_LSHIFTR(R, I, J) \
    R##J = add64(R##J, srli64(R##I, DIGIT_SIZE)); \
    R##I = and64(R##I, loadu64(VMASK52));

__INLINE void fe52mb8_add_mod25519(U64 vr[], const U64 va[], const U64 vb[])
{
    /* r = a+b */
    U64 r0 = add64(va[0], vb[0]);
    U64 r1 = add64(va[1], vb[1]);
    U64 r2 = add64(va[2], vb[2]);
    U64 r3 = add64(va[3], vb[3]);
    U64 r4 = add64(va[4], vb[4]);

    /* t = r-modulus (2^255-19) */
    U64 t0 = sub64(r0, loadu64(VPRIME25519_LO ));
    U64 t1 = sub64(r1, loadu64(VPRIME25519_MID));
    U64 t2 = sub64(r2, loadu64(VPRIME25519_MID));
    U64 t3 = sub64(r3, loadu64(VPRIME25519_MID));
    U64 t4 = sub64(r4, loadu64(VPRIME25519_HI ));

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

    storeu64(&vr[0], cmov_U64(t0, r0, cmask));
    storeu64(&vr[1], cmov_U64(t1, r1, cmask));
    storeu64(&vr[2], cmov_U64(t2, r2, cmask));
    storeu64(&vr[3], cmov_U64(t3, r3, cmask));
    storeu64(&vr[4], cmov_U64(t4, r4, cmask));
}

__INLINE void fe52mb8_sub_mod25519(U64 vr[], const U64 va[], const U64 vb[])
{
    /* r = a-b */
    U64 r0 = sub64(va[0], vb[0]);
    U64 r1 = sub64(va[1], vb[1]);
    U64 r2 = sub64(va[2], vb[2]);
    U64 r3 = sub64(va[3], vb[3]);
    U64 r4 = sub64(va[4], vb[4]);

    /* t = r+modulus (2^255-19) */
    U64 t0 = add64(r0, loadu64(VPRIME25519_LO ));
    U64 t1 = add64(r1, loadu64(VPRIME25519_MID));
    U64 t2 = add64(r2, loadu64(VPRIME25519_MID));
    U64 t3 = add64(r3, loadu64(VPRIME25519_MID));
    U64 t4 = add64(r4, loadu64(VPRIME25519_HI ));

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

    /* condition mask r4<0? (-1) : 0 */
    __mb_mask cmask = cmp64_mask(r4, get_zero64(), _MM_CMPINT_LT);

    storeu64(&vr[0], cmov_U64(r0, t0, cmask));
    storeu64(&vr[1], cmov_U64(r1, t1, cmask));
    storeu64(&vr[2], cmov_U64(r2, t2, cmask));
    storeu64(&vr[3], cmov_U64(r3, t3, cmask));
    storeu64(&vr[4], cmov_U64(r4, t4, cmask));
}

// #define USE_DUAL_MUL_SQR
// #define fe52_mul  fe52mb8_mul_mod25519
// #define fe52_sqr  fe52mb8_sqr_mod25519
#define fe52_mul  ed25519_mul
#define fe52_sqr  ed25519_sqr
#define fe52_add  fe52mb8_add_mod25519
#define fe52_sub  fe52mb8_sub_mod25519
#define fe52_mul121666  fe52mb8_mul121666_mod25519
#define fe52_sqr_power  MB_FUNC_NAME(ed25519_sqr_latency_)


/*
   Compute 1/z = z^(2^255 - 19 - 2)
   considering the exponent as
   2^255 - 21 = (2^5) * (2^250 - 1) + 11.
*/
__INLINE void fe52mb8_inv_mod25519(U64 out[], const U64 z[])
{
    __ALIGN64 U64 t0[5];
    __ALIGN64 U64 t1[5];
    __ALIGN64 U64 t2[5];
    __ALIGN64 U64 t3[5];

    /* t0 = z ** 2 */
    fe52_sqr(t0, z);

    /* t1 = t0 ** (2 ** 2) = z ** 8 */
    fe52_sqr(t1, t0);
    fe52_sqr(t1, t1);

    /* t1 = z * t1 = z ** 9 */
    fe52_mul(t1, z, t1);
    /* t0 = t0 * t1 = z ** 11 -- stash t0 away for the end. */
    fe52_mul(t0, t0, t1);

    /* t2 = t0 ** 2 = z ** 22 */
    fe52_sqr(t2, t0);

    /* t1 = t1 * t2 = z ** (2 ** 5 - 1) */
    fe52_mul(t1, t1, t2);

    /* t2 = t1 ** (2 ** 5) = z ** ((2 ** 5) * (2 ** 5 - 1)) */
    fe52_sqr_power(t2, t1, 5);

    /* t1 = t1 * t2 = z ** ((2 ** 5 + 1) * (2 ** 5 - 1)) = z ** (2 ** 10 - 1) */
    fe52_mul(t1, t2, t1);

    /* Continuing similarly... */

    /* t2 = z ** (2 ** 20 - 1) */
    fe52_sqr_power(t2, t1, 10);

    fe52_mul(t2, t2, t1);

    /* t2 = z ** (2 ** 40 - 1) */
    fe52_sqr_power(t3, t2, 20);

    fe52_mul(t2, t3, t2);

    /* t2 = z ** (2 ** 10) * (2 ** 40 - 1) */
    fe52_sqr_power(t2, t2, 10);

    /* t1 = z ** (2 ** 50 - 1) */
    fe52_mul(t1, t2, t1);

    /* t2 = z ** (2 ** 100 - 1) */
    fe52_sqr_power(t2, t1, 50);

    fe52_mul(t2, t2, t1);

    /* t2 = z ** (2 ** 200 - 1) */
    fe52_sqr_power(t3, t2, 100);

    fe52_mul(t2, t3, t2);

    /* t2 = z ** ((2 ** 50) * (2 ** 200 - 1) */
    fe52_sqr_power(t2, t2, 50);

    /* t1 = z ** (2 ** 250 - 1) */
    fe52_mul(t1, t2, t1);

    /* t1 = z ** ((2 ** 5) * (2 ** 250 - 1)) */
    fe52_sqr_power(t1, t1, 5);

    /* Recall t0 = z ** 11; out = z ** (2 ** 255 - 21) */
    fe52_mul(out, t1, t0);
}

#define cswap_U64(a, b, kmask) { \
    U64 ta = mask_mov64((a), (kmask), (b)); \
    (b)    = mask_mov64((b), (kmask), (a)); \
    (a)    = ta; \
}

static void fe52mb8_cswap(U64 a[], U64 b[], __mb_mask k)
{
   cswap_U64(a[0], b[0], k)
   cswap_U64(a[1], b[1], k)
   cswap_U64(a[2], b[2], k)
   cswap_U64(a[3], b[3], k)
   cswap_U64(a[4], b[4], k)
}

#if 0
static void x25519_scalar_mul(U64 out[], U64 scalar[], U64 point[])
{
    __ALIGN64 U64 x1[5], x2[5], x3[5];
    __ALIGN64 U64        z2[5], z3[5];
    __ALIGN64 U64 tmp0[5], tmp1[5];

    fe52mb8_copy(x1, point);
    fe52mb8_set(x2, 1);
    fe52mb8_set(z2, 0);
    fe52mb8_copy(x3, x1);
    fe52mb8_set(z3, 1);

    /* read high and remove (zero) bit 63 */
    U64 e = loadu64(&scalar[3]);
    e = slli64(e, 1);

    __mb_mask swap = get_mask(0);
    int bitpos;
    for (bitpos=254; bitpos>= 0; bitpos--) {
        if(63==(bitpos%64))
            e = loadu64(&scalar[bitpos/64]);

        __mb_mask b = cmp64_mask(e, get_zero64(), _MM_CMPINT_LT);

        swap = mask_xor (swap, b);
        fe52mb8_cswap(x2, x3, swap);
        fe52mb8_cswap(z2, z3, swap);
        swap = b;
        fe52_sub(tmp0, x3, z3);
        fe52_sub(tmp1, x2, z2);
        fe52_add(x2, x2, z2); 
        fe52_add(z2, x3, z3);

        #ifdef USE_DUAL_MUL_SQR
            ed25519_mul_dual(z3, z2, x2, tmp0, z2, tmp1);
        #else
            fe52_mul(z3, x2, tmp0);
            fe52_mul(z2, z2, tmp1);
        #endif

        #ifdef USE_DUAL_MUL_SQR
            ed25519_sqr_dual(tmp0, tmp1, tmp1, x2);
        #else
            fe52_sqr(tmp0, tmp1);
            fe52_sqr(tmp1, x2);
        #endif

        fe52_add(x3, z3, z2);
        fe52_sub(z2, z3, z2);
        fe52_mul(x2, tmp1, tmp0);
        fe52_sub(tmp1, tmp1, tmp0);
        fe52_sqr(z2, z2);
        fe52_mul121666(z3, tmp1);
        fe52_sqr(x3, x3);
        fe52_add(tmp0, tmp0, z3);

        #ifdef USE_DUAL_MUL_SQR
            ed25519_mul_dual(z3, z2, x1, z2, tmp1, tmp0);
        #else
            fe52_mul(z3, x1, z2);
            fe52_mul(z2, tmp1, tmp0);
        #endif

        e = slli64(e, 1);
    }

    fe52mb8_inv_mod25519(z2, z2);
    fe52_mul(out, x2, z2);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

__INLINE void ed25519_mul_dual_wonorm(U64 out0[], U64 out1[],
                const U64 a0[], const U64 b0[],
                const U64 a1[], const U64 b1[]) {

    U64 r00, r01, r02, r03, r04, r05, r06, r07, r08, r09;
    U64 r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;

    U64 *vr0 = (U64*) out0;
    U64 *vr1 = (U64*) out1;

    r00 = r01 = r02 = r03 = r04 = r05 = r06 = r07 = r08 = r09 = get_zero64();
    r10 = r11 = r12 = r13 = r14 = r15 = r16 = r17 = r18 = r19 = get_zero64();

    // Full multiplication
    U64 *va = (U64*) a0;
    U64 *vb = (U64*) b0;
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

    va = (U64*) a1;
    vb = (U64*) b1;
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

    REDUCE_R4_N_R9(r0)
    REDUCE_R4_N_R9(r1)

    REDUCE_ROUND_(r0, 0, 1, 5);
    REDUCE_ROUND_(r0, 1, 2, 6);
    REDUCE_ROUND_(r0, 2, 3, 7);
    REDUCE_ROUND_(r0, 3, 4, 8);

    REDUCE_ROUND_(r1, 0, 1, 5);
    REDUCE_ROUND_(r1, 1, 2, 6);
    REDUCE_ROUND_(r1, 2, 3, 7);
    REDUCE_ROUND_(r1, 3, 4, 8);

    storeu64(&vr0[0], r00);
    storeu64(&vr0[1], r01);
    storeu64(&vr0[2], r02);
    storeu64(&vr0[3], r03);
    storeu64(&vr0[4], r04);

    storeu64(&vr1[0], r10);
    storeu64(&vr1[1], r11);
    storeu64(&vr1[2], r12);
    storeu64(&vr1[3], r13);
    storeu64(&vr1[4], r14);
}

__INLINE void fe52mb8_mul_mod25519_wonorm(U64 vr[], const U64 va[], const U64 vb[])
{
    U64 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = get_zero64();

    // Full multiplication
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

    //REDUCE_ROUND(4, 5, 9);
    r4 = fma52lo(r4, r9, MOD_2_260); //r9 always contributes 0 to r5 (if input normalized?)
    REDUCE_ROUND(3, 4, 8);
    REDUCE_ROUND(2, 3, 7);
    REDUCE_ROUND(1, 2, 6);
    REDUCE_ROUND(0, 1, 5);

    // Reduce r4 upper bits
    r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);

    // Trim top r4 bits that were already reduced above
    r4 = and64(r4, MASK_47);

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

__INLINE void fe52mb8_mul121666_mod25519_wonorm(U64 vr[], const U64 va[])
{
    U64 multiplier = set64(121666);

    U64 r0, r1, r2, r3, r4, r5;
    r0 = r1 = r2 = r3 = r4 = r5 = get_zero64();

    // multiply
    r0 = fma52lo(r0, va[0], multiplier);
    r1 = fma52lo(r1, va[1], multiplier);
    r2 = fma52lo(r2, va[2], multiplier);
    r3 = fma52lo(r3, va[3], multiplier);
    r4 = fma52lo(r4, va[4], multiplier);

    r5 = fma52hi(r5, va[4], multiplier);
    r1 = fma52hi(r1, va[0], multiplier);
    r2 = fma52hi(r2, va[1], multiplier);
    r3 = fma52hi(r3, va[2], multiplier);
    r4 = fma52hi(r4, va[3], multiplier);

    // reduce
    REDUCE_ROUND(0, 1, 5);
    r0 = fma52lo(r0, srli64(r4, 47), MOD_2_255);

    // trim top r4 bits that were already reduced above
    r4 = and64(r4, MASK_47);

    storeu64(&vr[0], r0);
    storeu64(&vr[1], r1);
    storeu64(&vr[2], r2);
    storeu64(&vr[3], r3);
    storeu64(&vr[4], r4);
}

__INLINE void x25519_scalar_mul_dual(U64 out[], U64 scalar[], U64 point[])
{
    __ALIGN64 U64 x1[5], x2[5], x3[5];
    __ALIGN64 U64        z2[5], z3[5];
    __ALIGN64 U64 tmp0[5], tmp1[5];

    fe52mb8_copy(x1, point);
    fe52mb8_set(x2, 1);
    fe52mb8_set(z2, 0);
    fe52mb8_copy(x3, x1);
    fe52mb8_set(z3, 1);

    /* read high and remove (zero) bit 63 */
    U64 e = loadu64(&scalar[3]);
    U64 vmask = slli64(
        xor64(e, srli64(e, 1)), 1);
    __mb_mask swap = cmp64_mask(vmask, get_zero64(), _MM_CMPINT_LT);

    int bitpos;
    for (bitpos = 254; bitpos >= 0; bitpos--) {
        if (63 == (bitpos % 64)) {
            U64 t = e;
            e = loadu64(&scalar[bitpos/64]);
            vmask = xor64(slli64(t, 63),
                xor64(e, srli64(e, 1)));
            swap  = cmp64_mask(vmask, get_zero64(), _MM_CMPINT_LT);
        }

        fe52mb8_cswap(x2, x3, swap);
        fe52mb8_cswap(z2, z3, swap);

#if (defined(linux) && ((SIMD_LEN)==512))
        // Avoid reordering optimization by compiler
        U64 Z = get_zero64();
        __asm__ ("vpsllq $1, %0, %0 \n"
             "vpcmpq $1, %2, %0, %1\n"
             : "+x" (vmask), "=k" (swap): "x" (Z) : );
#else
        vmask = slli64(vmask, 1);
        swap  = cmp64_mask(vmask, get_zero64(), _MM_CMPINT_LT);
#endif

        fe52_sub(tmp0, x3, z3);
        fe52_sub(tmp1, x2, z2);
        fe52_add(x2, x2, z2); 
        fe52_add(z2, x3, z3);

        ed25519_mul_dual_wonorm(z3, z2, x2,tmp0, z2,tmp1);

        ed25519_sqr_dual(tmp0, tmp1, tmp1, x2);

        fe52_add(x3, z3, z2);
        fe52_sub(z2, z3, z2);

        fe52mb8_mul_mod25519_wonorm(x2, tmp1, tmp0);
        fe52_sub(tmp1, tmp1, tmp0);

        ed25519_sqr_dual(z2, x3, z2, x3);

        fe52mb8_mul121666_mod25519_wonorm(z3, tmp1);
        fe52_add(tmp0, tmp0, z3);

        ed25519_mul_dual_wonorm(z3, z2, x1,z2, tmp1,tmp0);
    }

    // normalize z2 and x2 before inversion
    {
      U64 r0 = z2[0];
      U64 r1 = z2[1];
      U64 r2 = z2[2];
      U64 r3 = z2[3];
      U64 r4 = z2[4];
      NORM_(r, 0,1)
      NORM_(r, 1,2)
      NORM_(r, 2,3)
      NORM_(r, 3,4)
     storeu64(&z2[0], r0);
     storeu64(&z2[1], r1);
     storeu64(&z2[2], r2);
     storeu64(&z2[3], r3);
     storeu64(&z2[4], r4);

      r0 = x2[0];
      r1 = x2[1];
      r2 = x2[2];
      r3 = x2[3];
      r4 = x2[4];
      NORM_(r, 0,1)
      NORM_(r, 1,2)
      NORM_(r, 2,3)
      NORM_(r, 3,4)
     storeu64(&x2[0], r0);
     storeu64(&x2[1], r1);
     storeu64(&x2[2], r2);
     storeu64(&x2[3], r3);
     storeu64(&x2[4], r4);
    }

    fe52mb8_inv_mod25519(z2, z2);
    fe52_mul(out, x2, z2);
}

DLL_PUBLIC
mbx_status MB_FUNC_NAME(mbx_x25519_)(int8u* const pa_shared_key[8],
                       const int8u* const pa_private_key[8],
                       const int8u* const pa_public_key[8])
{
    mbx_status status = 0;
    int buf_no;

    /* test input pointers */
    if(NULL==pa_shared_key || NULL==pa_private_key || NULL==pa_public_key) {
        status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
        return status;
    }

    /* check pointers and values */
    for(buf_no=0; buf_no<8; buf_no++) {
        int64u* shared = (int64u*) pa_shared_key[buf_no];
        const int64u* own_private = (const int64u*) pa_private_key[buf_no];
        const int64u* party_public = (const int64u*) pa_public_key[buf_no];

        /* if any of pointer NULL set error status */
        if(NULL==shared || NULL==own_private || NULL==party_public) {
            status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
            continue;
        }
    }

    /* continue processing if there are correct parameters */
    if( MBX_IS_ANY_OK_STS(status) ) {
        __ALIGN64 U64 private64_mb8[4];
        __ALIGN64 U64 pub52_mb8[5];
        __ALIGN64 U64 shared52_mb8[5];

        /* get scalars and convert to MB8 */
        ifma_BNU_transpose_copy((int64u (*)[8])private64_mb8, (const int64u * const*)pa_private_key, 256);
        /* decode keys into scalars according to RFC7748 */
        private64_mb8[0] = and64_const(private64_mb8[0], 0xfffffffffffffff8);
        private64_mb8[3] = and64_const(private64_mb8[3], 0x7fffffffffffffff);
        private64_mb8[3] = or64(private64_mb8[3], set64(0x4000000000000000));

        /* get peer's public keys and convert to MB8 */
        ifma_BNU_to_mb8((int64u (*)[8])pub52_mb8, (const int64u * const*)pa_public_key, 256);

        /* RFC7748: (x25519) ... MUST mask the most significant bit in the final byte.
           This is done to preserve compatibility with point formats .. (compact format??)
        */
         pub52_mb8[4] = and64(pub52_mb8[4], loadu64(VPRIME25519_HI));

        /* point multiplication */
        x25519_scalar_mul_dual(shared52_mb8, private64_mb8, pub52_mb8);

        /* test shared secret before return; all-zero output results when the input is a point of small order. */
        __ALIGN64 U64 shared52_sum = shared52_mb8[0];
        shared52_sum = or64(shared52_sum, shared52_mb8[1]);
        shared52_sum = or64(shared52_sum, shared52_mb8[2]);
        shared52_sum = or64(shared52_sum, shared52_mb8[3]);
        shared52_sum = or64(shared52_sum, shared52_mb8[4]);
        int8u stt_mask = cmpeq64_mask(shared52_sum, get_zero64());
        status |= MBX_SET_STS_BY_MASK(status, stt_mask, MBX_STATUS_LOW_ORDER_ERR);

        /* convert result back */
        ifma_mb8_to_BNU((int64u* const *)pa_shared_key, (const int64u (*)[8])shared52_mb8, 256);

        /* clear computed shared keys and it sum */
        MB_FUNC_NAME(zero_)((int64u (*)[8])shared52_mb8, sizeof(shared52_mb8)/sizeof(U64));
        MB_FUNC_NAME(zero_)((int64u (*)[8])&shared52_sum, 1);

        /* clear copy of the secret keys */
        MB_FUNC_NAME(zero_)((int64u (*)[8])private64_mb8, sizeof(private64_mb8)/sizeof(U64));
    }
    return status;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

/* the fe64 muTBL52[] table in radix52 */
__ALIGN64 static int64u muTBL52[255][NUMBER_OF_DIGITS(256,DIGIT_SIZE)] = {
   {0x000ffffffffffff3, 0x000fffffffffffff, 0x000fffffffffffff, 0x000fffffffffffff, 0x00005fffffffffff},
   {0x000220f416aafe96, 0x0002b4f566a346b8, 0x0005a5950f82ebeb, 0x00088f4d5a9a5b07, 0x00005142b2cf4b24},
   {0x000ebc750069680c, 0x00020a0f99c416aa, 0x000b56d0f489cf78, 0x00011a42a58d9183, 0x00004b5aca80e360},
   {0x000132348c29745d, 0x00016e1642fd7329, 0x000f67bc34f4a2e6, 0x0009b4a1e45bb03f, 0x0000306912d0f42a},
   {0x00086507e6af7154, 0x00013dfeec82fff8, 0x000abab5ce04f50e, 0x000f222aa512fe82, 0x0000174e251a68d5},
   {0x0006700d82028898, 0x000370a2c02c5cf9, 0x0004e86eaa1743e3, 0x000482e379eec98b, 0x00000c59888a51e0},
   {0x000bf1d699b5d189, 0x000d58e9fdc84fbc, 0x00031f7614acaef0, 0x000f972c1c20d062, 0x00002938218da274},
   {0x000f49beff1d7f18, 0x00022387ac9c2f6a, 0x000015c56bcc541c, 0x00013a996fcc9ef4, 0x000069c1627c6909},
   {0x0006fd2f4733db0e, 0x000f29e087de97a8, 0x000ea2a229fdb8c4, 0x0007a79095e4b1a8, 0x00001ad7a7c829b3},
   {0x000d89cad17ea0c0, 0x000a6cced2051342, 0x000bb42f7467bedd, 0x000acbb19ca31bf2, 0x00003df7b4c84980},
   {0x0006444dc80ad883, 0x0000366e3ab85a8c, 0x000164f6d8b91e44, 0x000e668c215cda00, 0x00003d867c6ef247},
   {0x000d582bcc3e658c, 0x00048ee0e5528c7d, 0x000c9f4f71fd2c47, 0x0005ddfa0fd9b95c, 0x00007529d871b067},
   {0x000568b42d3cbd78, 0x0001b91f3da82b8f, 0x000a7c3b62123301, 0x00086032dce6ccd4, 0x000075e7fc8e9e49},
   {0x000f13f1fcd0b6ec, 0x0001f29ff7a452f4, 0x000981e29bf1a8ca, 0x000b56ac249c1a72, 0x00006ebe0dbb8c83},
   {0x0004fa8d170bb222, 0x000d5bf93935f711, 0x00059c979a65a2dc, 0x0009289bdc41f68b, 0x00002f0eef79a2ce},
   {0x000cbf0c083c37ce, 0x00009ec49632242e, 0x000cfeac0d2930bc, 0x000bb80f294b0c19, 0x00003780aa4bedfa},
   {0x00017d3e7cead929, 0x000eb2e5722c556c, 0x000dbfe15ae7cb4b, 0x00052f80ce931732, 0x000041b883c76210},
   {0x00075ca0c3d25350, 0x000086eb1e351dbf, 0x0004a9b2122936be, 0x00025aac936e03cb, 0x00001d45bf823222},
   {0x000ab1036a024cc5, 0x0001c304c9a72e81, 0x000832b1fce21220, 0x0009581c5d73fba6, 0x000020ffdb5a4d83},
   {0x0003d367be5d0fad, 0x000ca8b164475a28, 0x000caaf22e6c2b25, 0x000ff499d4935467, 0x00005166408eee85},
   {0x0007baa2fab4e361, 0x000c67ef35cef3c6, 0x0001159b1cb3e433, 0x000ab33525972924, 0x00006a621892d5b0},
   {0x00074a387555cdcb, 0x0000e1208923f20b, 0x0002281dd1532aa1, 0x00044bfeaa17b776, 0x000061ab3443f05c},
   {0x000a6c422324def8, 0x0001017e3cf7f257, 0x000630a257131c6c, 0x000858023758739f, 0x0000295a407a01a7},
   {0x000443246d5da8d9, 0x000450c52fa5df8c, 0x00031bf83d19d775, 0x00047002afcfc927, 0x00007d10c8e81b2b},
   {0x0000271f70baa20b, 0x000867ca63957c8e, 0x000b7ed4bb993748, 0x00029755412efb3c, 0x00003196d36173e6},
   {0x000bcad141c7dffc, 0x000d2b395c848de5, 0x00011af3cb47cc8c, 0x000cec2a34cd942e, 0x00000256dbf2d04e},
   {0x000ab7e94b0e667f, 0x00083c0850d10875, 0x000e72c79fcad4dd, 0x000b19b47f12e8f4, 0x00005f1a87bb8c85},
   {0x0009d0b6437f51b8, 0x00055188790657ae, 0x000cf77aee12c7ce, 0x00056272ade09fe5, 0x000023a05a2f7d2c},
   {0x0008e128f17c169a, 0x000dd8ad0852d590, 0x000b102f64f77498, 0x000984574b4c4cea, 0x0000183abadd1013},
   {0x0005ba8daa92aaac, 0x0009599386705b16, 0x0008fc40d1d5c5ef, 0x0004514be2f8f0cf, 0x00002701e635ee20},
   {0x000fa80020156514, 0x0008764a8c1ce629, 0x000b3f060ef22386, 0x000a3fa5b894fff0, 0x000060d9944cf708},
   {0x000a001a1c7a201f, 0x000633ee2ce63aee, 0x000c7a07e1ebf16a, 0x00008cb6f7709594, 0x000079b958150d02},
   {0x00055e5301d410e7, 0x000dff3fdc84d24b, 0x00004032d8e3a34e, 0x000aeecd88768e49, 0x0000131384427b3a},
   {0x0005e51286234f14, 0x00039adb4c529840, 0x0000634ffd14dc47, 0x000ff93b8a2b5b25, 0x00002fe2a94ad8a7},
   {0x000c57efe843fadd, 0x00040f0bb9918ec5, 0x000f3d63052843ce, 0x000777ea4b561d6c, 0x0000743629bde8fb},
   {0x000edd46bbaf738f, 0x00028b101a651343, 0x00082c797aed9818, 0x0008730a401760b8, 0x00001fc223e28dc8},
   {0x00004e91fc0fba0e, 0x0008f052c6fa4486, 0x0009e9239cb637f7, 0x000687c91ccac3d0, 0x000023f7eed4437a},
   {0x0003b1118d9bd800, 0x000b63189d4a7517, 0x0008bbc58629d641, 0x0001df5fdbf17798, 0x00002959894fcad8},
   {0x000c8ef3b4bbc899, 0x0005ab26992b9aeb, 0x0004f92cfb414899, 0x000dee824e20b013, 0x000040d158894a05},
   {0x00000b1185af76f6, 0x0007873187a7946b, 0x000b8fff5f26bac7, 0x00024d73dc0bf95a, 0x00002a608bd89455},
   {0x00049588bd446302, 0x0001c0388439c264, 0x0003bd11b27c4bc2, 0x00076b98e98a4f38, 0x000026218d7bc9d8},
   {0x00081542997c178a, 0x000a86fb6606fe30, 0x000a2793743c2d29, 0x000b1fa5c217736f, 0x00007dde05734afe},
   {0x00010e3906d42bab, 0x0003e1980649c3bf, 0x000595bf7ae4f780, 0x0005530e6053bf89, 0x0000394faf38da24},
   {0x000efb58896928f4, 0x000e9cc6a113c7a8, 0x0000af596ffbc778, 0x0006cf772670ce33, 0x000048f222a81d3d},
   {0x000fce410d72caa7, 0x000c7213b5595f01, 0x0001fa14835a20ec, 0x000a7417bc21165c, 0x000007f89ae31da8},
   {0x0002c2b4c6830ff9, 0x0000fc631629305d, 0x0006d3a904d43e33, 0x00033b6a5a5590a9, 0x0000705edb91a653},
   {0x000ee15e0bb9a5f7, 0x000ca9e0aaf5d048, 0x000dc4a40b3240cf, 0x0004a6d8f4b71cee, 0x0000621c0da3de54},
   {0x00072836a08c4091, 0x000b010c91445928, 0x000f276394ce8375, 0x00036358a72eb524, 0x00002667fcfa7ec8},
   {0x000c173345e8752a, 0x000feee7079a57f4, 0x000f86ff34061b47, 0x000c89c25dd9afa9, 0x00003780cef5425d},
   {0x0006035a513bb4e9, 0x00079ac575ada1a4, 0x000fa24b503e1ef3, 0x0009f22c78c5f1c5, 0x0000321a967634fd},
   {0x000707b8826e27fa, 0x000d64c506fd0946, 0x0005e914363dca84, 0x0008484c18921807, 0x00006d9284169b3b},
   {0x0007e840383f2ddf, 0x000a30c4f9b753a6, 0x000783ef4733eec9, 0x000fbc43ec7c86fa, 0x000026ec449fbac9},
   {0x000f38cba09b9e7d, 0x000c762a3478c5c0, 0x0006fc121c81168c, 0x000dcdd3e23b0d30, 0x00005a238aa0a5ef},
   {0x00026121c4ea43ff, 0x0007f7c8832b51ba, 0x000adcf99a36f8c7, 0x000ebf988fbea0b0, 0x00005ca9938ec25b},
   {0x00036a5e51fccda0, 0x00097c2cd893bd54, 0x0003224a081dbc47, 0x000f46619346a65d, 0x00000f5034e49b9a},
   {0x000c3967a1e0b96e, 0x000fa867a4d88f23, 0x0007341679e58b08, 0x0006946fb2fabc6a, 0x00002a75381eb602},
   {0x000a3be4c19420ac, 0x000c681f2b6dcc80, 0x0001e9338866b1f6, 0x000a4c47cf703676, 0x000025abbbd8a660},
   {0x000a12ba14fd5198, 0x000fc4a3cffa991e, 0x0000f5ad28684950, 0x000a441f82684213, 0x00003ea988f75301},
   {0x0008109a695f8c6f, 0x0004a0530c3f3c97, 0x00044599951746eb, 0x0005cc7444d6d77b, 0x000075952b8c054e},
   {0x00003f7915f4d6aa, 0x000202f2647d8a37, 0x00011d644b66c346, 0x000d71fd01469df8, 0x000077fea47d81a5},
   {0x0009529ef57ca381, 0x000b9ce2f881ac5e, 0x0008009bd66eeeb4, 0x0003fecb6e91a28e, 0x00004b80be3e9afc},
   {0x000773c526aed2c5, 0x000b453c9a49d7e3, 0x000affb24d1b4afc, 0x000400ea920bdd7b, 0x00007c54699f122d},
   {0x0006c8e14fa94bc8, 0x000ce2952ed5eef4, 0x000bd885d5e0b074, 0x000712cbea450e1d, 0x000061b68649320f},
   {0x00085f7309ccbdd1, 0x0000d7d4d1a2d8a4, 0x00022dbef4bd0632, 0x000f770252329733, 0x0000445dc4758c17},
   {0x000434177cc8933c, 0x0002175ea059fdb0, 0x000053db34ed6fe8, 0x000af991efebefdc, 0x00004adbe867c65d},
   {0x000d71a2a90609df, 0x000856dd040503ac, 0x000157c23ce5e991, 0x000fe4d1ec69b688, 0x0000697427f6885c},
   {0x000e7b9b65e1a851, 0x000d522c536ddd7b, 0x000fd2b645a03d28, 0x00041e128399d658, 0x000049e5b7e17c26},
   {0x000c3a98700457a4, 0x000a25ebb67786f8, 0x000382960f5078f0, 0x00084b1d13c3ccbc, 0x00002e003258a7df},
   {0x0001f39be6296a1c, 0x000652a5fbfb28ad, 0x000d26f3cbc1eeaa, 0x0002ccc33ee0673f, 0x000059256173a69d},
   {0x000a07aa4e18fc41, 0x000527c87a51e41e, 0x000831ca6fd9fc19, 0x000694fbdaacb805, 0x0000445b652dc916},
   {0x0002a3a7f2172315, 0x0002de11b9964ce9, 0x00004c314a1edc28, 0x000f586a1823aafe, 0x0000790a2d94437c},
   {0x000447fb93f6e009, 0x000672284527671c, 0x00004f51698922a5, 0x00019febf70903b2, 0x00002f7a89891ba3},
   {0x00008eb577e2140c, 0x000d4427bdcf402a, 0x0004323cd1ed9a4e, 0x000355b5253ec44e, 0x00003e88363c14e9},
   {0x0006c14277110b8c, 0x0001610a23390aa6, 0x00093fc2a21ae039, 0x000c7ab2030bd12c, 0x00003ee141579555},
   {0x0004de3a6d6e7d41, 0x0008607f17efe921, 0x0008e112173ccdd8, 0x00093d0674f1288f, 0x00005682250f329f},
   {0x00000b136d2e396e, 0x0006f1014debf6cf, 0x000fcc4e836e4cf8, 0x00016b65930b1b5b, 0x0000047069b48aba},
   {0x000ce4ab69b20793, 0x0001a97d0fb9e0d4, 0x000e00d01db24db9, 0x000ddb5cdfa50f54, 0x0000221b1085368b},
   {0x00059468b1e3d8d2, 0x00063bd122f93e7e, 0x0000663f0953c565, 0x0003d42eee8a903e, 0x000061efa662cbbe},
   {0x0008ddddde6eab2a, 0x000d51435f2312cf, 0x0009f049739bf80a, 0x0009b275deadacec, 0x000029275b5d41d2},
   {0x000e0f0895ebf14f, 0x0006b054905a7cfd, 0x0001c420fdb9aab9, 0x000bbc7cae80dd9a, 0x00000a63bf2f1673},
   {0x000f6e11958fbc8c, 0x000e804822fad092, 0x0000d52517672a81, 0x00092f8cac835156, 0x00006f3f7722c8f1},
   {0x000a90ccc2e894b7, 0x000a438ff9f0df8b, 0x000ae523592c7557, 0x0003d69894d1d855, 0x000068e122157b74},
   {0x000e5570cfb919f3, 0x000cd95798db9d87, 0x0000c0a2ce3f2cde, 0x000c5b2212115471, 0x00003c66a115246d},
   {0x000dc562294ecb72, 0x000c36a280b16cbe, 0x0004078b67ba7143, 0x0004b1e9610c2efd, 0x00006144735d946a},
   {0x000f111ed75b3350, 0x0008c2041d81b536, 0x000e10413c0211db, 0x0008876f93cb1000, 0x0000149dfd3c039e},
   {0x0009dde46b63155b, 0x000e93c837976d47, 0x000f13e038b66e15, 0x0000b35dafde43b1, 0x00005fafda1a2e4b},
   {0x0000bbdf17197581, 0x0000bbe3cd2c2360, 0x000dd5be86397205, 0x000860f5938906db, 0x000034fce5e43f9b},
   {0x0008a4cd42d14d02, 0x000c53441df6575a, 0x0002e131d3828dab, 0x000d25f33dcabedd, 0x00003ebad76fb814},
   {0x00006f566f70e10f, 0x000aa51690f5ad49, 0x0006cefcf25d12f7, 0x000299945adb16e7, 0x000001f768aead23},
   {0x000cc77b6248febd, 0x00028ec3aaffd2b6, 0x0004ef486a3cd306, 0x0006c23ce1c0b80d, 0x00004c3bff2ea6f6},
   {0x000ec4094aeaeb5f, 0x000286e372ca73f2, 0x000e2a701d61b19b, 0x000e3ef5eefa966d, 0x000023b20565de55},
   {0x0001ca5279d58557, 0x000ce27c2874fe30, 0x000dcf1d6707b2d4, 0x000ff56a532cd8a9, 0x00002a52fee23f2b},
   {0x0004efb37cd8663d, 0x00020ffbd7594862, 0x0002d37445bbc7ac, 0x000ec6657b85e9c8, 0x00007b3052cb86a6},
   {0x0002f0ad2525e91e, 0x00043d28edca0348, 0x000e1b003a2cb680, 0x0001b0aaf4f6d052, 0x0000185f8c252978},
   {0x0001de5bd80ce0d6, 0x000416853e9d6aa4, 0x00057f4c3a9407b2, 0x0007bce563ec36e3, 0x00004cc4b8dd0e29},
   {0x000c1a52ffb8730e, 0x0006e67058e37a2f, 0x000ddf4ee11811f1, 0x000f09910f9a366c, 0x000072f4a0c4a0b9},
   {0x0006c06f663f4ea7, 0x000f74e970fba8c1, 0x00069ec345693b3a, 0x00080892102e7f1d, 0x00000ba53cbc968a},
   {0x000d9dc7fea15537, 0x000bb51536493ca3, 0x00044006b14c6824, 0x000cc60b98863148, 0x000040d2a72ab454},
   {0x0006a1b712570975, 0x00048debda657593, 0x00064330ea91b9d6, 0x00051d03344094bb, 0x0000006ba10d12ee},
   {0x00028468f5de5d58, 0x0004c38cc05b0192, 0x00056019900eb12f, 0x0000e0ba1039f9dd, 0x00004502d4ce4fff},
   {0x000054106837c189, 0x0004c6dd3b93ceb2, 0x000416d74fd0f654, 0x0002ef040727064c, 0x00006e15c6114b50},
   {0x0002a398cfb1a76b, 0x0007419f2f6b14df, 0x00066e604311256c, 0x0005b444a4979620, 0x0000705b3aab4135},
   {0x000ef536d797b1d8, 0x000d622ddf0db365, 0x0000575a8800076b, 0x000ca4d3bbf33b0e, 0x00003777aa05c8e4},
   {0x000745c85578db5f, 0x00049dbae5ae2392, 0x000adc98676fda41, 0x0001da3b1f0b00b8, 0x000009963437d36f},
   {0x00024e90a5dc3853, 0x000641f135cbd7e8, 0x0007ce8fccccb5f6, 0x000249f6736d86c8, 0x0000625f3ce26604},
   {0x000ac8059502f63f, 0x0000a2e351469af8, 0x00064b63050c05e7, 0x0003ac335292e9c7, 0x00001a394360c7e2},
   {0x0006d53251183264, 0x000bd43c2b74fd5c, 0x000b973f9b62065a, 0x0006e5eb5fbf5d03, 0x000013a3da366120},
   {0x000d5837725d94e5, 0x00012205016c5c6b, 0x0000033c6818e309, 0x00079872088ce157, 0x00007fba1f495c83},
   {0x000c7423f2f9079d, 0x0007b34023fc55a8, 0x0002fab351173515, 0x000e33ce4f9b49ad, 0x00006691ff72c878},
   {0x000c2adedc5eff3e, 0x000f1d8956cf4122, 0x000e9e5bdaf8dd4b, 0x000c743eb86205d9, 0x0000049b92b9d975},
   {0x00079730b0f6c05a, 0x000acc6f3a553a53, 0x00020dcd6d72a0ff, 0x000164ab0032c34b, 0x0000470e9dbc88d5},
   {0x000cf10ca237c047, 0x000711f6c81a2b19, 0x000dd80b43b65466, 0x000be8eb3321bd16, 0x000048c14f600c5f},
   {0x00051c264aa6c803, 0x00004a4fa7da6664, 0x0003128395b66e39, 0x000bc10d45f19b0b, 0x000031602627c3c9},
   {0x0000dc4832e4e10d, 0x0006756c717f7312, 0x0007280294eb20c4, 0x000c50900f52e3f6, 0x0000566d4fc14730},
   {0x000a5d40fd837206, 0x000dc7159547a7e3, 0x00068d6095c1e926, 0x000cea7216730fba, 0x000022e8c3843f69},
   {0x000074e8930e4b2b, 0x0000e84d1581633d, 0x0006ba2365b6e435, 0x000f3f35534c26ad, 0x00007773c12f89f1},
   {0x000a404da57962aa, 0x000a81999ce568cb, 0x00021692fc5b9897, 0x000c291508e862f1, 0x00003a81907fa093},
   {0x000ed0ff4725a510, 0x00010673fc5030dd, 0x000f1f4e8910d8cc, 0x000a44c5b9d151c9, 0x000032a5c1d5cb09},
   {0x000aa442b90541fb, 0x0007cc1b485db1e0, 0x000a9df2e55f85eb, 0x0002236bee595ce8, 0x000025e496c72242},
   {0x000f3c46cd0fe5b9, 0x0007ed2a433885ed, 0x000761e35234e75a, 0x000545ce488de11d, 0x00000e878a01a085},
   {0x00093c77e021bb04, 0x00043c7df899aba4, 0x000ae80d672b4d18, 0x00017949ea37a487, 0x000067a9958011e4},
   {0x0008051a6697b065, 0x0007d8d6ba6d44b5, 0x0003ca46c147e33f, 0x000db0dbb4da8d48, 0x000068becaa181c2},
   {0x000980e90b989aa5, 0x0004a2c93c99b8d8, 0x00096e73a2f95eb1, 0x000b56951c6c7c47, 0x00006e228363b5ef},
   {0x000bc0b02dd624c8, 0x0007dec8170eec6b, 0x0004cfafa9777eb4, 0x000bf9b3cde15a00, 0x00001dc6bc087160},
   {0x0007e043eec34002, 0x000677a68dc7f2e0, 0x000bd15b9a18e9fc, 0x0008253d8da03188, 0x000048fbc3bb0056},
   {0x00047d4cfb654ce1, 0x00082a058e2ad575, 0x000f154478d3565b, 0x000bb18f63eaf0bb, 0x000047531ef114df},
   {0x000c630a4278c587, 0x00046ca8e83f3e1e, 0x000adc0c2b5507d5, 0x000844e85e135c63, 0x00000aa7efa85682},
   {0x00091ba8b3e1f615, 0x000701fbe3ffa726, 0x0009bb786832b4e9, 0x00039e897b6d92e3, 0x00002cfe53dea02e},
   {0x000392cd85cd52b0, 0x000c910e29831687, 0x0009832d0627ff66, 0x000f8a097134556a, 0x0000269bb0360a84},
   {0x000e55457643f85c, 0x0008c9b597d1b706, 0x0006efa4723734a4, 0x000d9e07aee91e8c, 0x00005cd6abc198a9},
   {0x0004de06cb3ce41a, 0x000893402e1380e0, 0x00086e3772d8c6eb, 0x000a8c8904659bb6, 0x00007215c371746b},
   {0x0002a97eeae4a2d9, 0x000516394f2c5fd1, 0x000208f2949514b7, 0x00026b9266fd5809, 0x00005c847085619a},
   {0x00085410fed694ea, 0x000934a2ed254529, 0x000d3be4673c905b, 0x000e9e110bb47692, 0x0000063b3d2d69e5},
   {0x000726eedda57deb, 0x000ae10f41891472, 0x000b307614efb6c4, 0x0005b7c2b1641917, 0x0000117c554fc4f4},
   {0x000cf3118f9d8812, 0x0002050017939c07, 0x00071b282701dbd8, 0x00025ead7e803f41, 0x00001015e87487d2},
   {0x000de3fed23acc4d, 0x000c294a7be2dc58, 0x000c9cf45750db91, 0x000524a0b94d43d1, 0x00006b1640fa6e37},
   {0x000f346c5fda0d09, 0x00059fa4d3151692, 0x000777a296200b1c, 0x000fbcfb8c46f760, 0x00004b38395f3ffd},
   {0x00025e00be54d671, 0x00082bec8aba618d, 0x000b78b98260d505, 0x000043287ad8f263, 0x000050fdf64e9cda},
   {0x000567aac578dcf0, 0x0000ef2a3133b90f, 0x0002d9de71ef1e9b, 0x00001c70eebba924, 0x000015473c9bf031},
   {0x0007e8ae56b78095, 0x000666e6f078e7c7, 0x000348ba1fb678e7, 0x0003f0b2da0b9615, 0x00007cf931c1ff73},
   {0x000357f50a0a366c, 0x000f42b87d73226b, 0x00091cb2c0e9708c, 0x000bb4cc13aeea5f, 0x000035d90c991143},
   {0x0001c404a9a0d9dc, 0x000451972d25147c, 0x0003b38c31659e58, 0x0001f243875a8c47, 0x00001fbd9ed37956},
   {0x000abc6fd41ec28d, 0x000e3cd2a2dca11f, 0x000c4045957ef8df, 0x0002f2772e73b5d8, 0x00006135fa4954b7},
   {0x000c32a2de24b69c, 0x0008c1f095d88ccf, 0x000ac3f9293f5569, 0x0007eebbe3350ed5, 0x00005e9bf806ca47},
   {0x000e8fb63c309f68, 0x0003565e1f9f4e9c, 0x000a6393f15376f6, 0x0003506d1afcfb35, 0x00006632a1ede562},
   {0x000d6c390c2ded4c, 0x00081df04cb1f0b7, 0x0009ecc3c756cb32, 0x000a72a66305a124, 0x00005d588b60a38c},
   {0x000cbf78e8e5f42d, 0x0004b3c8a3eeca6e, 0x000bd2160486eeb4, 0x0006731ec219c48f, 0x00001aaf1af517c3},
   {0x0006a2836769bde7, 0x000622b1e2adbc30, 0x000bff94a6208280, 0x000f26b8027f51ff, 0x000076cfa1ce1124},
   {0x000b00562422abb6, 0x000d58f8c29c318e, 0x000531561af377c4, 0x0008a274dbbc207f, 0x00000253b7f08212},
   {0x000f091cb62c17e0, 0x000abd64628a93d1, 0x00009d42534860e1, 0x000e57652d174363, 0x0000356f97e13efa},
   {0x0001e11aa150535b, 0x000bb1dd878ccd35, 0x000ed92c983e6b45, 0x00085b80c776128b, 0x00001d34ae930328},
   {0x0000488ca85ba4c3, 0x000c33c9ce6ce4ba, 0x0007bda770985348, 0x000124a66124c6f9, 0x00000f81a0290654},
   {0x00009ca6569b86fd, 0x000fd18af9a2d9ed, 0x0003d8c20a811009, 0x000f26bff08d03f9, 0x000052a148199fae},
   {0x0003f9dc2d8d1b73, 0x0001873961a703e0, 0x0001a35970420580, 0x000d549c0d987f04, 0x000007aa1f15a1c0},
   {0x00046ce08cd27224, 0x0004f934e4239dfd, 0x0009897b596d0a02, 0x00095a2808a7a639, 0x00000a4556e9e13d},
   {0x000a991fe9c13045, 0x00048fe7751b8d21, 0x000bf300359b0e85, 0x000f7215da643cb4, 0x000077db28d63940},
   {0x000eeb614adc9011, 0x0009ae8c411ebfc5, 0x000d1dcf74522941, 0x0004cb59ec3e7787, 0x0000340d053e216e},
   {0x0007af39b48df2b4, 0x0002871a10a94cac, 0x000ca575edc0faec, 0x0003a4c140a69245, 0x00000cf1c3713427},
   {0x000e306ac224b8a5, 0x0007ccb4930b0c8e, 0x000acbe74f57eaee, 0x000657da1e806bda, 0x00007d9a62742eeb},
   {0x0006b6ef546c4830, 0x0001fddb36e2e9eb, 0x000f0d7105885cca, 0x0000412e6b9f383e, 0x000058654fef9d2e},
   {0x0005c4ffbe0e8e26, 0x000df9b31816ea90, 0x00002e88e1942de5, 0x000408d497d723f8, 0x000030684dea602f},
   {0x0005a278a3e6cb34, 0x0006f5b151dc421e, 0x000d77ca15aefb6e, 0x0008981b30b8e049, 0x000028c3c9cf53b9},
   {0x000fb721556cdd2a, 0x000a897022274287, 0x000a5432580d317c, 0x000642f7468c7423, 0x00004a7f11464eb5},
   {0x0007a4774d193aa6, 0x0006ea92129a1a23, 0x00087c1a88d86598, 0x000f5eb24c515ecf, 0x0000604003575f39},
   {0x0009f189570a9b27, 0x000de465e4b7847b, 0x000bb85c202b98ce, 0x0001901026df551d, 0x000074fcd91047e2},
   {0x0002a90a23c1bfa3, 0x0004e478519f613e, 0x000af6cf440cb007, 0x0002dbe5ff1cbbe3, 0x000067fe5438be81},
   {0x000cf64fa40f05b0, 0x0002f32283787d13, 0x000f0d2aea054dfb, 0x0000d4e4173915b7, 0x0000482f144f1f61},
   {0x00010201b47f8234, 0x000929e70b990f62, 0x000049567c5d0ae1, 0x0006f01dcd7f455b, 0x00007e93d0f1f091},
   {0x0009cbf18a7db4fa, 0x000bf6f74c62fdd7, 0x000b8291bdbe8391, 0x0001705027145d14, 0x0000585a73ea2cbf},
   {0x000ca03e928a0db2, 0x000a5742857e7485, 0x0006d551a710fc01, 0x000db8a2f482edbd, 0x00000f0433b5048f},
   {0x000a2e8dd7dc6247, 0x000d38cd4819a60d, 0x0001f6669788b4c9, 0x0007d7513033ac00, 0x0000273b24fe3b36},
   {0x0008f66a31b3b9d4, 0x000a494df49d5c6e, 0x0008b23da7281514, 0x000e548d1726fdfc, 0x00004b3ae7d103de},
   {0x00056e19ce4b9d7e, 0x000f186e3c61cc62, 0x000b8ec145ff5c5c, 0x0006574acc63ca34, 0x000074621888fee6},
   {0x000f409645290a1e, 0x000e3263a962e956, 0x000ec2647bef0bf8, 0x0007502ed6a50eb5, 0x00000694283a9dca},
   {0x000b963643a2dcd1, 0x000ea09fc5353769, 0x0003397eab42b7c8, 0x000d63a4f002aee1, 0x000063005e2c19b7},
   {0x000736da63023bea, 0x0006db12a99b7ca6, 0x000537c5e1966c7f, 0x00089eeace09390c, 0x00000b696063a1aa},
   {0x00003e97288c56e5, 0x0009f938c8be8ebb, 0x000b717f71432a9f, 0x0009d97a6a5a93d5, 0x00001a5fb4c3e18f},
   {0x0004e7ad1c60cdce, 0x00043fc02c4a01c9, 0x0007c46a20ee202a, 0x0007b588dafe4d86, 0x00000a10263c8ac2},
   {0x000ea9dfe4432a4a, 0x0007bbe9277c5d0d, 0x000212c71a856af8, 0x0001e91ce8472acc, 0x00006f151b6d9bbb},
   {0x00076c527ceed56a, 0x000b7fbf8faec267, 0x000d4609cc7d211c, 0x0000c4237ae66a6f, 0x00001f81b702d277},
   {0x0000b057eac58392, 0x000fe29744e9d2fb, 0x0007beb4f8e1dd89, 0x000d41ec964f8eb1, 0x000029571073c9a2},
   {0x0008a18981c0e254, 0x0009b65b22830a94, 0x000fcfd3c62df636, 0x000a01fa33eb2d75, 0x0000078cd6ec4199},
   {0x00084a41ad900d2f, 0x00078e2c74c524a5, 0x000431c97832142b, 0x0009fc268c4e8338, 0x00007f69ea900868},
   {0x0002c81e46a38265, 0x0002d04a832fd52f, 0x0005359e94fd7807, 0x00029d28cd7d5fa2, 0x00004de71b7454cc},
   {0x000b60ad1eda6ac9, 0x000dfdbc09c3a42e, 0x00033cc1910aad37, 0x000803c81004b71e, 0x000044e6be345122},
   {0x000e8388ba1920db, 0x00032150db00803f, 0x000af60c29f5d57c, 0x0001aee49c8c4281, 0x000021edb518de70},
   {0x00063e418f06dc99, 0x00099c166d7b87fb, 0x000e520a83a4460d, 0x000835824dd5248c, 0x00005ec3ad712b92},
   {0x00022a5fbd17930f, 0x00077d82570e3150, 0x0005783712a4f64a, 0x0000abb12bc8d691, 0x0000498194c0fc62},
   {0x0002d9d255686c82, 0x000d9193e21f038a, 0x00024a5484785c6b, 0x0000989e4d5c81ab, 0x000056307860b2e2},
   {0x000d55f78b4d74c4, 0x0004643350131429, 0x0008c71fff22f183, 0x00083ef1e60c2459, 0x000059f2f0149799},
   {0x00047d56eb494a44, 0x00054d636a18e46a, 0x0004491c3b3e22a8, 0x000cde7b346e1527, 0x00002ceafd4e5390},
   {0x000a8538be0d6675, 0x000bb50818e23ba8, 0x0005d304c34b9074, 0x00092c4cbdab8908, 0x000061a24fe0e561},
   {0x000615e6db525bcb, 0x00035a567e4cacb7, 0x000afcdd69dd7d8c, 0x0009766e6b4153ac, 0x00002d668e097f3c},
   {0x000e7e265ce55ef0, 0x000527cd4b967a57, 0x00092fd1e55d9f4e, 0x000f7aefbc836064, 0x0000090d52beb7c3},
   {0x0009515a1e7b4d7c, 0x0002599da44c009b, 0x0002c555041f266a, 0x00015cca1c49548e, 0x00007ef04287126f},
   {0x0001659dbd30ef15, 0x000eec4e0277bfed, 0x0005df32918b4ab9, 0x000f788884d6236a, 0x00001fd96ea6bf5c},
   {0x000161981f190d9a, 0x000507e6052c142a, 0x00085a2cd561d849, 0x00085d89fe113bf2, 0x00007c22d676dbad},
   {0x000770ed2bfbd27d, 0x000ece996f5a582e, 0x00009001504c05b2, 0x000bf64cd40a9c2b, 0x00005895319213d9},
   {0x000c5d703fea2e08, 0x0001258e2188ce7c, 0x0008205bf0b50c49, 0x0002d62cce30baa4, 0x0000537c659ccfa3},
   {0x0006623a98cfc088, 0x0001fa4d6aca437b, 0x0006a8d1b0fe9bed, 0x000957504d29b8e5, 0x0000725f71c40b51},
   {0x0007f89cd0339ce6, 0x0004469ddc18b28c, 0x0006a1652c8367b1, 0x0006c17883ada83a, 0x0000585f1974034d},
   {0x000fb266f1b19188, 0x00063e7c3521789c, 0x0004c0526ae63b48, 0x0004635d88c9da6b, 0x00003e035c9df095},
   {0x000d5412fb45de9d, 0x00032e4cff40ddd9, 0x00051d671cdd6845, 0x000f6904b5c999b1, 0x00002d8c2cc811e7},
   {0x0004be1d90055d40, 0x000df464aaf407f5, 0x0000e917bea464c5, 0x0006b3033979624f, 0x00002c018dc52735},
   {0x00015024e330b3d4, 0x00096691652d3a54, 0x000f9b59f173ff3d, 0x0008e5a94ec42c4e, 0x00000747201618d0},
   {0x000ca48aca411c53, 0x0002fcfa661194d6, 0x0001e227ff66415f, 0x000f7eb9c4dd4005, 0x000059810bc09a02},
   {0x000eb171b3dc101d, 0x000b99ffef68e2a7, 0x0003b359ea441c5a, 0x000112f32025c9b9, 0x00005e8ce0a71e9d},
   {0x000ccb92429503fd, 0x000752f095d55bfc, 0x00072d091ed271ba, 0x00003ba345ead5e9, 0x000018c8df11a831},
   {0x000d949a9aed0f4c, 0x000cb6660e37e90c, 0x0006c52e0bc5d1f4, 0x0008e0db8cac52d5, 0x00006e42e400c580},
   {0x00046966eeaefd23, 0x0000be39ecdcaa3b, 0x000683a51d0c4f1f, 0x000351b189dc8c9d, 0x000051f27f054c09},
   {0x00087ccd2a320682, 0x0005bb3df1c964c4, 0x00055cb8e8587ea9, 0x000d73dc8ccf79e5, 0x0000547dc829a206},
   {0x0002a6cd80c39b06, 0x000732000d4c6b82, 0x0001463b4de96d54, 0x0006e1d28535b6f9, 0x0000228f4660e248},
   {0x00099538de8d3abf, 0x0000045ebca6e987, 0x000221e7388cd833, 0x000d2bb79952a008, 0x00004322e1a7535c},
   {0x0004c11819d1801c, 0x000d84f3f5ec7b11, 0x0009260f4c2016e4, 0x0007266dd0e2df40, 0x00005ec362c0ae5f},
   {0x00062b18b8b2b4ee, 0x00050274d1afbc04, 0x00036b02d27cc8d9, 0x000ccd3f25f71054, 0x000043bbf8dcbff9},
   {0x000d1767a039e9df, 0x000a8f69d3583b6a, 0x00042931f5b0714d, 0x00009615e55fa18b, 0x00004ed5558f33c6},
   {0x00037901c647a5dd, 0x0001f8081d3571fe, 0x00013fd7a6593ddf, 0x000af610249a4fd8, 0x000069acca274e9c},
   {0x000ba3ea330721c9, 0x000c20e7e1ea0047, 0x0001314a6083423f, 0x00095271df4c0af0, 0x000009a62dab8928},
   {0x000325a49cc6cb00, 0x000c654b56cb6a5b, 0x000dc994a0e94b5d, 0x0004aad3be28779a, 0x00004296e8f8ba3a},
   {0x000689761e451eab, 0x0008bff59594a328, 0x0007a7084a2e4d59, 0x00020a849b96853d, 0x00004980a3196014},
   {0x0005b9e12f552c42, 0x000db7100fe96956, 0x0003add0d78a5318, 0x0004eda05c90b4d4, 0x0000538b4cd66a5d},
   {0x00094fc3e89f039f, 0x000f26f618045f4e, 0x000d4b9550592c9a, 0x000141908a36eb5f, 0x000025fffaf6c2ed},
   {0x00034459cc79d354, 0x000b4b1d5476b344, 0x0001615d99eeecbf, 0x000b773ddeb34a06, 0x00005129cecceb64},
   {0x0003215894993520, 0x0007cf14c0b3bee4, 0x0006bedad5772f9c, 0x0006a97d2e2fce30, 0x0000715f42b546f0},
   {0x000ecdceda5b5f1a, 0x00015a49741a9434, 0x0003edad2e0da171, 0x0009041680bd77c7, 0x0000487c02354edd},
   {0x000feff3a70ed9c4, 0x000a3e857e302b8e, 0x0008a2a5a056a32a, 0x000c444df3a68bd4, 0x000007f650b73176},
   {0x000b9b1626e0ccb1, 0x000c18b09fb36e38, 0x0009f9496479e053, 0x000f5c456d90319c, 0x00001ca941e7ac9f},
   {0x0004df29162fa0bb, 0x0003282b3330549c, 0x000abb437d8488cf, 0x000ad8695dfda14c, 0x00003391f78264d5},
   {0x000ae06ae2b5095d, 0x000d73259a946729, 0x00013921edd58a58, 0x000b592e9834262d, 0x000027fedafaa54b},
   {0x000dc5b829ad48bb, 0x00042499ee260a99, 0x000d7513fd5f0257, 0x000d938802c8ecd5, 0x000078ceb3ef3f6d},
   {0x0002f44f8a135d94, 0x00044828cdda3c34, 0x000537cfe77b9edb, 0x000b4c89436d11a0, 0x00005064b164ec1a},
   {0x0000eccfd37eb2fc, 0x0003ed90d25fc702, 0x000fa1bb341f31ea, 0x00030441b930d7bd, 0x00005344467a4811},
   {0x00073170f25e6dfb, 0x0001a50114cc8700, 0x0008fc4f00e385dc, 0x00040d82348698ac, 0x00002a77a55284dd},
   {0x0006afe0c98c6ce4, 0x00096dddfd6e4fe0, 0x0003bf1ed3c235df, 0x000bdaf1428d01e3, 0x0000785768ec9300},
   {0x0002e57a91deb63b, 0x000bfe5ce8b80970, 0x000d1d58ac61bdb8, 0x00057bc645b426f3, 0x00004804a82227a5},
   {0x0007048ab44d2601, 0x0001a4b3a69358e5, 0x0009e1c29368d650, 0x00063e2c39c9ec3f, 0x00004172f257d4de},
   {0x0008b450330c6401, 0x00017418f2391d36, 0x0000b7d90d040d30, 0x000d51f2c34bb609, 0x000016f649228fdf},
   {0x0006818e2b928ef5, 0x00091cdc11e72bea, 0x00077a36cde28ccf, 0x000fd0f594aaa68e, 0x0000313034806c7f},
   {0x000d27ac2249bd65, 0x00064018e95128a9, 0x0002b37ec719a3b4, 0x0007b21c26ccff35, 0x0000056f68341d79},
   {0x0009d6757efd2327, 0x000b6553afe155e7, 0x000eaf5a60fabdbc, 0x000743bd3e7222c6, 0x00007046c76d4dae},
   {0x000be872b18d4a55, 0x00018574e1496660, 0x00002bdcbb199925, 0x0008e8ec103053a3, 0x00003ed8e9800b21},
   {0x000b9239fa75e03e, 0x000684633c0837b0, 0x00091a7793efe9fb, 0x000fe3498a35fbe3, 0x00006065510fe2d0},
   {0x000b668548abad0c, 0x00048da87e52755c, 0x000107c1ddb45845, 0x000de352c43ecea0, 0x0000526028809372},
   {0x0005c56af9213b1f, 0x0004d017e98db341, 0x0005cf709b5bee1a, 0x0009ab613f6b105b, 0x00005ff20e3482b2},
   {0x00029c75cc2e6c90, 0x000ca3a70e2060aa, 0x0004b5c515fc7d73, 0x000c207899fc38fc, 0x0000250386b124ff},
   {0x000a28d5ae3d2b56, 0x0009dd6de60ce54e, 0x000f06d6c1991314, 0x0008fc716694fc58, 0x000046b23975eb01},
   {0x000a6a0fb4b7b4e2, 0x0005a8f7253de470, 0x000fbd3adb5d9247, 0x0006968abeee5b52, 0x00007fa20801a080},
   {0x0003faf19f7714d2, 0x000c12f4660c376f, 0x000212744eb3e840, 0x0002dd20fb4cd8df, 0x00004b065a251d3a},
   {0x000bde383d77cd4a, 0x000df882c9cb15ce, 0x00009af7596adf39, 0x0006422a2dd242eb, 0x00003147c0e50e5f},
   {0x000ca5101d1350db, 0x00079c33fc962164, 0x0003e5da08f8d134, 0x000f8bae640ce4d1, 0x00004bdee0c45061},
   {0x00046dc1a4edb1c9, 0x000b6437fd98ad7c, 0x0002a1c00b5514d7, 0x000710e58942f6bb, 0x00002dffb2ab1d70},
   {0x000fcf2fc18b6d68, 0x000a8b7806167ccd, 0x000e2937e3a8ebcb, 0x0006e8c980697f95, 0x000002fbba1cd012},
   {0x0008c360adff4565, 0x000d24bdb1519cb9, 0x0005fb15d5447101, 0x000a27d98c4c8747, 0x000005f8c76d6e22},
   {0x0009e5dffeb9f494, 0x0009d18406b75ab6, 0x0004a42d1dae09c7, 0x000fa84199b45fb9, 0x000079710aebae2a},
   {0x000deda7f334d2df, 0x00051af2a57b4a6a, 0x0006dceaa87bde9c, 0x000d07ba98fc64f8, 0x00006bbe0335c20e},
};

__ALIGN64 static const int64u U2_0[8] = 
    {0x000b1e0137d48290, 0x000b1e0137d48290, 0x000b1e0137d48290, 0x000b1e0137d48290,
     0x000b1e0137d48290, 0x000b1e0137d48290, 0x000b1e0137d48290, 0x000b1e0137d48290};
__ALIGN64 static const int64u U2_1[8] = 
    {0x00051eb4d1207816, 0x00051eb4d1207816, 0x00051eb4d1207816, 0x00051eb4d1207816,
     0x00051eb4d1207816, 0x00051eb4d1207816, 0x00051eb4d1207816, 0x00051eb4d1207816};
__ALIGN64 static const int64u U2_2[8] = 
    {0x000ca2b71d440f6a, 0x000ca2b71d440f6a, 0x000ca2b71d440f6a, 0x000ca2b71d440f6a,
     0x000ca2b71d440f6a, 0x000ca2b71d440f6a, 0x000ca2b71d440f6a, 0x000ca2b71d440f6a};
__ALIGN64 static const int64u U2_3[8] = 
    {0x00054cb52385f46d, 0x00054cb52385f46d, 0x00054cb52385f46d, 0x00054cb52385f46d,
     0x00054cb52385f46d, 0x00054cb52385f46d, 0x00054cb52385f46d, 0x00054cb52385f46d};
__ALIGN64 static const int64u U2_4[8] = 
    {0x0000215132111d83, 0x0000215132111d83, 0x0000215132111d83, 0x0000215132111d83,
     0x0000215132111d83, 0x0000215132111d83, 0x0000215132111d83, 0x0000215132111d83};

DLL_PUBLIC
mbx_status MB_FUNC_NAME(mbx_x25519_public_key_)(int8u* const pa_public_key[8],
                                    const int8u* const pa_private_key[8])
{
   mbx_status status = 0;

    /* test input pointers */
    if(NULL==pa_private_key || NULL==pa_public_key) {
      status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
      return status;
   }

   /* check pointers and values */
   int buf_no;
   for(buf_no=0; buf_no<8; buf_no++) {
      const int64u* own_private = (const int64u*) pa_private_key[buf_no];
      const int64u* party_public = (const int64u*) pa_public_key[buf_no];

      /* if any of pointer NULL set error status */
      if(NULL==own_private || NULL==party_public) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
         continue;
      }
   }

   /* continue processing if there are correct parameters */
   if( MBX_IS_ANY_OK_STS(status) ) {

      /* convert private to to MB8 and decode */
      __ALIGN64 U64 scalar_mb8[4];
      ifma_BNU_transpose_copy((int64u (*)[8])scalar_mb8, (const int64u * const*)pa_private_key, 256);
      /* decode keys into scalars according to RFC4448 */
      scalar_mb8[0] = and64_const(scalar_mb8[0], 0xfffffffffffffff8);
      scalar_mb8[3] = and64_const(scalar_mb8[3], 0x7fffffffffffffff);
      scalar_mb8[3] = or64(scalar_mb8[3], set64(0x4000000000000000));

      /* set up: "special" point S = {1:?:1} => {u1:z1} */
      __ALIGN64 U64 U1[5];
      __ALIGN64 U64 Z1[5];
      fe52mb8_set(U1, 1);
      fe52mb8_set(Z1, 1);

      /* set up: pre-computed G-S (base and "special") */
      __ALIGN64 U64 Z2[5];
      __ALIGN64 U64 U2[5] = { loadu64(U2_0), loadu64(U2_1), loadu64(U2_2), loadu64(U2_3), loadu64(U2_4) };
      fe52mb8_set(Z2, 1);

      __ALIGN64 U64 A[5], B[5], C[5], D[5];

      /*
         scalar template is usual [01xx..xxx] [xxx..xxx] [xxx..xxx] [xxx..000]
         processing scalar in L->R fashion:
         - first process scalar' = scalar >>3
         - than 3 doubling
      */
      __mb_mask swap = get_mask(0xff);

      /* read low and remove (zero) bits 0,1,2 */
      U64 e = loadu64(&scalar_mb8[0]);
      e = srli64(e, 3);

      int bitpos;
      for (bitpos=3; bitpos<255; bitpos++) {
         if(0==(bitpos%64))
            e = loadu64(&scalar_mb8[bitpos/64]);

         __mb_mask b = cmp64_mask(and64_const(e, 1), get_zero64(), _MM_CMPINT_NE);

         swap = mask_xor (swap, b);
         fe52mb8_cswap(U1, U2, swap);
         fe52mb8_cswap(Z1, Z2, swap);
         swap = b;

         /* load mu = muTBL[bitpos-3] */
         U64 mu[5];
         mu[0] = set64((long long)muTBL52[bitpos-3][0]);
         mu[1] = set64((long long)muTBL52[bitpos-3][1]);
         mu[2] = set64((long long)muTBL52[bitpos-3][2]);
         mu[3] = set64((long long)muTBL52[bitpos-3][3]);
         mu[4] = set64((long long)muTBL52[bitpos-3][4]);

         /* diff addition */
         fe52_sub(B, U1, Z1);          /* B = U1-Z1                 */
         fe52_add(A, U1, Z1);          /* A = U1+Z1                 */
         fe52_mul(C, mu, B);           /* C = mu*B                  */
         fe52_sub(B, A, C);            /* B = (U1+Z1) - mu*(U1-Z1)  */
         fe52_add(A, A, C);            /* A = (Ur+Z1) + mu*(U1-Z1)  */
         ed25519_sqr_dual(A, B, A, B);
         ed25519_mul_dual(U1, Z1, Z2, A, U2, B);

         e = srli64(e, 1);
      }

      /* 3 doublings */
      for(bitpos=0; bitpos<3; bitpos++) {
         fe52_add(A, U1, Z1);       /*  A = U1+Z1   */
         fe52_sub(B, U1, Z1);       /*  B = U1-Z1   */
         fe52_sqr(A, A);            /*  A = A^2     */
         fe52_sqr(B, B);            /*  B = B^*2    */
         fe52_sub(C, A, B);         /*  C = A-B     */
         fe52_mul121666(D, C);      /*  D = (A+2)/4 * C*/
         fe52_add(D, D, B);         /*  D = D+B     */
         fe52_mul(U1, A, B);        /*  U1 = A*B    */
         fe52_mul(Z1, C, D);        /*  Z1 = C*D    */
      }

      fe52mb8_inv_mod25519(A, Z1);
      fe52_mul(U1, U1, A);

      /* convert result back */
      ifma_mb8_to_BNU((int64u * const*)pa_public_key, (const int64u (*)[8])U1, 256);

      /* clear secret */
      MB_FUNC_NAME(zero_)((int64u (*)[8])scalar_mb8, sizeof(scalar_mb8)/sizeof(U64));
   }

   return status;
}

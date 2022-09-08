/*******************************************************************************
* Copyright 2019 Intel Corporation
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

#include "ocl/ocl_types.h"
#if WITH_ELTWISE == 1
#include "ocl/ocl_post_ops.h"
#endif

#if DT_F32 != 1
#error "Only f32 implemented."
#endif

#define DO_FMA_NN(hh, i_mod_16, i_div_16, i_mod_4, i_div_4) \
    do { \
        c[i_div_4].s##i_mod_4 \
                = mad(sub_group_broadcast(a[hh].s##i_div_16, i_mod_16), \
                        b.s##hh, c[i_div_4].s##i_mod_4); \
    } while (0)

#define DO_FMA_NT(hh, i_mod_16, i_div_16, i_mod_4, i_div_4) \
    do { \
        c[i_div_4].s##i_mod_4 \
                = mad(sub_group_broadcast(a[hh].s##i_div_16, i_mod_16), b[hh], \
                        c[i_div_4].s##i_mod_4); \
    } while (0)

#define DO_FMA_TN(hh, i, i_mod_4, i_div_4) \
    do { \
        c[i_div_4][0].s##i_mod_4 = mad(sub_group_broadcast(a.s##hh, i), \
                b[0].s##hh, c[i_div_4][0].s##i_mod_4); \
        c[i_div_4][1].s##i_mod_4 = mad(sub_group_broadcast(a.s##hh, i), \
                b[1].s##hh, c[i_div_4][1].s##i_mod_4); \
    } while (0)

#define DO_FMA_TT(hh, i, i_mod_4, i_div_4) \
    do { \
        c[i_div_4][0].s##i_mod_4 = mad(sub_group_broadcast(a.s##hh, i), \
                b[hh].s0, c[i_div_4][0].s##i_mod_4); \
        c[i_div_4][1].s##i_mod_4 = mad(sub_group_broadcast(a.s##hh, i), \
                b[hh].s1, c[i_div_4][1].s##i_mod_4); \
    } while (0)

#if !defined(TRANS_A)
#if !defined(TRANS_B)
#define NN
#define DO_FMA DO_FMA_NN
#else
#define NT
#define DO_FMA DO_FMA_NT
#endif
#else
#if !defined(TRANS_B)
#define TN
#define DO_FMA DO_FMA_TN
#else
#define TT
#define DO_FMA DO_FMA_TT
#endif
#endif

#if WITH_ELTWISE == 1
#define POST_OP(val) \
    do { \
        if (last_k_block) val = fwd_eltwise(val, eltwise_alpha, eltwise_beta); \
    } while (0)
#else
#define POST_OP(val)
#endif

#define FMA_I_LOOP_32_ROW(hh) \
    do { \
        DO_FMA(hh, 0, 0, 0, 0); \
        DO_FMA(hh, 1, 0, 1, 0); \
        DO_FMA(hh, 2, 0, 2, 0); \
        DO_FMA(hh, 3, 0, 3, 0); \
        DO_FMA(hh, 4, 0, 0, 1); \
        DO_FMA(hh, 5, 0, 1, 1); \
        DO_FMA(hh, 6, 0, 2, 1); \
        DO_FMA(hh, 7, 0, 3, 1); \
        DO_FMA(hh, 8, 0, 0, 2); \
        DO_FMA(hh, 9, 0, 1, 2); \
        DO_FMA(hh, 10, 0, 2, 2); \
        DO_FMA(hh, 11, 0, 3, 2); \
        DO_FMA(hh, 12, 0, 0, 3); \
        DO_FMA(hh, 13, 0, 1, 3); \
        DO_FMA(hh, 14, 0, 2, 3); \
        DO_FMA(hh, 15, 0, 3, 3); \
        DO_FMA(hh, 16, 1, 0, 4); \
        DO_FMA(hh, 17, 1, 1, 4); \
        DO_FMA(hh, 18, 1, 2, 4); \
        DO_FMA(hh, 19, 1, 3, 4); \
        DO_FMA(hh, 20, 1, 0, 5); \
        DO_FMA(hh, 21, 1, 1, 5); \
        DO_FMA(hh, 22, 1, 2, 5); \
        DO_FMA(hh, 23, 1, 3, 5); \
        DO_FMA(hh, 24, 1, 0, 6); \
        DO_FMA(hh, 25, 1, 1, 6); \
        DO_FMA(hh, 26, 1, 2, 6); \
        DO_FMA(hh, 27, 1, 3, 6); \
        DO_FMA(hh, 28, 1, 0, 7); \
        DO_FMA(hh, 29, 1, 1, 7); \
        DO_FMA(hh, 30, 1, 2, 7); \
        DO_FMA(hh, 31, 1, 3, 7); \
    } while (0)

#define FMA_I_LOOP_16_ROW(hh) \
    do { \
        DO_FMA(hh, 0, 0, 0); \
        DO_FMA(hh, 1, 1, 0); \
        DO_FMA(hh, 2, 2, 0); \
        DO_FMA(hh, 3, 3, 0); \
        DO_FMA(hh, 4, 0, 1); \
        DO_FMA(hh, 5, 1, 1); \
        DO_FMA(hh, 6, 2, 1); \
        DO_FMA(hh, 7, 3, 1); \
        DO_FMA(hh, 8, 0, 2); \
        DO_FMA(hh, 9, 1, 2); \
        DO_FMA(hh, 10, 2, 2); \
        DO_FMA(hh, 11, 3, 2); \
        DO_FMA(hh, 12, 0, 3); \
        DO_FMA(hh, 13, 1, 3); \
        DO_FMA(hh, 14, 2, 3); \
        DO_FMA(hh, 15, 3, 3); \
    } while (0)

#define UPDATE_C_ROW(i, ii, betaZero) \
    do { \
        if (jrem > 0) \
            if (irem > i) { \
                float val = alpha * c[i / 4].s##ii \
                        + ((betaZero) ? 0 : beta * *C); \
                POST_OP(val); \
                *C = val; \
            } \
        C++; \
    } while (0)

#define UPDATE_C_ROW_2X(i, ii, betaZero) \
    do { \
        if (irem > i) { \
            if (jrem > 0) { \
                float val = alpha * c[i / 4][0].s##ii \
                        + ((betaZero) ? 0 : beta * *(C_ptrs[0])); \
                POST_OP(val); \
                *(C_ptrs[0]) = val; \
            } \
            if (jrem > 16) { \
                float val = alpha * c[i / 4][1].s##ii \
                        + ((betaZero) ? 0 : beta * *(C_ptrs[1])); \
                POST_OP(val); \
                *(C_ptrs[1]) = val; \
            } \
        } \
        C_ptrs[0]++; \
        C_ptrs[1]++; \
    } while (0)

#define UPDATE_C_32_ROW(betaZero) \
    do { \
        UPDATE_C_ROW(0, 0, betaZero); \
        UPDATE_C_ROW(1, 1, betaZero); \
        UPDATE_C_ROW(2, 2, betaZero); \
        UPDATE_C_ROW(3, 3, betaZero); \
        UPDATE_C_ROW(4, 0, betaZero); \
        UPDATE_C_ROW(5, 1, betaZero); \
        UPDATE_C_ROW(6, 2, betaZero); \
        UPDATE_C_ROW(7, 3, betaZero); \
        UPDATE_C_ROW(8, 0, betaZero); \
        UPDATE_C_ROW(9, 1, betaZero); \
        UPDATE_C_ROW(10, 2, betaZero); \
        UPDATE_C_ROW(11, 3, betaZero); \
        UPDATE_C_ROW(12, 0, betaZero); \
        UPDATE_C_ROW(13, 1, betaZero); \
        UPDATE_C_ROW(14, 2, betaZero); \
        UPDATE_C_ROW(15, 3, betaZero); \
        UPDATE_C_ROW(16, 0, betaZero); \
        UPDATE_C_ROW(17, 1, betaZero); \
        UPDATE_C_ROW(18, 2, betaZero); \
        UPDATE_C_ROW(19, 3, betaZero); \
        UPDATE_C_ROW(20, 0, betaZero); \
        UPDATE_C_ROW(21, 1, betaZero); \
        UPDATE_C_ROW(22, 2, betaZero); \
        UPDATE_C_ROW(23, 3, betaZero); \
        UPDATE_C_ROW(24, 0, betaZero); \
        UPDATE_C_ROW(25, 1, betaZero); \
        UPDATE_C_ROW(26, 2, betaZero); \
        UPDATE_C_ROW(27, 3, betaZero); \
        UPDATE_C_ROW(28, 0, betaZero); \
        UPDATE_C_ROW(29, 1, betaZero); \
        UPDATE_C_ROW(30, 2, betaZero); \
        UPDATE_C_ROW(31, 3, betaZero); \
    } while (0)

#define UPDATE_C_16_ROW(betaZero) \
    do { \
        UPDATE_C_ROW_2X(0, 0, betaZero); \
        UPDATE_C_ROW_2X(1, 1, betaZero); \
        UPDATE_C_ROW_2X(2, 2, betaZero); \
        UPDATE_C_ROW_2X(3, 3, betaZero); \
        UPDATE_C_ROW_2X(4, 0, betaZero); \
        UPDATE_C_ROW_2X(5, 1, betaZero); \
        UPDATE_C_ROW_2X(6, 2, betaZero); \
        UPDATE_C_ROW_2X(7, 3, betaZero); \
        UPDATE_C_ROW_2X(8, 0, betaZero); \
        UPDATE_C_ROW_2X(9, 1, betaZero); \
        UPDATE_C_ROW_2X(10, 2, betaZero); \
        UPDATE_C_ROW_2X(11, 3, betaZero); \
        UPDATE_C_ROW_2X(12, 0, betaZero); \
        UPDATE_C_ROW_2X(13, 1, betaZero); \
        UPDATE_C_ROW_2X(14, 2, betaZero); \
        UPDATE_C_ROW_2X(15, 3, betaZero); \
    } while (0)

#ifdef NN
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_kernel(global float *A, global float *B, global float *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, float alpha, float beta, int last_k_block,
        float eltwise_alpha, float eltwise_beta) {
    float2 a[4]; // 32 x 4  block of A, 4x 32x1 block accesses   [col major]
    float4 b; // 4  x 16 block of B, 1x 4x16 scattered access [row major]
    float4 c[8]; // 32 x 16 block of C, 8x 4x16 scattered access [row major]

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 32;
    int j0 = idN;

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0;
    B += offset_b + j0 * ldb;
    C += offset_c + i0 + j0 * ldc;

    global float *A_cols[4] = {A, A + lda, A + 2 * lda, A + 3 * lda};

    int ldax4 = lda << 2;
    int ldbx4 = ldb << 2;

    for (int z = 0; z < 8; z++)
        c[z] = 0.f;

    for (int h = 0; h < (k >> 2); h++) {
        // Load A
        for (int j = 0; j < 4; j++) {
            a[j] = as_float2(
                    intel_sub_group_block_read2((global uint *)A_cols[j]));
            A_cols[j] += ldax4;
        }

        // Load B
        b = vload4(0, B);
        B += 4;

        // FMAs
        FMA_I_LOOP_32_ROW(0);
        FMA_I_LOOP_32_ROW(1);
        FMA_I_LOOP_32_ROW(2);
        FMA_I_LOOP_32_ROW(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        // Load A
        for (int j = 0; j < 4; j++)
            a[j] = as_float2(
                    intel_sub_group_block_read2((global uint *)A_cols[j]));

        // Load B
        b = vload4(0, B);

        FMA_I_LOOP_32_ROW(0);
        if (krem > 1) FMA_I_LOOP_32_ROW(1);
        if (krem > 2) FMA_I_LOOP_32_ROW(2);
    }

    // Update C.
    if (beta == 0)
        UPDATE_C_32_ROW(1);
    else
        UPDATE_C_32_ROW(0);
}
#endif

#ifdef NT
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_kernel(global float *A, global float *B, global float *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, float alpha, float beta, int last_k_block,
        float eltwise_alpha, float eltwise_beta) {
    float2 a[2]; // 32 x 2  block of A, 2x 32x1 block accesses   [col major]
    float b[2]; // 2  x 16 block of B, 2x 1x16 block accesses   [row major]
    float4 c[8]; // 32 x 16 block of C, 8x 4x16 scattered access [row major]

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 32;
    int j0 = idN;

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0;
    B += offset_b + j0;
    C += offset_c + i0 + j0 * ldc;

    global float *A_cols[2] = {A, A + lda};
    global float *B_rows[2] = {B, B + ldb};

    int ldax2 = lda << 1;
    int ldbx2 = ldb << 1;

    for (int z = 0; z < 8; z++)
        c[z] = 0.f;

    for (int h = 0; h < (k >> 1); h++) {
        // Load A
        for (int j = 0; j < 2; j++) {
            a[j] = as_float2(
                    intel_sub_group_block_read2((global uint *)A_cols[j]));
            A_cols[j] += ldax2;
        }

        // Load B
        for (int i = 0; i < 2; i++) {
            b[i] = as_float(
                    intel_sub_group_block_read((global uint *)B_rows[i]));
            B_rows[i] += ldbx2;
        }

        // FMAs
        FMA_I_LOOP_32_ROW(0);
        FMA_I_LOOP_32_ROW(1);
    }

    int krem = k & 1;
    if (krem > 0) {
        // Load A
        a[0] = as_float2(intel_sub_group_block_read2((global uint *)A_cols[0]));

        // Load B
        b[0] = as_float(intel_sub_group_block_read((global uint *)B_rows[0]));

        FMA_I_LOOP_32_ROW(0);
    }

    // Update C.
    if (beta == 0)
        UPDATE_C_32_ROW(1);
    else
        UPDATE_C_32_ROW(0);
}
#endif

#ifdef TN
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_kernel(global float *A, global float *B, global float *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, float alpha, float beta, int last_k_block,
        float eltwise_alpha, float eltwise_beta) {
    float4 a; // 16 x 4  block of A, 1x     16x4 scattered [col major]
    float4 b[2]; // 4  x 32 block of B, 2x     4x16 scattered [row major]
    float4 c[4][2]; // 16 x 32 block of C, (4x2)x 4x16 scattered [row major]

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 16;
    int j0 = ((idN & -0x10) << 1) + (idN & 0xF);

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0 * lda;
    B += offset_b + j0 * ldb;
    C += offset_c + i0 + j0 * ldc;

    A += get_sub_group_local_id() * lda;

    global float *B_ptrs[2] = {B, B + 16 * ldb};

    for (int ii = 0; ii < 4; ii++)
        for (int jj = 0; jj < 2; jj++)
            c[ii][jj] = 0.f;

    for (int h = 0; h < (k >> 2); h++) {
        // Load A
        a = vload4(0, A);
        A += 4;

        // Load B
        for (int jj = 0; jj < 2; jj++) {
            b[jj] = vload4(0, B_ptrs[jj]);
            B_ptrs[jj] += 4;
        }

        // FMAs
        FMA_I_LOOP_16_ROW(0);
        FMA_I_LOOP_16_ROW(1);
        FMA_I_LOOP_16_ROW(2);
        FMA_I_LOOP_16_ROW(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        // Load A
        a = vload4(0, A);

        // Load B
        for (int jj = 0; jj < 2; jj++)
            b[jj] = vload4(0, B_ptrs[jj]);

        FMA_I_LOOP_16_ROW(0);
        if (krem > 1) FMA_I_LOOP_16_ROW(1);
        if (krem > 2) FMA_I_LOOP_16_ROW(2);
    }

    // Update C.
    global float *C_ptrs[2] = {C, C + 16 * ldc};

    if (beta == 0)
        UPDATE_C_16_ROW(1);
    else
        UPDATE_C_16_ROW(0);
}
#endif

#ifdef TT
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_kernel(global float *A, global float *B, global float *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, float alpha, float beta, int last_k_block,
        float eltwise_alpha, float eltwise_beta) {
    float4 a; // 16 x 4  block of A, 1x     16x4 scattered [col major]
    float2 b[4]; // 4  x 32 block of B, 4x     1x32 block     [row major]
    float4 c[4][2]; // 16 x 32 block of C, (4x2)x 4x16 scattered [row major]

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 16;
    int j0 = ((idN & -0x10) * 2) + (idN & 0xF);

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0 * lda;
    B += offset_b + j0;
    C += offset_c + i0 + j0 * ldc;

    A += get_sub_group_local_id() * lda;

    global float *B_rows[4] = {B, B + ldb, B + 2 * ldb, B + 3 * ldb};

    int ldbx4 = ldb << 2;

    for (int ii = 0; ii < 4; ii++)
        for (int jj = 0; jj < 2; jj++)
            c[ii][jj] = 0.f;

    for (int h = 0; h < (k >> 2); h++) {
        // Load A
        a = vload4(0, A);
        A += 4;

        // Load B
        for (int i = 0; i < 4; i++) {
            b[i] = as_float2(
                    intel_sub_group_block_read2((global uint *)B_rows[i]));
            B_rows[i] += ldbx4;
        }

        // FMAs
        FMA_I_LOOP_16_ROW(0);
        FMA_I_LOOP_16_ROW(1);
        FMA_I_LOOP_16_ROW(2);
        FMA_I_LOOP_16_ROW(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        // Load A
        a = vload4(0, A);

        // Load B
        for (int i = 0; i < 4; i++)
            b[i] = as_float2(
                    intel_sub_group_block_read2((global uint *)B_rows[i]));

        FMA_I_LOOP_16_ROW(0);
        if (krem > 1) FMA_I_LOOP_16_ROW(1);
        if (krem > 2) FMA_I_LOOP_16_ROW(2);
    }

    // Update C.
    global float *C_ptrs[2] = {C, C + 16 * ldc};

    if (beta == 0)
        UPDATE_C_16_ROW(1);
    else
        UPDATE_C_16_ROW(0);
}
#endif

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

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#define VLOAD4_ALIGNED(o, p) *((global half4 *)p + o)

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

#define DO_FMA_NN(h, i, i_mod_2, i_div_2, i_mod_16, i_div_16) \
    do { \
        c[i][0] = mad(sub_group_broadcast(a[h].s##i_mod_2, i_div_2), \
                b[0].s##h, c[i][0]); \
        c[i][1] = mad(sub_group_broadcast(a[h].s##i_mod_2, i_div_2), \
                b[1].s##h, c[i][1]); \
    } while (0)

#define DO_FMA_NT(h, i, i_mod_2, i_div_2, i_mod_16, i_div_16) \
    do { \
        c[i][0] = mad(sub_group_broadcast(a[h].s##i_mod_2, i_div_2), b[h].s0, \
                c[i][0]); \
        c[i][1] = mad(sub_group_broadcast(a[h].s##i_mod_2, i_div_2), b[h].s1, \
                c[i][1]); \
    } while (0)

#define DO_FMA_TN(h, i, i_mod_2, i_div_2, i_mod_16, i_div_16) \
    do { \
        c[i][0] = mad(sub_group_broadcast(a[i_div_16].s##h, i_mod_16), \
                b[0].s##h, c[i][0]); \
        c[i][1] = mad(sub_group_broadcast(a[i_div_16].s##h, i_mod_16), \
                b[1].s##h, c[i][1]); \
    } while (0)

#define DO_FMA_TT(h, i, i_mod_2, i_div_2, i_mod_16, i_div_16) \
    do { \
        c[i][0] = mad(sub_group_broadcast(a[i_div_16].s##h, i_mod_16), \
                b[h].s0, c[i][0]); \
        c[i][1] = mad(sub_group_broadcast(a[i_div_16].s##h, i_mod_16), \
                b[h].s1, c[i][1]); \
    } while (0)

#if WITH_ELTWISE == 1
#define POST_OP(val) \
    do { \
        if (last_k_block) val = fwd_eltwise(val, eltwise_alpha, eltwise_beta); \
    } while (0)
#else
#define POST_OP(val)
#endif

#define FMA_I_LOOP(h) \
    do { \
        DO_FMA(h, 0, 0, 0, 0, 0); \
        DO_FMA(h, 1, 1, 0, 1, 0); \
        DO_FMA(h, 2, 0, 1, 2, 0); \
        DO_FMA(h, 3, 1, 1, 3, 0); \
        DO_FMA(h, 4, 0, 2, 4, 0); \
        DO_FMA(h, 5, 1, 2, 5, 0); \
        DO_FMA(h, 6, 0, 3, 6, 0); \
        DO_FMA(h, 7, 1, 3, 7, 0); \
        DO_FMA(h, 8, 0, 4, 8, 0); \
        DO_FMA(h, 9, 1, 4, 9, 0); \
        DO_FMA(h, 10, 0, 5, 10, 0); \
        DO_FMA(h, 11, 1, 5, 11, 0); \
        DO_FMA(h, 12, 0, 6, 12, 0); \
        DO_FMA(h, 13, 1, 6, 13, 0); \
        DO_FMA(h, 14, 0, 7, 14, 0); \
        DO_FMA(h, 15, 1, 7, 15, 0); \
        DO_FMA(h, 16, 0, 8, 0, 1); \
        DO_FMA(h, 17, 1, 8, 1, 1); \
        DO_FMA(h, 18, 0, 9, 2, 1); \
        DO_FMA(h, 19, 1, 9, 3, 1); \
        DO_FMA(h, 20, 0, 10, 4, 1); \
        DO_FMA(h, 21, 1, 10, 5, 1); \
        DO_FMA(h, 22, 0, 11, 6, 1); \
        DO_FMA(h, 23, 1, 11, 7, 1); \
        DO_FMA(h, 24, 0, 12, 8, 1); \
        DO_FMA(h, 25, 1, 12, 9, 1); \
        DO_FMA(h, 26, 0, 13, 10, 1); \
        DO_FMA(h, 27, 1, 13, 11, 1); \
        DO_FMA(h, 28, 0, 14, 12, 1); \
        DO_FMA(h, 29, 1, 14, 13, 1); \
        DO_FMA(h, 30, 0, 15, 14, 1); \
        DO_FMA(h, 31, 1, 15, 15, 1); \
    } while (0)

#define UPDATE_C_ROW(i, betaZero) \
    do { \
        if (irem > i) { \
            if (jrem > 0) { \
                half val = alpha * c[i][0] + ((betaZero) ? 0 : beta * *C); \
                POST_OP(val); \
                *C = val; \
            } \
            if (jrem > 16) { \
                half val = alpha * c[i][1] + ((betaZero) ? 0 : beta * *C2); \
                POST_OP(val); \
                *C2 = val; \
            } \
        } \
        C++; \
        C2++; \
    } while (0)

#define UPDATE_C(betaZero) \
    do { \
        UPDATE_C_ROW(0, betaZero); \
        UPDATE_C_ROW(1, betaZero); \
        UPDATE_C_ROW(2, betaZero); \
        UPDATE_C_ROW(3, betaZero); \
        UPDATE_C_ROW(4, betaZero); \
        UPDATE_C_ROW(5, betaZero); \
        UPDATE_C_ROW(6, betaZero); \
        UPDATE_C_ROW(7, betaZero); \
        UPDATE_C_ROW(8, betaZero); \
        UPDATE_C_ROW(9, betaZero); \
        UPDATE_C_ROW(10, betaZero); \
        UPDATE_C_ROW(11, betaZero); \
        UPDATE_C_ROW(12, betaZero); \
        UPDATE_C_ROW(13, betaZero); \
        UPDATE_C_ROW(14, betaZero); \
        UPDATE_C_ROW(15, betaZero); \
        UPDATE_C_ROW(16, betaZero); \
        UPDATE_C_ROW(17, betaZero); \
        UPDATE_C_ROW(18, betaZero); \
        UPDATE_C_ROW(19, betaZero); \
        UPDATE_C_ROW(20, betaZero); \
        UPDATE_C_ROW(21, betaZero); \
        UPDATE_C_ROW(22, betaZero); \
        UPDATE_C_ROW(23, betaZero); \
        UPDATE_C_ROW(24, betaZero); \
        UPDATE_C_ROW(25, betaZero); \
        UPDATE_C_ROW(26, betaZero); \
        UPDATE_C_ROW(27, betaZero); \
        UPDATE_C_ROW(28, betaZero); \
        UPDATE_C_ROW(29, betaZero); \
        UPDATE_C_ROW(30, betaZero); \
        UPDATE_C_ROW(31, betaZero); \
    } while (0)

#ifdef NN
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_f16_kernel(global half *A, global half *B, global half *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, half alpha, half beta, int last_k_block,
        half eltwise_alpha, half eltwise_beta) {

    half2 a[4]; // 32 x 4  block of A,      4x 32x1 block accesses
    half4 b[2]; // 4  x 32 block of B,      2x 4x16 scattered access
    half c[32][2]; // 32 x 32 block of C, (32x2)x 1x16 scattered access

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 32;
    int j0 = (idN & -0x10) * 2 + (idN & 0xF);

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0;
    B += offset_b + j0 * ldb;
    C += offset_c + i0 + j0 * ldc;

    global half *A_ptrs[4] = {A, A + lda, A + 2 * lda, A + 3 * lda};
    global half *B_ptrs[2] = {B, B + 16 * ldb};

    for (int y = 0; y < 32; y++)
        for (int z = 0; z < 2; z++)
            c[y][z] = 0;

    int k_align = k & ~3;

    for (int h = 0; h < k_align; h += 4) {
        // Load A
        for (int hh = 0; hh < 4; hh++)
            a[hh] = as_half2(intel_sub_group_block_read(
                    (global uint *)(A_ptrs[hh] + h * lda)));

        // Load B
        for (int j = 0; j < 2; j++)
            b[j] = VLOAD4_ALIGNED(0, (B_ptrs[j] + h));

        // FMAs
        FMA_I_LOOP(0);
        FMA_I_LOOP(1);
        FMA_I_LOOP(2);
        FMA_I_LOOP(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        for (int hh = 0; hh < 3; hh++)
            a[hh] = as_half2(intel_sub_group_block_read(
                    (global uint *)(A_ptrs[hh] + k_align * lda)));

        for (int j = 0; j < 2; j++)
            b[j] = VLOAD4_ALIGNED(0, (B_ptrs[j] + k_align));

        FMA_I_LOOP(0);
        if (krem > 1) FMA_I_LOOP(1);
        if (krem > 2) FMA_I_LOOP(2);
    }

    global half *C2 = C + 16 * ldc;

    // Update C.
    if (beta == 0)
        UPDATE_C(1);
    else
        UPDATE_C(0);
}
#endif

#ifdef NT
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_f16_kernel(global half *A, global half *B, global half *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, half alpha, half beta, int last_k_block,
        half eltwise_alpha, half eltwise_beta) {

    half2 a[4]; // 32 x 4  block of A,      4x 32x1 block access
    half2 b[4]; // 4  x 32 block of B,      4x 1x32 block access
    half c[32][2]; // 32 x 32 block of C, (32x2)x 1x16 scattered access

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 32;
    int j00 = (idN & -0x10) * 2;
    int j0 = j00 + (idN & 0xF);

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0;
    B += offset_b + j00;
    C += offset_c + i0 + j0 * ldc;

    global half *A_ptrs[4] = {A, A + lda, A + 2 * lda, A + 3 * lda};
    global half *B_ptrs[4] = {B, B + ldb, B + 2 * ldb, B + 3 * ldb};

    for (int y = 0; y < 32; y++)
        for (int z = 0; z < 2; z++)
            c[y][z] = 0;

    int k_align = k & ~3;

    for (int h = 0; h < k_align; h += 4) {
        for (int hh = 0; hh < 4; hh++) {
            a[hh] = as_half2(intel_sub_group_block_read(
                    (global uint *)(A_ptrs[hh] + h * lda)));
            b[hh] = as_half2(intel_sub_group_block_read_us2(
                    (global ushort *)(B_ptrs[hh] + h * ldb)));
        }

        FMA_I_LOOP(0);
        FMA_I_LOOP(1);
        FMA_I_LOOP(2);
        FMA_I_LOOP(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        for (int hh = 0; hh < 3; hh++) {
            a[hh] = as_half2(intel_sub_group_block_read(
                    (global uint *)(A_ptrs[hh] + k_align * lda)));
            b[hh] = as_half2(intel_sub_group_block_read_us2(
                    (global ushort *)(B_ptrs[hh] + k_align * ldb)));
        }

        FMA_I_LOOP(0);
        if (krem > 1) FMA_I_LOOP(1);
        if (krem > 2) FMA_I_LOOP(2);
    }

    global half *C2 = C + 16 * ldc;

    // Update C.
    if (beta == 0)
        UPDATE_C(1);
    else
        UPDATE_C(0);
}
#endif

#ifdef TN
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_f16_kernel(global half *A, global half *B, global half *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, half alpha, half beta, int last_k_block,
        half eltwise_alpha, half eltwise_beta) {

    half4 a[2]; // 32 x 4  block of A,      2x 16x4 scattered access
    half4 b[2]; // 4  x 32 block of B,      2x 4x16 scattered access
    half c[32][2]; // 32 x 32 block of C, (32x2)x 1x16 scattered access

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 32;
    int j0 = (idN & -0x10) * 2 + (idN & 0xF);

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0 * lda;
    B += offset_b + j0 * ldb;
    C += offset_c + i0 + j0 * ldc;

    A += get_sub_group_local_id() * lda;

    global half *A_ptrs[2] = {A, A + 16 * lda};
    global half *B_ptrs[2] = {B, B + 16 * ldb};

    for (int y = 0; y < 32; y++)
        for (int z = 0; z < 2; z++)
            c[y][z] = 0;

    int k_align = k & ~3;

    for (int h = 0; h < k_align; h += 4) {
        for (int z = 0; z < 2; z++) {
            a[z] = VLOAD4_ALIGNED(0, (A_ptrs[z] + h));
            b[z] = VLOAD4_ALIGNED(0, (B_ptrs[z] + h));
        }

        FMA_I_LOOP(0);
        FMA_I_LOOP(1);
        FMA_I_LOOP(2);
        FMA_I_LOOP(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        for (int z = 0; z < 2; z++) {
            a[z] = VLOAD4_ALIGNED(0, (A_ptrs[z] + k_align));
            b[z] = VLOAD4_ALIGNED(0, (B_ptrs[z] + k_align));
        }

        FMA_I_LOOP(0);
        if (krem > 1) FMA_I_LOOP(1);
        if (krem > 2) FMA_I_LOOP(2);
    }

    global half *C2 = C + 16 * ldc;

    // Update C.
    if (beta == 0)
        UPDATE_C(1);
    else
        UPDATE_C(0);
}
#endif

#ifdef TT
__attribute__((intel_reqd_sub_group_size(16))) // attr:no-format
kernel void
gen9_gemm_nocopy_f16_kernel(global half *A, global half *B, global half *C,
        long offset_a, long offset_b, long offset_c, int lda, int ldb, int ldc,
        int m, int n, int k, half alpha, half beta, int last_k_block,
        half eltwise_alpha, half eltwise_beta) {

    half4 a[2]; // 32 x 4  block of A,      2x 16x4 scattered access
    half2 b[4]; // 4  x 32 block of B,      4x 1x32 block access
    half c[32][2]; // 32 x 32 block of C, (32x2)x 1x16 scattered access

    int idM = get_global_id(1);
    int idN = get_global_id(0);

    int i0 = idM * 32;
    int j00 = (idN & -0x10) * 2;
    int j0 = j00 + (idN & 0xF);

    int irem = m - i0;
    int jrem = n - j0;
    if (irem < 0) irem = 0;
    if (jrem < 0) jrem = 0;

    A += offset_a + i0 * lda;
    B += offset_b + j00;
    C += offset_c + i0 + j0 * ldc;

    A += get_sub_group_local_id() * lda;

    global half *A_ptrs[2] = {A, A + 16 * lda};
    global half *B_ptrs[4] = {B, B + ldb, B + 2 * ldb, B + 3 * ldb};

    for (int y = 0; y < 32; y++)
        for (int z = 0; z < 2; z++)
            c[y][z] = 0;

    int k_align = k & ~3;

    for (int h = 0; h < k_align; h += 4) {
        for (int z = 0; z < 2; z++)
            a[z] = VLOAD4_ALIGNED(0, (A_ptrs[z] + h));

        for (int hh = 0; hh < 4; hh++)
            b[hh] = as_half2(intel_sub_group_block_read_us2(
                    (global ushort *)(B_ptrs[hh] + h * ldb)));

        FMA_I_LOOP(0);
        FMA_I_LOOP(1);
        FMA_I_LOOP(2);
        FMA_I_LOOP(3);
    }

    int krem = k & 3;
    if (krem > 0) {
        for (int z = 0; z < 2; z++)
            a[z] = VLOAD4_ALIGNED(0, (A_ptrs[z] + k_align));

        for (int hh = 0; hh < 3; hh++)
            b[hh] = as_half2(intel_sub_group_block_read_us2(
                    (global ushort *)(B_ptrs[hh] + k_align * ldb)));

        FMA_I_LOOP(0);
        if (krem > 1) FMA_I_LOOP(1);
        if (krem > 2) FMA_I_LOOP(2);
    }

    global half *C2 = C + 16 * ldc;

    // Update C.
    if (beta == 0)
        UPDATE_C(1);
    else
        UPDATE_C(0);
}
#endif

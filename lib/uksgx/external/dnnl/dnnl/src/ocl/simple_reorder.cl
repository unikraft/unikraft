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

#define DT_UNDEF 1
#include "ocl/ocl_types.h"

#include "ocl/ocl_math_utils.h"

#if SRC_DT_F16 || DST_DT_F16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#undef SRC_OFF
#undef DST_OFF

#define SRC_OFF(x0, x1, x2, x3, x4, x5) OFF_MD(SRC, x0, x1, x2, x3, x4, x5)
#define DST_OFF(x0, x1, x2, x3, x4, x5) OFF_MD(DST, x0, x1, x2, x3, x4, x5)

#if WITH_GROUP
#define SRC_OFF_G(gr, x0, x1, x2, x3, x4) OFF_MD(SRC, gr, x0, x1, x2, x3, x4)
#define DST_OFF_G(gr, x0, x1, x2, x3, x4) OFF_MD(DST, gr, x0, x1, x2, x3, x4)
#else
#define SRC_OFF_G(gr, x0, x1, x2, x3, x4) OFF_MD(SRC, x0, x1, x2, x3, x4, 0)
#define DST_OFF_G(gr, x0, x1, x2, x3, x4) OFF_MD(DST, x0, x1, x2, x3, x4, 0)
#endif

#if SRC_DT_S8 || SRC_DT_U8
#define SRC_BLOCK_READ(src) \
    as_char(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_char8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // SRC_DT_S8

#if SRC_DT_U8
#define SRC_BLOCK_READ(src) \
    as_uchar(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_uchar8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // SRC_DT_U8

#if SRC_DT_F16
#define SRC_BLOCK_READ(src) \
    as_half(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_half8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // SRC_DT_F16

#if SRC_DT_S32
#define SRC_BLOCK_READ(src) \
    as_int(intel_sub_group_block_read((const __global uint *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_int8(intel_sub_group_block_read8((const __global uint *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // SRC_DT_S32

#if SRC_DT_F32
#define SRC_BLOCK_READ(src) \
    as_float(intel_sub_group_block_read((const __global uint *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_float8(intel_sub_group_block_read8((const __global uint *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // SRC_DT_F32

#if SRC_DT_BF16
#define SRC_BLOCK_READ(src) \
    as_ushort(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define SRC_BLOCK_READ8(src) \
    as_ushort8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define SRC_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define SRC_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // SRC_DT_F16

#if DST_DT_S8
#define DST_BLOCK_READ(src) \
    as_char(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define DST_BLOCK_READ8(src) \
    as_char8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // DST_DT_S8

#if DST_DT_U8
#define DST_BLOCK_READ(src) \
    as_uchar(intel_sub_group_block_read_uc((const __global uchar *)(src)))
#define DST_BLOCK_READ8(src) \
    as_uchar8(intel_sub_group_block_read_uc8((const __global uchar *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_uc((__global uchar *)(dst), as_uchar(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_uc8((__global uchar *)(dst), as_uchar8(val))
#endif // SRC_DT_U8

#if DST_DT_F16
#define DST_BLOCK_READ(src) \
    as_half(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define DST_BLOCK_READ8(src) \
    as_half8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // DST_DT_F16

#if DST_DT_S32
#define DST_BLOCK_READ(src) \
    as_int(intel_sub_group_block_read((const __global uint *)(src)))
#define DST_BLOCK_READ8(src) \
    as_int8(intel_sub_group_block_read8((const __global uint *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // DST_DT_S32

#if DST_DT_F32
#define DST_BLOCK_READ(src) \
    as_float(intel_sub_group_block_read((const __global uint *)(src)))
#define DST_BLOCK_READ8(src) \
    as_float8(intel_sub_group_block_read8((const __global uint *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write((__global uint *)(dst), as_uint(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write8((__global uint *)(dst), as_uint8(val))
#endif // DST_DT_F32

#if DST_DT_BF16
#define DST_BLOCK_READ(src) \
    as_ushort(intel_sub_group_block_read_us((const __global ushort *)(src)))
#define DST_BLOCK_READ8(src) \
    as_ushort8(intel_sub_group_block_read_us8((const __global ushort *)(src)))
#define DST_BLOCK_WRITE(dst, val) \
    intel_sub_group_block_write_us((__global ushort *)(dst), as_ushort(val))
#define DST_BLOCK_WRITE8(dst, val) \
    intel_sub_group_block_write_us8((__global ushort *)(dst), as_ushort8(val))
#endif // SRC_DT_F16

#if SRC_DT_BF16 && DST_DT_BF16
#define SRC_TO_DST(x) (x)
#define SRC_TO_DST8(x) (x)
#else
#define SRC_TO_DST(x) TO_DST(SRC_TO_REF(x))
#define SRC_TO_DST8(x) TO_DST8(SRC_TO_REF8(x))
#endif

#if SCALE_QUANT

#define REORDER(_out, _src, _a, _b) \
    do { \
        const float _x = SRC_TO_REF(_src); \
        const float _s = _a * _x + _b; \
        _out = TO_DST(_s); \
    } while (0)
#define REORDER8(_out, _src, _a, _b) \
    do { \
        const float8 _x = convert_float8(SRC_TO_REF8(_src)); \
        const float8 _s = _a * _x + _b; \
        _out = TO_DST8(_s); \
    } while (0)

#elif WITH_SUM_A

#define REORDER(_dst, _src, _a, _b) \
    do { \
        const float _x = SRC_TO_REF(_src); \
        const float _s = _a * _x; \
        _dst = TO_DST(_s); \
    } while (0)
#define REORDER8(_dst, _src, _a, _b) \
    do { \
        const float8 _x = convert_float8(SRC_TO_REF8(_src)); \
        const float8 _s = _a * _x; \
        _dst = TO_DST8(_s); \
    } while (0)

#elif WITH_SUM_AB

#define REORDER(_dst, _src, _a, _b) \
    do { \
        const float _x = SRC_TO_REF(_src); \
        const float _y = DST_TO_REF(_dst); \
        const float _s = _a * _x + _b * _y; \
        _dst = TO_DST(_s); \
    } while (0)
#define REORDER8(_dst, _src, _a, _b) \
    do { \
        const float8 _x = convert_float8(SRC_TO_REF8(_src)); \
        const float8 _y = convert_float8(DST_TO_REF8(_dst)); \
        const float8 _s = _a * _x + _b * _y; \
        _dst = TO_DST8(_s); \
    } while (0)

#else // WITH_SUM_AB == 0

#define REORDER(_dst, _src, _a, _b) \
    do { \
        _dst = SRC_TO_DST(_src); \
    } while (0)
#define REORDER8(_dst, _src, _a, _b) \
    do { \
        _dst = SRC_TO_DST8(_src); \
    } while (0)

#endif // WITH_SUM_AB

#if SCALE_QUANT

#define MASK_D(_d) ((SCALE_MASK >> _d) & 1)

#define SCALE_D0 (MASK_D(0) ? SRC_D0 : 1)
#define SCALE_D1 (MASK_D(1) ? SRC_D1 : 1)
#define SCALE_D2 (MASK_D(2) ? SRC_D2 : 1)
#define SCALE_D3 (MASK_D(3) ? SRC_D3 : 1)
#define SCALE_D4 (MASK_D(4) ? SRC_D4 : 1)
#define SCALE_D5 (MASK_D(5) ? SRC_D5 : 1)

#define SCALE_S0 (SCALE_D1 * SCALE_D2 * SCALE_D3 * SCALE_D4 * SCALE_D5)
#define SCALE_S1 (SCALE_D2 * SCALE_D3 * SCALE_D4 * SCALE_D5)
#define SCALE_S2 (SCALE_D3 * SCALE_D4 * SCALE_D5)
#define SCALE_S3 (SCALE_D4 * SCALE_D5)
#define SCALE_S4 (SCALE_D5)
#define SCALE_S5 (1)

#define SCALE_OFF(x0, x1, x2, x3, x4, x5) \
    ((x0)*SCALE_S0 * MASK_D(0) + (x1)*SCALE_S1 * MASK_D(1) \
            + (x2)*SCALE_S2 * MASK_D(2) + (x3)*SCALE_S3 * MASK_D(3) \
            + (x4)*SCALE_S4 * MASK_D(4) + (x5)*SCALE_S5 * MASK_D(5))

#endif // SCALE_QUANT

#if SUB_GROUP_SIZE != 1
__attribute__((intel_reqd_sub_group_size(SUB_GROUP_SIZE))) // attr:no-format
#endif
__attribute__((reqd_work_group_size(LWS_0, LWS_1, LWS_2))) // attr:no-format
__kernel void
simple_reorder(__global SRC_DATA_T *src, __global DST_DATA_T *dst, float alpha,
        float beta, __global float *scales) {

    src += SRC_OFFSET0;
    dst += DST_OFFSET0;

#if REF_REORDER

#if NDIMS <= 3
    const int d0 = get_global_id(0);
    const int d1 = get_global_id(1);
    const int d2 = get_global_id(2);
#if PAD_FILL_ZERO
    if (d0 >= SRC_D0 || d1 >= SRC_D1 || d2 >= SRC_D2) {
        dst[DST_OFF(d0, d1, d2, 0, 0, 0)] = 0;
        return;
    }
#endif // PAD_FILL_ZERO
    {
        const int src_off = SRC_OFF(d0, d1, d2, 0, 0, 0);
        const int dst_off = DST_OFF(d0, d1, d2, 0, 0, 0);
#if SCALE_QUANT
        alpha = scales[SCALE_OFF(d0, d1, d2, 0, 0, 0)];
#endif
        REORDER(dst[dst_off], src[src_off], alpha, beta);
    }
#elif NDIMS <= 5
    const int d0 = get_global_id(0);
    const int d1 = get_global_id(1);
#if PAD_FILL_ZERO
    if (d0 >= SRC_D0 || d1 >= SRC_D1) {
        for (int d2 = 0; d2 < DST_D2; ++d2)
            for (int d3 = 0; d3 < DST_D3; ++d3)
                for (int d4 = 0; d4 < max(1, DST_D4); ++d4) {
                    dst[DST_OFF(d0, d1, d2, d3, d4, 0)] = 0;
                }
        return;
    }
#endif // PAD_FILL_ZERO
    for (int d2 = 0; d2 < SRC_D2; ++d2)
        for (int d3 = 0; d3 < SRC_D3; ++d3)
            for (int d4 = 0; d4 < max(1, SRC_D4); ++d4) {
                const int src_off = SRC_OFF(d0, d1, d2, d3, d4, 0);
                const int dst_off = DST_OFF(d0, d1, d2, d3, d4, 0);
#if SCALE_QUANT
                alpha = scales[SCALE_OFF(d0, d1, d2, d3, d4, 0)];
#endif
                REORDER(dst[dst_off], src[src_off], alpha, beta);
            }
#if PAD_FILL_ZERO
    for (int d2 = SRC_D2; d2 < DST_D2; ++d2)
        for (int d3 = SRC_D3; d3 < DST_D3; ++d3)
            for (int d4 = SRC_D4; d4 < max(1, DST_D4); ++d4) {
                dst[DST_OFF(d0, d1, d2, d3, d4, 0)] = 0;
            }
#endif // PAD_FILL_ZERO

#elif NDIMS == 6
    const int d0 = get_global_id(0);
    const int d1 = get_global_id(1);
    const int d2 = get_global_id(2);
#if PAD_FILL_ZERO
    if (d0 >= SRC_D0 || d1 >= SRC_D1 || d2 >= SRC_D2) {
        for (int d3 = 0; d3 < DST_D3; ++d3)
            for (int d4 = 0; d4 < DST_D4; ++d4)
                for (int d5 = 0; d5 < DST_D5; ++d5) {
                    dst[DST_OFF(d0, d1, d2, d3, d4, d5)] = 0;
                }
        return;
    }
#endif // PAD_FILL_ZERO
    for (int d3 = 0; d3 < DST_D3; ++d3)
        for (int d4 = 0; d4 < DST_D4; ++d4)
            for (int d5 = 0; d5 < DST_D5; ++d5) {
                const int src_off = SRC_OFF(d0, d1, d2, d3, d4, d5);
                const int dst_off = DST_OFF(d0, d1, d2, d3, d4, d5);
#if SCALE_QUANT
                alpha = scales[SCALE_OFF(d0, d1, d2, d3, d4, d5)];
#endif
                REORDER(dst[dst_off], src[src_off], alpha, beta);
            }
#if PAD_FILL_ZERO
    for (int d3 = SRC_D3; d3 < DST_D3; ++d3)
        for (int d4 = SRC_D4; d4 < DST_D4; ++d4)
            for (int d5 = SRC_D5; d5 < DST_D5; ++d5) {
                dst[DST_OFF(d0, d1, d2, d3, d4, d5)] = 0;
            }
#endif // PAD_FILL_ZERO
#endif // NDIMS == 6

#else // REF_REORDER == 0

#if SRC_16A16B || DST_16A16B || SRC_16B16A || DST_16B16A
    const int d0 = get_global_id(0) * 16;
    const int d1 = get_group_id(1) * 16;
    const int local_id = get_local_id(1);
    const int sp = get_group_id(2);

    const int d23 = sp / (max(1, DST_D4) * max(1, DST_D5));
    const int d45 = sp % (max(1, DST_D4) * max(1, DST_D5));
    const int d2 = d23 / max(1, DST_D3);
    const int d3 = d23 % max(1, DST_D3);
    const int d4 = d45 / max(1, DST_D5);
    const int d5 = d45 % max(1, DST_D5);

    src += SRC_OFF(d0, d1, d2, d3, d4, d5);
    dst += DST_OFF(d0, d1, d2, d3, d4, d5);

    SRC_DATA8_T in0, in1;
#if SRC_16A16B || SRC_16B16A
    in0 = SRC_BLOCK_READ8(&src[0]);
    in1 = SRC_BLOCK_READ8(&src[8 * 16]);
#else
    for (int i = 0; i < 8; i++) {
#if DST_16B16A
        in0[i] = src[SRC_OFF(local_id, i, 0, 0, 0, 0)];
        in1[i] = src[SRC_OFF(local_id, i + 8, 0, 0, 0, 0)];
#else
        in0[i] = src[SRC_OFF(i, local_id, 0, 0, 0, 0)];
        in1[i] = src[SRC_OFF(i + 8, local_id, 0, 0, 0, 0)];
#endif // DST_16B16A
    }
#endif // SRC_16A16B || SRC_16B16A

    DST_DATA8_T dst0, dst1;
#if (SRC_16A16B || SRC_16B16A) && (DST_16A16B || DST_16B16A)
#if WITH_SUM_AB
    for (int i = 0; i < 8; i++) {
#if SRC_16B16A
        dst0[i] = dst[DST_OFF(local_id, i + 0, 0, 0, 0, 0)];
        dst1[i] = dst[DST_OFF(local_id, i + 8, 0, 0, 0, 0)];
#else
        dst0[i] = dst[DST_OFF(i + 0, local_id, 0, 0, 0, 0)];
        dst1[i] = dst[DST_OFF(i + 8, local_id, 0, 0, 0, 0)];
#endif // SRC_16B16A
    }
#endif // WITH_SUM_AB
#elif DST_16A16B || DST_16B16A
#if WITH_SUM_AB
    dst0 = DST_BLOCK_READ8(&dst[0]);
    dst1 = DST_BLOCK_READ8(&dst[8 * 16]);
#endif // WITH_SUM_AB
#else
#if WITH_SUM_AB
    for (int i = 0; i < 8; i++) {
#if SRC_16B16A
        dst0[i] = dst[DST_OFF(local_id, i + 0, 0, 0, 0, 0)];
        dst1[i] = dst[DST_OFF(local_id, i + 8, 0, 0, 0, 0)];
#else
        dst0[i] = dst[DST_OFF(i + 0, local_id, 0, 0, 0, 0)];
        dst1[i] = dst[DST_OFF(i + 8, local_id, 0, 0, 0, 0)];
#endif // SRC_16B16A
    }
#endif // WITH_SUM_AB
#endif // (SRC_16A16B || SRC_16B16A) && (DST_16A16B || DST_16B16A)

    REORDER8(dst0, in0, alpha, beta);
    REORDER8(dst1, in1, alpha, beta);

#if (SRC_16A16B || SRC_16B16A) && (DST_16A16B || DST_16B16A)
    for (int i = 0; i < 8; i++) {
#if SRC_16B16A
        dst[DST_OFF(local_id, i + 0, 0, 0, 0, 0)] = dst0[i];
        dst[DST_OFF(local_id, i + 8, 0, 0, 0, 0)] = dst1[i];
#else
        dst[DST_OFF(i + 0, local_id, 0, 0, 0, 0)] = dst0[i];
        dst[DST_OFF(i + 8, local_id, 0, 0, 0, 0)] = dst1[i];
#endif // SRC_16B16A
    }
#elif DST_16A16B || DST_16B16A
    DST_BLOCK_WRITE8(&dst[0], dst0);
    DST_BLOCK_WRITE8(&dst[8 * 16], dst1);
#else
    for (int i = 0; i < 8; i++) {
#if SRC_16B16A
        dst[DST_OFF(local_id, i + 0, 0, 0, 0, 0)] = dst0[i];
        dst[DST_OFF(local_id, i + 8, 0, 0, 0, 0)] = dst1[i];
#else
        dst[DST_OFF(i + 0, local_id, 0, 0, 0, 0)] = dst0[i];
        dst[DST_OFF(i + 8, local_id, 0, 0, 0, 0)] = dst1[i];
#endif // SRC_16B16A
    }
#endif // (SRC_16A16B || SRC_16B16A) && (DST_16A16B || DST_16B16A)

#elif SRC_16B || DST_16B
    const int d0 = get_global_id(0);
    const int d1 = get_group_id(1) * 16;
    const int local_id = get_local_id(1);
    const int sp = get_group_id(2);

    const int d23 = sp / (max(1, DST_D4) * max(1, DST_D5));
    const int d45 = sp % (max(1, DST_D4) * max(1, DST_D5));
    const int d2 = d23 / max(1, DST_D3);
    const int d3 = d23 % max(1, DST_D3);
    const int d4 = d45 / max(1, DST_D5);
    const int d5 = d45 % max(1, DST_D5);

    SRC_DATA_T src_tmp;
#if SRC_16B
    src += SRC_OFF(d0, d1, d2, d3, d4, d5);
    src_tmp = SRC_BLOCK_READ(&src[0]);
#else
    src += SRC_OFF(d0, d1 + local_id, d2, d3, d4, d5);
    src_tmp = src[0];
#endif // SRC_16B

    DST_DATA_T dst_tmp;
#if DST_16B
    dst += DST_OFF(d0, d1, d2, d3, d4, d5);
#if WITH_SUM_AB
    dst_tmp = DST_BLOCK_READ(&dst[0]);
#endif // WITH_SUM_AB
#else
    dst += DST_OFF(d0, d1 + local_id, d2, d3, d4, d5);
#if WITH_SUM_AB
    dst_tmp = dst[0];
#endif // WITH_SUM_AB
#endif // DST_16B

    REORDER(dst_tmp, src_tmp, alpha, beta);

#if DST_16B
    DST_BLOCK_WRITE(&dst[0], dst_tmp);
#else
    dst[0] = dst_tmp;
#endif // DST_16B

#elif SRC_16B16C || DST_16B16C || SRC_16C16B || DST_16C16B
    const int g = get_global_id(0) / BLOCK_0;
    const int d1 = ((get_global_id(0) % BLOCK_0) / 16) * 16;
    const int d2 = get_group_id(1) * 16;
    const int local_id = get_local_id(0);
    const int sp = get_global_id(2);

    const int d34 = sp / max(1, DST_D5);
    const int d3 = d34 / max(1, DST_D4);
    const int d4 = d34 % max(1, DST_D4);
    const int d5 = sp % max(1, DST_D5);

    SRC_DATA8_T in0, in1;
#if SRC_16B16C || SRC_16C16B
    src += SRC_OFF_G(g, d1, d2, d3, d4, d5);
    in0 = SRC_BLOCK_READ8(&src[0]);
    in1 = SRC_BLOCK_READ8(&src[8 * 16]);
#else
    for (int i = 0; i < 8; i++) {
#if DST_16C16B
        in0[i] = src[SRC_OFF_G(g, d1 + local_id, d2 + i + 0, d3, d4, d5)];
        in1[i] = src[SRC_OFF_G(g, d1 + local_id, d2 + i + 8, d3, d4, d5)];
#else
        in0[i] = src[SRC_OFF_G(g, d1 + i + 0, d2 + local_id, d3, d4, d5)];
        in1[i] = src[SRC_OFF_G(g, d1 + i + 8, d2 + local_id, d3, d4, d5)];
#endif // DST_16C16B
    }
#endif // SRC_16B16C || SRC_16C16B

    DST_DATA8_T dst0, dst1;

#if (SRC_16B16C || SRC_16C16B) && (DST_16B16C || DST_16C16B)
#if WITH_SUM_AB
    for (int i = 0; i < 8; i++) {
#if SRC_16C16B
        dst0[i] = dst[DST_OFF_G(g, d1 + local_id, d2 + i + 0, d3, d4, d5)];
        dst1[i] = dst[DST_OFF_G(g, d1 + local_id, d2 + i + 8, d3, d4, d5)];
#else
        dst0[i] = dst[DST_OFF_G(g, d1 + i + 0, d2 + local_id, d3, d4, d5)];
        dst1[i] = dst[DST_OFF_G(g, d1 + i + 8, d2 + local_id, d3, d4, d5)];
#endif // SRC_16C16B
    }
#endif // WITH_SUM_AB
#elif DST_16B16C || DST_16C16B
    dst += DST_OFF_G(g, d1, d2, d3, d4, d5);
#if WITH_SUM_AB
    dst0 = DST_BLOCK_READ8(&dst[0]);
    dst1 = DST_BLOCK_READ8(&dst[8 * 16]);
#endif // WITH_SUM_AB
#else
#if WITH_SUM_AB
    for (int i = 0; i < 8; i++) {
#if SRC_16C16B
        dst0[i] = dst[DST_OFF_G(g, d1 + local_id, d2 + i + 0, d3, d4, d5)];
        dst1[i] = dst[DST_OFF_G(g, d1 + local_id, d2 + i + 8, d3, d4, d5)];
#else
        dst0[i] = dst[DST_OFF_G(g, d1 + i + 0, d2 + local_id, d3, d4, d5)];
        dst1[i] = dst[DST_OFF_G(g, d1 + i + 8, d2 + local_id, d3, d4, d5)];
#endif // SRC_16C16B
    }
#endif // WITH_SUM_AB
#endif // (SRC_16B16C || SRC_16C16B) && (DST_16B16C || DST_16C16B)

    REORDER8(dst0, in0, alpha, beta);
    REORDER8(dst1, in1, alpha, beta);

#if (SRC_16B16C || SRC_16C16B) && (DST_16B16C || DST_16C16B)
    for (int i = 0; i < 8; i++) {
#if SRC_16C16B
        dst[DST_OFF_G(g, d1 + local_id, d2 + i + 0, d3, d4, d5)] = dst0[i];
        dst[DST_OFF_G(g, d1 + local_id, d2 + i + 8, d3, d4, d5)] = dst1[i];
#else
        dst[DST_OFF_G(g, d1 + i + 0, d2 + local_id, d3, d4, d5)] = dst0[i];
        dst[DST_OFF_G(g, d1 + i + 8, d2 + local_id, d3, d4, d5)] = dst1[i];
#endif // SRC_16C16B
    }
#elif DST_16B16C || DST_16C16B
    DST_BLOCK_WRITE8(&dst[0], dst0);
    DST_BLOCK_WRITE8(&dst[8 * 16], dst1);
#else
    for (int i = 0; i < 8; i++) {
#if SRC_16C16B
        dst[DST_OFF_G(g, d1 + local_id, d2 + i + 0, d3, d4, d5)] = dst0[i];
        dst[DST_OFF_G(g, d1 + local_id, d2 + i + 8, d3, d4, d5)] = dst1[i];
#else
        dst[DST_OFF_G(g, d1 + i + 0, d2 + local_id, d3, d4, d5)] = dst0[i];
        dst[DST_OFF_G(g, d1 + i + 8, d2 + local_id, d3, d4, d5)] = dst1[i];
#endif // SRC_16C16B
    }
#endif // (SRC_16B16C || SRC_16C16B) && (DST_16B16C || DST_16C16B)
#endif // SRC_16B16C || DST_16B16C || SRC_16C16B || DST_16C16B

#endif // REF_REORDER
}

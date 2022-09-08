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

#if INNER_PRODUCT_FWD == 1

__kernel void ref_inner_product_fwd_kernel(__global SRC_DATA_T *src,
        __global WEI_DATA_T *wht, __global BIA_DATA_T *bias,
        __global DST_DATA_T *dst, float eltwise_alpha, float eltwise_beta,
        float sum_scale, float output_scale) {

    const int mb = get_global_id(0) / OC;
    const int oc = get_global_id(0) % OC;

    ACC_DATA_T d = 0;
#if HAS_SPATIAL == 1
    for (int ic = 0; ic < IC; ++ic)
        for (int kd = 0; kd < KD; ++kd)
            for (int kh = 0; kh < KH; ++kh)
                for (int kw = 0; kw < KW; ++kw) {
                    const uint src_off = SRC_OFF(mb, ic, kd, kh, kw);
                    const uint wht_off = WHT_OFF(0, oc, ic, kd, kh, kw);
#else
    for (int ic = 0; ic < IC_TOTAL; ++ic) {
        const uint src_off = mb * IC_TOTAL + ic;
        const uint wht_off = oc * IC_TOTAL + ic;
#endif
                    d += SRC_TO_REF(src[src_off]) * WEI_TO_REF(wht[wht_off]);
                }
    DATA_T tmp = d;
#if WITH_BIAS
    tmp += BIA_TO_REF(bias[oc]);
#endif

    tmp *= output_scale;

#if WITH_SUM_ELTWISE == 1
    tmp += sum_scale * DST_TO_REF(dst[mb * OC + oc]);
    tmp = fwd_eltwise(tmp, eltwise_alpha, eltwise_beta);
#else
#if WITH_ELTWISE == 1
    tmp = fwd_eltwise(tmp, eltwise_alpha, eltwise_beta);
#endif
#if WITH_SUM == 1
    tmp = sum_scale * DST_TO_REF(dst[mb * OC + oc]) + tmp;
#endif
#endif
    dst[mb * OC + oc] = TO_DST(tmp);
}
#endif

#if INNER_PRODUCT_BWD_DATA == 1
__kernel void ref_inner_product_bwd_data_kernel(__global SRC_DATA_T *diff_src,
        __global WEI_DATA_T *wht, __global DST_DATA_T *diff_dst) {

    const int mb = get_global_id(0) / IC_TOTAL;
    const int ic_total = get_global_id(0) % IC_TOTAL;
    const int ic = ic_total / (KD * KH * KW);
    const int kdhw = ic_total % (KD * KH * KW);
    const int kd = kdhw / (KH * KW);
    const int khw = kdhw % (KH * KW);
    const int kh = khw / KH;
    const int kw = khw % KH;

    float ds = 0.0f;
    for (int oc = 0; oc < OC; ++oc) {
        const uint diff_dst_off = DST_OFF(mb, oc, 0, 0, 0);
        const uint wht_off = WHT_OFF(0, oc, ic, kd, kh, kw);
        ds += DST_TO_REF(diff_dst[diff_dst_off]) * WEI_TO_REF(wht[wht_off]);
    }
    const uint diff_src_off = SRC_OFF(mb, ic, kd, kh, kw);
    diff_src[diff_src_off] = REF_TO_SRC(ds);
}
#endif

#if INNER_PRODUCT_BWD_WEIGHTS == 1
__kernel void ref_inner_product_bwd_weights_kernel(__global SRC_DATA_T *src,
        __global WEI_DATA_T *diff_wht, __global BIA_DATA_T *diff_bias,
        __global DST_DATA_T *diff_dst) {

    const int oc = get_global_id(0) / IC_TOTAL;
    const int ic_total = get_global_id(0) % IC_TOTAL;
    const int ic = ic_total / (KD * KH * KW);
    const int kdhw = ic_total % (KD * KH * KW);
    const int kd = kdhw / (KH * KW);
    const int khw = kdhw % (KH * KW);
    const int kh = khw / KH;
    const int kw = khw % KH;

    float ds = 0.0f;
    for (int mb = 0; mb < MB; ++mb) {
        const uint diff_dst_off = DST_OFF(mb, oc, 0, 0, 0);
        const uint src_off = SRC_OFF(mb, ic, kd, kh, kw);
        ds += DST_TO_REF(diff_dst[diff_dst_off]) * SRC_TO_REF(src[src_off]);
    }
    const uint diff_wht_off = WHT_OFF(0, oc, ic, kd, kh, kw);
    diff_wht[diff_wht_off] = REF_TO_WEI(ds);
#if WITH_BIAS == 1
    if (ic == 0) {
        float db = 0.0f;
        for (int mb = 0; mb < MB; ++mb) {
            const uint diff_dst_off = DST_OFF(mb, oc, 0, 0, 0);
            db += DST_TO_REF(diff_dst[diff_dst_off]);
        }
        diff_bias[oc] = REF_TO_BIA(db);
    }
#endif
}
#endif

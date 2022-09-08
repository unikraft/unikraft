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

#include "src/common/dnnl_thread.hpp"

#include "lnorm/lnorm.hpp"

namespace lnorm {

void compute_ref_fwd(const prb_t *p, const dnn_mem_t &src, dnn_mem_t &mean,
        dnn_mem_t &var, const dnn_mem_t &ss, dnn_mem_t &dst) {
    dnnl::impl::parallel_nd(p->n, [&](int64_t n) {
        float smean = ((float *)mean)[n];
        float svar = ((float *)var)[n];
        float sqrt_var = sqrtf(svar + p->eps);

        for (int64_t c = 0; c < p->c; ++c) {
            float gamma = (p->flags & USE_SCALESHIFT ? ((float *)ss)[c] : 1.0f)
                    / sqrt_var;
            float beta
                    = p->flags & USE_SCALESHIFT ? ((float *)ss)[p->c + c] : 0;
            auto off = n * p->c + c;
            float res = gamma * (((float *)src)[off] - smean) + beta;
            float &D = ((float *)dst)[off];
            maybe_post_ops(res, D, p->attr);
            D = maybe_saturate(p->dt, res);
        }
    });
}

void compute_ref_bwd(const prb_t *p, const dnn_mem_t &src,
        const dnn_mem_t &mean, const dnn_mem_t &var, const dnn_mem_t &d_dst,
        const dnn_mem_t &ss, dnn_mem_t &d_src, dnn_mem_t &d_ss) {

    dnnl::impl::parallel_nd(p->c, [&](int64_t c) {
        float gamma = p->flags & USE_SCALESHIFT ? ((float *)ss)[c] : 1;
        float d_gamma = 0;
        float d_beta = 0;

        for (int64_t n = 0; n < p->n; ++n) {
            float smean = ((float *)mean)[n];
            float svar = ((float *)var)[n];
            float rcp_denom = 1.f / sqrtf(svar + p->eps);
            auto off = n * p->c + c;
            float dd = ((float *)d_dst)[off];
            d_gamma += dd * (((float *)src)[off] - smean) * rcp_denom;
            d_beta += dd;
        }

        if ((p->flags & USE_SCALESHIFT) && (p->dir & FLAG_WEI)) {
            ((float *)d_ss)[c] = d_gamma;
            ((float *)d_ss)[p->c + c] = d_beta;
        }

        for (int64_t n = 0; n < p->n; ++n) {
            float smean = ((float *)mean)[n];
            float svar = ((float *)var)[n];
            float rcp_denom = 1.f / sqrtf(svar + p->eps);
            auto off = n * p->c + c;
            float ds = ((float *)d_dst)[off];
            if (!(p->flags & GLOB_STATS)) {
                const float x = ((float *)src)[off] - smean;
                ds -= (d_beta + x * d_gamma * rcp_denom) / p->c;
            }

            ((float *)d_src)[off] = rcp_denom * ds * gamma;
        }
    });
}

} // namespace lnorm

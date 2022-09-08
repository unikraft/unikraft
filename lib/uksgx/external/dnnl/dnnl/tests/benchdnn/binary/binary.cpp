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

#include <stdio.h>
#include <stdlib.h>

#include "dnnl.h"

#include "src/common/dnnl_thread.hpp"

#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"

#include "binary/binary.hpp"

namespace binary {

static int init_pd(const prb_t *p, dnnl_binary_desc_t &bd,
        dnnl_primitive_desc_t &bpd, res_t *r) {
    std::vector<dnnl_memory_desc_t> src_d;
    src_d.resize(p->n_inputs());

    std::vector<dnnl_memory_desc_t> scale_d;
    scale_d.resize(p->n_inputs());

    const std::vector<int> ndims
            = {(int)p->sdims[0].size(), (int)p->sdims[1].size()};

    for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
        const dims_t &i_sdims = p->sdims[i_input];
        DNN_SAFE(dnnl_memory_desc_init_by_tag(&src_d[i_input], ndims[i_input],
                         i_sdims.data(), p->sdt[i_input], p->stag[i_input]),
                WARN);
    }

    if ((int)p->sdims[1].size() < ndims[0]) { // need to reshape B
        dnnl_dims_t dims;
        for (int d = 0; d < (int)p->sdims[1].size(); ++d)
            dims[d] = p->sdims[1][d];
        for (int d = (int)p->sdims[1].size(); d < ndims[0]; ++d)
            dims[d] = 1;
        DNN_SAFE(dnnl_memory_desc_reshape(&src_d[1], &src_d[1], ndims[0], dims),
                WARN);
    }

    dnnl_memory_desc_t dst_d;
    DNN_SAFE(dnnl_memory_desc_init_by_tag(&dst_d, ndims[0], p->sdims[0].data(),
                     p->ddt, dnnl_format_tag_any),
            WARN);

    dnnl_alg_kind_t alg = alg2alg_kind(p->alg);

    if (p->scale_policy != policy_t::NONE) {
        // TODO: change with attributes
        for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
            dnnl_dims_t dims_scale = {1};
            DNN_SAFE(dnnl_memory_desc_init_by_tag(&scale_d[i_input], 1,
                             dims_scale, dnnl_f32, dnnl_x),
                    WARN);
        }
    }

    DNN_SAFE(dnnl_binary_desc_init(&bd, alg, &src_d[0], &src_d[1], &dst_d),
            WARN);

    dnnl_status_t init_status
            = dnnl_primitive_desc_create(&bpd, &bd, NULL, engine_tgt, NULL);

    if (init_status == dnnl_unimplemented)
        return r->state = UNIMPLEMENTED, OK;
    else
        SAFE(init_status, WARN);

    const char *impl_str = query_impl_info(bpd);
    print(5, "dnnl implementation: %s\n", impl_str);

    return OK;
}

static int compare(const prb_t *p, const dnn_mem_t &fp_mem,
        const dnn_mem_t &dt_mem, res_t *r) {
    const auto nelems = dt_mem.nelems();
    r->errors = 0;
    r->total = nelems;
    const float trh = (p->ddt == dnnl_f16 ? 1e-3 : 1e-7) * p->n_inputs();

    for (int64_t i = 0; i < nelems; i++) {
        const float dt = dt_mem.get_elem(i);
        const float fp0 = fp_mem.get_elem(i);
        const float fp = maybe_saturate(p->ddt, fp0);

        const float diff = fabsf(fp - dt);
        const float rel_diff = diff / (fabsf(fp) > FLT_MIN ? fabsf(fp) : 1);
        const bool ok = (fabsf(fp) > 1e-5 ? rel_diff : diff) <= trh;

        r->errors += !ok;

        const bool dump = false || (!ok && (r->errors < 10 || verbose >= 10))
                || (verbose >= 50 && i < 30) || (verbose >= 99);
        if (dump) {
            std::stringstream ss;
            dims_t dims_idx = off2dims_idx(p->sdims[0], i);
            ss << dims_idx;
            std::string ind_str = ss.str();

            print(0, "[%4ld][%s] fp0:%8g fp:%8g dt:%8g diff:%8g rdiff:%8g\n",
                    (long)i, ind_str.c_str(), fp0, fp, dt, diff, rel_diff);
        }
    }

    if (r->errors) r->state = FAILED;

    if (r->state == UNTESTED) r->state = PASSED; /* optimism */

    return r->state == FAILED ? FAIL : OK;
}

int fill_src(
        const prb_t *p, int input_idx, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp) {
    const auto nelems = mem_fp.nelems();
    const auto dt = p->sdt[input_idx];
    const int range = 16;
    const int f_min = dt == dnnl_u8 ? 0 : -range / 2;

    dnnl::impl::parallel_nd(nelems, [&](int64_t i) {
        const int gen = ((97 * i) - 17 * input_idx + 101) % (range + 1);
        const float value = (dt == dnnl_bf16 || dt == dnnl_f16)
                ? (f_min + gen) / range
                : (f_min + gen) * (1.0f + 4.0f / range);
        mem_fp.set_elem(i, maybe_saturate(dt, value));
    });

    SAFE(mem_dt.reorder(mem_fp), WARN);

    return OK;
}

int fill_scale(
        const prb_t *p, int input_idx, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp) {
    const auto nelems = mem_fp.nelems();
    const auto dt = p->sdt[input_idx];
    const float scales[] = {-4, -2, -0.5, -0.25, 0.25, 0.5, 2, 4};
    const int range = sizeof(scales) / sizeof(*scales);
    dnnl::impl::parallel_nd(nelems, [&](int64_t i) {
        const int gen = ((89 * i) - 23 * input_idx + 97) % range;
        const float value = scales[gen];
        mem_fp.set_elem(i, maybe_saturate(dt, value));
    });
    SAFE(mem_dt.reorder(mem_fp), WARN);

    return OK;
}

int doit(const prb_t *p, res_t *r) {
    if (bench_mode == LIST) return r->state = LISTED, OK;

    dnnl_binary_desc_t bd;
    dnnl_primitive_desc_t bpd;
    dnnl_primitive_t bo;

    SAFE(init_pd(p, bd, bpd, r), WARN);
    if (r->state == SKIPPED || r->state == UNIMPLEMENTED) return OK;

    DNN_SAFE(dnnl_primitive_create(&bo, bpd), WARN);

    const auto q = [=](dnnl_query_t query, int index = 0) {
        return *dnnl_primitive_desc_query_md(bpd, query, index);
    };

    const auto fp = dnnl_f32;
    const auto tag = get_default_tag((int)p->sdims[0].size());

    args_t args;

    std::vector<dnn_mem_t> src_fp, src_dt;
    src_fp.reserve(p->n_inputs());
    src_dt.reserve(p->n_inputs());

    for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
        const auto src_d = q(dnnl_query_src_md, i_input);
        src_fp.emplace_back(src_d, fp, tag, engine_tgt);
        src_dt.emplace_back(src_d, engine_tgt);

        SAFE(fill_src(p, i_input, src_dt[i_input], src_fp[i_input]), WARN);

        auto arg_num = i_input == 0 ? DNNL_ARG_SRC_0 : DNNL_ARG_SRC_1;
        args.set(arg_num, src_dt[i_input]);
    }

    std::vector<dnn_mem_t> scale_fp, scale_dt;
    scale_fp.reserve(p->n_inputs());
    scale_dt.reserve(p->n_inputs());

    if (p->scale_policy != policy_t::NONE) {
        for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
            const auto scale_d = q(dnnl_query_weights_md, i_input);
            scale_fp.emplace_back(scale_d, fp, dnnl_x, engine_tgt);
            scale_dt.emplace_back(scale_d, engine_tgt);

            SAFE(fill_scale(p, i_input, scale_dt[i_input], scale_fp[i_input]),
                    WARN);
        }
    }

    dnn_mem_t &dst_fp = src_fp[0]; // in-place in ref code
    dnn_mem_t placeholder_dst_dt;
    if (!p->inplace) {
        const auto dst_d = q(dnnl_query_dst_md);
        placeholder_dst_dt = dnn_mem_t(dst_d, engine_tgt);
    }
    dnn_mem_t &dst_dt = p->inplace ? src_dt[0] : placeholder_dst_dt;

    args.set(DNNL_ARG_DST, dst_dt);

    DNN_SAFE(execute_and_wait(bo, stream_tgt, args), WARN);

    if (bench_mode & CORR) {
        compute_ref(p, src_fp, scale_fp, dst_fp);
        dnn_mem_t dst(dst_dt, fp, tag, engine_tgt);
        SAFE(compare(p, dst_fp, dst, r), WARN);
    }

    measure_perf(r->timer, bo, args);

    DNN_SAFE(dnnl_primitive_desc_destroy(bpd), CRIT);
    DNN_SAFE(dnnl_primitive_destroy(bo), CRIT);

    return OK;
}

} // namespace binary

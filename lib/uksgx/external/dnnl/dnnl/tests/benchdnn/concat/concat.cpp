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

#include "concat/concat.hpp"

namespace concat {

static int init_pd(const prb_t *p, dnnl_primitive_desc_t &cpd, res_t *r) {
    std::vector<dnnl_memory_desc_t> src_d;
    src_d.resize(p->n_inputs());

    dnnl_memory_desc_t dst_d;
    const int ndims = (int)p->ddims.size();

    for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
        const dims_t &i_sdims = p->sdims[i_input];
        DNN_SAFE(dnnl_memory_desc_init_by_tag(&src_d[i_input], ndims,
                         i_sdims.data(), p->sdt, p->stag[i_input]),
                WARN);
    }

    if (p->dtag != dnnl_format_tag_undef) {
        DNN_SAFE(dnnl_memory_desc_init_by_tag(
                         &dst_d, ndims, p->ddims.data(), p->ddt, p->dtag),
                WARN);
    }

    dnnl_status_t init_status = dnnl_concat_primitive_desc_create(&cpd,
            p->dtag != dnnl_format_tag_undef ? &dst_d : NULL, p->n_inputs(),
            p->axis, src_d.data(), NULL, engine_tgt);

    if (init_status == dnnl_unimplemented)
        return r->state = UNIMPLEMENTED, OK;
    else
        SAFE(init_status, WARN);

    const char *impl_str = query_impl_info(cpd);
    print(5, "dnnl implementation: %s\n", impl_str);

    return OK;
}

static int compare(const prb_t *p, const dnnl_data_type_t dst_data_type,
        const dnn_mem_t &fp_mem, const dnn_mem_t &dt_mem, res_t *r) {
    const auto nelems = dt_mem.nelems();
    r->errors = 0;
    r->total = nelems;

    for (int64_t i = 0; i < nelems; i++) {
        const float dt = dt_mem.get_elem(i);
        const float fp0 = fp_mem.get_elem(i);
        const float fp = maybe_saturate(dst_data_type, fp0);

        const bool ok = dt == fp; // expect exact answer due to int values

        r->errors += !ok;

        const bool dump = false || (!ok && (r->errors < 10 || verbose >= 10))
                || (verbose >= 50 && i < 30) || (verbose >= 99);
        if (dump) {
            std::stringstream ss;
            dims_t ddims_idx = off2dims_idx(p->ddims, i);
            ss << ddims_idx;
            std::string ind_str = ss.str();

            print(0, "[%4ld][%s] fp0:%8g fp:%8g dt:%8g\n", (long)i,
                    ind_str.c_str(), fp0, fp, dt);
        }
    }

    if (r->errors) r->state = FAILED;

    if (r->state == UNTESTED) r->state = PASSED; /* optimism */

    return r->state == FAILED ? FAIL : OK;
}

int fill_src(
        const prb_t *p, int input_idx, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp) {
    auto get_range = [](const dnnl_data_type_t dt) {
        if (dt == dnnl_s8 || dt == dnnl_u8)
            return 256;
        else if (dt == dnnl_bf16 || dt == dnnl_f16)
            return 128;
        return 1024;
    };

    const auto nelems = mem_fp.nelems();
    const int range = get_range(p->sdt);
    const int f_min = p->sdt == dnnl_u8 ? 0 : -range / 2;

    dnnl::impl::parallel_nd(nelems, [&](int64_t i) {
        const float gen = ((97 * i) - 17 * input_idx + 101) % range;
        const float value = f_min + gen;
        mem_fp.set_elem(i, maybe_saturate(p->sdt, value));
    });

    SAFE(mem_dt.reorder(mem_fp), WARN);

    return OK;
}

int doit(const prb_t *p, res_t *r) {
    if (bench_mode == LIST) return r->state = LISTED, OK;

    dnnl_primitive_desc_t cpd;
    dnnl_primitive_t c;

    SAFE(init_pd(p, cpd, r), WARN);
    if (r->state == SKIPPED || r->state == UNIMPLEMENTED) return OK;

    DNN_SAFE(dnnl_primitive_create(&c, cpd), WARN);

    const auto q = [=](dnnl_query_t query, int index = 0) {
        return *dnnl_primitive_desc_query_md(cpd, query, index);
    };

    const auto fp = dnnl_f32;
    const auto tag = get_default_tag((int)p->sdims[0].size());

    const auto dst_dt_d = q(dnnl_query_dst_md);
    const auto dst_data_type = dst_dt_d.data_type; // needed for deduced dst
    dnn_mem_t dst_fp(dst_dt_d, fp, tag, engine_tgt),
            dst_dt(dst_dt_d, engine_tgt);

    args_t args;
    args.set(DNNL_ARG_DST, dst_dt);

    std::vector<dnn_mem_t> src_fp, src_dt;
    src_fp.reserve(p->n_inputs());
    src_dt.reserve(p->n_inputs());

    for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
        const auto src_dt_d = q(dnnl_query_src_md, i_input);
        src_fp.emplace_back(src_dt_d, fp, tag, engine_tgt);
        src_dt.emplace_back(src_dt_d, engine_tgt);

        SAFE(fill_src(p, i_input, src_dt[i_input], src_fp[i_input]), WARN);

        args.set(DNNL_ARG_MULTIPLE_SRC + i_input, src_dt[i_input]);
    }

    DNN_SAFE(execute_and_wait(c, stream_tgt, args), WARN);

    if (bench_mode & CORR) {
        compute_ref(p, src_fp, dst_fp);
        dnn_mem_t dst(dst_dt, fp, tag, engine_tgt);
        SAFE(compare(p, dst_data_type, dst_fp, dst, r), WARN);
    }

    measure_perf(r->timer, c, args);

    DNN_SAFE(dnnl_primitive_desc_destroy(cpd), CRIT);
    DNN_SAFE(dnnl_primitive_destroy(c), CRIT);

    return OK;
}

} // namespace concat

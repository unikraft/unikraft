/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
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

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "dnnl.h"

#include "src/common/dnnl_thread.hpp"

#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"
#include "norm.hpp"

#include "conv/conv_common.hpp"

namespace conv {

double get_trust_nz_level(
        const prb_t *p, data_kind_t kind, bool final_compare) {
    if (!final_compare) return p->cfg[kind].f_sparsity;

    auto negative_to_zero = [&]() {
        using pk = attr_t::post_ops_t::kind_t;
        const auto &po = p->attr.post_ops;
        int count = 0;
        for (int i = 0; i < po.len; ++i) {
            auto k = po.entry[i].kind;
            count += k == pk::RELU || k == pk::ELU || k == pk::SQRT
                    || k == pk::BRELU;
        }
        return !!count;
    };

    double trust = 0.3; /* why? */
    switch (kind) {
        case SRC: trust /= p->sd * p->sh * p->sw; break;
        case WEI:
            trust /= 1. * p->kd * p->kh * p->kw
                    / MIN3(p->kd * p->kh * p->kw, p->id * p->ih * p->iw,
                            p->od * p->oh * p->ow);
            break;
        case BIA:
            trust = 0.8 * p->cfg[DST].f_sparsity; /* why? */
            break;
        case DST: trust /= negative_to_zero() == 0 ? 1 : 2; break;
    }

    return trust;
}

inline bool post_ops_require_integral_check(const prb_t *p) {
    if (p->attr.post_ops.len == 0) return false;

    using pk = attr_t::post_ops_t::kind_t;
    const auto &ops = p->attr.post_ops;

    // assumptions: at most 1 eltwise, scale = 1.
    for (int idx = 0; idx < ops.len; ++idx) {
        const auto &e = ops.entry[idx];
        if (e.kind == pk::SUM || e.kind == pk::ABS) continue;
        if (e.kind == pk::RELU && e.eltwise.alpha == 0.f) continue;
        return true;
    }

    return false;
}

inline double get_eps(const prb_t *p, const data_kind_t kind) {
    // Winograd specifics
    if (p->alg & WINO && p->dir & FLAG_WEI) {
        /*This is an empirical equation derived by observing growth error
          with increasing 'k' dimension in gemm of winograd*/
        return p->cfg[kind].eps
                * (MAX2(1,
                        pow(10, 0.4 * log10(0.125 * p->mb * p->oh * p->ow))));
    }

    // post-ops specifics
    if (post_ops_require_integral_check(p)) return MAX2(1e-5, p->cfg[kind].eps);

    return p->cfg[kind].eps;
}

inline void get_result(const prb_t *p, const data_kind_t kind, res_t *r,
        const diff_norm_t diff_norm) {
    const float eps = get_eps(p, kind);

    /* Ignoring element-wise errors for Winograd and in some cases of post-ops,
     * since large relative error in few elements (which are anyways close
     * to zero) results in false positive failures */

    bool wino_test = (p->alg & WINO) && diff_norm.rel_diff(norm_t::L2) <= eps;
    if (wino_test) r->errors = 0;

    bool post_ops_test = post_ops_require_integral_check(p)
            && diff_norm.rel_diff(norm_t::L2) <= eps;
    if (post_ops_test) r->errors = 0;

    if (r->errors) r->state = FAILED;
}

inline int compare_dat(const prb_t *p, data_kind_t kind, dnn_mem_t &mem_dt,
        dnn_mem_t &mem_fp, res_t *r, bool final_compare = false) {
    const bool dont_complain
            = false || (p->alg & WINO) || post_ops_require_integral_check(p);

    const auto nelems = mem_dt.nelems();

    const char *skind = data_kind2str(kind);

    int in = 0, below = 0, above = 0;
    int in_ok = 0, below_ok = 0, above_ok = 0;
    int non_zero = 0;

    diff_norm_t diff_norm;

    r->errors = 0;
    r->total = nelems;

    for (int64_t i = 0; i < nelems; ++i) {
        const float dt = mem_dt.get_elem(i);
        const float fp0 = mem_fp.get_elem(i);
        const float fp = maybe_saturate(p->cfg[kind].dt, fp0);

        const float diff = fabsf(fp - dt);
        const float rel_diff = diff / (fabsf(fp) > FLT_MIN ? fabsf(fp) : 1);

        bool ok = true;
        if (fp < p->cfg[kind].min) {
            diff_norm.update(p->cfg[kind].min, dt);
            ok = dt == p->cfg[kind].min;
            below += 1;
            below_ok += ok;
        } else if (fp > p->cfg[kind].max) {
            diff_norm.update(p->cfg[kind].max, dt);
            ok = dt == p->cfg[kind].max;
            above += 1;
            above_ok += ok;
        } else {
            diff_norm.update(fp, dt);
            ok = (fabs(fp) > 1e-5 ? rel_diff : diff) <= get_eps(p, kind);
            in += 1;
            in_ok += ok;
        }
        if (!ok) {
            r->errors++;
            if ((!dont_complain && r->errors < 10) || verbose >= 10) {
                int64_t mb_or_g = 0, g_or_oc = 0, c = 0, d = 0, h = 0, w = 0;
                switch (kind) {
                    case SRC:
                        inv_src_off_f(p, i, mb_or_g, g_or_oc, c, d, h, w);
                        break;
                    case WEI:
                        inv_wei_off_f(p, i, mb_or_g, g_or_oc, c, d, h, w);
                        break;
                    case BIA: inv_bia_off_f(p, i, mb_or_g, g_or_oc); break;
                    case DST:
                        inv_dst_off_f(p, i, mb_or_g, g_or_oc, c, d, h, w);
                        break;
                }
                print(0,
                        "[%4ld][%s%s]"
                        "[" IFMT "," IFMT "," IFMT "," IFMT "," IFMT "," IFMT
                        "] "
                        "fp:%8g fp0:%8g dt:%8g diff:%8g rdiff:%8g\n",
                        (long)i, final_compare ? "" : "REORDER ", skind,
                        mb_or_g, g_or_oc, c, d, h, w, fp, fp0, dt, diff,
                        rel_diff);
            }
        }

        /* for debug purposes only: dump the output */
        if (final_compare && verbose >= 50 && i < 30) {
            int64_t mb_or_g = 0, g_or_oc = 0, c = 0, d = 0, h = 0, w = 0;
            switch (kind) {
                case SRC:
                    inv_src_off_f(p, i, mb_or_g, g_or_oc, c, d, h, w);
                    break;
                case WEI:
                    inv_wei_off_f(p, i, mb_or_g, g_or_oc, c, d, h, w);
                    break;
                case BIA: inv_bia_off_f(p, i, mb_or_g, g_or_oc); break;
                case DST:
                    inv_dst_off_f(p, i, mb_or_g, g_or_oc, c, d, h, w);
                    break;
            }

            print(0,
                    "[%4ld][%s]"
                    "[" IFMT "," IFMT "," IFMT "," IFMT "," IFMT "," IFMT
                    "] "
                    "fp:%8g fp0:%8g dt:%8g\n",
                    (long)i, skind, mb_or_g, g_or_oc, c, d, h, w, fp, fp0, dt);
        }

        non_zero += fp != 0;
    }

    diff_norm.done();
    get_result(p, kind, r, diff_norm);

    if (final_compare || r->errors) {
        const int vl = r->errors ? 0 : 2;
        print(vl,
                "@@@ [%s] %sdiff: err:%d, l0(``%g``) "
                "l1:(%g,%g,%g,``%g``) "
                "l2:(%g,%g,%g,``%g``) "
                "l8:(%g,%g,%g,``%g``)\n",
                skind, final_compare ? "final: " : "", (int)r->errors,
                diff_norm.rel_diff(norm_t::L0), diff_norm.a_[norm_t::L1],
                diff_norm.b_[norm_t::L1], diff_norm.diff_[norm_t::L1],
                diff_norm.rel_diff(norm_t::L1), diff_norm.a_[norm_t::L2],
                diff_norm.b_[norm_t::L2], diff_norm.diff_[norm_t::L2],
                diff_norm.rel_diff(norm_t::L2), diff_norm.a_[norm_t::L8],
                diff_norm.b_[norm_t::L8], diff_norm.diff_[norm_t::L8],
                diff_norm.rel_diff(norm_t::L8));
    }

    const double trust_rg_level = 0.3;
    const double trust_nz_level = get_trust_nz_level(p, kind, final_compare);

    const double trust_rg = (double)in / r->total;
    const double trust_nz = (double)non_zero / r->total;

    const bool no_trust = true /* ...in the test ...at all */
            && final_compare
            && (trust_rg < trust_rg_level || trust_nz < trust_nz_level);

    const bool dump = verbose >= 20
            || (verbose >= 10 && (trust_rg < 1. || trust_nz < 1.));
    if (dump) {
        print(0,
                "@@@ [%s] %strust range:%.2f nz:%.2f "
                "(level range:%.2f nz:%.2f). "
                "in:%d (ok:%d) below:%d (ok:%d) above:%d (ok:%d) nz:%d "
                "total:%lu\n",
                skind, final_compare ? "final: " : "", trust_rg, trust_nz,
                trust_rg_level, trust_nz_level, in, in_ok, below, below_ok,
                above, above_ok, non_zero, (unsigned long)r->total);
    }

    if (no_trust) {
        if (r->state != FAILED) r->state = MISTRUSTED;
        print(0,
                "@@@ [%s] test-bug: trust is too low. "
                "range:%.2f (?<%.2f) nz:%.2f (?<%.2f) (nz: %d total: %lu)\n",
                skind, trust_rg, trust_rg_level, trust_nz, trust_nz_level,
                non_zero, (unsigned long)r->total);
    }

    if (final_compare && r->state == UNTESTED) r->state = PASSED; /* optimism */

    return r->state == FAILED ? FAIL : OK;
}

int compare_src(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r,
        bool final_compare) {
    return compare_dat(p, SRC, mem_dt, mem_fp, r, final_compare);
}
int compare_wei(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r,
        bool final_compare) {
    return compare_dat(p, WEI, mem_dt, mem_fp, r, final_compare);
}
int compare_bia(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r,
        bool final_compare) {
    return compare_dat(p, BIA, mem_dt, mem_fp, r, final_compare);
}
int compare_dst(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r,
        bool final_compare) {
    return compare_dat(p, DST, mem_dt, mem_fp, r, final_compare);
}

int fill_src(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r) {
    const bool need_extra_mem = mem_dt.dt() != mem_fp.dt();
    dnn_mem_t extra_mem;
    if (need_extra_mem) {
        const auto tag = get_default_tag(mem_dt.md_.ndims);
        extra_mem = dnn_mem_t(mem_dt.md_, dnnl_f32, tag, engine_tgt);
    }
    dnn_mem_t &mem_00 = need_extra_mem ? extra_mem : mem_fp;

    const auto &c = p->cfg[SRC];
    const int range = c.f_max - c.f_min + 1;

    dnnl::impl::parallel_nd(p->mb, p->ic, p->id, p->ih, p->iw,
            [&](int mb, int ic, int id, int ih, int iw) {
                const int gen
                        = 5 * id + 17 * ih + 13 * iw + 13 * mb + 19 * ic + 1637;
                const bool non_base = flip_coin(gen, c.f_sparsity);
                const float value = non_base ? c.f_min + gen * c.f_step % range
                                             : c.f_base;

                ((float *)mem_00)[src_off_f(p, mb, 0, ic, id, ih, iw)] = value;
            });

    SAFE(mem_dt.reorder(mem_00), WARN);
    if (need_extra_mem) {
        SAFE(mem_fp.reorder(mem_dt), WARN);
        SAFE(compare_src(p, mem_fp, mem_00, r), WARN);
    }

    return OK;
}

int fill_wei(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r) {
    const bool wino_s8 = p->alg == WINO && p->cfg[WEI].dt == dnnl_s8;
    const bool s8_s8 = p->cfg[WEI].dt == dnnl_s8 && p->cfg[SRC].dt == dnnl_s8;
    const bool diff_data_type = mem_dt.dt() != mem_fp.dt();
    const bool check_reorder = diff_data_type && !wino_s8 && !s8_s8;

    dnn_mem_t extra_mem;
    if (check_reorder) {
        const auto tag = get_default_tag(mem_dt.md_.ndims);
        extra_mem = dnn_mem_t(mem_dt.md_, dnnl_f32, tag, engine_tgt);
    }
    dnn_mem_t &mem_00 = check_reorder ? extra_mem : mem_fp;

    const auto &c = p->cfg[WEI];
    const int range = c.f_max - c.f_min + 1;

    dnnl::impl::parallel_nd(p->g, p->oc / p->g, p->ic / p->g, p->kd, p->kh,
            p->kw, [&](int g, int oc, int ic, int kd, int kh, int kw) {
                const int gen
                        = 5 * kd + 17 * kh + 13 * kw + 13 * oc + 19 * ic + 38;
                const bool non_base = flip_coin(gen, c.f_sparsity);
                const float value = non_base ? c.f_min + gen * c.f_step % range
                                             : c.f_base;

                ((float *)mem_00)[wei_off_f(p, g, oc, ic, kd, kh, kw)] = value;
            });

    SAFE(mem_dt.reorder(mem_00), WARN);
    if (check_reorder) {
        SAFE(mem_fp.reorder(mem_dt), WARN);
        SAFE(compare_wei(p, mem_fp, mem_00, r), WARN);
    }

    return OK;
}

int fill_bia(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r) {
    const bool need_extra_mem = mem_dt.dt() != mem_fp.dt();
    dnn_mem_t extra_mem;
    if (need_extra_mem)
        extra_mem = dnn_mem_t(mem_dt.md_, dnnl_f32, dnnl_x, engine_tgt);
    dnn_mem_t &mem_00 = need_extra_mem ? extra_mem : mem_fp;

    const auto &c = p->cfg[BIA];
    const int range = c.f_max - c.f_min + 1;

    const size_t sz = mem_00.nelems();
    for (size_t i = 0; i < sz; ++i) {
        const int gen = (int)(19 * i);
        const bool non_base = flip_coin(gen, c.f_sparsity);
        const float value
                = non_base ? c.f_min + gen * c.f_step % range : c.f_base;

        ((float *)mem_00)[i] = value;
    }

    SAFE(mem_dt.reorder(mem_00), WARN);
    if (need_extra_mem) {
        SAFE(mem_fp.reorder(mem_dt), WARN);
        SAFE(compare_bia(p, mem_fp, mem_00, r), WARN);
    }

    return OK;
}

int fill_dst(const prb_t *p, dnn_mem_t &mem_dt, dnn_mem_t &mem_fp, res_t *r) {
    const bool need_extra_mem = mem_dt.dt() != mem_fp.dt();
    dnn_mem_t extra_mem;
    if (need_extra_mem) {
        const auto tag = get_default_tag(mem_dt.md_.ndims);
        extra_mem = dnn_mem_t(mem_dt.md_, dnnl_f32, tag, engine_tgt);
    }
    dnn_mem_t &mem_00 = need_extra_mem ? extra_mem : mem_fp;

    const auto &c = p->cfg[DST];
    const int range = c.f_max - c.f_min + 1;

    dnnl::impl::parallel_nd(p->mb, p->oc, p->od, p->oh, p->ow,
            [&](int mb, int oc, int od, int oh, int ow) {
                const int gen
                        = 7 * od + 19 * oh + 17 * ow + 13 * mb + 13 * oc + 223;
                const bool non_base = flip_coin(gen, c.f_sparsity);
                const float value = non_base ? c.f_min + gen * c.f_step % range
                                             : c.f_base;

                ((float *)mem_00)[dst_off_f(p, mb, 0, oc, od, oh, ow)] = value;
            });

    SAFE(mem_dt.reorder(mem_00), WARN);
    if (need_extra_mem) {
        SAFE(mem_fp.reorder(mem_dt), WARN);
        SAFE(compare_dst(p, mem_fp, mem_00, r), WARN);
    }

    return OK;
}

inline int init_pd(const prb_t *p, dnnl_convolution_desc_t &cd,
        dnnl_primitive_desc_t &cpd, res_t *r) {
    dnnl_memory_desc_t src_d, wei_d, bia_d, dst_d;

    int ndims = is_problem_3d(p) ? 5 : is_problem_1d(p) ? 3 : 4;
    dnnl_dims_t src_1d_dims = {p->mb, p->ic, p->iw};
    dnnl_dims_t src_2d_dims = {p->mb, p->ic, p->ih, p->iw};
    dnnl_dims_t src_3d_dims = {p->mb, p->ic, p->id, p->ih, p->iw};

    dnnl_dims_t wei_1d_dims = {p->g, p->oc / p->g, p->ic / p->g, p->kw};
    dnnl_dims_t wei_2d_dims = {p->g, p->oc / p->g, p->ic / p->g, p->kh, p->kw};
    dnnl_dims_t wei_3d_dims
            = {p->g, p->oc / p->g, p->ic / p->g, p->kd, p->kh, p->kw};

    dnnl_dims_t bia_dims = {p->oc};

    dnnl_dims_t dst_1d_dims = {p->mb, p->oc, p->ow};
    dnnl_dims_t dst_2d_dims = {p->mb, p->oc, p->oh, p->ow};
    dnnl_dims_t dst_3d_dims = {p->mb, p->oc, p->od, p->oh, p->ow};

    DNN_SAFE(dnnl_memory_desc_init_by_tag(&src_d, ndims,
                     is_problem_3d(p)
                             ? src_3d_dims
                             : is_problem_1d(p) ? src_1d_dims : src_2d_dims,
                     p->cfg[SRC].dt, p->stag),
            WARN);

    DNN_SAFE(dnnl_memory_desc_init_by_tag(&wei_d, ndims + p->has_groups,
                     is_problem_3d(p)
                             ? &wei_3d_dims[!p->has_groups]
                             : is_problem_1d(p) ? &wei_1d_dims[!p->has_groups]
                                                : &wei_2d_dims[!p->has_groups],
                     p->cfg[WEI].dt, p->wtag),
            WARN);

    DNN_SAFE(dnnl_memory_desc_init_by_tag(
                     &bia_d, 1, bia_dims, p->cfg[BIA].dt, dnnl_format_tag_any),
            WARN);

    DNN_SAFE(dnnl_memory_desc_init_by_tag(&dst_d, ndims,
                     is_problem_3d(p)
                             ? dst_3d_dims
                             : is_problem_1d(p) ? dst_1d_dims : dst_2d_dims,
                     p->cfg[DST].dt, p->dtag),
            WARN);

    dnnl_dim_t strides_nd[] = {p->sd, p->sh, p->sw};
    dnnl_dim_t dilates_nd[] = {p->dd, p->dh, p->dw};
    dnnl_dim_t padding_nd[] = {p->pd, p->ph, p->pw};

    auto bph = [&](int64_t ih, int64_t oh, int64_t kh, int64_t sh, int64_t ph,
                       int64_t dh) {
        return (oh - 1) * sh - ih + ((kh - 1) * (dh + 1) + 1) - ph;
    };
    dnnl_dim_t padding_r_nd[] = {bph(p->id, p->od, p->kd, p->sd, p->pd, p->dd),
            bph(p->ih, p->oh, p->kh, p->sh, p->ph, p->dh),
            bph(p->iw, p->ow, p->kw, p->sw, p->pw, p->dw)};

    dnnl_dim_t *strides = strides_nd + (5 - ndims);
    dnnl_dim_t *dilates = dilates_nd + (5 - ndims);
    dnnl_dim_t *padding = padding_nd + (5 - ndims);
    dnnl_dim_t *padding_r = padding_r_nd + (5 - ndims);

    dnnl_alg_kind_t alg = dnnl_convolution_direct;
    if (p->alg == WINO) alg = dnnl_convolution_winograd;
    if (p->alg == AUTO) alg = dnnl_convolution_auto;

    switch (p->dir) {
        case FWD_D:
        case FWD_B:
        case FWD_I:
            DNN_SAFE(dnnl_dilated_convolution_forward_desc_init(&cd,
                             p->dir == FWD_I ? dnnl_forward_inference
                                             : dnnl_forward_training,
                             alg, &src_d, &wei_d,
                             p->dir == FWD_B ? &bia_d : NULL, &dst_d, strides,
                             dilates, padding, padding_r),
                    WARN);
            break;
        case BWD_D:
            DNN_SAFE(dnnl_dilated_convolution_backward_data_desc_init(&cd, alg,
                             &src_d, &wei_d, &dst_d, strides, dilates, padding,
                             padding_r),
                    WARN);
            break;
        case BWD_W:
        case BWD_WB:
            DNN_SAFE(dnnl_dilated_convolution_backward_weights_desc_init(&cd,
                             alg, &src_d, &wei_d,
                             p->dir == BWD_W ? NULL : &bia_d, &dst_d, strides,
                             dilates, padding, padding_r),
                    WARN);
            break;
        default: DNN_SAFE(dnnl_invalid_arguments, CRIT);
    }

    DNN_SAFE(cd.accum_data_type == p->cfg[ACC].dt ? dnnl_success
                                                  : dnnl_unimplemented,
            CRIT);

    auto dnnl_attr = create_dnnl_attr(p->attr, p->oc, p->scales);

    dnnl_status_t init_status = dnnl_success;
    init_status = dnnl_primitive_desc_create(
            &cpd, &cd, dnnl_attr, engine_tgt, NULL);

    dnnl_primitive_attr_destroy(dnnl_attr);

    if (init_status == dnnl_unimplemented)
        return r->state = UNIMPLEMENTED, OK;
    else
        SAFE(init_status, WARN);

    const char *impl_str = query_impl_info(cpd);
    if (maybe_skip(skip_impl, impl_str)) {
        print(2, "SKIPPED: dnnl implementation: %s\n", impl_str);
        DNN_SAFE(dnnl_primitive_desc_destroy(cpd), WARN);
        return r->state = SKIPPED, OK;
    } else {
        print(5, "dnnl implementation: %s\n", impl_str);
    }

    auto q = [=](dnnl_query_t query, int index = 0) {
        return *dnnl_primitive_desc_query_md(cpd, query, index);
    };

    if (p->alg == AUTO) {
        dnnl_convolution_desc_t *temp_conv_desc = {0};
        DNN_SAFE(dnnl_primitive_desc_query(
                         cpd, dnnl_query_convolution_d, 0, &temp_conv_desc),
                CRIT);
        cd.alg_kind = temp_conv_desc->alg_kind;
    }

    if (p->dir == BWD_D)
        cd.diff_src_desc = q(dnnl_query_diff_src_md);
    else
        cd.src_desc = q(dnnl_query_src_md);

    if (p->dir & FLAG_WEI)
        cd.diff_weights_desc = q(dnnl_query_diff_weights_md);
    else
        cd.weights_desc = q(dnnl_query_weights_md);

    if (p->dir & FLAG_BIA) {
        if (p->dir & FLAG_BWD)
            cd.diff_bias_desc = q(dnnl_query_diff_weights_md, 1);
        else
            cd.bias_desc = q(dnnl_query_weights_md, 1);
    }

    if (p->dir & FLAG_BWD)
        cd.diff_dst_desc = q(dnnl_query_diff_dst_md);
    else
        cd.dst_desc = q(dnnl_query_dst_md);

    return OK;
}

int doit(const prb_t *p, res_t *r) {
    if (bench_mode == LIST) return r->state = LISTED, OK;

    dnnl_convolution_desc_t cd;
    dnnl_primitive_desc_t cpd;
    dnnl_primitive_t c;

    SAFE(init_pd(p, cd, cpd, r), WARN);

    if (r->state == SKIPPED || r->state == UNIMPLEMENTED) return OK;

    const auto alg = alg_kind2alg(cd.alg_kind);
    const auto cfg = auto_cfg(alg, p->cfg);
    prb_t p_new((desc_t)*p, p->dir, cfg, p->stag, p->wtag, p->dtag, alg,
            p->attr, p->mb);
    p = &p_new;

    DNN_SAFE(dnnl_primitive_create(&c, cpd), WARN);
    DNN_SAFE(dnnl_primitive_desc_destroy(cpd), CRIT);

    auto &src_dt_d = p->dir == BWD_D ? cd.diff_src_desc : cd.src_desc;
    auto &wei_dt_d = p->dir & FLAG_WEI ? cd.diff_weights_desc : cd.weights_desc;
    auto &bia_dt_d = p->dir & FLAG_BWD ? cd.diff_bias_desc : cd.bias_desc;
    auto &dst_dt_d = p->dir & FLAG_BWD ? cd.diff_dst_desc : cd.dst_desc;

    dnn_mem_t src_dt(src_dt_d, p->cfg[SRC].dt, engine_tgt);
    dnn_mem_t wei_dt(wei_dt_d, p->cfg[WEI].dt, engine_tgt);
    dnn_mem_t dst_dt(dst_dt_d, p->cfg[DST].dt, engine_tgt);
    dnn_mem_t bia_dt;
    if (p->dir & FLAG_BIA)
        bia_dt = dnn_mem_t(bia_dt_d, p->cfg[BIA].dt, engine_tgt);

    auto src_tag = get_default_tag(src_dt.md_.ndims);
    auto wei_tag = get_default_tag(wei_dt.md_.ndims);

    const auto fp = dnnl_f32;
    dnn_mem_t src_fp(src_dt_d, fp, src_tag, engine_tgt);
    dnn_mem_t wei_fp(wei_dt_d, fp, wei_tag, engine_tgt);
    dnn_mem_t dst_fp(dst_dt_d, fp, src_tag, engine_tgt);
    dnn_mem_t bia_fp;
    if (p->dir & FLAG_BIA) bia_fp = dnn_mem_t(bia_dt_d, fp, dnnl_x, engine_tgt);

    SAFE(fill_src(p, src_dt, src_fp, r), WARN);
    SAFE(fill_wei(p, wei_dt, wei_fp, r), WARN);
    SAFE(fill_dst(p, dst_dt, dst_fp, r), WARN);
    if (p->dir & FLAG_BIA) SAFE(fill_bia(p, bia_dt, bia_fp, r), WARN);

    args_t args;

    if (p->dir & FLAG_FWD) {
        args.set(DNNL_ARG_SRC, src_dt);
        args.set(DNNL_ARG_WEIGHTS, wei_dt);
        if (p->dir & FLAG_BIA) args.set(DNNL_ARG_BIAS, bia_dt);
        args.set(DNNL_ARG_DST, dst_dt);

        DNN_SAFE(execute_and_wait(c, stream_tgt, args), WARN);

        if (bench_mode & CORR) {
            compute_ref_fwd(p, src_fp, wei_fp, bia_fp, dst_fp);
            dnn_mem_t dst(dst_dt, fp, src_tag, engine_tgt);
            SAFE(compare_dst(p, dst, dst_fp, r, true), WARN);
        }
    } else if (p->dir == BWD_D) {
        args.set(DNNL_ARG_DIFF_DST, dst_dt);
        args.set(DNNL_ARG_WEIGHTS, wei_dt);
        args.set(DNNL_ARG_DIFF_SRC, src_dt);

        DNN_SAFE(execute_and_wait(c, stream_tgt, args), WARN);

        if (bench_mode & CORR) {
            compute_ref_bwd_d(p, src_fp, wei_fp, bia_fp, dst_fp);
            dnn_mem_t src(src_dt, fp, src_tag, engine_tgt);
            SAFE(compare_src(p, src, src_fp, r, true), WARN);
        }
    } else if (p->dir & FLAG_BWD && p->dir & FLAG_WEI) {
        args.set(DNNL_ARG_SRC, src_dt);
        args.set(DNNL_ARG_DIFF_DST, dst_dt);
        args.set(DNNL_ARG_DIFF_WEIGHTS, wei_dt);
        if (p->dir & FLAG_BIA) args.set(DNNL_ARG_DIFF_BIAS, bia_dt);

        DNN_SAFE(execute_and_wait(c, stream_tgt, args), WARN);

        if (bench_mode & CORR) {
            compute_ref_bwd_w(p, src_fp, wei_fp, bia_fp, dst_fp);
            dnn_mem_t wei(wei_dt, fp, wei_tag, engine_tgt);
            SAFE(compare_wei(p, wei, wei_fp, r, true), WARN);
            if (p->dir & FLAG_BIA) {
                dnn_mem_t bia(bia_dt, fp, dnnl_x, engine_tgt);
                SAFE(compare_bia(p, bia, bia_fp, r, true), WARN);
            }
        }
    } else {
        SAFE(FAIL, CRIT);
    }

    measure_perf(r->timer, c, args);

    DNN_SAFE(dnnl_primitive_destroy(c), CRIT);

    return OK;
}

} // namespace conv

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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>

#include "dnnl.h"

#include "src/common/dnnl_thread.hpp"

#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"
#include "norm.hpp"

#include "bnorm/bnorm.hpp"

namespace bnorm {

inline bool is_3d(const prb_t *p) {
    return p->id > 1;
}

inline bool is_1d(const prb_t *p) {
    return !is_3d(p) && p->ih == 1;
}

static int prepare_fwd_with_stats(const prb_t *p, dnn_mem_t &src,
        dnn_mem_t &mean, dnn_mem_t &var, dnn_mem_t &ss) {
    dnnl::impl::parallel_nd(p->ic, p->mb, p->id, p->ih, p->iw,
            [&](int64_t c, int64_t mb, int64_t d, int64_t h, int64_t w) {
                int64_t l_base = mb * p->id * p->ih * p->iw + c * 239 * 2;
                float *s = (float *)src + data_off(p, mb, c, 0, 0, 0);

                const int64_t sp = d * p->ih * p->iw + h * p->iw + w;
                const int64_t l = l_base + sp;
                const int64_t value = (l % 65) - 32;
                s[sp] = maybe_saturate(p->dt, value);

                ((float *)mean)[c] = 4 * ((c % 5) - 2);
                ((float *)var)[c] = ((c % 7) << 1);

                if (p->flags & USE_SCALESHIFT) {
                    ((float *)ss)[c] = (1 << (c % 7));
                    ((float *)ss)[p->ic + c] = ((c % 3) - 1) * ((float *)ss)[c];
                } else {
                    ((float *)ss)[c] = 1;
                    ((float *)ss)[p->ic + c] = 0;
                }
            });

    return OK;
}

static int prepare_fwd_no_stats(const prb_t *p, dnn_mem_t &src, dnn_mem_t &mean,
        dnn_mem_t &var, dnn_mem_t &ss) {
    /** Idea: choose src[] values so that both mean and variance are computed
     * exactly (independently of the order of the computations).
     *
     * The `exactness` is achieved via [a1]: src[i] + src[i+1] = 2 * mean.
     *
     * The variation in src is allowed in the last flex_bits bits.
     * If the sequence (L) is too big (flex_bits <= min_flex_bits), the mean
     * value is set to 0 and src is partially filled with zeros (according to
     * density so that at least want_flex_bits is reserved for src variation.
     * Once src is set, variance is computed.
     *
     * ALG_0: mean is set to 0
     * ALG_1: mean is set to 2^p, where p \in {-2, -1, ..., 4}
     * ALG_AUTO: choose between ALG_0 and ALG_1 automatically */
    const int64_t exact_bits = digits_dt(p->dt);
    const int64_t L = p->mb * p->id * p->ih * p->iw;
    const int64_t logL = (int64_t)ceilf(log2f(L));

    assert(logL <= 0 || (1LL << (logL - 1)) < L);
    assert(L <= (1LL << logL));

    const int64_t min_flex_bits = 3;
    const int64_t want_flex_bits = MIN2(6, exact_bits / 2);

    check_alg_t alg = p->check_alg;
    if (alg == ALG_AUTO) /* choose appropriate checking algorithm */
        alg = (exact_bits - logL) / 2 - 1 >= min_flex_bits ? ALG_1 : ALG_0;

    const int64_t flex_bits = alg == ALG_0
            ? want_flex_bits /* BFloat16 has only 7 bits of mantissa */
            : MIN2(p->dt == dnnl_bf16 ? 7 : exact_bits,
                    (exact_bits - logL) / 2 - 1);

    if (flex_bits < min_flex_bits) return FAIL;

    const int64_t flex_mask = (1 << flex_bits) - 1;

    /* density: (exact_bits - log_2(L * density)) / 2 >= flex_bits */
    const float density = alg == ALG_0
            ? 1.f * (1 << (exact_bits - 2 * flex_bits)) / L
            : 1.f;
    assert((exact_bits - ceilf(log2f(L * density))) / 2 >= flex_bits);

    print(6, "check_alg: %s, density = %g, flex_bits = " IFMT "\n",
            check_alg2str(alg), density, flex_bits);

    dnnl::impl::parallel_nd(p->ic, [&](int64_t c) {
        const float m = ((float *)mean)[c]
                = alg == ALG_0 ? 0.f : 0.25f * (1 << (c % 7));
        float v = 0; /* current variance */

        for (int64_t mb = 0; mb < p->mb; ++mb) {
            int64_t l_base = mb * p->id * p->ih * p->iw
                    + c * 239 * 2; // l[0] must be even
            float *s = (float *)src + data_off(p, mb, c, 0, 0, 0);

            for_(int64_t d = 0; d < p->id; ++d)
            for_(int64_t h = 0; h < p->ih; ++h)
            for (int64_t w = 0; w < p->iw; ++w) {

                const int64_t sp = d * p->ih * p->iw + h * p->iw + w;
                const int64_t l = l_base + sp;

                if (alg == ALG_0 && !flip_coin(l / 2 * 257ULL, density)) {
                    s[sp] = 0;
                    continue;
                }

                const int64_t gen = (l / 2 * 1637) & flex_mask;
                const int sgn = l % 2 == 0 ? 1 : -1; /* [a1] */
                const float f = 1.f * sgn * gen / (1 << flex_bits);

                s[sp] = alg == ALG_0 ? f : m * (1.f + f);
                if (L % 2 && (mb * p->id * p->ih * p->iw + sp == L - 1)) {
                    s[sp] = m;
                }
                v += (s[sp] - m) * (s[sp] - m);
            }
        }

        ((float *)var)[c] = v / (p->mb * p->id * p->ih * p->iw);

        if (p->flags & USE_SCALESHIFT) {
            ((float *)ss)[c] = 1.f / 8 * (1 << (c % 7));
            ((float *)ss)[p->ic + c] = ((c % 3) - 1) * ((float *)ss)[c] / 64;
        } else {
            ((float *)ss)[c] = 1;
            ((float *)ss)[p->ic + c] = 0;
        }
    });

    return OK;
}

static int prepare_fwd(const prb_t *p, dnn_mem_t &src, dnn_mem_t &mean,
        dnn_mem_t &var, dnn_mem_t &ss) {
    if (p->flags & GLOB_STATS)
        return prepare_fwd_with_stats(p, src, mean, var, ss);
    else
        return prepare_fwd_no_stats(p, src, mean, var, ss);
}

/** @brief L = 2^k * P, P % 2 != 0 */
static void decompose2(int64_t L, int64_t &k, int64_t &P) {
    P = L;
    for (k = 0; P % 2 == 0; ++k)
        P /= 2;
}

static int prepare_bwd(const prb_t *p, dnn_mem_t &src, dnn_mem_t &d_dst,
        dnn_mem_t &mean, dnn_mem_t &var, dnn_mem_t &ss, dnn_mem_t &mask) {
    const int64_t exact_bits = 24;

    const int64_t L = p->mb * p->id * p->ih * p->iw;
    if (L < 2) return FAIL;

    /** Stabilization idea...
     * Since
     *      d_src = func(d_beta / L, d_gamma' / L, ...)
     * try to make d_beta = L / 2^t_beta and d_gamma' = L / 2^t_gamma,
     * where both t_beta and t_gamma are in {1, .., max_k}.
     * Currently, with no obvious reason, max_k is set to 4 for
     * reasonably small problems and to 8 for big problems.
     *
     * Here d_gamma' = d_gamma / sqrt(var + eps).
     * We might hope that division by L would be exact in that case,
     * but that might happen iff L is less than 2^exact_bits, hence
     * restriction [r1]. */

    int64_t k, P;
    decompose2(L, k, P);

    int64_t log2P = (int64_t)ceilf(log2f(P));
    if (log2P >= exact_bits) return FAIL; /* [r1] */

    const int64_t max_k = L > (1 << 20) ? 8 : 4;
    if (k > max_k && exact_bits - log2P > max_k + 4) {
        log2P += (k - max_k);
        P <<= k - max_k;
        k = max_k;
    }

    const int64_t param_dd_p2 = 7; // factor_dd <- 2^{0, .., -param_db_p2+1}
    const int64_t param_dd_gen = 32; // gen_dd <- {1, .., param_dd_gen}

    const int64_t param_f_p2 = 1; // factor_f <- 2^{-param_dg_p2}
    const int64_t param_f_gen = 16; // gen_f <- {2..param_s_gen}

    const float ub_dg = param_dd_gen * param_f_gen / 2 * L;
    const float ub_db = param_dd_gen * L;
    const float density
            = MIN3(1.f, (1 << exact_bits) / ub_dg, (1 << exact_bits) / ub_db);

    print(5, "prep_bwd: k:" IFMT ", P:" IFMT " log2P:" IFMT ", density = %g\n",
            k, P, log2P, density);

    dnnl::impl::parallel_nd(p->ic, [&](int64_t c) {
        const float m = ((float *)mean)[c] = c % 2;

        /* var + eps \in {1/4, 1, 4} */
        const float ve_denom = 4.f / (1 << 2 * (c % 3));
        ((float *)var)[c] = ve_denom - p->eps;

        const int64_t dd_p2 = (c * 127 % param_dd_p2);
        const float factor_dd = 1.f / (1 << dd_p2);

        const int64_t f_p2 = 1 + (c % param_f_p2);
        const float factor_f = 1.f / (1 << f_p2);

        const float target_db = factor_dd * P;
        const float target_dg = ve_denom * 2 * target_db;

        float dg = 0, db = 0; /* current d_beta and d_gamma */
        for (int64_t mb = 0; mb < p->mb; ++mb) {
            const int64_t l_base = mb * p->id * p->ih * p->iw;

            const auto off = data_off(p, mb, c, 0, 0, 0);
            float *s = (float *)src + off;
            float *dd = (float *)d_dst + off;
            float *rmask = (float *)mask + off;

            for_(int64_t d = 0; d < p->id; ++d)
            for_(int64_t h = 0; h < p->ih; ++h)
            for (int64_t w = 0; w < p->iw; ++w) {

                const int64_t sp = d * p->ih * p->iw + h * p->iw + w;
                if (!flip_coin(l_base + sp, density) && l_base + sp + 100 < L) {
                    dd[sp] = 0;
                    s[sp] = m;
                    rmask[sp] = 1;
                    continue;
                }
                if (l_base + sp + 2 >= L) continue; /* last 2 are special */
                const int64_t l = l_base + sp * 7 + c * 19 + mb * 13;

                int64_t rmask_v = 1;
                if (p->flags & FUSE_NORM_RELU) rmask[sp] = rmask_v = l % 5 != 1;

                const int sgn_dd = db < target_db ? 1 : -1;
                dd[sp] = sgn_dd * factor_dd * (1 + (l * 3 % param_dd_gen));
                if (rmask_v) db += dd[sp];

                const int sgn_f = dg < target_dg ? 1 : -1;
                const float f
                        = sgn_f * factor_f * (2 + (l * 7 % (param_f_gen - 1)));

                if (rmask_v) dg += f * dd[sp];
                s[sp] = f + m;
            }
        }

        if (1) {
            /* the last 2 elements in src and d_dst are set, so that:
             *      db == target_db
             *      dg == target_dg
             * For this we need to solve the system:
             *      d_dst[l1]           + d_dst[l0]           = target_db - db
             *      d_dst[l1] * src[l1] + d_dst[l0] * src[l0] = target_dg - dg
             *
             * Here l0 -- last index, l1 -- last but one.
             * More over, let's assume src[l1] = 1 and src[l0] = -1. */
            int64_t l0 = data_off(
                    p, p->mb - 1, c, p->id - 1, p->ih - 1, p->iw - 1);
            int64_t l1 = l0 - 1;
            if (p->id == 1 && p->ih == 1 && p->iw == 1)
                l1 = data_off(p, p->mb - 2, c, p->id - 1, p->ih - 1, p->iw - 1);

            ((float *)src)[l1] = 1.f;
            ((float *)src)[l0] = -1.f;
            if (p->flags & FUSE_NORM_RELU)
                ((float *)mask)[l0] = ((float *)mask)[l1] = 1;

            float f1 = ((target_db - db) + (target_dg - dg)) / 2;
            float f0 = ((target_db - db) - (target_dg - dg)) / 2;

            ((float *)d_dst)[l1] = f1 + m;
            ((float *)d_dst)[l0] = f0 + m;

            if (p->dt == dnnl_bf16) { // truncate to bf16
                ((uint16_t *)(&((float *)d_dst)[l1]))[0] = 0;
                ((uint16_t *)(&((float *)d_dst)[l0]))[0] = 0;
            }
        }

        if (p->flags & USE_SCALESHIFT) {
            ((float *)ss)[c] = 1.f / 2 * (1 << (c % 7));
            ((float *)ss)[p->ic + c] = ((float *)ss)[c] / 64;
        } else {
            ((float *)ss)[c] = 1;
            ((float *)ss)[p->ic + c] = 0;
        }
    });

    return OK;
}

static int compare(const prb_t *p, data_kind_t kind, const dnn_mem_t &fp_mem,
        const dnn_mem_t &dt_mem, res_t *r, const dnn_mem_t *ss = nullptr) {
    const char *skind = data_kind2str(kind);

    const int f32_mant_digits = 24;
    const float eps_coeff = (1 << (f32_mant_digits - digits_dt(p->dt)));
    const float eps = eps_coeff
            * (p->dir & FLAG_FWD ? (kind == DATA ? 5e-7 : 0)
                                 : (kind == DATA ? 2e-7 : 0));

    /* With all the stability tricks bwd_d is still pretty unstable.
     * So let's rely on relative error in L1, L2, and L_inf norms.
     * TODO: make computations for bwd_d more stable and use `L0` here. */
    const bool rely_on_norm = false
            || (kind == DATA && (p->dir & FLAG_BWD) && (p->flags | GLOB_STATS));

    const int64_t N = kind == DATA ? p->mb : 1;
    const int64_t C = kind == DATA ? p->ic : p->ic * (kind == SS ? 2 : 1);
    const int64_t SP = kind == DATA ? p->id * p->ih * p->iw : 1;
    const auto nelems = N * C * SP;
    r->total += rely_on_norm ? 1 : nelems;

    diff_norm_t diff_norm;
    for_(int64_t n = 0; n < N; n++)
    for_(int64_t c = 0; c < C; c++)
    for (int64_t sp = 0; sp < SP; ++sp) {
        int64_t i = (n * C + c) * SP + sp;
        const float dt = dt_mem.get_elem(i);
        const float fp = fp_mem.get_elem(i);
        diff_norm.update(fp, dt);

        if (rely_on_norm) continue;

        const float diff = fabsf(fp - dt);
        const float rel_diff = diff / (fabsf(fp) > FLT_MIN ? fabsf(fp) : 1);
        bool ok = (fabsf(fp) > 1e-5 ? rel_diff : diff) <= eps;

        /* When the error is larger than eps, It could be
         * due to catastrophic cancellation in final result
         * which is computed as `Y = a * X + b`.
         * When `a * X`  is close to `b` and `sign(a * X) = - sign(b)`.
         * Then large error in `a * X` could result in a final
         * result (which has a cancellation i.e. `|Y| = |a*X - (-b)|`)
         * which has no meaningful digits left in mantissa.*/
        if (!ok && (p->dir & FLAG_FWD) && kind == DATA && ss) {
            const float beta = ((float *)*ss)[p->ic + c];
            /* Using an empirically derived threshold,
             * check if cancellation error
             * in `|Y| = |a*X - (-b)|` is huge.*/
            bool maybe_cancellation_error
                    = (fabsf(fp - beta) / (fabsf(fp) > FLT_MIN ? fabsf(fp) : 1))
                    > 1.0f;
            if (maybe_cancellation_error) {
                /* Check for error in `a * X` */
                float diff_aX = fabsf((fp - beta) - (dt - beta));
                float rel_diff_aX = diff_aX
                        / (fabsf(fp - beta) > FLT_MIN ? fabsf(fp - beta) : 1);
                ok = rel_diff_aX <= eps;
            }
        }

        r->errors += !ok;

        bool dump = false || (!ok && (r->errors < 10 || verbose >= 10))
                || (verbose >= 50 && i < 30) || (verbose >= 99);
        if (dump) {
            std::stringstream ss;
            if (kind == DATA) {
                int64_t mb, c, d, h, w;
                inv_data_off(p, i, mb, c, d, h, w);
                ss << mb << "," << c << "," << d << "," << h << "," << w;
            } else if (kind == SS) {
                ss << i / p->ic << "," << i % p->ic;
            } else {
                ss << i;
            }

            std::string ind_str = ss.str();
            print(0, "[%4ld][%s%s][%s] fp:%8g dt:%8g diff:%8g rdiff:%8g\n",
                    (long)i, p->dir & FLAG_BWD ? "D_" : "", skind,
                    ind_str.c_str(), fp, dt, diff, rel_diff);
        }
    }

    diff_norm.done();

    if (rely_on_norm) {
        r->errors += false || diff_norm.rel_diff(norm_t::L1) > eps
                || diff_norm.rel_diff(norm_t::L2) > eps
                || diff_norm.rel_diff(norm_t::L8) > eps;
    }

    if (r->errors || verbose >= 5) {
        const int vl = r->errors ? 0 : 2;
        print(vl,
                "@@@ [%s%s] diff: l0(``%g``) "
                "l1:(%g,%g,%g,``%g``) "
                "l2:(%g,%g,%g,``%g``) "
                "l8:(%g,%g,%g,``%g``)\n",
                p->dir & FLAG_BWD ? "D_" : "", skind,
                diff_norm.rel_diff(norm_t::L0), diff_norm.a_[norm_t::L1],
                diff_norm.b_[norm_t::L1], diff_norm.diff_[norm_t::L1],
                diff_norm.rel_diff(norm_t::L1), diff_norm.a_[norm_t::L2],
                diff_norm.b_[norm_t::L2], diff_norm.diff_[norm_t::L2],
                diff_norm.rel_diff(norm_t::L2), diff_norm.a_[norm_t::L8],
                diff_norm.b_[norm_t::L8], diff_norm.diff_[norm_t::L8],
                diff_norm.rel_diff(norm_t::L8));
    }

    if (r->errors) r->state = FAILED;

    if (r->state == UNTESTED) r->state = PASSED; /* optimism */

    return r->state == FAILED ? FAIL : OK;
}

int check_fwd_ws(const dnn_mem_t &dst_dt, const dnn_mem_t &ws_dt, res_t *r) {
    /* so far we know ws is just bit-mask of whether value was negative or
     * positive */
    const auto nelems = dst_dt.nelems(true);
    const uint8_t *ws = (const uint8_t *)ws_dt;

    /* some internal knowledge: flags in ws are either stored as bytes (e.g.
     * for the ref implementation) or as bits (e.g. for the jitted one); in
     * the latter case the ws memory has fewer elements than the data memory */
    enum { ws_byte, ws_bit } ws_type;
    ws_type = ws_dt.nelems(true) < nelems ? ws_bit : ws_byte;

    /* more internal knowledge: dst_dt and ws_dt are expected to have exactly
     * the same data layout, and dst_dt padded regions are expected to be
     * zero, and the respective ws_dt elements should be set accordingly */
    for (int64_t i = 0; i < nelems; i += 8) {
        for (int64_t j = 0; j < MIN2(8, nelems - i); ++j) {
            const float data = dst_dt.get_elem(i + j);
            const bool want = data > 0.f;
            const bool bit_set = ws_type == ws_byte ? *ws : !!(*ws & (1 << j));

            const bool ok = bit_set == want;
            r->errors += !ok;

            bool dump = false || (!ok && (r->errors < 10 || verbose >= 10))
                    || (verbose >= 50 && i < 30);
            if (dump) {
                print(0, "[%4ld] ws exp:%d got:%d (data:%g:%a)\n",
                        (long)(i + j), want, bit_set, data, data);
            }

            // XXX: GPU implementation uses int32_t for workspace
            if (engine_tgt_kind == dnnl_gpu) {
                ws += sizeof(int32_t);
            } else {
                if (ws_type == ws_byte) ++ws;
            }
        }
        if (ws_type == ws_bit) ++ws;
    }

    if (r->errors) r->state = FAILED;

    if (r->state == UNTESTED) r->state = PASSED; /* optimism */

    return r->state == FAILED ? FAIL : OK;
}

static int init_pd(const prb_t *p, dnnl_batch_normalization_desc_t &bd,
        dnnl_primitive_desc_t &bpd, res_t *r) {
    dnnl_memory_desc_t data_d;

    const int ndims = is_3d(p) ? 5 : is_1d(p) ? 3 : 4;

    dnnl_dims_t data_dims_1d = {p->mb, p->ic, p->iw};
    dnnl_dims_t data_dims_2d = {p->mb, p->ic, p->ih, p->iw};
    dnnl_dims_t data_dims_3d = {p->mb, p->ic, p->id, p->ih, p->iw};

    dnnl_dim_t *data_dims
            = is_3d(p) ? data_dims_3d : is_1d(p) ? data_dims_1d : data_dims_2d;

    DNN_SAFE(dnnl_memory_desc_init_by_tag(
                     &data_d, ndims, data_dims, p->dt, p->tag),
            WARN);

    auto flags = (dnnl_normalization_flags_t)p->flags;
    if (p->dir & FLAG_FWD) {
        auto prop = p->dir & FLAG_INF ? dnnl_forward_inference
                                      : dnnl_forward_training;
        DNN_SAFE(dnnl_batch_normalization_forward_desc_init(
                         &bd, prop, &data_d, p->eps, flags),
                WARN);

    } else {
        dnnl_memory_desc_t diff_data_d;
        DNN_SAFE(dnnl_memory_desc_init_by_tag(&diff_data_d, ndims, data_dims,
                         p->dt, dnnl_format_tag_any),
                WARN);
        auto prop = p->dir & FLAG_WEI ? dnnl_backward : dnnl_backward_data;
        DNN_SAFE(dnnl_batch_normalization_backward_desc_init(
                         &bd, prop, &diff_data_d, &data_d, p->eps, flags),
                WARN);
    }

    dnnl_primitive_desc_t hint_fwd_pd = NULL;
    if (p->dir & FLAG_BWD) {
        dnnl_batch_normalization_desc_t bd_fwd;
        DNN_SAFE(dnnl_batch_normalization_forward_desc_init(&bd_fwd,
                         dnnl_forward_training, &data_d, p->eps, flags),
                WARN);
        dnnl_status_t init_fwd_status = dnnl_primitive_desc_create(
                &hint_fwd_pd, &bd_fwd, NULL, engine_tgt, NULL);
        if (init_fwd_status == dnnl_unimplemented)
            return r->state = UNIMPLEMENTED, OK;
        else
            SAFE(init_fwd_status, WARN);
    }
    auto dnnl_attr = create_dnnl_attr(p->attr, 1, NULL);
    dnnl_status_t init_status = dnnl_primitive_desc_create(
            &bpd, &bd, dnnl_attr, engine_tgt, hint_fwd_pd);

    dnnl_primitive_desc_destroy(hint_fwd_pd);
    dnnl_primitive_attr_destroy(dnnl_attr);

    if (init_status == dnnl_unimplemented)
        return r->state = UNIMPLEMENTED, OK;
    else
        SAFE(init_status, WARN);

    const char *impl_str = query_impl_info(bpd);
    if (maybe_skip(skip_impl, impl_str)) {
        print(2, "SKIPPED: dnnl implementation: %s\n", impl_str);
        DNN_SAFE(dnnl_primitive_desc_destroy(bpd), WARN);
        return r->state = SKIPPED, OK;
    } else {
        print(5, "dnnl implementation: %s\n", impl_str);
        if (!strstr(impl_str, "jit")) {
            print(2, "WARNING: %s",
                    "accuracy of the implementation being tested "
                    "depends on the compiler and might give "
                    "false-positives.\n");
            print(2, "         %s",
                    "please consider recompiling the sources with"
                    " `-prec-div -fp-model precise` for a reliable testing.\n");
        }
    }

    return OK;
}

/** converts benchdnn-understandable mask of {0, 1} to workspace */
static int cvt_mask_to_ws(
        const prb_t *p, const dnn_mem_t &mask_fp, dnn_mem_t &ws_dt) {
    const int ndims = is_3d(p) ? 5 : is_1d(p) ? 3 : 4;

    dnnl_dims_t data_dims_1d = {p->mb, p->ic, p->iw};
    dnnl_dims_t data_dims_2d = {p->mb, p->ic, p->ih, p->iw};
    dnnl_dims_t data_dims_3d = {p->mb, p->ic, p->id, p->ih, p->iw};

    dnnl_dim_t *data_dims
            = is_3d(p) ? data_dims_3d : is_1d(p) ? data_dims_1d : data_dims_2d;

    dnn_mem_t data(ndims, data_dims, dnnl_f32, p->tag, engine_tgt);
    SAFE(data.reorder(mask_fp), WARN);

    dnn_mem_t mean(1, &p->ic, dnnl_f32, dnnl_x, engine_tgt);
    dnn_mem_t var(1, &p->ic, dnnl_f32, dnnl_x, engine_tgt);

    for (int64_t c = 0; c < p->ic; ++c)
        ((float *)mean)[c] = 0.5;
    for (int64_t c = 0; c < p->ic; ++c)
        ((float *)var)[c] = 1;

    dnnl_batch_normalization_desc_t bd;
    auto flags = (dnnl_normalization_flags_t)(
            dnnl_use_global_stats | dnnl_fuse_norm_relu);
    DNN_SAFE(dnnl_batch_normalization_forward_desc_init(
                     &bd, dnnl_forward_training, &data.md_, 0, flags),
            WARN);

    dnnl_primitive_desc_t bpd;
    DNN_SAFE(dnnl_primitive_desc_create(&bpd, &bd, NULL, engine_tgt, NULL),
            WARN);

    dnnl_primitive_t b;
    DNN_SAFE(dnnl_primitive_create(&b, bpd), WARN);
    DNN_SAFE(dnnl_primitive_desc_destroy(bpd), CRIT);

    args_t args;
    args.set(DNNL_ARG_SRC, data);
    args.set(DNNL_ARG_MEAN, mean);
    args.set(DNNL_ARG_VARIANCE, var);
    args.set(DNNL_ARG_DST, data);
    args.set(DNNL_ARG_WORKSPACE, ws_dt);
    DNN_SAFE(execute_and_wait(b, stream_tgt, args), WARN);
    DNN_SAFE(dnnl_primitive_destroy(b), CRIT);

    return OK;
}

int doit(const prb_t *p, res_t *r) {
    if (bench_mode == LIST) return r->state = LISTED, OK;

    dnnl_batch_normalization_desc_t bd;
    dnnl_primitive_desc_t bpd;
    dnnl_primitive_t b;

    SAFE(init_pd(p, bd, bpd, r), WARN);
    if (r->state == SKIPPED || r->state == UNIMPLEMENTED) return OK;

    const auto fp = dnnl_f32;
    const auto tag = get_default_tag(bd.data_desc.ndims);
    const auto &data_desc = bd.data_desc;

    dnn_mem_t src_fp(data_desc, fp, tag, engine_tgt);
    dnn_mem_t src_dt(data_desc, engine_tgt);

    dnn_mem_t &dst_fp = src_fp; // in-place in ref code
    dnn_mem_t placeholder_dst_dt;
    if (!p->inplace) { placeholder_dst_dt = dnn_mem_t(data_desc, engine_tgt); }
    dnn_mem_t &dst_dt = p->inplace ? src_dt : placeholder_dst_dt;

    const dnnl_dims_t dims1d = {p->ic};
    dnn_mem_t mean_fp(1, dims1d, fp, dnnl_x, engine_tgt),
            mean_dt(mean_fp.md_, engine_tgt);
    dnn_mem_t var_fp(1, dims1d, fp, dnnl_x, engine_tgt),
            var_dt(var_fp.md_, engine_tgt);

    const dnnl_dims_t dims2d = {2, p->ic};
    dnn_mem_t ss_fp(2, dims2d, fp, dnnl_nc, engine_tgt),
            ss_dt(ss_fp.md_, engine_tgt);
    dnn_mem_t d_ss_fp(2, dims2d, fp, dnnl_nc, engine_tgt),
            d_ss_dt(d_ss_fp.md_, engine_tgt);

    dnn_mem_t ws_fp(src_fp.md_, engine_tgt);
    dnn_mem_t ws_dt;
    if ((p->flags & FUSE_NORM_RELU) && !(p->dir & FLAG_INF)) {
        const auto ws_md
                = dnnl_primitive_desc_query_md(bpd, dnnl_query_workspace_md, 0);
        SAFE(ws_md != NULL ? OK : FAIL, WARN);
        ws_dt = dnn_mem_t(*ws_md, engine_tgt);
    }

    DNN_SAFE(dnnl_primitive_create(&b, bpd), WARN);
    DNN_SAFE(dnnl_primitive_desc_destroy(bpd), CRIT);

    dnn_mem_t d_dst_dt, placeholder_d_src_dt;

    args_t args;

    if (p->dir & FLAG_FWD) {
        if (prepare_fwd(p, src_fp, mean_fp, var_fp, ss_fp) != OK)
            return r->state = MISTRUSTED, OK;

        SAFE(src_dt.reorder(src_fp), WARN);

        args.set(DNNL_ARG_SRC, src_dt);
        args.set(DNNL_ARG_DST, p->inplace ? src_dt : dst_dt);

        if (p->flags & GLOB_STATS) {
            /* prepare mean & var if they are inputs */
            SAFE(mean_dt.reorder(mean_fp), WARN);
            SAFE(var_dt.reorder(var_fp), WARN);
        }
        args.set(DNNL_ARG_MEAN, mean_dt);
        args.set(DNNL_ARG_VARIANCE, var_dt);

        if (p->flags & USE_SCALESHIFT) {
            SAFE(ss_dt.reorder(ss_fp), WARN);
            args.set(DNNL_ARG_SCALE_SHIFT, ss_dt);
        }

        if (p->flags & FUSE_NORM_RELU) args.set(DNNL_ARG_WORKSPACE, ws_dt);

        DNN_SAFE(execute_and_wait(b, stream_tgt, args), WARN);

        if (bench_mode & CORR) {
            compute_ref_fwd(p, src_fp, mean_fp, var_fp, ss_fp, dst_fp);
            if (!(p->flags & GLOB_STATS) && !(p->dir & FLAG_INF)) {
                SAFE(compare(p, MEAN, mean_fp, mean_dt, r), WARN);

                SAFE(compare(p, VAR, var_fp, var_dt, r), WARN);
            }
            dnn_mem_t dst(dst_dt, fp, tag, engine_tgt);
            SAFE(compare(p, DATA, dst_fp, dst, r, &ss_fp), WARN);
            if ((p->flags & FUSE_NORM_RELU) && !(p->dir & FLAG_INF)) {
                SAFE(check_fwd_ws(dst_dt, ws_dt, r), WARN);
            }
        }
    } else {
        const_dnnl_primitive_desc_t const_bpd;
        DNN_SAFE(dnnl_primitive_get_primitive_desc(b, &const_bpd), CRIT);
        const auto &d_data_desc = *dnnl_primitive_desc_query_md(
                const_bpd, dnnl_query_diff_src_md, 0);

        dnn_mem_t d_dst_fp(d_data_desc, fp, tag, engine_tgt);
        d_dst_dt = dnn_mem_t(d_data_desc, engine_tgt);

        dnn_mem_t &d_src_fp = d_dst_fp; // in-place in ref code
        if (!p->inplace) {
            placeholder_d_src_dt = dnn_mem_t(d_data_desc, engine_tgt);
        }
        dnn_mem_t &d_src_dt = p->inplace ? d_dst_dt : placeholder_d_src_dt;

        if (prepare_bwd(p, src_fp, d_dst_fp, mean_fp, var_fp, ss_fp, ws_fp)
                != OK)
            return r->state = MISTRUSTED, OK;

        SAFE(src_dt.reorder(src_fp), WARN);
        SAFE(d_dst_dt.reorder(d_dst_fp), WARN);

        args.set(DNNL_ARG_SRC, src_dt);

        args.set(DNNL_ARG_DIFF_DST, d_dst_dt);
        args.set(DNNL_ARG_DIFF_SRC, d_src_dt);

        SAFE(mean_dt.reorder(mean_fp), WARN);
        SAFE(var_dt.reorder(var_fp), WARN);
        args.set(DNNL_ARG_MEAN, mean_dt);
        args.set(DNNL_ARG_VARIANCE, var_dt);

        if (p->flags & USE_SCALESHIFT) {
            SAFE(ss_dt.reorder(ss_fp), WARN);
            args.set(DNNL_ARG_SCALE_SHIFT, ss_dt);
            args.set(DNNL_ARG_DIFF_SCALE_SHIFT, d_ss_dt);
        }

        if (p->flags & FUSE_NORM_RELU) {
            SAFE(cvt_mask_to_ws(p, ws_fp, ws_dt), WARN);
            args.set(DNNL_ARG_WORKSPACE, ws_dt);
        }

        DNN_SAFE(execute_and_wait(b, stream_tgt, args), WARN);

        if (bench_mode & CORR) {
            compute_ref_bwd(p, src_fp, mean_fp, var_fp, d_dst_fp, ss_fp, ws_fp,
                    d_src_fp, d_ss_fp);
            if ((p->flags & USE_SCALESHIFT) && (p->dir & FLAG_WEI)) {
                SAFE(compare(p, SS, d_ss_fp, d_ss_dt, r), WARN);
            }
            dnn_mem_t d_src(d_src_dt, fp, tag, engine_tgt);
            SAFE(compare(p, DATA, d_src_fp, d_src, r), WARN);
        }
    }

    measure_perf(r->timer, b, args);

    DNN_SAFE(dnnl_primitive_destroy(b), CRIT);

    return OK;
}

} // namespace bnorm

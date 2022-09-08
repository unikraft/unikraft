/*******************************************************************************
* Copyright 2018 Intel Corporation
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

/*
  General architecture

  for diff states, we have n_states + 1 as we have n_states diff
  to propagate to the previous iteration and 1 states to propagate
  to the previous layer
  index 0 is dh for cell(t-1, l) to consume
  index 1 is dc for cell(t-1, l) to consume
  index 2 is dh for cell(t, l-1) to consume
  this indexing enables to have the same indexing for states in elemwise
  function
  only the cell execution function should be impacted

 */

#include "dnnl_thread.hpp"
#include "math_utils.hpp"

#include "../gemm/gemm.hpp"
#include "../simple_q10n.hpp"
#include "gemm/gemm_pack.hpp"
#include "ref_rnn.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

using namespace dnnl::impl::utils;
using namespace dnnl::impl::memory_tracking::names;
using namespace rnn_utils;
#define AOC array_offset_calculator

// GEMM functions wrapper definitions

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_gemm_sig(
        (_ref_rnn_common_t<aprop, src_type, weights_type, acc_type>::gemm)) {
    assert(!"non packed gemm is unavailable for this data type");
}

template <>
rnn_gemm_sig((ref_rnn_fwd_f32_t::gemm)) {
    assert(ldA * ldB * ldC != 0);
    auto st = extended_sgemm(&transA, &transB, &m, &n, &k, &alpha, a_, &ldA, b_,
            &ldB, &beta, c_, &ldC, nullptr, pd()->rnn_.force_nocopy);
    assert(st == dnnl_success);
    MAYBE_UNUSED(st);
}

template <>
rnn_gemm_sig((ref_rnn_bwd_f32_t::gemm)) {
    assert(ldA * ldB * ldC != 0);
    auto st = extended_sgemm(&transA, &transB, &m, &n, &k, &alpha, a_, &ldA, b_,
            &ldB, &beta, c_, &ldC, nullptr, pd()->rnn_.force_nocopy);
    assert(st == dnnl_success);
    MAYBE_UNUSED(st);
}

template <>
rnn_gemm_sig((ref_rnn_fwd_bf16_t::gemm)) {
    assert(ldA * ldB * ldC != 0);
    dim_t M = m, N = n, K = k, LDA = ldA, LDB = ldB, LDC = ldC;
    auto st = gemm_bf16bf16f32(&transA, &transB, &M, &N, &K, &alpha, a_, &LDA,
            b_, &LDB, &beta, c_, &LDC);
    assert(st == dnnl_success);
    MAYBE_UNUSED(st);
}

template <>
rnn_gemm_sig((ref_rnn_bwd_bf16_t::gemm)) {
    assert(ldA * ldB * ldC != 0);
    dim_t M = m, N = n, K = k, LDA = ldA, LDB = ldB, LDC = ldC;
    auto st = gemm_bf16bf16f32(&transA, &transB, &M, &N, &K, &alpha, a_, &LDA,
            b_, &LDB, &beta, c_, &LDC);
    assert(st == dnnl_success);
    MAYBE_UNUSED(st);
}

// packed GEMM functions wrapper definitions

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_gemm_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::packed_gemm)) {
    assert(!"packed gemm is unavailable for this datatype");
}

template <>
rnn_gemm_sig(ref_rnn_fwd_f32_t::packed_gemm) {
    assert(transA == 'N' && transB == 'N' && alpha == 1.);
    auto st = sgemm_compute(
            "P", "N", &m, &n, &k, a_, &ldA, b_, &ldB, &beta, c_, &ldC);
    assert(st == dnnl_success);
    MAYBE_UNUSED(st);
}

template <>
rnn_gemm_sig(ref_rnn_bwd_f32_t::packed_gemm) {
    assert(transA == 'N' && transB == 'N' && alpha == 1.);
    auto st = sgemm_compute(
            "P", "N", &m, &n, &k, a_, &ldA, b_, &ldB, &beta, c_, &ldC);
    assert(st == dnnl_success);
    MAYBE_UNUSED(st);
}

template <>
rnn_gemm_sig(ref_rnn_fwd_u8s8_t::packed_gemm) {
    assert(transA == 'N' && transB == 'N' && alpha == 1.);
    int32_t offsetc = 0;
    gemm_s8u8s32_compute("P", "N", "F", &m, &n, &k, a_, &ldA, b_, &ldB, &beta,
            c_, &ldC, &offsetc);
}

//*************** Grid computations strategy: linear ***************//
template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_grid_execution_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::linear_execution)) {
    AOC<src_data_t, 4> ws_states(ws_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.states_nld * rnn.states_ws_ld);
    AOC<float, 4> ws_c_states(ws_c_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.states_nld * rnn.states_ws_ld);
    AOC<acc_data_t, 5> ws_diff_states(ws_diff_states_, rnn.n_layer + 1,
            rnn.n_dir, (rnn.n_states + 1), rnn.n_iter + 1,
            rnn.states_nld * rnn.states_ws_ld);
    AOC<src_data_t, 4> ws_gates(ws_gates_, rnn.n_layer, rnn.n_dir, rnn.n_iter,
            rnn.gates_nld * rnn.gates_ws_ld);
    AOC<weights_data_t *, 3> weights_layer(
            weights_layer_, rnn.n_layer, rnn.n_dir, rnn.n_parts_weights_layer);
    AOC<weights_data_t *, 3> weights_iter(
            weights_iter_, rnn.n_layer, rnn.n_dir, rnn.n_parts_weights_iter);
    AOC<float *, 3> bias(bias_, rnn.n_layer, rnn.n_dir, rnn.n_parts_bias);
    AOC<acc_data_t, 3> diff_weights_layer(diff_weights_layer_, rnn.n_layer,
            rnn.n_dir, rnn.diff_weights_layer_nld * rnn.diff_weights_layer_ld);
    AOC<acc_data_t, 3> diff_weights_iter(diff_weights_iter_, rnn.n_layer,
            rnn.n_dir, rnn.diff_weights_iter_nld * rnn.diff_weights_iter_ld);
    AOC<acc_data_t, 3> diff_bias(
            diff_bias_, rnn.n_layer, rnn.n_dir, rnn.n_bias * rnn.dic);
    AOC<src_data_t, 4> ws_grid(
            ws_grid_, rnn.n_layer, rnn.n_dir, rnn.n_iter, (int)rnn.ws_per_cell);

    // We run the grid of computation
    for (int dir = 0; dir < rnn.n_dir; dir++) {
        for (int j = 0; j < rnn.n_layer; j++) {
            int lay = (aprop == prop_kind::forward) ? j : rnn.n_layer - j - 1;

            if ((aprop == prop_kind::forward) && rnn.merge_gemm_layer) {
                (this->*gemm_layer_func)('N', 'N', rnn.n_gates * rnn.dic,
                        rnn.mb * rnn.n_iter, rnn.slc, 1.0,
                        weights_layer(lay, dir, 0), rnn.weights_layer_ld,
                        &(ws_states(lay, dir, 1, 0)), rnn.states_ws_ld, 0.0,
                        (acc_data_t *)scratch_gates_, rnn.gates_ws_ld);
            }

            for (int i = 0; i < rnn.n_iter; i++) {
                int iter = (aprop == prop_kind::forward) ? i
                                                         : rnn.n_iter - i - 1;
                (this->*cell_func)(rnn, &(ws_states(lay + 1, dir, iter + 1, 0)),
                        &(ws_c_states(lay + 1, dir, iter + 1, 0)),
                        &(ws_diff_states(lay, dir, 0, iter, 0)),
                        &(weights_layer(lay, dir, 0)),
                        &(weights_iter(lay, dir, 0)), &(bias(lay, dir, 0)),
                        &(ws_states(lay, dir, iter + 1, 0)),
                        &(ws_states(lay + 1, dir, iter, 0)),
                        &(ws_c_states(lay + 1, dir, iter, 0)),
                        &(ws_diff_states(lay + 1, dir, 0, iter, 0)),
                        &(ws_diff_states(lay, dir, 0, iter + 1, 0)),
                        &(diff_weights_layer(lay, dir, 0)),
                        &(diff_weights_iter(lay, dir, 0)),
                        &(diff_bias(lay, dir, 0)),
                        &(ws_gates(lay, dir, iter, 0)),
                        rnn.n_iter_scratch_gates == 1 ? scratch_gates_
                                                      : scratch_gates_
                                        + iter * rnn.gates_nld
                                                * rnn.gates_ws_ld,
                        &(ws_grid(lay, dir, iter, 0)), scratch_cell_);
            }

            if ((aprop == prop_kind::backward) && rnn.merge_gemm_layer) {
                (this->*gemm_layer_func)('N', 'N', rnn.slc, rnn.mb * rnn.n_iter,
                        rnn.n_gates * rnn.dic, 1.0, weights_layer(lay, dir, 0),
                        rnn.weights_layer_ld, (src_data_t *)scratch_gates_,
                        rnn.gates_ws_ld, 0.0,
                        &(ws_diff_states(lay, dir, rnn.n_states, 0, 0)),
                        rnn.states_ws_ld);
                gemm('N', 'T', rnn.n_gates * rnn.dic, rnn.slc,
                        rnn.mb * rnn.n_iter, 1.0,
                        (weights_data_t *)scratch_gates_, rnn.gates_ws_ld,
                        &(ws_states(lay, dir, 1, 0)), rnn.states_ws_ld, 1.0,
                        &(diff_weights_layer(lay, dir, 0)),
                        rnn.diff_weights_layer_ld);
            }
            if ((aprop == prop_kind::backward) && rnn.merge_gemm_iter) {
                gemm('N', 'T', rnn.n_gates * rnn.dic, rnn.sic,
                        rnn.mb * rnn.n_iter, 1.0,
                        (weights_data_t *)scratch_gates_, rnn.gates_ws_ld,
                        &(ws_states(lay + 1, dir, 0, 0)), rnn.states_ws_ld, 1.0,
                        &(diff_weights_iter(lay, dir, 0)),
                        rnn.diff_weights_iter_ld);
            }
        }
    }
}

//********* GRID computations strategy: utility functions **********//

template <typename src_data_t>
void copy_init_layer_fwd_template(const rnn_conf_t &rnn,
        src_data_t *__restrict ws_states_, const src_data_t *__restrict xt_,
        const memory_desc_wrapper &xt_d) {

    AOC<src_data_t, 4> ws_states(
            ws_states_, rnn.n_dir, rnn.n_iter + 1, rnn.mb, rnn.states_ws_ld);

    parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
        auto xxt = xt_ + xt_d.blk_off(it, b);
        src_data_t *ws_l2r_ptr = &(ws_states(0, it + 1, b, 0));
        src_data_t *ws_r2l_ptr
                = &(ws_states(rnn.n_dir - 1, rnn.n_iter - it, b, 0));
        if (rnn.exec_dir != r2l) PRAGMA_OMP_SIMD()
        for (int c = 0; c < rnn.slc; c++)
            ws_l2r_ptr[c] = xxt[c];
        if (rnn.exec_dir != l2r) PRAGMA_OMP_SIMD()
        for (int c = 0; c < rnn.slc; c++)
            ws_r2l_ptr[c] = xxt[c];
    });
}

template <typename acc_data_t>
void copy_init_layer_bwd_template(const rnn_conf_t &rnn,
        acc_data_t *ws_diff_states_, const acc_data_t *diff_dst_layer_,
        const memory_desc_wrapper &diff_dst_layer_d) {
    AOC<acc_data_t, 6> ws_diff_states(ws_diff_states_, rnn.n_layer + 1,
            rnn.n_dir, (rnn.n_states + 1), rnn.n_iter + 1, rnn.mb,
            rnn.states_ws_ld);

    switch (rnn.exec_dir) {
        case bi_concat:
            parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
                auto diff_dst_layer_x
                        = diff_dst_layer_ + diff_dst_layer_d.blk_off(it, b);
                for (int s = 0; s < rnn.dic; s++) {
                    ws_diff_states(rnn.n_layer, 0, rnn.n_states, it, b, s)
                            = diff_dst_layer_x[s];
                    ws_diff_states(rnn.n_layer, 1, rnn.n_states,
                            rnn.n_iter - it - 1, b, s)
                            = diff_dst_layer_x[rnn.dic + s];
                }
            });
            break;
        case bi_sum:
            parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
                auto diff_dst_layer_x
                        = diff_dst_layer_ + diff_dst_layer_d.blk_off(it, b);
                for (int s = 0; s < rnn.dic; s++) {
                    ws_diff_states(rnn.n_layer, 0, rnn.n_states, it, b, s)
                            = diff_dst_layer_x[s];
                    ws_diff_states(rnn.n_layer, 1, rnn.n_states,
                            rnn.n_iter - it - 1, b, s)
                            = diff_dst_layer_x[s];
                }
            });
            break;
        case l2r:
            parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
                auto diff_dst_layer_x
                        = diff_dst_layer_ + diff_dst_layer_d.blk_off(it, b);
                for (int s = 0; s < rnn.dic; s++) {
                    ws_diff_states(rnn.n_layer, 0, rnn.n_states, it, b, s)
                            = diff_dst_layer_x[s];
                }
            });
            break;
        case r2l:
            parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
                auto diff_dst_layer_x = diff_dst_layer_
                        + diff_dst_layer_d.blk_off(rnn.n_iter - it - 1, b);
                for (int s = 0; s < rnn.dic; s++) {
                    ws_diff_states(rnn.n_layer, 0, rnn.n_states, it, b, s)
                            = diff_dst_layer_x[s];
                }
            });
            break;
        default: assert(!"Unsupported direction"); break;
    }
}

#define RNN_DECL_COPY_INIT_LAYER_FWD(cname) \
    template <> \
    void cname::copy_init_layer(const rnn_conf_t &rnn, src_data_t *ws_states_, \
            acc_data_t *ws_diff_states_, const src_data_t *xt_, \
            const acc_data_t *diff_dst_layer_) const { \
        copy_init_layer_fwd_template( \
                rnn, ws_states_, xt_, memory_desc_wrapper(pd()->src_md(0))); \
    }

RNN_DECL_COPY_INIT_LAYER_FWD(ref_rnn_fwd_f32_t)
RNN_DECL_COPY_INIT_LAYER_FWD(ref_rnn_fwd_bf16_t)
RNN_DECL_COPY_INIT_LAYER_FWD(ref_rnn_fwd_u8s8_t)

#define RNN_DECL_COPY_INIT_LAYER_BWD(cname) \
    template <> \
    void cname::copy_init_layer(const rnn_conf_t &rnn, src_data_t *ws_states_, \
            acc_data_t *ws_diff_states_, const src_data_t *xt_, \
            const acc_data_t *diff_dst_layer_) const { \
        copy_init_layer_bwd_template(rnn, ws_diff_states_, diff_dst_layer_, \
                memory_desc_wrapper(pd()->diff_dst_md(0))); \
    }

RNN_DECL_COPY_INIT_LAYER_BWD(ref_rnn_bwd_f32_t)
RNN_DECL_COPY_INIT_LAYER_BWD(ref_rnn_bwd_bf16_t)

/* For int8 configuration, input iteration states may be of types f32 or u8
 * Internally h_state is always stored in u8 and c_state is always stored in f32
 * If input states are of type u8 then h state is copied and c state is dequantized
 * If input states are of type f32 then h state is quantized and c_state is copied
 * */
template <typename src_data_t, typename input_data_t>
void copy_init_iter_fwd_template(const rnn_conf_t &rnn, const rnn_pd_t *pd,
        src_data_t *__restrict ws_states_, float *__restrict ws_c_states_,
        const input_data_t *__restrict src_iter_,
        const memory_desc_wrapper &src_iter_d,
        const float *__restrict src_iter_c_,
        const memory_desc_wrapper &src_iter_c_d) {
    AOC<src_data_t, 5> ws_states(ws_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.mb, rnn.states_ws_ld);
    AOC<float, 5> ws_c_states(ws_c_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.mb, rnn.states_ws_ld);
    float data_shift = pd->attr()->rnn_data_qparams_.shift_;
    float data_scale = pd->attr()->rnn_data_qparams_.scale_;

    const bool quantize = pd->with_src_iter()
            && pd->src_md(1)->data_type == data_type::f32 && rnn.is_int8();
    auto maybe_q = [&](input_data_t f) {
        if (quantize) {
            float qf = f * data_scale + data_shift;
            return qz_a1b0<float, src_data_t>()(qf);
        } else
            return (src_data_t)f;
    };

    if (src_iter_) {
        parallel_nd(
                rnn.n_layer, rnn.n_dir, rnn.mb, [&](int lay, int dir, int b) {
                    const auto *ss
                            = &src_iter_[src_iter_d.blk_off(lay, dir, b, 0)];
                    auto *dd = &ws_states(lay + 1, dir, 0, b, 0);
                    PRAGMA_OMP_SIMD()
                    for (int s = 0; s < rnn.sic; s++)
                        dd[s] = maybe_q(ss[s]);

                    if (pd->cell_kind() == alg_kind::vanilla_lstm) {
                        const auto *ss = &src_iter_c_[src_iter_c_d.blk_off(
                                lay, dir, b, 0)];
                        auto *dd = &ws_c_states(lay + 1, dir, 0, b, 0);
                        PRAGMA_OMP_SIMD()
                        for (int s = 0; s < rnn.dic; s++)
                            dd[s] = ss[s];
                    }
                });
    } else {
        parallel_nd(
                rnn.n_layer, rnn.n_dir, rnn.mb, [&](int lay, int dir, int b) {
                    for (int j = 0; j < rnn.sic; j++)
                        ws_states(lay + 1, dir, 0, b, j) = (src_data_t)0;
                    for (int j = 0; j < rnn.dic; j++)
                        ws_c_states(lay + 1, dir, 1, b, j) = 0.0f;
                });
    }
}

template <typename acc_data_t>
void copy_init_iter_bwd_template(const rnn_conf_t &rnn, const rnn_pd_t *pd,
        acc_data_t *ws_diff_states_, const acc_data_t *diff_dst_iter_,
        const memory_desc_wrapper diff_dst_iter_d,
        const float *diff_dst_iter_c_,
        const memory_desc_wrapper diff_dst_iter_c_d) {
    AOC<acc_data_t, 6> ws_diff_states(ws_diff_states_, rnn.n_layer + 1,
            rnn.n_dir, rnn.n_states + 1, rnn.n_iter + 1, rnn.mb,
            rnn.states_ws_ld);
    if (diff_dst_iter_) {
        parallel_nd(
                rnn.n_layer, rnn.n_dir, rnn.mb, [&](int lay, int dir, int b) {
                    array_copy(&(ws_diff_states(lay, dir, 0, rnn.n_iter, b, 0)),
                            diff_dst_iter_
                                    + diff_dst_iter_d.blk_off(lay, dir, b),
                            rnn.dic);
                    if (pd->cell_kind() == alg_kind::vanilla_lstm)
                        array_copy(&(ws_diff_states(
                                           lay, dir, 1, rnn.n_iter, b, 0)),
                                diff_dst_iter_c_
                                        + diff_dst_iter_c_d.blk_off(
                                                lay, dir, b),
                                rnn.dic);
                });
    } else {
        parallel_nd(rnn.n_layer, rnn.n_dir, rnn.n_states, rnn.mb,
                [&](int lay, int dir, int state, int i) {
                    for (int j = 0; j < rnn.dic; j++)
                        ws_diff_states(lay, dir, state, rnn.n_iter, i, j)
                                = 0.0f;
                });
    }
}

#define RNN_DECL_COPY_INIT_ITER_FWD(cname) \
    template <> \
    template <typename input_data_t> \
    void cname::copy_init_iter(const rnn_conf_t &rnn, \
            src_data_t *__restrict ws_states_, float *__restrict ws_c_states_, \
            acc_data_t *__restrict ws_diff_states_, \
            const input_data_t *__restrict src_iter_, \
            const float *__restrict src_iter_c_, \
            const acc_data_t *__restrict diff_dst_iter_, \
            const float *__restrict diff_dst_iter_c_) const { \
        auto src_iter_d = memory_desc_wrapper(pd()->src_md(1)); \
        auto src_iter_c_d = memory_desc_wrapper(pd()->src_md(2)); \
        copy_init_iter_fwd_template(rnn, pd(), ws_states_, ws_c_states_, \
                src_iter_, src_iter_d, src_iter_c_, src_iter_c_d); \
    }

RNN_DECL_COPY_INIT_ITER_FWD(ref_rnn_fwd_f32_t)
RNN_DECL_COPY_INIT_ITER_FWD(ref_rnn_fwd_bf16_t)
RNN_DECL_COPY_INIT_ITER_FWD(ref_rnn_fwd_u8s8_t)

#define RNN_DECL_COPY_INIT_ITER_BWD(cname) \
    template <> \
    template <typename input_data_t> \
    void cname::copy_init_iter(const rnn_conf_t &rnn, src_data_t *ws_states_, \
            float *ws_c_states_, acc_data_t *ws_diff_states_, \
            const input_data_t *src_iter_, const float *src_iter_c_, \
            const acc_data_t *diff_dst_iter_, const float *diff_dst_iter_c_) \
            const { \
        auto diff_dst_iter_d = memory_desc_wrapper(pd()->diff_dst_md(1)); \
        auto diff_dst_iter_c_d = memory_desc_wrapper(pd()->diff_dst_md(2)); \
        copy_init_iter_bwd_template(rnn, pd(), ws_diff_states_, \
                diff_dst_iter_, diff_dst_iter_d, diff_dst_iter_c_, \
                diff_dst_iter_c_d); \
    }

RNN_DECL_COPY_INIT_ITER_BWD(ref_rnn_bwd_f32_t)
RNN_DECL_COPY_INIT_ITER_BWD(ref_rnn_bwd_bf16_t)

template <typename src_data_t, typename dst_data_t>
void copy_res_layer_fwd_template(const rnn_conf_t &rnn, const rnn_pd_t *pd,
        dst_data_t *dst_layer_, memory_desc_wrapper &dst_layer_d,
        const src_data_t *ws_states_) {

    AOC<const src_data_t, 5> ws_states(ws_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.mb, rnn.states_ws_ld);
    float shift = (pd->attr()->rnn_data_qparams_.shift_);
    float scale = (pd->attr()->rnn_data_qparams_.scale_);

    const bool dequantize
            = pd->dst_md(0)->data_type == data_type::f32 && rnn.is_int8();
    auto maybe_deq = [&](src_data_t s) {
        if (dequantize)
            return (dst_data_t)(((float)s - shift) / scale);
        else
            return (dst_data_t)s;
    };

    parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
        int dir = 0;

        if (rnn.exec_dir != r2l) {
            const auto *ss = &ws_states(rnn.n_layer, dir, it + 1, b, 0);
            auto *dd = &dst_layer_[dst_layer_d.blk_off(it, b, dir * rnn.dic)];
            PRAGMA_OMP_SIMD()
            for (int s = 0; s < rnn.dic; s++)
                dd[s] = maybe_deq(ss[s]);

            dir = 1;
        }

        if (rnn.exec_dir != l2r) {
            const auto *ss
                    = &ws_states(rnn.n_layer, dir, rnn.n_iter - it, b, 0);

            if (rnn.exec_dir == bi_sum) {
                auto *dd = &dst_layer_[dst_layer_d.blk_off(it, b, 0)];
                PRAGMA_OMP_SIMD()
                for (int s = 0; s < rnn.dic; s++)
                    dd[s] += maybe_deq(ss[s]);
            } else {
                auto *dd = &dst_layer_[dst_layer_d.blk_off(
                        it, b, dir * rnn.dic)];
                PRAGMA_OMP_SIMD()
                for (int s = 0; s < rnn.dic; s++)
                    dd[s] = maybe_deq(ss[s]);
            }
        }
    });
}

template <typename acc_data_t>
void copy_res_layer_bwd_template(const rnn_conf_t &rnn,
        acc_data_t *diff_src_layer_, memory_desc_wrapper &diff_src_layer_d,
        const acc_data_t *ws_diff_states_) {
    AOC<const acc_data_t, 6> ws_diff_states(ws_diff_states_, rnn.n_layer + 1,
            rnn.n_dir, rnn.n_states + 1, rnn.n_iter + 1, rnn.mb,
            rnn.states_ws_ld);

    parallel_nd(rnn.n_iter, rnn.mb, [&](int it, int b) {
        int dir = 0;
        for (int s = 0; s < rnn.slc; s++) {
            acc_data_t *dst_addr = diff_src_layer_
                    + diff_src_layer_d.blk_off(
                            (rnn.exec_dir == r2l) ? rnn.n_iter - 1 - it : it, b,
                            dir * rnn.slc + s);
            acc_data_t res = ws_diff_states(0, 0, rnn.n_states, it, b, s);
            if (rnn.n_dir - 1)
                res += ws_diff_states(
                        0, 1, rnn.n_states, rnn.n_iter - 1 - it, b, s);
            dst_addr[0] = res;
        }
    });
}

#define RNN_DECL_COPY_RES_LAYER_FWD(cname) \
    template <> \
    template <typename dst_data_t> \
    void cname::copy_res_layer(const rnn_conf_t &rnn, dst_data_t *dst_layer_, \
            acc_data_t *diff_src_layer, const src_data_t *ws_states_, \
            const acc_data_t *ws_diff_states_) const { \
        auto dst_layer_d = memory_desc_wrapper(pd()->dst_md(0)); \
        copy_res_layer_fwd_template( \
                rnn, pd(), dst_layer_, dst_layer_d, ws_states_); \
    }

RNN_DECL_COPY_RES_LAYER_FWD(ref_rnn_fwd_f32_t)
RNN_DECL_COPY_RES_LAYER_FWD(ref_rnn_fwd_bf16_t)
RNN_DECL_COPY_RES_LAYER_FWD(ref_rnn_fwd_u8s8_t)

#define RNN_DECL_COPY_RES_LAYER_BWD(cname) \
    template <> \
    template <typename dst_data_t> \
    void cname::copy_res_layer(const rnn_conf_t &rnn, dst_data_t *dst_layer_, \
            acc_data_t *diff_src_layer_, const src_data_t *ws_states_, \
            const acc_data_t *ws_diff_states_) const { \
        auto diff_src_layer_d = memory_desc_wrapper(pd()->diff_src_md(0)); \
        copy_res_layer_bwd_template( \
                rnn, diff_src_layer_, diff_src_layer_d, ws_diff_states_); \
    }

RNN_DECL_COPY_RES_LAYER_BWD(ref_rnn_bwd_f32_t)
RNN_DECL_COPY_RES_LAYER_BWD(ref_rnn_bwd_bf16_t)

template <typename src_data_t, typename output_data_t>
void copy_res_iter_fwd_template(const rnn_conf_t &rnn, const rnn_pd_t *pd,
        output_data_t *dst_iter_, memory_desc_wrapper &dst_iter_d,
        float *dst_iter_c_, memory_desc_wrapper dst_iter_c_d,
        const src_data_t *ws_states_, float *ws_c_states_) {
    if (dst_iter_ == nullptr) return;

    AOC<const src_data_t, 5> ws_states(ws_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.mb, rnn.states_ws_ld);
    AOC<const float, 5> ws_c_states(ws_c_states_, rnn.n_layer + 1, rnn.n_dir,
            rnn.n_iter + 1, rnn.mb, rnn.states_ws_ld);

    float data_shift = pd->attr()->rnn_data_qparams_.shift_;
    float data_scale = pd->attr()->rnn_data_qparams_.scale_;

    const bool dequantize = pd->with_dst_iter()
            && pd->dst_md(1)->data_type == data_type::f32 && rnn.is_int8();
    auto maybe_deq = [&](src_data_t s) {
        if (dequantize)
            return (output_data_t)(((float)s - data_shift) / data_scale);
        else
            return (output_data_t)s;
    };

    parallel_nd(rnn.n_layer, rnn.n_dir, rnn.mb, [&](int lay, int dir, int b) {
        const auto *ss = &ws_states(lay + 1, dir, rnn.n_iter, b, 0);
        auto *dd = &dst_iter_[dst_iter_d.blk_off(lay, dir, b, 0)];
        PRAGMA_OMP_SIMD()
        for (int s = 0; s < rnn.dic; s++)
            dd[s] = maybe_deq(ss[s]);

        if (pd->cell_kind() == alg_kind::vanilla_lstm) {
            const auto *ss = &ws_c_states(lay + 1, dir, rnn.n_iter, b, 0);
            auto *dd = &dst_iter_c_[dst_iter_c_d.blk_off(lay, dir, b, 0)];
            PRAGMA_OMP_SIMD()
            for (int s = 0; s < rnn.dic; s++)
                dd[s] = ss[s];
        }
    });
}

template <typename acc_data_t>
void copy_res_iter_bwd_template(const rnn_conf_t &rnn, const rnn_pd_t *pd,
        acc_data_t *diff_src_iter_, memory_desc_wrapper &diff_src_iter_d,
        float *diff_src_iter_c_, memory_desc_wrapper &diff_src_iter_c_d,
        const acc_data_t *ws_diff_states_) {
    AOC<const acc_data_t, 6> ws_diff_states(ws_diff_states_, rnn.n_layer + 1,
            rnn.n_dir, rnn.n_states + 1, rnn.n_iter + 1, rnn.mb,
            rnn.states_ws_ld);
    if (diff_src_iter_) {
        parallel_nd(
                rnn.n_layer, rnn.n_dir, rnn.mb, [&](int lay, int dir, int b) {
                    for (int s = 0; s < rnn.sic; s++) {
                        diff_src_iter_[diff_src_iter_d.blk_off(lay, dir, b, s)]
                                = ws_diff_states(lay, dir, 0, 0, b, s);
                    }
                    if (pd->cell_kind() == alg_kind::vanilla_lstm)
                        for (int s = 0; s < rnn.dic; s++) {
                            diff_src_iter_c_[diff_src_iter_c_d.blk_off(
                                    lay, dir, b, s)]
                                    = ws_diff_states(lay, dir, 1, 0, b, s);
                        }
                });
    }
}

#define RNN_DECL_COPY_RES_ITER_FWD(cname) \
    template <> \
    template <typename output_data_t> \
    void cname::copy_res_iter(const rnn_conf_t &rnn, output_data_t *dst_iter_, \
            float *dst_iter_c_, acc_data_t *diff_src_iter_, \
            float *diff_src_iter_c_, const src_data_t *ws_states_, \
            float *ws_c_states_, const acc_data_t *ws_diff_states_) const { \
        auto dst_iter_d = memory_desc_wrapper(pd()->dst_md(1)); \
        auto dst_iter_c_d = memory_desc_wrapper(pd()->dst_md(2)); \
        copy_res_iter_fwd_template(rnn, pd(), dst_iter_, dst_iter_d, \
                dst_iter_c_, dst_iter_c_d, ws_states_, ws_c_states_); \
    }

RNN_DECL_COPY_RES_ITER_FWD(ref_rnn_fwd_f32_t)
RNN_DECL_COPY_RES_ITER_FWD(ref_rnn_fwd_bf16_t)
RNN_DECL_COPY_RES_ITER_FWD(ref_rnn_fwd_u8s8_t)

#define RNN_DECL_COPY_RES_ITER_BWD(cname) \
    template <> \
    template <typename output_data_t> \
    void cname::copy_res_iter(const rnn_conf_t &rnn, output_data_t *dst_iter_, \
            float *dst_iter_c_, acc_data_t *diff_src_iter_, \
            float *diff_src_iter_c_, const src_data_t *ws_states_, \
            float *ws_c_states_, const acc_data_t *ws_diff_states_) const { \
        auto diff_src_iter_d = memory_desc_wrapper(pd()->diff_src_md(1)); \
        auto diff_src_iter_c_d = memory_desc_wrapper(pd()->diff_src_md(2)); \
        copy_res_iter_bwd_template(rnn, pd(), diff_src_iter_, diff_src_iter_d, \
                diff_src_iter_c_, diff_src_iter_c_d, ws_diff_states_); \
    }

RNN_DECL_COPY_RES_ITER_BWD(ref_rnn_bwd_f32_t)
RNN_DECL_COPY_RES_ITER_BWD(ref_rnn_bwd_bf16_t)

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_bias_prepare_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::bias_prepare)) {
    /* Original set of bias provided by the user */
    AOC<const float, 5> b(b_, rnn.n_layer, rnn.n_dir, rnn.n_bias * rnn.dic);
    /* Array of pointers initialized in packing */
    AOC<float *, 3> bias(bias_, rnn.n_layer, rnn.n_dir, rnn.n_parts_bias);
    AOC<float, 3> scratch_bias(
            scratch_bias_, rnn.n_layer, rnn.n_dir, rnn.n_bias * rnn.dic);

    if (rnn.copy_bias) {
        parallel_nd(rnn.n_layer * rnn.n_dir * rnn.n_bias * rnn.dic,
                [&](size_t i) { scratch_bias_[i] = b_[i]; });
    }

    for (int i = 0; i < rnn.n_layer; i++) {
        for (int d = 0; d < rnn.n_dir; d++) {
            int offset_bias = 0;
            for (int p = 0; p < rnn.n_parts_bias; p++) {
                bias(i, d, p) = rnn.copy_bias
                        ? (float *)&scratch_bias(i, d, offset_bias)
                        : (float *)&b(i, d, offset_bias);
                offset_bias += rnn.parts_bias[p] * rnn.dic;
            }
        }
    }
}

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_bias_finalize_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::bias_finalize)) {
    if (rnn.is_int8()) {
        float data_shift = pd()->attr()->rnn_data_qparams_.shift_;
        float data_scale = pd()->attr()->rnn_data_qparams_.scale_;
        float *weights_scales = pd()->attr()->rnn_weights_qparams_.scales_;
        bool scale_per_oc = pd()->attr()->rnn_weights_qparams_.mask_ != 0;
        for (int i = 0; i < rnn.n_layer * rnn.n_dir; i++)
            for (int j = 0; j < rnn.n_bias * rnn.dic; j++) {
                size_t off = i * rnn.n_bias * rnn.dic + j;
                float weights_scale
                        = scale_per_oc ? weights_scales[j] : weights_scales[0];
                scratch_bias_[off] -= (w_iter_comp[off] + w_layer_comp[off])
                        * data_shift / (weights_scale * data_scale);
            }
    }
}

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_weights_assign_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::assign_packed_weights)) {
    assert(md->format_kind == format_kind::rnn_packed);
    const auto packed_desc = md->format_desc.rnn_packed_desc;
    AOC<weights_data_t *, 3> weights(
            weights_, rnn.n_layer, rnn.n_dir, packed_desc.n_parts);

    size_t offset_packed = 0;
    for (int l = 0; l < rnn.n_layer; l++)
        for (int d = 0; d < rnn.n_dir; d++) {
            for (int p = 0; p < packed_desc.n_parts; p++) {
                weights(l, d, p) = (weights_data_t *)&w_[offset_packed];
                offset_packed += packed_desc.part_pack_size[p]
                        / sizeof(weights_data_t);
            }
        }
}

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_weights_assign_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::assign_weights)) {
    assert(md->format_kind == format_kind::blocked);
    const auto &blk = md->format_desc.blocking;
    /* Original set of weights provided by the user */
    AOC<const weights_data_t, 3> w(
            w_, rnn.n_layer, rnn.n_dir, (int)blk.strides[1]);
    /* Array of pointers for each part of weights */
    AOC<weights_data_t *, 3> weights(weights_, rnn.n_layer, rnn.n_dir, n_parts);

    for (int i = 0; i < rnn.n_layer; i++)
        for (int d = 0; d < rnn.n_dir; d++) {
            size_t offset_weights = 0;
            for (int p = 0; p < n_parts; p++) {
                weights(i, d, p) = (weights_data_t *)&w(i, d, offset_weights);
                offset_weights += gates_per_part[p] * blk.strides[3];
            }
        }
}

//********************* Execution function *********************//
template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
void _ref_rnn_common_t<aprop, src_type, weights_type, acc_type>::execute_(
        const exec_ctx_t &ctx) const {
    const rnn_conf_t &rnn = this->pd()->rnn_;
    auto input = CTX_IN_MEM(const src_data_t *, DNNL_ARG_SRC_LAYER);
    auto states = CTX_IN_MEM(const char *, DNNL_ARG_SRC_ITER);
    auto c_states = CTX_IN_MEM(const float *, DNNL_ARG_SRC_ITER_C);
    auto layer_weights_n_comp
            = CTX_IN_MEM(const char *, DNNL_ARG_WEIGHTS_LAYER);
    auto iter_weights_n_comp = CTX_IN_MEM(const char *, DNNL_ARG_WEIGHTS_ITER);
    auto bias = CTX_IN_MEM(const float *, DNNL_ARG_BIAS);

    auto dst_last_layer = rnn.is_fwd
            ? CTX_OUT_MEM(char *, DNNL_ARG_DST_LAYER)
            : const_cast<char *>(CTX_IN_MEM(const char *, DNNL_ARG_DST_LAYER));
    auto dst_last_iter = rnn.is_fwd
            ? CTX_OUT_MEM(char *, DNNL_ARG_DST_ITER)
            : const_cast<char *>(CTX_IN_MEM(const char *, DNNL_ARG_DST_ITER));
    auto dst_last_iter_c = rnn.is_fwd
            ? CTX_OUT_MEM(float *, DNNL_ARG_DST_ITER_C)
            : const_cast<float *>(
                    CTX_IN_MEM(const float *, DNNL_ARG_DST_ITER_C));

    auto diff_dst_layer
            = CTX_IN_MEM(const acc_data_t *, DNNL_ARG_DIFF_DST_LAYER);
    auto diff_dst_iter = CTX_IN_MEM(const acc_data_t *, DNNL_ARG_DIFF_DST_ITER);
    auto diff_dst_iter_c = CTX_IN_MEM(const float *, DNNL_ARG_DIFF_DST_ITER_C);

    auto w_layer
            = reinterpret_cast<const weights_data_t *>(layer_weights_n_comp);
    auto w_iter = reinterpret_cast<const weights_data_t *>(iter_weights_n_comp);
    auto w_iter_comp = reinterpret_cast<const float *>(
            iter_weights_n_comp + rnn.weights_iter_comp_offset);
    auto w_layer_comp = reinterpret_cast<const float *>(
            layer_weights_n_comp + rnn.weights_layer_comp_offset);

    auto scratchpad = ctx.get_scratchpad_grantor();

    auto ptr_wei_layer
            = scratchpad.template get<weights_data_t *>(key_rnn_ptrs_wei_layer);
    auto ptr_wei_iter
            = scratchpad.template get<weights_data_t *>(key_rnn_ptrs_wei_iter);
    auto ptr_bias = scratchpad.template get<float *>(key_rnn_ptrs_bia);
    // Here we use scratch_gates for the output of GEMMs on FWD and on input of GEMMs for BWD.
    // None of the values are kept for bwd
    auto scratch_gates = scratchpad.template get<scratch_data_t>(key_rnn_gates);
    auto scratch_cell = scratchpad.template get<scratch_data_t>(key_rnn_cell);

    // Fetching buffers from the workspace
    // if no workspace was provided we use the scratchpad
    char *scratch_ptr = scratchpad.template get<char>(key_rnn_space);
    char *ws_ptr = nullptr;
    if (rnn.use_workspace)
        ws_ptr = rnn.is_fwd ? CTX_OUT_MEM(char *, DNNL_ARG_WORKSPACE)
                            : const_cast<char *>(CTX_IN_MEM(
                                    const char *, DNNL_ARG_WORKSPACE));

    char *base_ptr = rnn.use_workspace ? ws_ptr : scratch_ptr;
    // ws_gates is only used to pass data from FWD to BWD.
    // assumption: in training, src_data_t and weights_data_t match
    src_data_t *ws_gates = (src_data_t *)(base_ptr + ws_gates_offset_);
    src_data_t *ws_states = (src_data_t *)(base_ptr + ws_states_offset_);
    float *ws_c_states = (float *)(base_ptr + ws_c_states_offset_);
    acc_data_t *ws_diff_states
            = (acc_data_t *)(base_ptr + ws_diff_states_offset_);
    src_data_t *ws_grid = (src_data_t *)(base_ptr + ws_grid_comp_offset_);

    auto diff_src_layer = CTX_OUT_MEM(acc_data_t *, DNNL_ARG_DIFF_SRC_LAYER);
    auto diff_src_iter = CTX_OUT_MEM(acc_data_t *, DNNL_ARG_DIFF_SRC_ITER);
    auto diff_src_iter_c = CTX_OUT_MEM(float *, DNNL_ARG_DIFF_SRC_ITER_C);

    auto diff_weights_layer
            = CTX_OUT_MEM(acc_data_t *, DNNL_ARG_DIFF_WEIGHTS_LAYER);
    auto diff_weights_iter
            = CTX_OUT_MEM(acc_data_t *, DNNL_ARG_DIFF_WEIGHTS_ITER);
    auto diff_bias = CTX_OUT_MEM(acc_data_t *, DNNL_ARG_DIFF_BIAS);

    // Fetching extra buffers from scratchpad
    float *ws_bias = (float *)(scratch_ptr + ws_bias_offset_);

    // initialize diff_states to 0
    if (aprop == prop_kind::backward) {
        const size_t ws_sz = rnn.ws_diff_states_size / sizeof(float);
        parallel(0, [&](const int ithr, const int nthr) {
            size_t start = 0, end = 0;
            balance211(ws_sz, nthr, ithr, start, end);
            array_set(ws_diff_states + start, 0.f, end - start);
        });
    }

    /* Pack(if using packed gemm API) or copy(if input arrays have bad leading
     * dimension */
    (this->*bias_preparation_func)(rnn, ptr_bias, bias, ws_bias);

    (this->*weights_iter_assign_func)(rnn, pd()->weights_md(1),
            rnn.weights_iter_nld, rnn.weights_iter_ld, rnn.dic, rnn.sic,
            rnn.n_parts_weights_iter, rnn.parts_weights_iter,
            rnn.part_weights_iter_pack_size, ptr_wei_iter, w_iter, ptr_bias,
            bias, ws_bias);
    (this->*weights_layer_assign_func)(rnn, pd()->weights_md(0),
            rnn.weights_layer_nld, rnn.weights_layer_ld, rnn.dic, rnn.slc,
            rnn.n_parts_weights_layer, rnn.parts_weights_layer,
            rnn.part_weights_layer_pack_size, ptr_wei_layer, w_layer, ptr_bias,
            bias, ws_bias);

    (this->*bias_finalization_func)(rnn, ws_bias, w_iter_comp, w_layer_comp);

    // we first need to copy the initial states and input into ws
    copy_init_layer(rnn, ws_states, ws_diff_states, input, diff_dst_layer);
    if (pd()->src_md(1)->data_type == data_type::f32)
        copy_init_iter(rnn, ws_states, ws_c_states, ws_diff_states,
                (const float *)states, c_states, diff_dst_iter,
                diff_dst_iter_c);
    else
        copy_init_iter(rnn, ws_states, ws_c_states, ws_diff_states,
                (const src_data_t *)states, c_states, diff_dst_iter,
                diff_dst_iter_c);

    // run the execution on the grid
    (this->*grid_computation)(rnn, ptr_wei_layer, ptr_wei_iter, ptr_bias,
            ws_states, ws_c_states, ws_diff_states, ws_gates, ws_grid,
            scratch_gates, scratch_cell, diff_weights_layer, diff_weights_iter,
            diff_bias);

    // Finally we copy the results to the result buffers
    if (pd()->dst_md(0)->data_type == data_type::f32)
        copy_res_layer(rnn, (float *)dst_last_layer, diff_src_layer, ws_states,
                ws_diff_states);
    else
        copy_res_layer(rnn, (src_data_t *)dst_last_layer, diff_src_layer,
                ws_states, ws_diff_states);

    if (pd()->dst_md(1)->data_type == data_type::f32)
        copy_res_iter(rnn, (float *)dst_last_iter, dst_last_iter_c,
                diff_src_iter, diff_src_iter_c, ws_states, ws_c_states,
                ws_diff_states);
    else
        copy_res_iter(rnn, (src_data_t *)dst_last_iter, dst_last_iter_c,
                diff_src_iter, diff_src_iter_c, ws_states, ws_c_states,
                ws_diff_states);
};

/* Fix for MSVS warning C4661 */
template <>
rnn_cell_execution_sig(ref_rnn_fwd_f32_t::cell_execution);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_f32_t::cell_execution_gru);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_f32_t::cell_execution_gru_lbr);
template <>
rnn_cell_execution_sig(ref_rnn_bwd_f32_t::cell_execution);
template <>
rnn_cell_execution_sig(ref_rnn_bwd_f32_t::cell_execution_gru);
template <>
rnn_cell_execution_sig(ref_rnn_bwd_f32_t::cell_execution_gru_lbr);

template <>
rnn_cell_execution_sig(ref_rnn_fwd_bf16_t::cell_execution);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_bf16_t::cell_execution_gru);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_bf16_t::cell_execution_gru_lbr);
template <>
rnn_cell_execution_sig(ref_rnn_bwd_bf16_t::cell_execution);
template <>
rnn_cell_execution_sig(ref_rnn_bwd_bf16_t::cell_execution_gru);
template <>
rnn_cell_execution_sig(ref_rnn_bwd_bf16_t::cell_execution_gru_lbr);

template <>
rnn_cell_execution_sig(ref_rnn_fwd_u8s8_t::cell_execution);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_u8s8_t::cell_execution_gru);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_u8s8_t::cell_execution_gru_lbr);

template struct _ref_rnn_common_t<prop_kind::forward, data_type::f32,
        data_type::f32, data_type::f32>;
template struct _ref_rnn_common_t<prop_kind::backward, data_type::f32,
        data_type::f32, data_type::f32>;

template struct _ref_rnn_common_t<prop_kind::forward, data_type::bf16,
        data_type::bf16, data_type::f32>;
template struct _ref_rnn_common_t<prop_kind::backward, data_type::bf16,
        data_type::bf16, data_type::f32>;

template struct _ref_rnn_common_t<prop_kind::forward, data_type::u8,
        data_type::s8, data_type::s32>;

#undef AOC
} // namespace cpu
} // namespace impl
} // namespace dnnl

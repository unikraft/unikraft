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
 * Cell execution GRU with linear before reset
 */
#pragma warning(disable : 4503) /* name is too long */

#include "dnnl_thread.hpp"
#include "math_utils.hpp"

#include "ref_rnn.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

using namespace dnnl::impl::utils;
using namespace dnnl::impl::math;
using namespace rnn_utils;
#define AOC array_offset_calculator

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type,
        data_type_t acc_type>
rnn_cell_execution_sig((_ref_rnn_common_t<aprop, src_type, weights_type,
        acc_type>::cell_execution_gru_lbr)) {
    if (!rnn.merge_gemm_layer) {
        (this->*gemm_layer_func)('N', 'N', rnn.n_gates * rnn.dic, rnn.mb,
                rnn.slc, 1.0, w_layer_[0], rnn.weights_layer_ld, states_t_lm1_,
                rnn.states_ws_ld, 0.0, scratch_gates_, rnn.gates_ws_ld);
    }
    (this->*gemm_iter_func)('N', 'N', rnn.n_gates * rnn.dic, rnn.mb, rnn.sic,
            1.0, w_iter_[0], rnn.weights_iter_ld, states_tm1_l_,
            rnn.states_ws_ld, 0.0, scratch_cell_, rnn.gates_ws_ld);
    rnn_postgemm_->execute(rnn, ws_gates_, scratch_gates_, states_t_l_,
            c_states_t_l_, states_tm1_l_, c_states_tm1_l_, diff_states_t_l_,
            diff_states_t_lp1_, diff_states_tp1_l_, bias_[0], ws_grid_,
            scratch_cell_);
}

template rnn_cell_execution_sig(ref_rnn_fwd_f32_t::cell_execution_gru_lbr);
template rnn_cell_execution_sig(ref_rnn_fwd_bf16_t::cell_execution_gru_lbr);
template <>
rnn_cell_execution_sig(ref_rnn_fwd_u8s8_t::cell_execution_gru_lbr) {
    assert(!"GRU LBR int8 is not supported");
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
        typename weights_data_t, typename src_data_t, typename acc_data_t,
        typename scratch_data_t>
void common_bwd_cell_exec_template(T1 gemm_layer_f, T2 gemm_iter_f,
        T3 gemm_weights_layer_f, T4 gemm_weights_iter_f, T5 rnn_postgemm,
        const rnn_utils::rnn_conf_t &rnn, src_data_t *states_t_l_,
        acc_data_t *diff_states_t_l_, weights_data_t **w_layer_,
        weights_data_t **w_iter_, float **bias_, src_data_t *states_t_lm1_,
        src_data_t *states_tm1_l_, acc_data_t *diff_states_t_lp1_,
        acc_data_t *diff_states_tp1_l_, acc_data_t *diff_w_layer_,
        acc_data_t *diff_w_iter_, acc_data_t *diff_bias_, src_data_t *ws_gates_,
        src_data_t *ws_grid_, scratch_data_t *scratch_gates_,
        scratch_data_t *scratch_cell_) {
    ws_gates_aoc<scratch_data_t> scratch_gates_r(rnn, scratch_cell_);
    ws_diff_states_aoc<acc_data_t> diff_states_t_l(rnn, diff_states_t_l_);

    rnn_postgemm->execute(rnn, ws_gates_, scratch_gates_, states_t_l_, nullptr,
            states_tm1_l_, nullptr, diff_states_t_l_, diff_states_t_lp1_,
            diff_states_tp1_l_, bias_[0], ws_grid_, scratch_cell_);

    if (!rnn.merge_gemm_layer) {
        //  dx = dG * Wx^t
        gemm_layer_f(w_layer_[0], scratch_gates_,
                &diff_states_t_l(rnn.n_states, 0, 0));
        // dWx +=  dG^t * x
        gemm_weights_layer_f(scratch_gates_, states_t_lm1_, diff_w_layer_);
    }
    // dh +=  dGr * Wh^t
    gemm_iter_f(w_iter_[0], scratch_cell_, diff_states_t_l_);

    // dWh += dGr^t * h
    gemm_weights_iter_f(scratch_cell_, states_tm1_l_, diff_w_iter_);

    // db1-3 += e * dG
    // db4 += e * (r * dG2)
    gates_reduction(rnn, scratch_gates_, diff_bias_);

    parallel_nd(rnn.dic, [&](int j) {
        for (int i = 0; i < rnn.mb; i++) {
            diff_bias_[3 * rnn.dic + j] += scratch_gates_r(i, 2, j);
        }
    });
}

#undef AOC

template <>
rnn_cell_execution_sig(ref_rnn_bwd_f32_t::cell_execution_gru_lbr) {
    auto gemm_layer = [&](float *A, float *B, float *C) {
        (this->*gemm_layer_func)('N', 'N', rnn.slc, rnn.mb,
                rnn.n_gates * rnn.dic, 1.0f, A, rnn.weights_layer_ld, B,
                rnn.gates_ws_ld, 0.0f, C, rnn.states_ws_ld);
    };
    auto gemm_iter = [&](float *A, float *B, float *C) {
        (this->*gemm_iter_func)('N', 'N', rnn.sic, rnn.mb,
                rnn.n_gates * rnn.dic, 1.0f, A, rnn.weights_iter_ld, B,
                rnn.gates_ws_ld, 1.0f, C, rnn.states_ws_ld);
    };
    auto gemm_weights_layer = [&](float *A, float *B, float *C) {
        gemm('N', 'T', rnn.n_gates * rnn.dic, rnn.slc, rnn.mb, 1.0f, A,
                rnn.gates_ws_ld, B, rnn.states_ws_ld, 1.0f, C,
                rnn.diff_weights_layer_ld);
    };
    auto gemm_weights_iter = [&](float *A, float *B, float *C) {
        gemm('N', 'T', rnn.n_gates * rnn.dic, rnn.sic, rnn.mb, 1.0f, A,
                rnn.gates_ws_ld, B, rnn.states_ws_ld, 1.0f, C,
                rnn.diff_weights_iter_ld);
    };

    common_bwd_cell_exec_template(gemm_layer, gemm_iter, gemm_weights_layer,
            gemm_weights_iter, rnn_postgemm_, rnn, states_t_l_,
            diff_states_t_l_, w_layer_, w_iter_, bias_, states_t_lm1_,
            states_tm1_l_, diff_states_t_lp1_, diff_states_tp1_l_,
            diff_w_layer_, diff_w_iter_, diff_bias_, ws_gates_, ws_grid_,
            scratch_gates_, scratch_cell_);
}

template <>
rnn_cell_execution_sig(ref_rnn_bwd_bf16_t::cell_execution_gru_lbr) {
    auto gemm_layer = [&](bfloat16_t *A, bfloat16_t *B, float *C) {
        (this->*gemm_layer_func)('N', 'N', rnn.slc, rnn.mb,
                rnn.n_gates * rnn.dic, 1.0f, A, rnn.weights_layer_ld, B,
                rnn.gates_ws_ld, 0.0f, C, rnn.states_ws_ld);
    };
    auto gemm_iter = [&](bfloat16_t *A, bfloat16_t *B, float *C) {
        (this->*gemm_iter_func)('N', 'N', rnn.sic, rnn.mb,
                rnn.n_gates * rnn.dic, 1.0f, A, rnn.weights_iter_ld, B,
                rnn.gates_ws_ld, 1.0f, C, rnn.states_ws_ld);
    };
    auto gemm_weights_layer = [&](bfloat16_t *A, bfloat16_t *B, float *C) {
        gemm('N', 'T', rnn.n_gates * rnn.dic, rnn.slc, rnn.mb, 1.0f, A,
                rnn.gates_ws_ld, B, rnn.states_ws_ld, 1.0f, C,
                rnn.diff_weights_layer_ld);
    };
    auto gemm_weights_iter = [&](bfloat16_t *A, bfloat16_t *B, float *C) {
        gemm('N', 'T', rnn.n_gates * rnn.dic, rnn.sic, rnn.mb, 1.0f, A,
                rnn.gates_ws_ld, B, rnn.states_ws_ld, 1.0f, C,
                rnn.diff_weights_iter_ld);
    };

    common_bwd_cell_exec_template(gemm_layer, gemm_iter, gemm_weights_layer,
            gemm_weights_iter, rnn_postgemm_, rnn, states_t_l_,
            diff_states_t_l_, w_layer_, w_iter_, bias_, states_t_lm1_,
            states_tm1_l_, diff_states_t_lp1_, diff_states_tp1_l_,
            diff_w_layer_, diff_w_iter_, diff_bias_, ws_gates_, ws_grid_,
            scratch_gates_, scratch_cell_);
}

} // namespace cpu
} // namespace impl
} // namespace dnnl

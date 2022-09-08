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

#ifndef OCL_RNN_UTILS_HPP
#define OCL_RNN_UTILS_HPP

#define OFF6(i0, d0, i1, d1, i2, d2, i3, d3, i4, d4, i5, d5) \
    ((((((i0) * (d1) + (i1)) * (d2) + (i2)) * (d3) + (i3)) * (d4) + (i4)) \
                    * (d5) \
            + (i5))
#define OFF5(i0, d0, i1, d1, i2, d2, i3, d3, i4, d4) \
    (((((i0) * (d1) + (i1)) * (d2) + (i2)) * (d3) + (i3)) * (d4) + (i4))
#define OFF4(i0, d0, i1, d1, i2, d2, i3, d3) \
    ((((i0) * (d1) + (i1)) * (d2) + (i2)) * (d3) + (i3))
#define OFF3(i0, d0, i1, d1, i2, d2) (((i0) * (d1) + (i1)) * (d2) + (i2))
#define OFF2(i0, d0, i1, d1) ((i0) * (d1) + (i1))

#define elemwise_sig(f) \
    void f(const exec_ctx_t &ctx, int dir, int lay, int iter, int dic, \
            int wic, int batch, const memory_storage_t &workspace, \
            const memory_storage_t &scales, const memory_storage_t &bias, \
            const memory_storage_t &tm_scales) const

#define cell_execution_sig(f) \
    void f(const exec_ctx_t &ctx, int dir, int lay, int iter, int dic, \
            int slc, int sic, int wic, int batch, int n_layer, int n_dir, \
            int n_iter, int n_gates, int n_states, int n_bias, \
            size_t *weights_input, int n_parts_weights_layer, \
            size_t *weights_states, int n_parts_weights_iter, \
            const memory_storage_t &bias, const memory_storage_t &workspace, \
            const memory_storage_t &w_input, const memory_storage_t &w_state, \
            const memory_storage_t &diff_weights_layer, \
            const memory_storage_t &diff_weights_iter, \
            const memory_storage_t &diff_bias, const memory_storage_t &scales, \
            const memory_storage_t &tm_scales) const

#define grid_execution_sig(f) \
    void f(const exec_ctx_t &ctx, int dic, int slc, int sic, int wic, \
            int batch, int n_layer, int n_dir, int n_iter, int n_gates, \
            int n_states, int n_bias, size_t *weights_input, \
            int n_parts_weights_layer, size_t *weights_states, \
            int n_parts_weights_iter, const memory_storage_t &bias, \
            const memory_storage_t &workspace, \
            const memory_storage_t &w_input, const memory_storage_t &w_state, \
            const memory_storage_t &diff_weights_layer, \
            const memory_storage_t &diff_weights_iter, \
            const memory_storage_t &diff_bias, const memory_storage_t &scales, \
            const memory_storage_t &tm_scales) const

#define gemm_sig(f) \
    void f(const exec_ctx_t &ctx, int m, int n, int k, int strideA_m, \
            int strideA_k, int strideB_n, int strideB_k, int strideC_m, \
            int strideC_n, const memory_storage_t &a, size_t off_a, \
            const memory_storage_t &b, size_t off_b, \
            const memory_storage_t &c, size_t off_c, bool is_B_trans, \
            float beta, gemm_kind_t gemm_kind) const

#define packing_sig(f) \
    void f(int n_layer, int n_dir, int n_weights, int n_gates, int batch, \
            int OC_size, int IC_size, size_t *weights_, int n_parts, \
            const int *gates_per_part, const memory_storage_t &w_) const

#define free_packed_sig(f) \
    void f(int n_layer, int n_dir, int n_parts, size_t *weights_)

namespace dnnl {
namespace impl {
namespace ocl {

namespace rnn_utils {

enum execution_direction_t {
    l2r,
    r2l,
    bi_concat,
    bi_sum,
};

enum data_type_conf_t {
    all_f32,
    all_f16,
    u8u8u8f32,
    f32u8f32f32,
    u8u8u8u8,
    f32u8f32u8
};

enum ws_part_t { gates, states, c_states, diff_states, cell, grid, bias };

struct rnn_conf_t {
    execution_direction_t exec_dir;
    data_type_conf_t dt_conf;
    int n_layer, n_iter, n_dir, n_gates, n_states;
    int mb;
    int slc, sic, dic, dlc;

    int gates_ld, gates_nld, gates_ws_ld;

    int n_parts_weights_layer, parts_weights_layer[DNNL_RNN_MAX_N_PARTS];
    int n_parts_weights_iter, parts_weights_iter[DNNL_RNN_MAX_N_PARTS];
    int n_bias, n_parts_bias, parts_bias[DNNL_RNN_MAX_N_PARTS];

    size_t part_weights_iter_pack_size[DNNL_RNN_MAX_N_PARTS],
            part_weights_layer_pack_size[DNNL_RNN_MAX_N_PARTS];

    // Size of packed data in bytes
    size_t weights_layer_comp_offset, weights_layer_pack_size,
            weights_iter_comp_offset, weights_iter_pack_size;

    bool copy_bias;
    int weights_layer_ld, weights_layer_nld;
    int diff_weights_layer_ld, diff_weights_layer_nld;
    int weights_iter_ld, weights_iter_nld;
    int diff_weights_iter_ld, diff_weights_iter_nld;
    int states_nld, states_ws_ld;
    int weights_iter_compensation_size, weights_layer_compensation_size;
    bool is_fwd, is_training, is_lbr, is_int8, is_testmode;
    bool use_workspace;

    // for test mode (--skip_nonliner=true of benchdnn)
    float tm_cscale;
    int tm_ngates;

    // Size of workspace for each tensor in bytes
    size_t ws_gates_size, ws_states_size, ws_c_states_size, ws_diff_states_size,
            ws_cell_comp_size, ws_grid_comp_size, ws_per_cell, ws_bias_size;

    bool merge_gemm_iter, merge_gemm_layer, use_jit_gemm, use_layer_packed_gemm,
            use_iter_packed_gemm;

    // Element size of each workspace part in bytes
    int ws_gates_elsz, ws_states_elsz, ws_c_states_elsz, ws_diff_states_elsz,
            ws_cell_comp_elsz, ws_grid_comp_elsz, ws_bias_elsz;

    data_type_t acc_data_type;
    int acc_data_type_elsz;
    data_type_t precise_data_type;
    data_type_t input_data_type;
    data_type_t output_data_type;
    data_type_t dst_data_type;
};

bool is_ldigo(const memory_desc_wrapper &md);
bool is_ldgoi(const memory_desc_wrapper &md);

int get_good_ld(int dim, int sizeof_dt);
void init_rnn_conf(rnn_conf_t &rnn, const rnn_desc_t &rd,
        const memory_desc_wrapper &src_layer_d,
        const memory_desc_wrapper &src_iter_d,
        const memory_desc_wrapper &weights_layer_d,
        const memory_desc_wrapper &weights_iter_d,
        const memory_desc_wrapper &dst_layer_d);
void init_test_mode(rnn_conf_t &rnn, const primitive_attr_t &attr);
void set_rnn_conf(rnn_conf_t &rnn, const rnn_desc_t &rd,
        const memory_desc_wrapper &weights_layer_d,
        const memory_desc_wrapper &weights_iter_d,
        const memory_desc_wrapper &diff_weights_layer_d,
        const memory_desc_wrapper &diff_weights_iter_d);
void set_offsets(const rnn_conf_t &rnn, size_t &ws_gates_offset,
        size_t &ws_h_state_offset, size_t &ws_c_state_offset,
        size_t &ws_diff_states_offset, size_t &ws_grid_comp_offset,
        size_t &ws_cell_comp_offset, size_t &ws_bias_offset,
        size_t &scratchpad_size, size_t &workspace_size);
void get_scratchpad_and_workspace_sizes(
        const rnn_conf_t &rnn, size_t &scratchpad_size, size_t &workspace_size);
status_t set_expected_desc(
        rnn_conf_t &rnn, memory_desc_t &weights_md, bool is_iter);
status_t set_good_strides(memory_desc_t &weights_md, format_tag_t tag);
} // namespace rnn_utils

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif

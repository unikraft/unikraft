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
/*
 * Common for RNN and LSTM cell execution
 */
#include "ref_rnn.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type>
cell_execution_sig((_ref_rnn_common_t<aprop, src_type,
        weights_type>::cell_execution_gru_lbr)) {
    assert(!"unimplemented");
}
template cell_execution_sig(ref_rnn_fwd_u8s8_t::cell_execution_gru_lbr);
template cell_execution_sig(ref_rnn_fwd_f16_t::cell_execution_gru_lbr);
template cell_execution_sig(ref_rnn_fwd_f32_t::cell_execution_gru_lbr);
template cell_execution_sig(ref_rnn_bwd_f32_t::cell_execution_gru_lbr);

template <prop_kind_t aprop, data_type_t src_type, data_type_t weights_type>
cell_execution_sig((
        _ref_rnn_common_t<aprop, src_type, weights_type>::cell_execution_gru)) {
    assert(!"unimplemented");
}
template cell_execution_sig(ref_rnn_fwd_u8s8_t::cell_execution_gru);
template cell_execution_sig(ref_rnn_fwd_f16_t::cell_execution_gru);
template cell_execution_sig(ref_rnn_fwd_f32_t::cell_execution_gru);
template cell_execution_sig(ref_rnn_bwd_f32_t::cell_execution_gru);

} // namespace ocl
} // namespace impl
} // namespace dnnl

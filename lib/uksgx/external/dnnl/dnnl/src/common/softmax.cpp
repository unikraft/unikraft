/*******************************************************************************
* Copyright 2016-2018 Intel Corporation
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

#include <assert.h>
#include "dnnl.h"

#include "c_types_map.hpp"
#include "memory_desc_wrapper.hpp"
#include "utils.hpp"

using namespace dnnl::impl;
using namespace dnnl::impl::utils;
using namespace dnnl::impl::status;
using namespace dnnl::impl::prop_kind;
using namespace dnnl::impl::alg_kind;
using namespace dnnl::impl::types;

namespace {
status_t softmax_desc_init(softmax_desc_t *softmax_desc, prop_kind_t prop_kind,
        const memory_desc_t *data_desc, const memory_desc_t *diff_desc,
        int softmax_axis) {
    bool args_ok = true && !any_null(softmax_desc, data_desc)
            && IMPLICATION(prop_kind == backward_data, diff_desc != nullptr)
            && 0 <= softmax_axis && softmax_axis < data_desc->ndims;
    if (!args_ok) return invalid_arguments;

    auto sd = softmax_desc_t();
    sd.primitive_kind = primitive_kind::softmax;
    sd.prop_kind = prop_kind;

    sd.data_desc = *data_desc;
    if (sd.prop_kind == backward_data) sd.diff_desc = *diff_desc;
    sd.softmax_axis = softmax_axis;

    *softmax_desc = sd;
    return success;
}
} // namespace

status_t dnnl_softmax_forward_desc_init(softmax_desc_t *softmax_desc,
        prop_kind_t prop_kind, const memory_desc_t *data_desc,
        int softmax_axis) {
    if (!one_of(prop_kind, forward_inference, forward_training))
        return invalid_arguments;
    return softmax_desc_init(
            softmax_desc, prop_kind, data_desc, nullptr, softmax_axis);
}

status_t dnnl_softmax_backward_desc_init(softmax_desc_t *softmax_desc,
        const memory_desc_t *diff_desc, const memory_desc_t *data_desc,
        int softmax_axis) {
    return softmax_desc_init(softmax_desc, prop_kind::backward_data, data_desc,
            diff_desc, softmax_axis);
}
// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

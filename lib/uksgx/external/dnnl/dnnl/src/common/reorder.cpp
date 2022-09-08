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
#include "engine.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

#include "reorder_pd.hpp"

using namespace dnnl::impl;
using namespace dnnl::impl::utils;
using namespace dnnl::impl::status;

static engine_t *get_reorder_engine(
        engine_t *src_engine, engine_t *dst_engine) {
    auto s_ek = src_engine->kind();
    auto d_ek = dst_engine->kind();
    auto s_rk = src_engine->runtime_kind();
    auto d_rk = dst_engine->runtime_kind();

    if (is_native_runtime(d_rk)) return src_engine;

    if (is_native_runtime(s_rk)) return dst_engine;

    if (d_ek == engine_kind::cpu) return src_engine;

    if (s_ek == engine_kind::cpu) return dst_engine;

    assert(s_ek == engine_kind::gpu);
    assert(d_ek == engine_kind::gpu);
    return src_engine;
}

status_t dnnl_reorder_primitive_desc_create(primitive_desc_t **reorder_pd,
        const memory_desc_t *src_md, engine_t *src_engine,
        const memory_desc_t *dst_md, engine_t *dst_engine,
        const primitive_attr_t *attr) {
    if (any_null(reorder_pd, src_engine, src_md, dst_engine, dst_md))
        return invalid_arguments;

    auto s_ek = src_engine->kind();
    auto d_ek = dst_engine->kind();
    if (!IMPLICATION(s_ek != d_ek, one_of(engine_kind::cpu, s_ek, d_ek)))
        return invalid_arguments;

    auto r_pd = reinterpret_cast<reorder_pd_t **>(reorder_pd);
    auto s_mdw = memory_desc_wrapper(*src_md);
    auto d_mdw = memory_desc_wrapper(*dst_md);

    if (!s_mdw.consistent_with(d_mdw)) return invalid_arguments;

    const primitive_attr_t dummy_attr;
    if (attr == NULL) attr = &dummy_attr;

    auto e = get_reorder_engine(src_engine, dst_engine);
    for (auto r = e->get_reorder_implementation_list(); *r; ++r) {
        if ((*r)(r_pd, e, attr, src_engine, src_md, dst_engine, dst_md)
                == success) {
            return success;
        }
    }
    return unimplemented;
}

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

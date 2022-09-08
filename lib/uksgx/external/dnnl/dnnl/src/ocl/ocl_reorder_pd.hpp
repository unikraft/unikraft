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

#ifndef OCL_REORDER_PD_HPP
#define OCL_REORDER_PD_HPP

#include "common/reorder_pd.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

struct ocl_reorder_pd_t : public reorder_pd_t {
    using reorder_pd_t::reorder_pd_t;
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#define DECLARE_OCL_REORDER_CREATE() \
    static status_t create(reorder_pd_t **reorder_pd, engine_t *engine, \
            const primitive_attr_t *attr, engine_t *src_engine, \
            const memory_desc_t *src_md, engine_t *dst_engine, \
            const memory_desc_t *dst_md) { \
        auto _pd = new pd_t( \
                engine, attr, src_engine, src_md, dst_engine, dst_md); \
        if (_pd == nullptr) return status::out_of_memory; \
        if (_pd->init() != status::success) { \
            delete _pd; \
            return status::unimplemented; \
        } \
        _pd->init_info(); \
        _pd->init_scratchpad_md(); \
        return safe_ptr_assign<reorder_pd_t>(*reorder_pd, _pd); \
    }

#endif

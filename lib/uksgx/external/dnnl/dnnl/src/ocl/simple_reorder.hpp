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

#ifndef OCL_SIMPLE_REORDER_HPP
#define OCL_SIMPLE_REORDER_HPP

#include "common/c_types_map.hpp"
#include "common/memory.hpp"
#include "common/utils.hpp"
#include "ocl/jit_simple_reorder_kernel.hpp"
#include "ocl/ocl_reorder_pd.hpp"
#include "ocl/ocl_utils.hpp"

extern const char *simple_reorder_kernel;

namespace dnnl {
namespace impl {
namespace ocl {

struct simple_reorder_t : public primitive_impl_t {
    struct pd_t : public ocl_reorder_pd_t {
        using ocl_reorder_pd_t::ocl_reorder_pd_t;

        DECLARE_COMMON_PD_T("ocl:simple:any", simple_reorder_t);

        DECLARE_OCL_REORDER_CREATE();

        status_t init() {
            const auto &post_ops = attr()->post_ops_;

            bool ok = true && (src_engine() == dst_engine())
                    && (src_engine()->kind() == engine_kind::gpu)
                    && utils::one_of(src_md()->data_type, data_type::u8,
                            data_type::s8, data_type::f16, data_type::s32,
                            data_type::f32, data_type::bf16)
                    && utils::one_of(dst_md()->data_type, data_type::u8,
                            data_type::s8, data_type::f16, data_type::s32,
                            data_type::f32, data_type::bf16)
                    && IMPLICATION(
                            utils::one_of(data_type::f16, src_md()->data_type,
                                    dst_md()->data_type),
                            utils::downcast<compute::compute_engine_t *>(
                                    src_engine())
                                    ->mayiuse(compute::device_ext_t::khr_fp16))
                    && (attr()->has_default_values()
                            || IMPLICATION(post_ops.len_ != 0,
                                    post_ops.len_ == 1
                                            && post_ops.entry_[0].kind
                                                    == primitive_kind::sum));

            if (!ok) return status::unimplemented;

            auto *compute_engine = utils::downcast<compute::compute_engine_t *>(
                    dst_engine()->kind() == engine_kind::gpu ? dst_engine()
                                                             : src_engine());

            ok = ok
                    && compute_engine->mayiuse(
                            compute::device_ext_t::intel_subgroups)
                    && IMPLICATION(
                            utils::one_of(data_type::f16, src_md()->data_type,
                                    dst_md()->data_type),
                            true
                                    && compute_engine->mayiuse(
                                            compute::device_ext_t::khr_fp16)
                                    && compute_engine->mayiuse(
                                            compute::device_ext_t::
                                                    intel_subgroups_short));

            if (!ok) return status::unimplemented;

            return jit_simple_reorder_kernel::init_conf(
                    this, jrp_, src_md(), dst_md());
        }

        jit_reorder_conf_t jrp_;
    };

    virtual status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;

        auto status = jit_simple_reorder_kernel::init_const_def(
                kernel_ctx, pd()->jrp_, pd()->src_md(), pd()->dst_md());
        if (status != status::success) return status;

        compute_engine->create_kernel(&kernel_, "simple_reorder", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        if (pd()->jrp_.scale_quant) {
            size_t size = pd()->attr()->output_scales_.count_ * sizeof(float);
            memory_storage_t *scales_ptr;
            engine()->create_memory_storage(&scales_ptr, size);
            scales.reset(scales_ptr);
            if (!scales) return status::runtime_error;
        }

        return status::success;
    }

    simple_reorder_t(const pd_t *apd) : primitive_impl_t(apd) {}

    virtual status_t execute(const exec_ctx_t &ctx) const override;

private:
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    compute::kernel_t kernel_;
    std::unique_ptr<memory_storage_t> scales;
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif

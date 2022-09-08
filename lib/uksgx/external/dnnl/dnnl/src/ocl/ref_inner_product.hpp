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

#ifndef OCL_REF_INNER_PRODUCT_HPP
#define OCL_REF_INNER_PRODUCT_HPP

#include <assert.h>

#include "common/c_types_map.hpp"
#include "compute/compute.hpp"
#include "ocl/jit_ref_inner_product_common_kernel.hpp"
#include "ocl/ocl_inner_product_pd.hpp"
#include "ocl/ocl_stream.hpp"
#include "ocl/ocl_utils.hpp"

extern const char *ref_inner_product_kernel;

namespace dnnl {
namespace impl {
namespace ocl {

struct ref_inner_product_fwd_t : public primitive_impl_t {
    struct pd_t : public ocl_inner_product_fwd_pd_t {
        pd_t(engine_t *engine, const inner_product_desc_t *adesc,
                const primitive_attr_t *attr,
                const inner_product_fwd_pd_t *hint_fwd_pd)
            : ocl_inner_product_fwd_pd_t(engine, adesc, attr, hint_fwd_pd)
            , jit_off_() {}

        DECLARE_COMMON_PD_T("ocl:ref:any", ref_inner_product_fwd_t);

        status_t init() {
            using namespace data_type;
            using namespace prop_kind;
            using namespace data_type;
            assert(engine()->kind() == engine_kind::gpu);
            auto *compute_engine
                    = utils::downcast<compute::compute_engine_t *>(engine());

            bool ok = true
                    && utils::one_of(desc()->prop_kind, forward_training,
                            forward_inference)
                    && set_default_params() == status::success
                    && utils::one_of(true,
                            expect_data_types(
                                    u8, s8, data_type::undef, s8, s32),
                            expect_data_types(
                                    u8, s8, data_type::undef, u8, s32),
                            expect_data_types(
                                    u8, s8, data_type::undef, s32, s32),
                            expect_data_types(
                                    s8, s8, data_type::undef, s8, s32),
                            expect_data_types(
                                    s8, s8, data_type::undef, u8, s32),
                            expect_data_types(
                                    s8, s8, data_type::undef, s32, s32),
                            expect_data_types(
                                    bf16, bf16, data_type::undef, bf16, f32),
                            expect_data_types(
                                    bf16, bf16, data_type::undef, f32, f32),
                            expect_data_types(f32, f32, f32, f32, f32),
                            expect_data_types(f16, f16, f16, f16, f16))
                    && IMPLICATION(with_bias(),
                            utils::one_of(desc()->bias_desc.data_type, u8, s8,
                                    bf16, f16, f32))
                    && attr()->output_scales_.count_ == 1
                    && dense_consitency_check(src_md(), weights_md(), dst_md())
                    && IMPLICATION(desc()->src_desc.data_type == f16,
                            compute_engine->mayiuse(
                                    compute::device_ext_t::khr_fp16));
            if (!ok) return status::unimplemented;

            return jit_ref_inner_product_fwd_kernel::init_conf(jip_, desc_,
                    src_md(), weights_md(), dst_md(), *this->attr(), jit_off_,
                    desc()->accum_data_type);
        }
        bool with_eltwise() const {
            return attr()->post_ops_.find(primitive_kind::eltwise) != -1;
        }

        bool with_sum() const {
            return attr()->post_ops_.find(primitive_kind::sum) != -1;
        }

        float eltwise_alpha() const {
            const int eltwise_idx
                    = attr()->post_ops_.find(primitive_kind::eltwise);
            return with_eltwise()
                    ? attr()->post_ops_.entry_[eltwise_idx].eltwise.alpha
                    : 1.0f;
        }

        float eltwise_beta() const {
            const int eltwise_idx
                    = attr()->post_ops_.find(primitive_kind::eltwise);
            return with_eltwise()
                    ? attr()->post_ops_.entry_[eltwise_idx].eltwise.beta
                    : 0.0f;
        }

        float sum_scale() const {
            const int sum_idx = attr()->post_ops_.find(primitive_kind::sum);
            return with_sum() ? attr()->post_ops_.entry_[sum_idx].sum.scale
                              : 1.0f;
        }

        alg_kind_t eltwise_alg_kind() const {
            const int eltwise_idx
                    = attr()->post_ops_.find(primitive_kind::eltwise);
            return with_eltwise()
                    ? attr()->post_ops_.entry_[eltwise_idx].eltwise.alg
                    : dnnl_alg_kind_undef;
        }

        jit_inner_product_conf_t jip_;
        jit_offsets jit_off_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;
        jit_ref_inner_product_fwd_kernel::init_const_def(kernel_ctx, pd()->jip_,
                pd()->jit_off_, pd()->with_eltwise(), pd()->with_sum(),
                pd()->eltwise_alg_kind());

        compute_engine->create_kernel(
                &kernel_, "ref_inner_product_fwd_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    ref_inner_product_fwd_t(const pd_t *apd) : primitive_impl_t(apd) {
        ker_ = new jit_ref_inner_product_fwd_kernel(pd()->jip_);
    }
    ~ref_inner_product_fwd_t() { delete ker_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_forward(ctx);
    }

private:
    status_t execute_forward(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_ref_inner_product_fwd_kernel *ker_;
    compute::kernel_t kernel_;
};

struct ref_inner_product_bwd_data_t : public primitive_impl_t {
    struct pd_t : public ocl_inner_product_bwd_data_pd_t {
        pd_t(engine_t *engine, const inner_product_desc_t *adesc,
                const primitive_attr_t *attr,
                const inner_product_fwd_pd_t *hint_fwd_pd)
            : ocl_inner_product_bwd_data_pd_t(engine, adesc, attr, hint_fwd_pd)
            , jit_off_() {}

        DECLARE_COMMON_PD_T("ref:any", ref_inner_product_bwd_data_t);

        status_t init() {
            using namespace data_type;
            using namespace prop_kind;
            assert(engine()->kind() == engine_kind::gpu);

            bool ok = true
                    && utils::one_of(
                            this->desc()->prop_kind, backward, backward_data)
                    && this->set_default_params() == status::success
                    && utils::one_of(true,
                            expect_data_types(
                                    bf16, bf16, data_type::undef, bf16, f32),
                            expect_data_types(
                                    f32, bf16, data_type::undef, bf16, f32),
                            expect_data_types(
                                    f32, f32, data_type::undef, f32, f32))
                    && attr()->has_default_values();
            if (!ok) return status::unimplemented;

            return jit_ref_inner_product_fwd_kernel::init_conf(jip_, desc_,
                    diff_src_md(), weights_md(), diff_dst_md(), *this->attr(),
                    jit_off_, desc()->accum_data_type);
        }
        jit_inner_product_conf_t jip_;
        jit_offsets jit_off_;
    };

    virtual status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;
        jit_ref_inner_product_fwd_kernel::init_const_def(kernel_ctx, pd()->jip_,
                pd()->jit_off_, false, false, dnnl_alg_kind_undef);

        compute_engine->create_kernel(
                &kernel_, "ref_inner_product_bwd_data_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    ref_inner_product_bwd_data_t(const pd_t *apd) : primitive_impl_t(apd) {
        ker_ = new jit_ref_inner_product_fwd_kernel(pd()->jip_);
    }
    ~ref_inner_product_bwd_data_t() { delete ker_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_backward_data(ctx);
    }

private:
    status_t execute_backward_data(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_ref_inner_product_fwd_kernel *ker_;
    compute::kernel_t kernel_;
};

struct ref_inner_product_bwd_weights_t : public primitive_impl_t {
    struct pd_t : public ocl_inner_product_bwd_weights_pd_t {
        pd_t(engine_t *engine, const inner_product_desc_t *adesc,
                const primitive_attr_t *attr,
                const inner_product_fwd_pd_t *hint_fwd_pd)
            : ocl_inner_product_bwd_weights_pd_t(
                    engine, adesc, attr, hint_fwd_pd)
            , jit_off_() {}

        DECLARE_COMMON_PD_T("ref:any", ref_inner_product_bwd_weights_t);

        status_t init() {
            using namespace data_type;
            using namespace prop_kind;
            assert(engine()->kind() == engine_kind::gpu);
            bool ok = true
                    && utils::one_of(
                            this->desc()->prop_kind, backward, backward_weights)
                    && this->set_default_params() == status::success
                    && utils::one_of(true,
                            expect_data_types(bf16, bf16, bf16, bf16, f32),
                            expect_data_types(bf16, f32, f32, bf16, f32),
                            expect_data_types(f32, f32, f32, f32, f32))
                    && attr()->has_default_values();
            if (!ok) return status::unimplemented;

            return jit_ref_inner_product_fwd_kernel::init_conf(jip_, desc_,
                    src_md(), diff_weights_md(), diff_dst_md(), *this->attr(),
                    jit_off_, desc()->accum_data_type);
        }
        jit_inner_product_conf_t jip_;
        jit_offsets jit_off_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;
        jit_ref_inner_product_fwd_kernel::init_const_def(kernel_ctx, pd()->jip_,
                pd()->jit_off_, false, false, dnnl_alg_kind_undef);

        compute_engine->create_kernel(
                &kernel_, "ref_inner_product_bwd_weights_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    ref_inner_product_bwd_weights_t(const pd_t *apd) : primitive_impl_t(apd) {
        ker_ = new jit_ref_inner_product_fwd_kernel(pd()->jip_);
    }
    ~ref_inner_product_bwd_weights_t() { delete ker_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_backward_weights(ctx);
    }

private:
    status_t execute_backward_weights(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_ref_inner_product_fwd_kernel *ker_;
    compute::kernel_t kernel_;
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

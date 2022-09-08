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

#ifndef OCL_REF_CONVOLUTION_HPP
#define OCL_REF_CONVOLUTION_HPP

#include "common/c_types_map.hpp"

#include "compute/compute.hpp"
#include "ocl/ocl_convolution_pd.hpp"
#include "ocl/ocl_stream.hpp"
#include "ocl/ref_convolution_kernel.hpp"

extern const char *ref_convolution_kernel;

namespace dnnl {
namespace impl {
namespace ocl {

struct ref_convolution_fwd_t : public primitive_impl_t {
    struct pd_t : public ocl_convolution_fwd_pd_t {
        using ocl_convolution_fwd_pd_t::ocl_convolution_fwd_pd_t;

        DECLARE_COMMON_PD_T("ocl:ref:any", ref_convolution_fwd_t);

        status_t init() {
            const auto *compute_engine
                    = utils::downcast<compute::compute_engine_t *>(engine());
            bool ok = set_default_alg_kind(alg_kind::convolution_direct)
                    && utils::one_of(desc()->prop_kind,
                            prop_kind::forward_training,
                            prop_kind::forward_inference)
                    && desc()->alg_kind == alg_kind::convolution_direct
                    && IMPLICATION(
                            utils::one_of(data_type::f16, src_md_.data_type,
                                    weights_md_.data_type, dst_md_.data_type),
                            compute_engine->mayiuse(
                                    compute::device_ext_t::khr_fp16))
                    && this->set_default_formats();
            if (!ok) return status::unimplemented;

            return kernel_.init(*this->desc(), *this->src_md(),
                    *this->weights_md(), *this->weights_md(1), *this->dst_md(),
                    *this->attr());
        }
        bool with_eltwise(int position) const {
            return attr()->post_ops_.contain(primitive_kind::eltwise, position);
        }

        bool with_sum() const {
            return attr()->post_ops_.find(primitive_kind::sum) != -1;
        }

        float eltwise_alpha() const {
            const int eltwise_idx
                    = attr()->post_ops_.find(primitive_kind::eltwise);
            return with_eltwise(0) || with_eltwise(1)
                    ? attr()->post_ops_.entry_[eltwise_idx].eltwise.alpha
                    : 1.0f;
        }

        float eltwise_beta() const {
            const int eltwise_idx
                    = attr()->post_ops_.find(primitive_kind::eltwise);
            return with_eltwise(0) || with_eltwise(1)
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
            return with_eltwise(0) || with_eltwise(1)
                    ? attr()->post_ops_.entry_[eltwise_idx].eltwise.alg
                    : dnnl_alg_kind_undef;
        }

        const ref_convolution_kernel_t *kernel() const { return &kernel_; }

    private:
        bool set_default_formats() {
            using namespace format_tag;
            auto dat_tag = utils::pick(ndims() - 3, ncw, nchw, ncdhw);
            auto wei_tag = with_groups()
                    ? utils::pick(ndims() - 3, goiw, goihw, goidhw)
                    : utils::pick(ndims() - 3, oiw, oihw, oidhw);
            return set_default_formats_common(dat_tag, wei_tag, dat_tag);
        }

        ref_convolution_kernel_t kernel_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;

        auto status = pd()->kernel()->apply_const(kernel_ctx);
        if (status != status::success) return status;

        compute_engine->create_kernel(
                &kernel_, "ref_convolution_fwd_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    ref_convolution_fwd_t(const pd_t *apd) : primitive_impl_t(apd) {}

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_forward(ctx);
    }

private:
    status_t execute_forward(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    compute::kernel_t kernel_;
};

struct ref_convolution_bwd_data_t : public primitive_impl_t {
    struct pd_t : public ocl_convolution_bwd_data_pd_t {
        using ocl_convolution_bwd_data_pd_t::ocl_convolution_bwd_data_pd_t;

        DECLARE_COMMON_PD_T("ocl:ref:any", ref_convolution_bwd_data_t);

        status_t init() {
            bool ok = set_default_alg_kind(alg_kind::convolution_direct)
                    && desc()->prop_kind == prop_kind::backward_data
                    && desc()->alg_kind == alg_kind::convolution_direct
                    && this->set_default_formats();
            if (!ok) return status::unimplemented;

            return kernel_.init(*this->desc(), *this->diff_src_md(),
                    *this->weights_md(), *this->weights_md(1),
                    *this->diff_dst_md(), *this->attr());
        }

        const ref_convolution_kernel_t *kernel() const { return &kernel_; }

    private:
        bool set_default_formats() {
            using namespace format_tag;
            auto dat_tag = utils::pick(ndims() - 3, ncw, nchw, ncdhw);
            auto wei_tag = with_groups()
                    ? utils::pick(ndims() - 3, goiw, goihw, goidhw)
                    : utils::pick(ndims() - 3, oiw, oihw, oidhw);
            return set_default_formats_common(dat_tag, wei_tag, dat_tag);
        }

        ref_convolution_kernel_t kernel_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;

        auto status = pd()->kernel()->apply_const(kernel_ctx);
        if (status != status::success) return status;

        compute_engine->create_kernel(
                &kernel_, "ref_convolution_bwd_data_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    ref_convolution_bwd_data_t(const pd_t *apd) : primitive_impl_t(apd) {}

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_backward_data(ctx);
    }

private:
    status_t execute_backward_data(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    compute::kernel_t kernel_;
};

struct ref_convolution_bwd_weights_t : public primitive_impl_t {
    struct pd_t : public ocl_convolution_bwd_weights_pd_t {
        using ocl_convolution_bwd_weights_pd_t::
                ocl_convolution_bwd_weights_pd_t;

        DECLARE_COMMON_PD_T("ocl:ref:any", ref_convolution_bwd_weights_t);

        status_t init() {
            bool ok = set_default_alg_kind(alg_kind::convolution_direct)
                    && desc()->prop_kind == prop_kind::backward_weights
                    && desc()->alg_kind == alg_kind::convolution_direct
                    && this->set_default_formats();
            if (!ok) return status::unimplemented;

            return kernel_.init(*this->desc(), *this->src_md(),
                    *this->diff_weights_md(), *this->diff_weights_md(1),
                    *this->diff_dst_md(), *this->attr());
        }

        const ref_convolution_kernel_t *kernel() const { return &kernel_; }

    private:
        bool set_default_formats() {
            using namespace format_tag;
            auto dat_tag = utils::pick(ndims() - 3, ncw, nchw, ncdhw);
            auto wei_tag = with_groups()
                    ? utils::pick(ndims() - 3, goiw, goihw, goidhw)
                    : utils::pick(ndims() - 3, oiw, oihw, oidhw);
            return set_default_formats_common(dat_tag, wei_tag, dat_tag);
        }

        ref_convolution_kernel_t kernel_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());
        compute::kernel_ctx_t kernel_ctx;

        auto status = pd()->kernel()->apply_const(kernel_ctx);
        if (status != status::success) return status;

        compute_engine->create_kernel(
                &kernel_, "ref_convolution_bwd_weights_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    ref_convolution_bwd_weights_t(const pd_t *apd) : primitive_impl_t(apd) {}

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_backward_weights(ctx);
    }

private:
    status_t execute_backward_weights(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    compute::kernel_t kernel_;
};

} // namespace ocl
} // namespace impl
} // namespace dnnl
#endif

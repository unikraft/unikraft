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

#ifndef JIT_GEN9_COMMON_CONVOLUTION_HPP
#define JIT_GEN9_COMMON_CONVOLUTION_HPP

#include <assert.h>

#include "common/c_types_map.hpp"
#include "compute/compute.hpp"
#include "ocl/jit_gen9_common_conv_kernel.hpp"
#include "ocl/ocl_convolution_pd.hpp"
#include "ocl/ocl_stream.hpp"
#include "ocl/ocl_utils.hpp"

extern const char *gen9_common_conv_fwd_data_f32_kernel;
extern const char *gen9_common_conv_bwd_data_kernel;
extern const char *gen9_common_conv_bwd_wht_f32_kernel;
extern const char *gen9_common_conv_fwd_data_f16_kernel;

extern const char *gen9_common_conv_dw_fwd_data_kernel;

namespace dnnl {
namespace impl {
namespace ocl {

struct jit_gen9_common_convolution_fwd_t : public primitive_impl_t {
    struct pd_t : public ocl_convolution_fwd_pd_t {
        pd_t(engine_t *engine, const convolution_desc_t *adesc,
                const primitive_attr_t *attr,
                const convolution_fwd_pd_t *hint_fwd_pd)
            : ocl_convolution_fwd_pd_t(engine, adesc, attr, hint_fwd_pd)
            , jcp_() {}

        DECLARE_COMMON_PD_T(
                "ocl:gen9:blocked", jit_gen9_common_convolution_fwd_t);

        status_t init() {
            using namespace prop_kind;
            using namespace data_type;
            assert(this->engine()->kind() == engine_kind::gpu);
            auto *compute_engine
                    = utils::downcast<compute::compute_engine_t *>(engine());

            auto src_data_t = this->desc()->src_desc.data_type;

            bool ok = set_default_alg_kind(alg_kind::convolution_direct)
                    && utils::one_of(this->desc()->prop_kind, forward_training,
                            forward_inference)
                    && this->desc()->alg_kind == alg_kind::convolution_direct
                    && utils::one_of(true,
                            expect_data_types(f32, f32, f32, f32, f32),
                            expect_data_types(f16, f16, f16, f16, f16))
                    && compute_engine->mayiuse(
                            compute::device_ext_t::intel_subgroups)
                    && IMPLICATION(src_data_t == f16,
                            true
                                    && compute_engine->mayiuse(
                                            compute::device_ext_t::khr_fp16)
                                    && compute_engine->mayiuse(
                                            compute::device_ext_t::
                                                    intel_subgroups_short))
                    && !has_zero_dim_memory();
            if (!ok) return status::unimplemented;

            status_t status = jit_gen9_common_conv_fwd_kernel::init_conf(jcp_,
                    *this->desc(), *this->src_md(), *this->weights_md(),
                    *this->dst_md(), *this->weights_md(1), *this->attr());
            if (status != status::success) return status;

            ok = set_default_formats_common(
                    jcp_.src_tag, jcp_.wei_tag, jcp_.dst_tag);
            return ok ? status::success : status::unimplemented;
        }
        jit_conv_conf_t jcp_;
    };

    status_t init() override {
        const char *kernel_name = nullptr;
        if (pd()->jcp_.is_depthwise)
            kernel_name = "gen9_common_conv_dw_fwd_kernel";
        else if (pd()->desc()->src_desc.data_type == data_type::f16)
            kernel_name = "gen9_common_conv_fwd_f16_kernel";
        else if (pd()->desc()->src_desc.data_type == data_type::f32)
            kernel_name = "gen9_common_conv_fwd_f32_kernel";
        else
            assert(!"not expected");

        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());

        compute::kernel_ctx_t kernel_ctx;
        auto status = jit_gen9_common_conv_fwd_kernel::init_const_def(
                kernel_ctx, pd()->jcp_);
        if (status != status::success) return status;

        compute_engine->create_kernel(&kernel_, kernel_name, kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    jit_gen9_common_convolution_fwd_t(const pd_t *apd) : primitive_impl_t(apd) {
        ker_ = new jit_gen9_common_conv_fwd_kernel(pd()->jcp_);
    }

    ~jit_gen9_common_convolution_fwd_t() { delete ker_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_forward(ctx);
    }

private:
    status_t execute_forward(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_gen9_common_conv_fwd_kernel *ker_;
    compute::kernel_t kernel_;
};

struct jit_gen9_common_convolution_bwd_data_t : public primitive_impl_t {
    struct pd_t : public ocl_convolution_bwd_data_pd_t {
        pd_t(engine_t *engine, const convolution_desc_t *adesc,
                const primitive_attr_t *attr,
                const convolution_fwd_pd_t *hint_fwd_pd)
            : ocl_convolution_bwd_data_pd_t(engine, adesc, attr, hint_fwd_pd) {}

        DECLARE_COMMON_PD_T(
                "ocl:ncsp:any", jit_gen9_common_convolution_bwd_data_t);

        status_t init() {
            using namespace data_type;
            using namespace prop_kind;
            assert(this->engine()->kind() == engine_kind::gpu);
            auto *compute_engine
                    = utils::downcast<compute::compute_engine_t *>(engine());

            bool ok = set_default_alg_kind(alg_kind::convolution_direct)
                    && this->desc()->prop_kind == backward_data
                    && this->desc()->alg_kind == alg_kind::convolution_direct
                    && utils::one_of(true,
                            expect_data_types(
                                    f32, f32, data_type::undef, f32, f32),
                            expect_data_types(
                                    f16, f16, data_type::undef, f16, f16))
                    && (IMPLICATION(this->with_bias()
                                        && this->desc()->diff_dst_desc.data_type
                                                != f16,
                                this->desc()->bias_desc.data_type == f32)
                            || IMPLICATION(this->with_bias()
                                            && this->desc()->diff_dst_desc
                                                            .data_type
                                                    == f16,
                                    this->desc()->bias_desc.data_type == f16))
                    && compute_engine->mayiuse(
                            compute::device_ext_t::intel_subgroups)
                    && !has_zero_dim_memory();
            if (!ok) return status::unimplemented;

            status_t status = jit_gen9_common_conv_bwd_data_kernel::init_conf(
                    jcp_, *this->desc(), *this->diff_src_md(),
                    *this->weights_md(), *this->diff_dst_md(),
                    *this->weights_md(1), *this->attr());
            if (status != status::success) return status;

            ok = set_default_formats_common(
                    jcp_.src_tag, jcp_.wei_tag, jcp_.dst_tag);
            return ok ? status::success : status::unimplemented;
        }
        jit_conv_conf_t jcp_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());

        compute::kernel_ctx_t kernel_ctx;
        auto status = jit_gen9_common_conv_bwd_data_kernel::init_const_def(
                kernel_ctx, pd()->jcp_);
        if (status != status::success) return status;

        compute_engine->create_kernel(
                &kernel_, "gen9_common_conv_bwd_data_kernel", kernel_ctx);
        if (!kernel_) return status::runtime_error;

        return status::success;
    }

    jit_gen9_common_convolution_bwd_data_t(const pd_t *apd)
        : primitive_impl_t(apd) {
        ker_ = new jit_gen9_common_conv_bwd_data_kernel(pd()->jcp_);
    }

    ~jit_gen9_common_convolution_bwd_data_t() { delete ker_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_backward_data(ctx);
    }

private:
    status_t execute_backward_data(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_gen9_common_conv_bwd_data_kernel *ker_;
    compute::kernel_t kernel_;
};

struct jit_gen9_common_convolution_bwd_weights_t : public primitive_impl_t {
    struct pd_t : public ocl_convolution_bwd_weights_pd_t {
        pd_t(engine_t *engine, const convolution_desc_t *adesc,
                const primitive_attr_t *attr,
                const convolution_fwd_pd_t *hint_fwd_pd)
            : ocl_convolution_bwd_weights_pd_t(
                    engine, adesc, attr, hint_fwd_pd) {}

        DECLARE_COMMON_PD_T(
                "ocl:ncsp:any", jit_gen9_common_convolution_bwd_weights_t);

        status_t init() {
            using namespace data_type;
            using namespace prop_kind;
            assert(this->engine()->kind() == engine_kind::gpu);
            auto *compute_engine
                    = utils::downcast<compute::compute_engine_t *>(engine());

            bool ok = set_default_alg_kind(alg_kind::convolution_direct)
                    && this->desc()->prop_kind == backward_weights
                    && this->desc()->alg_kind == alg_kind::convolution_direct
                    && expect_data_types(f32, f32, f32, f32, f32)
                    && compute_engine->mayiuse(
                            compute::device_ext_t::intel_subgroups)
                    && !has_zero_dim_memory();
            if (!ok) return status::unimplemented;

            status_t status
                    = jit_gen9_common_conv_bwd_weights_kernel::init_conf(jcp_,
                            *this->desc(), *this->src_md(),
                            *this->diff_weights_md(), *this->diff_weights_md(1),
                            *this->diff_dst_md(), *this->attr());

            if (status != status::success) return status;

            ok = set_default_formats_common(
                    jcp_.src_tag, jcp_.wei_tag, jcp_.dst_tag);
            return ok ? status::success : status::unimplemented;
        }
        jit_conv_conf_t jcp_;
    };

    status_t init() override {
        auto *compute_engine
                = utils::downcast<compute::compute_engine_t *>(engine());

        compute::kernel_ctx_t kernel_ctx;
        auto status = jit_gen9_common_conv_bwd_weights_kernel::init_const_def(
                kernel_ctx, pd()->jcp_);
        if (status != status::success) return status;

        std::vector<const char *> kernel_names(3);

        kernel_names[0] = "gen9_common_conv_bwd_weights_kernel";

        if (pd()->jcp_.ver == ver_16mb16c || pd()->jcp_.ver == ver_8ow16c
                || pd()->jcp_.ver == ver_1stconv) {
            kernel_names[1] = "gen9_reduce_bwd_weights_kernel";
        }
        if (pd()->jcp_.ver == ver_8ow16c) {
            kernel_names[2] = "gen9_load_tails_bwd_weights_kernel";
        }

        std::vector<compute::kernel_t> kernels;
        status = compute_engine->create_kernels(
                &kernels, kernel_names, kernel_ctx);
        CHECK(status);

        kernel_ = kernels[0];
        reduce_kernel_ = kernels[1];
        load_tails_ = kernels[2];

        if (pd()->jcp_.ver == ver_16mb16c || pd()->jcp_.ver == ver_8ow16c
                || pd()->jcp_.ver == ver_1stconv) {
            size_t size = pd()->jcp_.ngroups * pd()->jcp_.nchunk * pd()->jcp_.oc
                    * pd()->jcp_.ic * pd()->jcp_.kh * pd()->jcp_.kw
                    * pd()->jcp_.kd * sizeof(float);
            memory_storage_t *wht_work_ptr;
            engine()->create_memory_storage(&wht_work_ptr, size);
            wht_work.reset(wht_work_ptr);
            if (!wht_work) return status::runtime_error;

            size = pd()->jcp_.ngroups * pd()->jcp_.nchunk * pd()->jcp_.oc
                    * sizeof(float);
            memory_storage_t *bias_work_ptr;
            engine()->create_memory_storage(&bias_work_ptr, size);
            bias_work.reset(bias_work_ptr);
            if (!bias_work) return status::runtime_error;
        }
        if (pd()->jcp_.ver == ver_8ow16c) {
            size_t size = 2 * 16
                    * (2 * pd()->jcp_.l_pad + pd()->jcp_.iw + pd()->jcp_.kw + 8)
                    * sizeof(float);
            memory_storage_t *tails_ptr;
            engine()->create_memory_storage(&tails_ptr, size);
            tails.reset(tails_ptr);
            if (!tails) return status::runtime_error;
        }

        return status::success;
    }

    jit_gen9_common_convolution_bwd_weights_t(const pd_t *apd)
        : primitive_impl_t(apd) {
        ker_ = new jit_gen9_common_conv_bwd_weights_kernel(pd()->jcp_);
    }

    ~jit_gen9_common_convolution_bwd_weights_t() { delete ker_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        return execute_backward_weights(ctx);
    }

private:
    status_t execute_backward_weights(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_gen9_common_conv_bwd_weights_kernel *ker_;
    compute::kernel_t kernel_;
    compute::kernel_t reduce_kernel_;
    compute::kernel_t load_tails_;
    std::unique_ptr<memory_storage_t> wht_work;
    std::unique_ptr<memory_storage_t> bias_work;
    std::unique_ptr<memory_storage_t> tails;
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

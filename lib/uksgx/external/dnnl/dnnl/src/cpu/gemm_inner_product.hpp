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

#ifndef CPU_GEMM_INNER_PRODUCT_HPP
#define CPU_GEMM_INNER_PRODUCT_HPP

#include <assert.h>

#include "c_types_map.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

#include "gemm/gemm.hpp"
#include "gemm_inner_product_utils.hpp"

#include "cpu_inner_product_pd.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

template <impl::data_type_t data_type>
struct gemm_inner_product_fwd_t : public primitive_impl_t {
    struct pd_t : public cpu_inner_product_fwd_pd_t {
        using cpu_inner_product_fwd_pd_t::cpu_inner_product_fwd_pd_t;

        DECLARE_COMMON_PD_T(GEMM_IMPL_STR, gemm_inner_product_fwd_t);

        status_t init() {
            using namespace utils;

            bool ok = true && is_fwd() && !has_zero_dim_memory()
                    && everyone_is(data_type, src_md()->data_type,
                            weights_md()->data_type, dst_md()->data_type,
                            with_bias() ? weights_md(1)->data_type : data_type)
                    && attr()->output_scales_.has_default_values()
                    && post_ops_ok() && set_default_params() == status::success
                    && dense_gemm_consitency_check(
                            src_md(), weights_md(), dst_md());
            return ok ? status::success : status::unimplemented;
        }

    protected:
        bool post_ops_ok() const {
            auto const &po = attr()->post_ops_;
            auto is_eltwise
                    = [&](int idx) { return po.entry_[idx].is_eltwise(false); };
            auto is_sum = [&](int idx) { return po.entry_[idx].is_sum(false); };
            switch (po.len_) {
                case 0: return true; // no post_ops
                case 1: return is_eltwise(0) || is_sum(0); // sum OR eltwise
                case 2: return is_sum(0) && is_eltwise(1); // sum -> eltwise
                default: return false;
            }
            return false;
        }
    };

    gemm_inner_product_fwd_t(const pd_t *apd)
        : primitive_impl_t(apd), pp_kernel_(nullptr), postops_in_ip_(false) {
        bool has_bias = pd()->with_bias(),
             has_eltwise
                = pd()->attr()->post_ops_.find(primitive_kind::eltwise) >= 0;
        postops_in_ip_ = has_bias || has_eltwise;

        pp_kernel_ = new inner_product_utils::pp_kernel_t<data_type, data_type>(
                apd, true);

        auto sum_idx = pd()->attr()->post_ops_.find(primitive_kind::sum);
        beta_ = sum_idx >= 0 ? pd()->attr()->post_ops_.entry_[sum_idx].sum.scale
                             : 0.0;
    }
    ~gemm_inner_product_fwd_t() { delete pp_kernel_; }

    typedef typename prec_traits<data_type>::type data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_forward(ctx);
        return status::success;
    }

private:
    void execute_forward(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }

    inner_product_utils::pp_kernel_t<data_type, data_type> *pp_kernel_;
    bool postops_in_ip_;
    float beta_;
};

template <impl::data_type_t data_type>
struct gemm_inner_product_bwd_data_t : public primitive_impl_t {
    struct pd_t : public cpu_inner_product_bwd_data_pd_t {
        using cpu_inner_product_bwd_data_pd_t::cpu_inner_product_bwd_data_pd_t;

        DECLARE_COMMON_PD_T(GEMM_IMPL_STR, gemm_inner_product_bwd_data_t);

        status_t init() {
            bool ok = true && desc()->prop_kind == prop_kind::backward_data
                    && !has_zero_dim_memory()
                    && utils::everyone_is(data_type, diff_src_md()->data_type,
                            weights_md()->data_type, diff_dst_md()->data_type)
                    && attr()->has_default_values()
                    && set_default_params() == status::success
                    && dense_gemm_consitency_check(
                            diff_src_md(), weights_md(), diff_dst_md());
            return ok ? status::success : status::unimplemented;
        }
    };

    gemm_inner_product_bwd_data_t(const pd_t *apd) : primitive_impl_t(apd) {}
    typedef typename prec_traits<data_type>::type data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_backward_data(ctx);
        return status::success;
    }

private:
    void execute_backward_data(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
};

template <impl::data_type_t data_type>
struct gemm_inner_product_bwd_weights_t : public primitive_impl_t {
    struct pd_t : public cpu_inner_product_bwd_weights_pd_t {
        using cpu_inner_product_bwd_weights_pd_t::
                cpu_inner_product_bwd_weights_pd_t;

        DECLARE_COMMON_PD_T(GEMM_IMPL_STR, gemm_inner_product_bwd_weights_t);

        status_t init() {
            bool ok = true && desc()->prop_kind == prop_kind::backward_weights
                    && !has_zero_dim_memory()
                    && utils::everyone_is(data_type, src_md()->data_type,
                            diff_weights_md()->data_type,
                            diff_dst_md()->data_type,
                            with_bias() ? diff_weights_md(1)->data_type
                                        : data_type)
                    && attr()->has_default_values()
                    && set_default_params() == status::success
                    && dense_gemm_consitency_check(
                            src_md(), diff_weights_md(), diff_dst_md());

            return ok ? status::success : status::unimplemented;
        }
    };

    gemm_inner_product_bwd_weights_t(const pd_t *apd) : primitive_impl_t(apd) {}
    typedef typename prec_traits<data_type>::type data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_backward_weights(ctx);
        return status::success;
    }

private:
    void execute_backward_weights(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

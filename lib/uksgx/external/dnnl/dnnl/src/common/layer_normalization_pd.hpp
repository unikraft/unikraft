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

#ifndef LAYER_NORMALIZATION_PD_HPP
#define LAYER_NORMALIZATION_PD_HPP

#include "dnnl.h"

#include "c_types_map.hpp"
#include "primitive_desc.hpp"
#include "utils.hpp"

namespace dnnl {
namespace impl {

struct layer_normalization_fwd_pd_t;

struct layer_normalization_pd_t : public primitive_desc_t {
    static constexpr auto base_pkind = primitive_kind::layer_normalization;

    layer_normalization_pd_t(engine_t *engine,
            const layer_normalization_desc_t *adesc,
            const primitive_attr_t *attr,
            const layer_normalization_fwd_pd_t *hint_fwd_pd)
        : primitive_desc_t(engine, attr, base_pkind)
        , desc_(*adesc)
        , hint_fwd_pd_(hint_fwd_pd)
        , data_md_(desc_.data_desc)
        , stat_md_(desc_.stat_desc)
        , scaleshift_md_(desc_.data_scaleshift_desc) {}

    const layer_normalization_desc_t *desc() const { return &desc_; }
    virtual const op_desc_t *op_desc() const override {
        return reinterpret_cast<const op_desc_t *>(this->desc());
    }
    virtual void init_info() override { impl::init_info(this, this->info_); }

    virtual status_t query(query_t what, int idx, void *result) const override {
        switch (what) {
            case query::prop_kind:
                *(prop_kind_t *)result = desc()->prop_kind;
                break;
            case query::layer_normalization_d:
                *(const layer_normalization_desc_t **)result = desc();
                break;
            default: return primitive_desc_t::query(what, idx, result);
        }
        return status::success;
    }

    /* common layer_normalization aux functions */
    int ndims() const { return desc_.data_desc.ndims; }
    dim_t across_axis() const {
        return utils::array_product(desc_.data_desc.dims, ndims() - 1);
    }
    dim_t norm_axis() const { return desc_.data_desc.dims[ndims() - 1]; }

    bool stats_are_src() const { return desc_.flags & dnnl_use_global_stats; }
    bool stats_are_tmp() const { return !(stats_are_src() || is_training()); }

    bool use_scaleshift() const { return desc_.flags & dnnl_use_scaleshift; }
    bool use_global_stats() const {
        return desc_.flags & dnnl_use_global_stats;
    }

    bool is_fwd() const {
        return utils::one_of(desc_.prop_kind, prop_kind::forward_training,
                prop_kind::forward_inference);
    }
    bool is_bwd() const { return !this->is_fwd(); }
    bool is_training() const {
        return desc_.prop_kind == prop_kind::forward_training;
    }

    bool has_zero_dim_memory() const {
        return memory_desc_wrapper(desc_.data_desc).has_zero_dim();
    }

    const memory_desc_t *stat_md() const { return &stat_md_; }

protected:
    layer_normalization_desc_t desc_;
    const layer_normalization_fwd_pd_t *hint_fwd_pd_;

    memory_desc_t data_md_;
    memory_desc_t stat_md_;
    memory_desc_t scaleshift_md_;

private:
    const memory_desc_t &data_desc() const { return desc_.data_desc; }
};

struct layer_normalization_fwd_pd_t : public layer_normalization_pd_t {
    typedef layer_normalization_fwd_pd_t base_class;
    typedef layer_normalization_fwd_pd_t hint_class;

    layer_normalization_fwd_pd_t(engine_t *engine,
            const layer_normalization_desc_t *adesc,
            const primitive_attr_t *attr,
            const layer_normalization_fwd_pd_t *hint_fwd_pd)
        : layer_normalization_pd_t(engine, adesc, attr, hint_fwd_pd) {}

    virtual arg_usage_t arg_usage(int arg) const override {
        if (arg == DNNL_ARG_SRC) return arg_usage_t::input;
        if (arg == DNNL_ARG_DST) return arg_usage_t::output;

        if (utils::one_of(arg, DNNL_ARG_MEAN, DNNL_ARG_VARIANCE)) {
            if (stats_are_src()) return arg_usage_t::input;
            if (!stats_are_src() && is_training()) return arg_usage_t::output;
            return arg_usage_t::unused;
        }

        if (arg == DNNL_ARG_SCALE_SHIFT && use_scaleshift())
            return arg_usage_t::input;

        return primitive_desc_t::arg_usage(arg);
    }

    virtual const memory_desc_t *src_md(int index = 0) const override {
        if (index == 0) return &data_md_;
        if (stats_are_src() && (index == 1 || index == 2)) return &stat_md_;
        return &glob_zero_md;
    }

    virtual const memory_desc_t *dst_md(int index = 0) const override {
        if (index == 0) return &data_md_;
        if (!stats_are_src() && is_training() && (index == 1 || index == 2))
            return &stat_md_;
        return &glob_zero_md;
    }

    virtual const memory_desc_t *weights_md(int index = 0) const override {
        return index == 0 ? &scaleshift_md_ : &glob_zero_md;
    }

    virtual int n_inputs() const override {
        return 1 + 2 * stats_are_src() + use_scaleshift();
    }
    virtual int n_outputs() const override {
        return 1 + 2 * (!stats_are_src()) * is_training();
    }
};

struct layer_normalization_bwd_pd_t : public layer_normalization_pd_t {
    typedef layer_normalization_bwd_pd_t base_class;
    typedef layer_normalization_fwd_pd_t hint_class;

    layer_normalization_bwd_pd_t(engine_t *engine,
            const layer_normalization_desc_t *adesc,
            const primitive_attr_t *attr,
            const layer_normalization_fwd_pd_t *hint_fwd_pd)
        : layer_normalization_pd_t(engine, adesc, attr, hint_fwd_pd)
        , diff_data_md_(desc_.diff_data_desc)
        , diff_scaleshift_md_(desc_.diff_data_scaleshift_desc) {}

    virtual arg_usage_t arg_usage(int arg) const override {
        if (utils::one_of(arg, DNNL_ARG_SRC, DNNL_ARG_MEAN, DNNL_ARG_VARIANCE,
                    DNNL_ARG_DIFF_DST))
            return arg_usage_t::input;

        if (arg == DNNL_ARG_SCALE_SHIFT && use_scaleshift())
            return arg_usage_t::input;

        if (arg == DNNL_ARG_DIFF_SRC) return arg_usage_t::output;

        if (arg == DNNL_ARG_DIFF_SCALE_SHIFT && use_scaleshift())
            return arg_usage_t::output;

        return primitive_desc_t::arg_usage(arg);
    }

    virtual const memory_desc_t *src_md(int index = 0) const override {
        return index == 0 ? &data_md_ : index <= 2 ? &stat_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *dst_md(int index = 0) const override {
        return (index == 0) ? &data_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *diff_dst_md(int index = 0) const override {
        return index == 0 ? &diff_data_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *diff_src_md(int index = 0) const override {
        return index == 0 ? &diff_data_md_ : &glob_zero_md;
    }

    virtual const memory_desc_t *weights_md(int index = 0) const override {
        return index == 0 ? &scaleshift_md_ : &glob_zero_md;
    }
    virtual const memory_desc_t *diff_weights_md(int index = 0) const override {
        return index == 0 ? &diff_scaleshift_md_ : &glob_zero_md;
    }

    virtual int n_inputs() const override { return 4 + use_scaleshift(); }
    virtual int n_outputs() const override {
        return 1 + (desc_.prop_kind == prop_kind::backward);
    }

protected:
    memory_desc_t diff_data_md_;
    memory_desc_t diff_scaleshift_md_;

    bool set_default_formats_common() {
        if (diff_data_md_.format_kind != format_kind::any) return true;

        return memory_desc_init_by_md_and_dt(
                       diff_data_md_, data_md_, diff_data_md_.data_type)
                == status::success;
    }
};

} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino^=l0,\:0,N-s

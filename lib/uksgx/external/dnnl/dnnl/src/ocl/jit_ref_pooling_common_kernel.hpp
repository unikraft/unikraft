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

#ifndef JIT_REF_POOLING_COMMON_KERNEL_HPP
#define JIT_REF_POOLING_COMMON_KERNEL_HPP

#include "common/c_types_map.hpp"
#include "compute/compute.hpp"
#include "ocl/jit_primitive_conf.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

struct jit_ref_pooling_fwd_kernel {

    jit_ref_pooling_fwd_kernel(jit_pool_conf_t ajpp) : jpp(ajpp) {};

    ~jit_ref_pooling_fwd_kernel() {};

    static status_t init_conf(jit_pool_conf_t &jpp, const pooling_desc_t &pd,
            const memory_desc_wrapper &src_d, const memory_desc_wrapper &dst_d,
            jit_offsets &jit_off) {
        using namespace dnnl::impl::format_tag;

        const int ndims = src_d.ndims();
        const auto &src_dims = src_d.padded_dims();
        const auto &dst_dims = dst_d.padded_dims();

        jpp.ndims = ndims;
        jpp.mb = src_dims[0];

        jpp.c = src_dims[1];
        jpp.id = (ndims == 5) ? src_dims[2] : 1;
        jpp.ih = (ndims == 3) ? 1 : src_dims[ndims - 2];
        jpp.iw = src_dims[ndims - 1];
        jpp.od = (ndims == 5) ? dst_dims[2] : 1;
        jpp.oh = (ndims == 3) ? 1 : dst_dims[ndims - 2];
        jpp.ow = dst_dims[ndims - 1];

        jpp.stride_d = (ndims == 5) ? pd.strides[0] : 1;
        jpp.stride_h = (ndims == 3) ? 1 : pd.strides[ndims - 4];
        jpp.stride_w = pd.strides[ndims - 3];
        jpp.kd = (ndims == 5) ? pd.kernel[0] : 1;
        jpp.kh = (ndims == 3) ? 1 : pd.kernel[ndims - 4];
        jpp.kw = pd.kernel[ndims - 3];

        jpp.f_pad = (ndims == 5) ? pd.padding[0][0] : 0;
        jpp.t_pad = (ndims == 3) ? 0 : pd.padding[0][ndims - 4];
        jpp.l_pad = pd.padding[0][ndims - 3];

        jpp.alg = pd.alg_kind;

        jpp.src_dt = src_d.data_type();

        jpp.is_training = pd.prop_kind == prop_kind::forward_training;
        jpp.is_backward = pd.prop_kind == prop_kind::backward_data;

        set_offsets(src_d, jit_off.src_off);
        set_offsets(dst_d, jit_off.dst_off);

        jpp.lws_d[0] = 1;
        jpp.lws_d[1] = 1;
        jpp.lws_d[2] = 1;
        jpp.gws_d[0] = jpp.mb;
        jpp.gws_d[1] = jpp.c;
        jpp.gws_d[2] = 1;
        jpp.sub_group_size = 1;
        jpp.use_16mb_unroll = 0;
        jpp.use_16c_unroll = 0;
        // disable subgroup optimization for s8
        if (utils::one_of(src_d.data_type(), data_type::f32, data_type::f16)
                && ((src_d.matches_tag(nCw16c) && dst_d.matches_tag(nCw16c))
                        || (src_d.matches_tag(nChw16c)
                                && dst_d.matches_tag(nChw16c))
                        || (src_d.matches_tag(nCdhw16c)
                                && dst_d.matches_tag(nCdhw16c))
                        || (src_d.matches_tag(NCw16n16c)
                                && dst_d.matches_tag(NCw16n16c))
                        || (src_d.matches_tag(NChw16n16c)
                                && dst_d.matches_tag(NChw16n16c))
                        || (src_d.matches_tag(NCdhw16n16c)
                                && dst_d.matches_tag(NCdhw16n16c)))) {
            jpp.use_16mb_unroll = src_d.matches_one_of_tag(
                    NCw16n16c, NChw16n16c, NCdhw16n16c);
            jpp.use_16c_unroll = 1;
            jpp.sub_group_size = 16;
            jpp.lws_d[0] = 1;
            jpp.lws_d[1] = 16;
            jpp.lws_d[2] = 1;
            jpp.gws_d[0] = jpp.is_backward ? jpp.id * jpp.ih * jpp.iw
                                           : jpp.od * jpp.oh;
            jpp.gws_d[1] = jpp.c;
            jpp.gws_d[2] = jpp.use_16mb_unroll ? jpp.mb / 16 : jpp.mb;
        }

        return status::success;
    };

    static status_t init_const_def(compute::kernel_ctx_t &kernel_ctx,
            const jit_pool_conf_t &jpp, const jit_offsets &jit_off) {
        using namespace dnnl::impl::alg_kind;
        status_t status = status::success;

        kernel_ctx.set_data_type(jpp.src_dt);

        kernel_ctx.define_int("NDIMS", jpp.ndims);
        kernel_ctx.define_int("MB", jpp.mb);
        kernel_ctx.define_int("C", jpp.c);
        kernel_ctx.define_int("ID", jpp.id);
        kernel_ctx.define_int("IH", jpp.ih);
        kernel_ctx.define_int("IW", jpp.iw);
        kernel_ctx.define_int("OD", jpp.od);
        kernel_ctx.define_int("OH", jpp.oh);
        kernel_ctx.define_int("OW", jpp.ow);
        kernel_ctx.define_int("KD", jpp.kd);
        kernel_ctx.define_int("KH", jpp.kh);
        kernel_ctx.define_int("KW", jpp.kw);
        kernel_ctx.define_int("SD", jpp.stride_d);
        kernel_ctx.define_int("SH", jpp.stride_h);
        kernel_ctx.define_int("SW", jpp.stride_w);
        kernel_ctx.define_int("PD", jpp.f_pad);
        kernel_ctx.define_int("PH", jpp.t_pad);
        kernel_ctx.define_int("PW", jpp.l_pad);
        kernel_ctx.define_int("LWS_0", jpp.lws_d[0]);
        kernel_ctx.define_int("LWS_1", jpp.lws_d[1]);
        kernel_ctx.define_int("LWS_2", jpp.lws_d[2]);
        kernel_ctx.define_int("SUB_GROUP_SIZE", jpp.sub_group_size);
        kernel_ctx.define_int("USE_16MB_UNROLL", jpp.use_16mb_unroll);
        kernel_ctx.define_int("USE_16C_UNROLL", jpp.use_16c_unroll);
        kernel_ctx.define_int("IS_TRAINING", jpp.is_training);
        if (jpp.is_backward)
            kernel_ctx.define_int("POOLING_BWD", 1);
        else
            kernel_ctx.define_int("POOLING_FWD", 1);
        switch (jpp.alg) {
            case pooling_max: kernel_ctx.define_int("POOLING_MAX", 1); break;
            case pooling_avg_exclude_padding:
                kernel_ctx.define_int("POOLING_AVG_EXCLUDE_PADDING", 1);
                break;
            case pooling_avg_include_padding:
                kernel_ctx.define_int("POOLING_AVG_INCLUDE_PADDING", 1);
                break;
            default: status = status::unimplemented;
        }
        if (status != status::success) return status;

        def_offsets(jit_off.src_off, kernel_ctx, "SRC", jpp.ndims);
        def_offsets(jit_off.dst_off, kernel_ctx, "DST", jpp.ndims);

        return status::success;
    }

    jit_pool_conf_t jpp;

private:
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif

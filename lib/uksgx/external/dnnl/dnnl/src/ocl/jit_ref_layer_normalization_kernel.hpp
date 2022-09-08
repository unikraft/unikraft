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

#ifndef JIT_REF_LAYER_NORMALIZATION_KERNEL_HPP
#define JIT_REF_LAYER_NORMALIZATION_KERNEL_HPP

#include "common/c_types_map.hpp"
#include "common/layer_normalization_pd.hpp"
#include "common/utils.hpp"
#include "compute/compute.hpp"
#include "ocl/jit_primitive_conf.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

struct jit_ref_layer_normalization_kernel_t {
    static status_t init_conf(
            jit_lnorm_conf_t &jln, const layer_normalization_pd_t *pd) {
        memory_desc_wrapper src_mdw(pd->src_md());
        memory_desc_wrapper stat_mdw(pd->stat_md());
        memory_desc_wrapper dst_mdw(pd->dst_md());

        int ndims = src_mdw.ndims();

        jln.data_type = src_mdw.data_type();
        jln.ndims = ndims;
        jln.norm_axis = pd->norm_axis();

        jln.src_md_info = jit_memory_desc_info_t::create(src_mdw);
        jln.dst_md_info = jit_memory_desc_info_t::create(dst_mdw);
        jln.stat_md_info = jit_memory_desc_info_t::create(stat_mdw);

        if (pd->is_fwd()) {
            auto &dims = src_mdw.dims();
            jln.gws_d[0] = dims[0];
            jln.gws_d[1] = ndims > 2 ? dims[1] : 1;
            jln.gws_d[2]
                    = ndims > 3 ? utils::array_product(&dims[2], ndims - 3) : 1;
        } else {
            jln.gws_d[0] = pd->norm_axis();
            jln.gws_d[1] = 1;
            jln.gws_d[2] = 1;
        }

        jln.use_scaleshift = pd->use_scaleshift();
        jln.calculate_stats = !pd->stats_are_src();
        jln.save_stats = pd->is_training();
        jln.eps = pd->desc()->layer_norm_epsilon;

        return status::success;
    }

    static status_t init_const_def(
            const jit_lnorm_conf_t &jln, compute::kernel_ctx_t &kernel_ctx) {
        kernel_ctx.set_data_type(jln.data_type);

        kernel_ctx.define_int("C", jln.norm_axis);
        kernel_ctx.define_int("NDIMS", jln.ndims);
        kernel_ctx.define_int("USE_SCALESHIFT", jln.use_scaleshift);
        kernel_ctx.define_int("CALCULATE_STATS", jln.calculate_stats);
        kernel_ctx.define_int("SAVE_STATS", jln.save_stats);

        def_memory_desc_info(kernel_ctx, jln.src_md_info, "SRC");
        def_memory_desc_info(kernel_ctx, jln.dst_md_info, "DST");
        def_memory_desc_info(kernel_ctx, jln.stat_md_info, "STAT");

        return status::success;
    }
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif // JIT_REF_LAYER_NORMALIZATION_KERNEL_HPP

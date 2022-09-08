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

#include "ocl/ocl_stream.hpp"

#include "ocl/gemm_inner_product.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

status_t gemm_inner_product_fwd_t::execute_forward(
        const exec_ctx_t &ctx) const {

    compute::compute_stream_t *compute_stream
            = utils::downcast<compute::compute_stream_t *>(ctx.stream());

    exec_args_t gemm_args;
    gemm_args[DNNL_ARG_SRC_0] = ctx.args().at(DNNL_ARG_WEIGHTS);
    gemm_args[DNNL_ARG_SRC_1] = ctx.args().at(DNNL_ARG_SRC);
    gemm_args[DNNL_ARG_DST] = ctx.args().at(DNNL_ARG_DST);

    exec_ctx_t gemm_ctx(ctx.stream(), std::move(gemm_args));
    status_t gemm_exec_status = gemm_->execute(gemm_ctx);
    if (gemm_exec_status != status::success) return gemm_exec_status;

    if (pd()->with_bias()) {
        auto &bias = CTX_IN_STORAGE(DNNL_ARG_BIAS);
        auto &dst = CTX_OUT_STORAGE(DNNL_ARG_DST);

        compute::kernel_arg_list_t arg_list;
        arg_list.set(0, bias);
        arg_list.set(1, dst);

        auto nd_range = compute::nd_range_t({pd()->MB() * pd()->OC()});
        status_t bias_status = compute_stream->parallel_for(
                nd_range, bias_kernel_, arg_list);
        if (bias_status != status::success) return bias_status;
    }

    return status::success;
}

status_t gemm_inner_product_bwd_data_t::execute_backward_data(
        const exec_ctx_t &ctx) const {
    exec_args_t gemm_args;
    gemm_args[DNNL_ARG_SRC_0] = ctx.args().at(DNNL_ARG_WEIGHTS);
    gemm_args[DNNL_ARG_SRC_1] = ctx.args().at(DNNL_ARG_DIFF_DST);
    gemm_args[DNNL_ARG_DST] = ctx.args().at(DNNL_ARG_DIFF_SRC);

    exec_ctx_t gemm_ctx(ctx.stream(), std::move(gemm_args));
    status_t gemm_exec_status = gemm_->execute(gemm_ctx);
    if (gemm_exec_status != status::success) return gemm_exec_status;

    return status::success;
}

status_t gemm_inner_product_bwd_weights_t::execute_backward_weights(
        const exec_ctx_t &ctx) const {
    compute::compute_stream_t *compute_stream
            = utils::downcast<compute::compute_stream_t *>(ctx.stream());

    exec_args_t gemm_args;
    if (pd()->wei_tr()) {
        gemm_args[DNNL_ARG_SRC_0] = ctx.args().at(DNNL_ARG_DIFF_DST);
        gemm_args[DNNL_ARG_SRC_1] = ctx.args().at(DNNL_ARG_SRC);
    } else {
        gemm_args[DNNL_ARG_SRC_0] = ctx.args().at(DNNL_ARG_SRC);
        gemm_args[DNNL_ARG_SRC_1] = ctx.args().at(DNNL_ARG_DIFF_DST);
    }
    gemm_args[DNNL_ARG_DST] = ctx.args().at(DNNL_ARG_DIFF_WEIGHTS);

    exec_ctx_t gemm_ctx(ctx.stream(), std::move(gemm_args));
    status_t gemm_exec_status = gemm_->execute(gemm_ctx);
    if (gemm_exec_status != status::success) return gemm_exec_status;

    if (pd()->with_bias()) {
        auto &diff_dst = CTX_IN_STORAGE(DNNL_ARG_DIFF_DST);
        auto &diff_bias = CTX_OUT_STORAGE(DNNL_ARG_DIFF_BIAS);

        compute::kernel_arg_list_t arg_list;
        arg_list.set(0, diff_dst);
        arg_list.set(1, diff_bias);

        auto nd_range = compute::nd_range_t({pd()->OC()});
        status_t bias_status = compute_stream->parallel_for(
                nd_range, bias_kernel_, arg_list);
        if (bias_status != status::success) return bias_status;
    }

    return status::success;
}

} // namespace ocl
} // namespace impl
} // namespace dnnl

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

#include "common/c_types_map.hpp"
#include "common/dnnl_thread.hpp"
#include "common/dnnl_traits.hpp"
#include "common/math_utils.hpp"
#include "common/type_helpers.hpp"

#include "ocl/ref_batch_normalization.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

status_t ref_batch_normalization_fwd_t::execute_forward(
        const exec_ctx_t &ctx) const {

    compute::compute_stream_t *compute_stream
            = utils::downcast<compute::compute_stream_t *>(ctx.stream());

    auto &src = CTX_IN_STORAGE(DNNL_ARG_SRC);

    auto &mean_ = pd()->stats_is_src() ? CTX_IN_STORAGE(DNNL_ARG_MEAN)
                                       : CTX_OUT_STORAGE(DNNL_ARG_MEAN);

    auto &variance_ = pd()->stats_is_src() ? CTX_IN_STORAGE(DNNL_ARG_VARIANCE)
                                           : CTX_OUT_STORAGE(DNNL_ARG_VARIANCE);

    //auto idx_scaleshift = 1 + 2*pd()->stats_is_src();
    auto &scaleshift = CTX_IN_STORAGE(DNNL_ARG_SCALE_SHIFT);

    auto &dst = CTX_OUT_STORAGE(DNNL_ARG_DST);
    auto &ws = CTX_OUT_STORAGE(DNNL_ARG_WORKSPACE);

    const auto &jbn = ker_->jbn;

    auto *mean_ptr = &mean_;
    auto *variance_ptr = &variance_;
    if (jbn.use_16mb_unroll && jbn.calculate_stats && !jbn.save_stats) {
        mean_ptr = temp_reduce.get();
        variance_ptr = temp_reduce.get();
    }

    auto &mean = *mean_ptr;
    auto &variance = *variance_ptr;

    if (jbn.use_16mb_unroll && jbn.calculate_stats) {
        status_t status;

        compute::kernel_arg_list_t calc_mean_arg_list;
        calc_mean_arg_list.set(0, src);
        calc_mean_arg_list.set(1, *temp_reduce);

        auto nd_range_mean = compute::nd_range_t(
                {jbn.sp_chunk, jbn.mb_chunk, jbn.ic}, {1, 1, 16});
        status = compute_stream->parallel_for(
                nd_range_mean, calculate_mean_kernel_, calc_mean_arg_list);
        if (status != status::success) return status;

        compute::kernel_arg_list_t reduce_mean_arg_list;
        reduce_mean_arg_list.set(0, *temp_reduce);
        reduce_mean_arg_list.set(1, mean);

        status = compute_stream->parallel_for(
                compute::nd_range_t({jbn.ic}, {1}), reduce_mean_kernel_,
                reduce_mean_arg_list);
        if (status != status::success) return status;

        compute::kernel_arg_list_t calc_var_arg_list;
        calc_var_arg_list.set(0, src);
        calc_var_arg_list.set(1, mean);
        calc_var_arg_list.set(2, *temp_reduce);

        auto nd_range_calculate_variance = compute::nd_range_t(
                {jbn.sp_chunk, jbn.mb_chunk, jbn.ic}, {1, 1, 16});
        status = compute_stream->parallel_for(nd_range_calculate_variance,
                calculate_variance_kernel_, calc_var_arg_list);
        if (status != status::success) return status;

        compute::kernel_arg_list_t reduce_var_arg_list;
        reduce_var_arg_list.set(0, *temp_reduce);
        reduce_var_arg_list.set(1, variance);

        auto nd_range_reduce_variance = compute::nd_range_t({jbn.ic}, {1});
        status = compute_stream->parallel_for(nd_range_reduce_variance,
                reduce_variance_kernel_, reduce_var_arg_list);
        if (status != status::success) return status;
    }

    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, src);
    arg_list.set(1, mean);
    arg_list.set(2, variance);
    arg_list.set(3, dst);
    arg_list.set(4, scaleshift);
    arg_list.set(5, ws);
    arg_list.set(6, jbn.eps);

    auto nd_range_kernel = compute::nd_range_t(jbn.gws_d, jbn.lws_d);
    status_t status
            = compute_stream->parallel_for(nd_range_kernel, kernel_, arg_list);

    return status;
}

status_t ref_batch_normalization_bwd_t::execute_backward(
        const exec_ctx_t &ctx) const {

    auto *compute_stream
            = utils::downcast<compute::compute_stream_t *>(ctx.stream());

    auto &src = CTX_IN_STORAGE(DNNL_ARG_SRC);
    auto &mean = CTX_IN_STORAGE(DNNL_ARG_MEAN);
    auto &variance = CTX_IN_STORAGE(DNNL_ARG_VARIANCE);
    auto &diff_dst = CTX_IN_STORAGE(DNNL_ARG_DIFF_DST);
    auto &scaleshift = CTX_IN_STORAGE(DNNL_ARG_SCALE_SHIFT);
    auto &ws = CTX_IN_STORAGE(DNNL_ARG_WORKSPACE);

    auto &diff_src = CTX_OUT_STORAGE(DNNL_ARG_DIFF_SRC);
    auto &diff_scaleshift_ = CTX_OUT_STORAGE(DNNL_ARG_DIFF_SCALE_SHIFT);

    const auto &jbn = ker_->jbn;

    auto &diff_scaleshift = (jbn.use_16mb_unroll && !jbn.diff_scaleshift)
            ? *temp_reduce
            : diff_scaleshift_;

    if (jbn.use_16mb_unroll) {
        status_t status;

        compute::kernel_arg_list_t calc_stats_arg_list;
        calc_stats_arg_list.set(0, src);
        calc_stats_arg_list.set(1, mean);
        calc_stats_arg_list.set(2, diff_dst);
        calc_stats_arg_list.set(3, ws);
        calc_stats_arg_list.set(4, *temp_reduce);

        auto nd_range = compute::nd_range_t(
                {jbn.sp_chunk, jbn.mb_chunk, jbn.ic}, {1, 1, 16});
        status = compute_stream->parallel_for(
                nd_range, calculate_stats_kernel_, calc_stats_arg_list);
        if (status != status::success) return status;

        compute::kernel_arg_list_t reduce_stats_arg_list;
        reduce_stats_arg_list.set(0, *temp_reduce);
        reduce_stats_arg_list.set(1, diff_scaleshift);
        reduce_stats_arg_list.set(2, variance);
        reduce_stats_arg_list.set(3, jbn.eps);

        status = compute_stream->parallel_for(
                compute::nd_range_t({jbn.ic}, {1}), reduce_stats_kernel_,
                reduce_stats_arg_list);
        if (status != status::success) return status;
    }

    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, src);
    arg_list.set(1, mean);
    arg_list.set(2, variance);
    arg_list.set(3, diff_dst);
    arg_list.set(4, scaleshift);
    arg_list.set(5, ws);
    arg_list.set(6, diff_src);
    arg_list.set(7, diff_scaleshift);
    arg_list.set(8, jbn.eps);

    auto kernel_nd_range = compute::nd_range_t(jbn.gws_d, jbn.lws_d);
    status_t status
            = compute_stream->parallel_for(kernel_nd_range, kernel_, arg_list);

    return status;
}

} // namespace ocl
} // namespace impl
} // namespace dnnl

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

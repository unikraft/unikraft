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
#include "common/dnnl_traits.hpp"
#include "common/float16.hpp"
#include "common/type_helpers.hpp"

#include "ocl/jit_gen9_gemm.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

template <data_type_t c_type, bool nocopy>
struct jit_gen9_gemm_driver_params {};

template <>
struct jit_gen9_gemm_driver_params<data_type::f32, false> {
    static constexpr auto block_m = 512 * 16;
    static constexpr auto block_n = 64 * 32;
    static constexpr auto block_k = 1024;
};

template <>
struct jit_gen9_gemm_driver_params<data_type::f16, false> {
    static constexpr auto block_m = 512 * 16;
    static constexpr auto block_n = 64 * 32;
    static constexpr auto block_k = 2048;
};

template <data_type_t c_type>
struct jit_gen9_gemm_driver_params<c_type, true> {
    static constexpr auto block_m = 4096;
    static constexpr auto block_n = 2048;
    static constexpr auto block_k = 2048;
};

union plan_element_t {
    struct {
        int i0 : 31;
        int kid0 : 1;
        int j0 : 31;
        int kid1 : 1;
    };
    struct {
        int32_t next_id;
        int32_t _;
    };
};
static_assert(sizeof(plan_element_t) == 8,
        "Plan element structure has been padded by the compiler.");

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::launch_beta(
        compute::compute_stream_t *compute_stream, int64_t m, int64_t n,
        c_t alpha, const memory_storage_t &a, int64_t offset_a,
        int64_t lda) const {
    assert(beta_kernel_);

    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, m);
    arg_list.set(1, n);
    arg_list.set(2, alpha);
    arg_list.set(3, a);
    arg_list.set(4, offset_a);
    arg_list.set(5, lda);

    size_t gws[3] = {1, size_t(n), 1};
    size_t lws[3] = {1, 1, 1};
    auto nd_range = compute::nd_range_t(gws, lws);

    return compute_stream->parallel_for(nd_range, beta_kernel_, arg_list);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::launch_copy(
        compute::compute_stream_t *compute_stream, int64_t x, int64_t y,
        const memory_storage_t &a, int64_t offset_a, int64_t lda, c_t alpha,
        const memory_storage_t &b, int64_t offset_b, bool outer,
        bool trans) const {
    auto &kernel = copy_kernel_[outer][trans];

    assert(kernel);
    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, x);
    arg_list.set(1, y);
    arg_list.set(2, a);
    arg_list.set(3, offset_a);
    arg_list.set(4, lda);
    arg_list.set(5, alpha);
    arg_list.set(6, b);
    arg_list.set(7, offset_b);

    int unroll_m, unroll_n;
    jit_gen9_gemm_copy_kernel<c_type>::get_unrolls(unroll_m, unroll_n);

    auto unroll = outer ? unroll_n : unroll_m;

    size_t gws[3] = {size_t(x), size_t((y + unroll - 1) / unroll), 1};
    size_t lws[3] = {1, 1, 1};

    auto nd_range = compute::nd_range_t(gws, lws);

    return compute_stream->parallel_for(nd_range, kernel, arg_list);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::launch_compute(
        compute::compute_stream_t *compute_stream, int64_t m, int64_t n,
        int64_t k, const memory_storage_t &base, int32_t offset_a,
        int32_t offset_b, const memory_storage_t &c, int64_t offset_c,
        int64_t ldc, bool beta0) const {
    auto &kernel = compute_kernel_[beta0];

    assert(kernel);
    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, m);
    arg_list.set(1, n);
    arg_list.set(2, k);
    arg_list.set(3, base);
    arg_list.set(4, offset_a);
    arg_list.set(5, offset_b);
    arg_list.set(6, c);
    arg_list.set(7, offset_c);
    arg_list.set(8, ldc);

    int unroll_m, unroll_n;
    jit_gen9_gemm_compute_kernel<c_type>::get_unrolls(unroll_m, unroll_n);

    int nthreads_x = (m + unroll_m - 1) / unroll_m;
    int nthreads_y = (n + unroll_n - 1) / unroll_n;

    int lws_y = 8;
    while (nthreads_y % lws_y)
        lws_y--;

    size_t gws[3] = {size_t(nthreads_x) * 8, size_t(nthreads_y), 1};
    size_t lws[3] = {8, size_t(lws_y), 1};

    if (c_type == data_type::f16) lws[1] = 1;

    auto nd_range = compute::nd_range_t(gws, lws);

    return compute_stream->parallel_for(nd_range, kernel, arg_list);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::launch_nocopy(
        compute::compute_stream_t *compute_stream, const memory_storage_t &a,
        const memory_storage_t &b, const memory_storage_t &c, int64_t offset_a,
        int64_t offset_b, int64_t offset_c, int32_t lda, int32_t ldb,
        int32_t ldc, int32_t m, int32_t n, int32_t k, c_t alpha, c_t beta,
        int last_k_block, c_t eltwise_alpha, c_t eltwise_beta) const {

    auto &kernel = nocopy_kernel_;

    assert(kernel);
    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, a);
    arg_list.set(1, b);
    arg_list.set(2, c);
    arg_list.set(3, offset_a);
    arg_list.set(4, offset_b);
    arg_list.set(5, offset_c);
    arg_list.set(6, lda);
    arg_list.set(7, ldb);
    arg_list.set(8, ldc);
    arg_list.set(9, m);
    arg_list.set(10, n);
    arg_list.set(11, k);
    arg_list.set(12, alpha);
    arg_list.set(13, beta);
    arg_list.set(14, last_k_block);
    arg_list.set(15, eltwise_alpha);
    arg_list.set(16, eltwise_beta);

    bool transa = (pd()->desc()->transa == dnnl_trans);
    bool transb = (pd()->desc()->transb == dnnl_trans);

    int unroll_m, unroll_n;

    jit_gen9_gemm_nocopy_kernel<c_type>::get_unrolls(
            transa, transb, unroll_m, unroll_n);

    size_t nthreads_x = (n + unroll_n - 1) / unroll_n;
    size_t nthreads_y = (m + unroll_m - 1) / unroll_m;

    size_t lthreads_x = 2;
    size_t lthreads_y = 8;

#ifndef CL_VERSION_2_0
    while (nthreads_x % lthreads_x)
        lthreads_x--;
    while (nthreads_y % lthreads_y)
        lthreads_y--;
#endif

    static constexpr size_t subgroup_size = 16;
    size_t gws[3] = {nthreads_x * subgroup_size, nthreads_y, 1};
    size_t lws[3] = {lthreads_x * subgroup_size, lthreads_y, 1};

    auto nd_range = compute::nd_range_t(gws, lws);
    return compute_stream->parallel_for(nd_range, kernel, arg_list);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::launch_nocopy_superkernel(
        compute::compute_stream_t *compute_stream, const memory_storage_t &plan,
        int32_t threads, const memory_storage_t &a, const memory_storage_t &b,
        const memory_storage_t &c, int64_t offset_a, int64_t offset_b,
        int64_t offset_c, int32_t lda, int32_t ldb, int32_t ldc, int32_t m,
        int32_t n, int32_t k, c_t alpha, c_t beta, int last_k_block,
        c_t eltwise_alpha, c_t eltwise_beta) const {

    auto &kernel = nocopy_superkernel_;

    assert(kernel);
    compute::kernel_arg_list_t arg_list;
    arg_list.set(0, plan);
    arg_list.set(1, threads);
    arg_list.set(2, a);
    arg_list.set(3, b);
    arg_list.set(4, c);
    arg_list.set(5, offset_a);
    arg_list.set(6, offset_b);
    arg_list.set(7, offset_c);
    arg_list.set(8, lda);
    arg_list.set(9, ldb);
    arg_list.set(10, ldc);
    arg_list.set(11, m);
    arg_list.set(12, n);
    arg_list.set(13, k);
    arg_list.set(14, alpha);
    arg_list.set(15, beta);
    arg_list.set(16, last_k_block);
    arg_list.set(17, eltwise_alpha);
    arg_list.set(18, eltwise_beta);

    size_t lthreads = nstl::min(hw_threads_, threads);

    static constexpr size_t subgroup_size = 16;
    size_t lws[3] = {subgroup_size, 1, 1};
    size_t gws[3] = {lthreads * subgroup_size, 1, 1};

    auto nd_range = compute::nd_range_t(gws, lws);
    return compute_stream->parallel_for(nd_range, kernel, arg_list);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
size_t jit_gen9_gemm_t<a_type, b_type, c_type>::max_plan_size() const {

    auto m = pd()->desc()->m;
    auto n = pd()->desc()->n;
    bool transa = (pd()->desc()->transa == dnnl_trans);
    bool transb = (pd()->desc()->transb == dnnl_trans);

    int unroll_m[2], unroll_n;
    jit_gen9_gemm_nocopy_superkernel<c_type>::get_unrolls(
            transa, transb, unroll_m, unroll_n);

    auto max_threads
            = utils::div_up(m, unroll_m[1]) * utils::div_up(n, unroll_n);

    return sizeof(plan_element_t) * (max_threads + 1);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::execute(
        const exec_ctx_t &ctx) const {
    if (gemm_type_ == type::no_copy_superkernel)
        return execute_superkernel(ctx);
    else
        return execute_standard(ctx);
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::execute_standard(
        const exec_ctx_t &ctx) const {

    auto *compute_stream
            = utils::downcast<compute::compute_stream_t *>(ctx.stream());

    using a_t = typename prec_traits<a_type>::type;
    using b_t = typename prec_traits<b_type>::type;
    using c_t = typename prec_traits<c_type>::type;

    auto m = pd()->desc()->m;
    auto n = pd()->desc()->n;
    auto k = pd()->desc()->k;

    bool transa = (pd()->desc()->transa == dnnl_trans);
    bool transb = (pd()->desc()->transb == dnnl_trans);

    auto lda = pd()->desc()->lda;
    auto ldb = pd()->desc()->ldb;
    auto ldc = pd()->desc()->ldc;

    auto alpha = pd()->desc()->alpha;
    auto beta = pd()->desc()->beta;

    auto eltwise_alpha = pd()->eltwise_alpha();
    auto eltwise_beta = pd()->eltwise_beta();

    c_t alpha_native, beta_native, one_native;
    alpha_native = alpha;
    beta_native = beta;
    one_native = 1.0f;

    auto &a = CTX_IN_STORAGE(DNNL_ARG_SRC_0);
    auto &b = CTX_IN_STORAGE(DNNL_ARG_SRC_1);
    auto &c = CTX_OUT_STORAGE(DNNL_ARG_DST);

    size_t off_a0 = a.get_offset() / sizeof(a_t) + pd()->dyn_offset_a;
    size_t off_b0 = b.get_offset() / sizeof(b_t) + pd()->dyn_offset_b;
    size_t off_c0 = c.get_offset() / sizeof(c_t) + pd()->dyn_offset_c;

    bool nocopy = (gemm_type_ == type::no_copy)
            || (gemm_type_ == type::no_copy_if_even_off && !(off_a0 & 1)
                    && !(off_b0 & 1));

    status_t status;
    constexpr int64_t align = 0x1000;
    int block_m, block_n, block_k;
    if (!nocopy) {
        block_m = jit_gen9_gemm_driver_params<c_type, false>::block_m;
        block_n = jit_gen9_gemm_driver_params<c_type, false>::block_n;
        block_k = jit_gen9_gemm_driver_params<c_type, false>::block_k;
    } else {
        block_m = jit_gen9_gemm_driver_params<c_type, true>::block_m;
        block_n = jit_gen9_gemm_driver_params<c_type, true>::block_n;
        block_k = jit_gen9_gemm_driver_params<c_type, true>::block_k;
    }

    if (!nocopy && beta != 0. && beta != 1.) {
        status = launch_beta(compute_stream, m, n, beta_native, c, off_c0, ldc);
        if (status) return status;
    }

    int64_t off_b_packed = 0;
    int64_t off_a_packed
            = ((off_b_packed + block_n * block_k) + align - 1) & -align;

    for (int64_t Bk = 0; Bk < k; Bk += block_k) {
        int64_t size_k = k - Bk;
        bool last_k_block = (size_k <= block_k);
        if (!last_k_block) size_k = block_k;

        for (int64_t Bm = 0; Bm < m; Bm += block_m) {
            int64_t size_m = m - Bm;
            if (size_m > block_m) size_m = block_m;

            auto off_a_src
                    = off_a0 + (!transa ? (Bm + Bk * lda) : (Bk + Bm * lda));

            if (!nocopy) {
                status = launch_copy(compute_stream, size_k, size_m, a,
                        off_a_src, lda, alpha_native, *temp_buf_, off_a_packed,
                        false, !transa);
                if (status) return status;
            }

            for (int64_t Bn = 0; Bn < n; Bn += block_n) {
                int64_t size_n = n - Bn;
                if (size_n > block_n) size_n = block_n;

                auto off_b_src = off_b0
                        + (!transb ? (Bk + Bn * ldb) : (Bn + Bk * ldb));

                if (!nocopy && ((Bn == 0) || (n > block_n))) {
                    status = launch_copy(compute_stream, size_k, size_n, b,
                            off_b_src, ldb, one_native, *temp_buf_,
                            off_b_packed, true, transb);
                    if (status) return status;
                }

                auto off_c = off_c0 + Bm + Bn * ldc;

                if (nocopy) {
                    float eff_beta = (Bk == 0) ? beta : 1.0f;
                    status = launch_nocopy(compute_stream, a, b, c, off_a_src,
                            off_b_src, off_c, lda, ldb, ldc, size_m, size_n,
                            size_k, alpha, eff_beta, (int)last_k_block,
                            eltwise_alpha, eltwise_beta);
                } else {
                    bool beta0 = (beta == 0) && (Bk == 0);
                    status = launch_compute(compute_stream, size_m, size_n,
                            size_k, *temp_buf_, off_a_packed, off_b_packed, c,
                            off_c, ldc, beta0);
                }
                if (status) return status;
            }
        }
    }

    return status::success;
}

template <data_type_t a_type, data_type_t b_type, data_type_t c_type>
status_t jit_gen9_gemm_t<a_type, b_type, c_type>::execute_superkernel(
        const exec_ctx_t &ctx) const {

    auto *compute_stream
            = utils::downcast<compute::compute_stream_t *>(ctx.stream());

    using a_t = typename prec_traits<a_type>::type;
    using b_t = typename prec_traits<b_type>::type;
    using c_t = typename prec_traits<c_type>::type;

    auto m = pd()->desc()->m;
    auto n = pd()->desc()->n;
    auto k = pd()->desc()->k;

    bool transa = (pd()->desc()->transa == dnnl_trans);
    bool transb = (pd()->desc()->transb == dnnl_trans);

    auto lda = pd()->desc()->lda;
    auto ldb = pd()->desc()->ldb;
    auto ldc = pd()->desc()->ldc;

    auto alpha = pd()->desc()->alpha;
    auto beta = pd()->desc()->beta;

    auto eltwise_alpha = pd()->eltwise_alpha();
    auto eltwise_beta = pd()->eltwise_beta();

    auto &a = CTX_IN_STORAGE(DNNL_ARG_SRC_0);
    auto &b = CTX_IN_STORAGE(DNNL_ARG_SRC_1);
    auto &c = CTX_OUT_STORAGE(DNNL_ARG_DST);

    size_t off_a = a.get_offset() / sizeof(a_t) + pd()->dyn_offset_a;
    size_t off_b = b.get_offset() / sizeof(b_t) + pd()->dyn_offset_b;
    size_t off_c = c.get_offset() / sizeof(c_t) + pd()->dyn_offset_c;

    int unroll_m[2], unroll_n;
    jit_gen9_gemm_nocopy_superkernel<c_type>::get_unrolls(
            transa, transb, unroll_m, unroll_n);

    int km = utils::div_up(m, unroll_m[0]);
    int kn = utils::div_up(n, unroll_n);
    int last_ldispatch = 0;
    int km_large = km;
    int best_km_large = 0;

    auto good_enough = [=](int ldispatch, int threads) -> bool {
        return (threads < hw_threads_) && (ldispatch >= eu_count_ * 3);
    };

    while (km_large >= 0) {
        int km_small = utils::div_up(m - (km_large * unroll_m[0]), unroll_m[1]);
        km_small = nstl::max(0, km_small);

        auto threads = (km_large + km_small) * kn;
        auto ldispatch = threads % hw_threads_;

        if (ldispatch == 0 || good_enough(ldispatch, threads)) {
            best_km_large = km_large;
            break;
        } else if (ldispatch < last_ldispatch)
            break;
        else if (ldispatch > last_ldispatch)
            best_km_large = km_large;

        last_ldispatch = ldispatch;
        km_large--;
    }

    km_large = best_km_large;

    int km_small = utils::div_up(m - (km_large * unroll_m[0]), unroll_m[1]);
    km_small = nstl::max(0, km_small);

    km = km_small + km_large;

    auto n_block_target = nstl::max<int>(1, 128 / unroll_n);
    auto columns = utils::div_up(kn, n_block_target);
    auto kn_left = (n_block_target - kn) % n_block_target;
    if (kn_left < 0) kn_left += n_block_target;
    auto spread = nstl::min(kn_left, columns);

    int bn0, bn1, columns_small;
    if (spread == columns) {
        bn0 = utils::div_up(kn, columns);
        bn1 = bn0 - 1;
        columns_small = (bn0 * columns) - kn;
    } else {
        bn0 = n_block_target;
        bn1 = n_block_target - 1;
        columns_small = spread;
    }

    void *plan_void;
    temp_buf_->map_data(&plan_void);

    if (!plan_void) return status::runtime_error;

    auto plan = (plan_element_t *)plan_void;

    plan[0].next_id = hw_threads_;
    plan[0]._ = 0;

    int p = 1, j0 = 0;
    for (int column = 0; column < columns; column++) {
        auto bn = (column >= (columns - columns_small)) ? bn1 : bn0;
        int i0 = 0;
        for (int ki = 0; ki < km; ki++) {
            int m_idx = (ki >= (km - km_small));
            for (int bj = 0; bj < bn; bj++, p++) {
                plan[p].i0 = i0;
                plan[p].j0 = j0 + bj * unroll_n;
                plan[p].kid0 = m_idx;
                plan[p].kid1 = 0;
            }
            auto um = m_idx ? unroll_m[1] : unroll_m[0];
            i0 += um;
        }
        j0 += bn * unroll_n;
    }

    temp_buf_->unmap_data(plan_void);

    int32_t threads = km * kn;
    bool last_k_block = true;

    return launch_nocopy_superkernel(compute_stream, *temp_buf_, threads, a, b,
            c, off_a, off_b, off_c, lda, ldb, ldc, m, n, k, alpha, beta,
            (int)last_k_block, eltwise_alpha, eltwise_beta);
}

using namespace data_type;

template struct jit_gen9_gemm_t<f16>;
template struct jit_gen9_gemm_t<f32>;

} // namespace ocl
} // namespace impl
} // namespace dnnl

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

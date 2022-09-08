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

#ifndef JIT_GEN9_GEMM_KERNEL_HPP
#define JIT_GEN9_GEMM_KERNEL_HPP

#include "common/c_types_map.hpp"
#include "compute/compute.hpp"
#include "ocl/jit_primitive_conf.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

struct jit_gen9_gemm_kernel {
    template <impl::data_type_t type>
    static status_t init_cl_options(compute::kernel_ctx_t &kernel_ctx) {
        using namespace data_type;

        if (type != f32 && type != f16) return status::unimplemented;

        kernel_ctx.define_int("DT_F32", type == f32);
        kernel_ctx.define_int("DT_F16", type == f16);

        kernel_ctx.add_option("-cl-mad-enable");
        kernel_ctx.add_option("-cl-strict-aliasing");
#ifdef CL_VERSION_2_0
        kernel_ctx.add_option("-cl-std=CL2.0");
#else
        kernel_ctx.add_option("-Dget_enqueued_local_size=get_local_size");
#endif
        return status::success;
    }

    struct copy_params {
        static constexpr auto unroll_m = 16;
        static constexpr auto unroll_n = 32;
    };
};

template <impl::data_type_t type>
struct jit_gen9_gemm_beta_kernel : public jit_gen9_gemm_kernel {
    static status_t init_const_def(compute::kernel_ctx_t &kernel_ctx) {
        auto status = init_cl_options<type>(kernel_ctx);
        if (status) return status;

        kernel_ctx.print_options();
        return status::success;
    }
};

template <impl::data_type_t type>
struct jit_gen9_gemm_copy_kernel : public jit_gen9_gemm_kernel {
    static status_t init_const_def(
            compute::kernel_ctx_t &kernel_ctx, bool outer, bool trans) {
        auto status = init_cl_options<type>(kernel_ctx);
        if (status) return status;

        kernel_ctx.define_int("COPY_UNROLL",
                !outer ? copy_params::unroll_m : copy_params::unroll_n);

        kernel_ctx.add_option(trans ? "-DUSE_TRANS" : "-DUSE_NOTRANS");

        kernel_ctx.print_options();
        return status::success;
    }

    static void get_unrolls(int &unroll_m, int &unroll_n) {
        unroll_m = copy_params::unroll_m;
        unroll_n = copy_params::unroll_n;
    }
};

template <impl::data_type_t type>
struct jit_gen9_gemm_compute_kernel : public jit_gen9_gemm_kernel {
    static status_t init_const_def(
            compute::kernel_ctx_t &kernel_ctx, bool beta0) {
        auto status = init_cl_options<type>(kernel_ctx);
        if (status) return status;

        if (beta0) kernel_ctx.add_option("-DBETA_ZERO");

        kernel_ctx.define_int("UNROLL_M", copy_params::unroll_m);
        kernel_ctx.define_int("UNROLL_N", copy_params::unroll_n);

        kernel_ctx.print_options();
        return status::success;
    }

    static void get_unrolls(int &unroll_m, int &unroll_n) {
        unroll_m = copy_params::unroll_m;
        unroll_n = copy_params::unroll_n;
    }
};

template <impl::data_type_t type>
struct jit_gen9_gemm_nocopy_kernel : public jit_gen9_gemm_kernel {
    static status_t init_const_def(compute::kernel_ctx_t &kernel_ctx,
            bool trans_a, bool trans_b, bool with_eltwise, alg_kind_t alg) {

        auto status = init_cl_options<type>(kernel_ctx);
        if (status) return status;

        if (trans_a) kernel_ctx.add_option("-DTRANS_A");
        if (trans_b) kernel_ctx.add_option("-DTRANS_B");

        kernel_ctx.define_int("WITH_ELTWISE", with_eltwise);
        if (with_eltwise) def_postops(kernel_ctx, alg);

        kernel_ctx.print_options();
        return status::success;
    }

    static void get_unrolls(
            bool trans_a, bool trans_b, int &unroll_m, int &unroll_n) {

        unroll_m = unroll_n = 0;

        if (type == data_type::f32) {
            static constexpr int unroll_m_table[2][2] = {{32, 32}, {16, 16}};
            static constexpr int unroll_n_table[2][2] = {{16, 16}, {32, 32}};

            unroll_m = unroll_m_table[trans_a][trans_b];
            unroll_n = unroll_n_table[trans_a][trans_b];
        } else if (type == data_type::f16)
            unroll_m = unroll_n = 32;
    }
};

template <impl::data_type_t type>
struct jit_gen9_gemm_nocopy_superkernel : public jit_gen9_gemm_kernel {
    static status_t init_const_def(compute::kernel_ctx_t &kernel_ctx,
            bool trans_a, bool trans_b, bool with_eltwise, alg_kind_t alg) {

        if (trans_a) return status::unimplemented;

        return jit_gen9_gemm_nocopy_kernel<type>::init_const_def(
                kernel_ctx, trans_a, trans_b, with_eltwise, alg);
    }

    static void get_unrolls(
            bool trans_a, bool trans_b, int (&unroll_m)[2], int &unroll_n) {

        unroll_m[0] = 32;
        unroll_m[1] = 16;
        unroll_n = 16;
    }
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif

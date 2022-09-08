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

#ifndef BLAS_STRUCTURE_HPP
#define BLAS_STRUCTURE_HPP

#include <cstdint>
#include <memory>
#include "c_types_map.hpp"
#include "gemm_pack_storage.hpp"
#include "gemm_threading.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

enum class pack_type { none, pack_a, pack_b };

enum class offset_type {
    none,
    fixed,
    column,
    row,
};

// Indices for kernel arrays. TODO Is it okay to place this here?
enum { no_sum = 0, do_sum = 1 };
enum { no_trans = 0, do_trans = 1, packed = 2 };
enum { no_beta0 = 0, do_beta0 = 1 };
enum { no_alpha1 = 0, do_alpha1 = 1 };
enum { no_col_offset = 0, do_col_offset = 1 };
enum { no_row_offset = 0, do_row_offset = 1 };

template <typename a_type, typename b_type, typename c_type>
struct gemm_info_t {

    // Interface arguments.
    int transa, transb;
    offset_type offsetc;
    dim_t m, n, k;
    dim_t lda, ldb, ldc;
    const a_type *a;
    const b_type *b;
    c_type *c;
    float alpha, beta;

    a_type ao;
    uint8_t bo;
    const c_type *co;

    pack_type packing;
    gemm_pack_storage_t *pack_dst;
    bool measure_only;
    std::shared_ptr<const gemm_pack_storage_t> a_packed, b_packed;

    // Kernel parameters.
    dim_t um, un, uk, bm, bn, bk;
    dim_t bn_small_k, bk_traditional, blocking_small_k;

    void (*copyA)(const dim_t *m, const dim_t *n, const a_type *a,
            const dim_t *lda, const float *alpha, a_type *b,
            const dim_t *dummy1, const dim_t *dummy2, c_type *row_col_sum);

    void (*copyB)(const dim_t *m, const dim_t *n, const b_type *a,
            const dim_t *lda, const float *alpha, b_type *b,
            const dim_t *dummy1, const dim_t *dummy2, c_type *row_col_sum);

    void (*kernel[2][2][2])(const dim_t *m, const dim_t *n, const dim_t *k,
            const float *alpha, const a_type *a, const b_type *b, c_type *c,
            const dim_t ldc, const c_type *col_offset,
            const c_type *row_offset);

    // Gemv kernels
    void (*gemv_kernel[2])(const dim_t *m, const dim_t *n, const float *alpha,
            const a_type *a, const dim_t *lda, const b_type *x,
            const dim_t *incy, c_type *y);

    void (*gemv_s8s8s32_kernel)(const dim_t, const dim_t, const float,
            const int8_t *, const dim_t, const int8_t *, const float,
            int32_t *);

    void (*gemv_s8u8s32_kernel)(const dim_t, const dim_t, const float,
            const int8_t *, const dim_t, const uint8_t *, const float,
            int32_t *);

    void (*gemv_u8s8s32_kernel)(const dim_t, const dim_t, const float,
            const uint8_t *, const dim_t, const int8_t *, const float,
            int32_t *);

    // Gemv parameters
    int swap;

    bool force_nocopy;

    gemm_info_t(const char *transA, const char *transB, const char *offsetC,
            const int *m, const int *n, const int *k, const float *alpha,
            const a_type *a, const int *lda, const a_type *oa, const b_type *b,
            const int *ldb, const b_type *ob, const float *beta, c_type *c,
            const int *ldc, const c_type *oc, bool force_nocopy,
            pack_type packing, gemm_pack_storage_t *pack_dst,
            bool measure_only);

    bool hasKernels(void);

    void update_blocking(const gemm_threading_t &thread_info);

private:
    void jit_init(void);
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif // BLAS_STRUCTURE_HPP

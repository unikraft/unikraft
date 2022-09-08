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

#include "dnnl_thread.hpp"
#include "dnnl_types.h"

#include "gemm_pack.hpp"

#include "cpu_isa_traits.hpp"

#include "gemm.hpp"
#include "gemm_driver.hpp"
#include "os_blas.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

#if USE_MKL_PACKED_GEMM
static inline CBLAS_IDENTIFIER cblas_identifier(const char *identifier) {
    return utils::one_of(*identifier, 'a', 'A') ? CblasAMatrix : CblasBMatrix;
}

static inline CBLAS_TRANSPOSE cblas_transpose(const char *trans) {
    return utils::one_of(*trans, 'n', 'N') ? CblasNoTrans : CblasTrans;
}

static inline MKL_INT cblas_storage(const char *trans) {
    switch (*trans) {
        case 'N':
        case 'n': return CblasNoTrans;
        case 'T':
        case 't': return CblasTrans;
        default: return CblasPacked;
    }
}

static inline CBLAS_OFFSET cblas_offset(const char *offset) {
    switch (*offset) {
        case 'R':
        case 'r': return CblasRowOffset;
        case 'C':
        case 'c': return CblasColOffset;
        default: return CblasFixOffset;
    }
}
#endif

#if !USE_MKL_PACKED_GEMM
static inline bool use_reference_igemm() {
    return !(mayiuse(avx512_core) || mayiuse(avx512_core_vnni));
}

template <typename T>
static bool is_good_ld(dim_t ld) {
    static constexpr auto align = 64 / sizeof(T);
    static constexpr auto no_align = 2048 / sizeof(T);

    return ((ld % align) == 0) && ((ld % no_align) != 0);
}
#endif

static dnnl_status_t check_pack_get_size_input(const char *identifier,
        const char *transa, const char *transb, const int *M, const int *N,
        const int *K, const int *lda, const int *ldb) {

    if (utils::any_null(identifier, transa, transb, M, N, K, lda, ldb))
        return dnnl_invalid_arguments;

    bool is_transa = utils::one_of(*transa, 'T', 't');
    bool is_transb = utils::one_of(*transb, 'T', 't');

    bool ok = true && utils::one_of(*transa, 'T', 't', 'N', 'n')
            && utils::one_of(*transb, 'T', 't', 'N', 'n')
            && utils::one_of(*identifier, 'A', 'a', 'B', 'b') && *M >= 0
            && *N >= 0 && *K >= 0 && *lda >= nstl::max(1, !is_transa ? *M : *K)
            && *ldb >= nstl::max(1, !is_transb ? *K : *N);

    if (!ok) return dnnl_invalid_arguments;

    return dnnl_success;
}

static dnnl_status_t check_pack_input(const char *identifier,
        const char *transa, const char *transb, const int *M, const int *N,
        const int *K, const float *alpha, const int *lda, const int *ldb,
        const void *src, void *dst) {
    if (utils::any_null(src, dst, alpha)) return dnnl_invalid_arguments;

    return check_pack_get_size_input(
            identifier, transa, transb, M, N, K, lda, ldb);
}

template <typename a_dt, typename b_dt, typename c_dt>
static dnnl_status_t gemm_pack_driver(const char *identifier,
        const char *transa, const char *transb, const int *M, const int *N,
        const int *K, const float *alpha, const int *lda, const int *ldb,
        const void *src, gemm_pack_storage_t *pack_dst, bool measure_only) {

    a_dt oa = 0;
    b_dt ob = 0;

    const a_dt *a = nullptr;
    const b_dt *b = nullptr;
    pack_type packing;

    if (utils::one_of(*identifier, 'a', 'A')) {
        a = (const a_dt *)src;
        packing = pack_type::pack_a;
    } else {
        b = (const b_dt *)src;
        packing = pack_type::pack_b;
    }

    return gemm_driver<a_dt, b_dt, c_dt>(transa, transb, "N", M, N, K, alpha, a,
            lda, &oa, b, ldb, &ob, nullptr, nullptr, nullptr, nullptr, false,
            packing, pack_dst, measure_only);
}

dnnl_status_t sgemm_pack_get_size(const char *identifier, const char *transa,
        const char *transb, const int *M, const int *N, const int *K,
        const int *lda, const int *ldb, size_t *size, bool *pack) {

    if (!pack_sgemm_supported()) return dnnl_unimplemented;

    dnnl_status_t result;
    *size = 0;
    if (pack) *pack = true;

    result = check_pack_get_size_input(
            identifier, transa, transb, M, N, K, lda, ldb);
    if (result != dnnl_success) return result;

#if USE_MKL_PACKED_GEMM
    *size = cblas_sgemm_pack_get_size(cblas_identifier(identifier), *M, *N, *K);
#else
    bool do_a = utils::one_of(*identifier, 'a', 'A');
    float alpha = 1.0f;
    gemm_pack_storage_shell_t shell {dnnl_get_max_threads()};

    result = gemm_pack_driver<float, float, float>(identifier, transa, transb,
            M, N, K, &alpha, lda, ldb, nullptr, &shell, true);
    if (result != dnnl_success) return result;

    *size = shell.size();
    if (pack) {
        *pack = !(shell.single_nocopy()
                && utils::one_of(do_a ? *transa : *transb, 'n', 'N')
                && is_good_ld<float>(do_a ? *lda : *ldb));
    }
#endif

    return dnnl_success;
}

dnnl_status_t gemm_s8u8s32_pack_get_size(const char *identifier,
        const char *transa, const char *transb, const int *M, const int *N,
        const int *K, const int *lda, const int *ldb, size_t *size,
        bool *pack) {

    dnnl_status_t result;
    *size = 0;
    if (pack) *pack = true;

    result = check_pack_get_size_input(
            identifier, transa, transb, M, N, K, lda, ldb);
    if (result != dnnl_success) return result;

#if USE_MKL_PACKED_GEMM
    *size = cblas_gemm_s8u8s32_pack_get_size(
            cblas_identifier(identifier), *M, *N, *K);
#else
    bool do_a = utils::one_of(*identifier, 'a', 'A');
    float alpha = 1.0f;
    gemm_pack_storage_shell_t shell {dnnl_get_max_threads(), do_a, !do_a};

    if (!use_reference_igemm()) {
        result = gemm_pack_driver<int8_t, uint8_t, int32_t>(identifier, transa,
                transb, M, N, K, &alpha, lda, ldb, nullptr, &shell, true);

        if (result != dnnl_success) return result;
    } else {
        auto rows = do_a ? *M : *K;
        auto cols = do_a ? *K : *N;
        prep_ref_gemm_s8u8s32_pack(do_a, rows, cols, &shell);
    }

    *size = shell.size();
    if (pack) {
        *pack = !(shell.single_nocopy()
                && utils::one_of(do_a ? *transa : *transb, 'n', 'N')
                && is_good_ld<float>(do_a ? *lda : *ldb));
    }
#endif

    return dnnl_success;
}

dnnl_status_t sgemm_pack(const char *identifier, const char *transa,
        const char *transb, const int *M, const int *N, const int *K,
        const int *lda, const int *ldb, const float *src, float *dst) {
    float one = 1.f, *alpha = &one;

    if (!pack_sgemm_supported()) return dnnl_unimplemented;

    auto result = check_pack_input(
            identifier, transa, transb, M, N, K, alpha, lda, ldb, src, dst);
    if (result != dnnl_success) return result;

#if USE_MKL_PACKED_GEMM
    auto cblas_id = cblas_identifier(identifier);
    auto ld = (cblas_id == CblasAMatrix) ? *lda : *ldb;
    auto trans = (cblas_id == CblasAMatrix) ? transa : transb;
    cblas_sgemm_pack(CblasColMajor, cblas_id, cblas_transpose(trans), *M, *N,
            *K, *alpha, src, ld, dst);
    return dnnl_success;
#else
    gemm_pack_storage_t pack_dst {dst};

    return gemm_pack_driver<float, float, float>(identifier, transa, transb, M,
            N, K, alpha, lda, ldb, src, &pack_dst, false);
#endif
}

dnnl_status_t gemm_s8u8s32_pack(const char *identifier, const char *transa,
        const char *transb, const int *M, const int *N, const int *K,
        const int *lda, const int *ldb, const void *src, void *dst) {

    float alpha = 1.0f; // Not used with igemm.
    auto result = check_pack_input(
            identifier, transa, transb, M, N, K, &alpha, lda, ldb, src, dst);
    if (result != dnnl_success) return result;

#if USE_MKL_PACKED_GEMM
    auto cblas_id = cblas_identifier(identifier);
    auto ld = (cblas_id == CblasAMatrix) ? *lda : *ldb;
    auto trans = (cblas_id == CblasAMatrix) ? transa : transb;
    cblas_gemm_s8u8s32_pack(CblasColMajor, cblas_id, cblas_transpose(trans), *M,
            *N, *K, src, ld, dst);
    return dnnl_success;
#else
    gemm_pack_storage_t pack_dst {dst};

    if (!use_reference_igemm()) {
        return gemm_pack_driver<int8_t, uint8_t, int32_t>(identifier, transa,
                transb, M, N, K, &alpha, lda, ldb, src, &pack_dst, false);
    } else {
        bool do_a = utils::one_of(*identifier, 'a', 'A');
        bool is_trans = utils::one_of(do_a ? *transa : *transb, 't', 'T');
        auto ld = do_a ? *lda : *ldb;
        auto rows = do_a ? *M : *K;
        auto cols = do_a ? *K : *N;

        prep_ref_gemm_s8u8s32_pack(do_a, rows, cols, &pack_dst);
        return ref_gemm_s8u8s32_pack(src, ld, rows, cols, is_trans, &pack_dst);
    }
#endif
}

dnnl_status_t sgemm_compute(const char *transa, const char *transb,
        const int *M, const int *N, const int *K, const float *A,
        const int *lda, const float *B, const int *ldb, const float *beta,
        float *C, const int *ldc) {

#if USE_MKL_PACKED_GEMM
    if (utils::any_null(transa, transb, M, N, K, A, lda, B, ldb, beta, C, ldc))
        return dnnl_invalid_arguments;
    cblas_sgemm_compute(CblasColMajor, cblas_storage(transa),
            cblas_storage(transb), *M, *N, *K, A, *lda, B, *ldb, *beta, C,
            *ldc);
    return dnnl_success;
#else
    if (!pack_sgemm_supported()) return dnnl_unimplemented;

    float one = 1.0f;

    return extended_sgemm(
            transa, transb, M, N, K, &one, A, lda, B, ldb, beta, C, ldc);
#endif
}

dnnl_status_t gemm_s8u8s32_compute(const char *transa, const char *transb,
        const char *offsetc, const int *M, const int *N, const int *K,
        const int8_t *A, const int *lda, const uint8_t *B, const int *ldb,
        const float *beta, int32_t *C, const int *ldc, const int32_t *co) {
    const float one = 1.f, *alpha = &one;

    const int8_t zero_s8 = 0, *ao = &zero_s8;
    const uint8_t zero_u8 = 0, *bo = &zero_u8;

#if USE_MKL_PACKED_GEMM
    if (utils::any_null(transa, transb, offsetc, M, N, K, alpha, A, lda, ao, B,
                ldb, bo, beta, C, ldc, co))
        return dnnl_invalid_arguments;
    cblas_gemm_s8u8s32_compute(CblasColMajor, cblas_storage(transa),
            cblas_storage(transb), cblas_offset(offsetc), *M, *N, *K, *alpha, A,
            *lda, *ao, B, *ldb, *bo, *beta, C, *ldc, co);
    return dnnl_success;
#else
    auto lda_eff = *lda, ldb_eff = *ldb;
    auto transa_eff = *transa, transb_eff = *transb;

    if (use_reference_igemm()) {
        dim_t ld, td;

        if (transa_eff == 'p' || transa_eff == 'P') {
            gemm_pack_storage_t a_packed {A};
            if (!a_packed.get_nocopy(ld, td)) return dnnl_invalid_arguments;
            A = a_packed.matrix<int8_t>();
            lda_eff = ld;
            transa_eff = 'N';
        }

        if (transb_eff == 'p' || transb_eff == 'P') {
            gemm_pack_storage_t b_packed {B};
            if (!b_packed.get_nocopy(ld, td)) return dnnl_invalid_arguments;
            B = b_packed.matrix<uint8_t>();
            ldb_eff = ld;
            transb_eff = 'N';
        }
    }

    return gemm_s8x8s32(&transa_eff, &transb_eff, offsetc, M, N, K, alpha, A,
            &lda_eff, ao, B, &ldb_eff, bo, beta, C, ldc, co);
#endif
}

} // namespace cpu
} // namespace impl
} // namespace dnnl

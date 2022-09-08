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

#include "dnnl_types.h"

#include "c_types_map.hpp"
#include "common/bfloat16.hpp"
#include "dnnl_thread.hpp"
#include "gemm_bf16_convolution.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

using namespace dnnl::impl::status;
using namespace dnnl::impl::memory_tracking::names;
using namespace dnnl::impl::utils;
using namespace dnnl::impl::cpu::bf16_support;

// This code is moved out from execute_backward_data() and
// execute_backward_weights() to avoid warnings with gcc 7.x compilers:
// "declared with greater visibility than the type of its field"
// when one lambda  function is delcared whithin the other one
void store_bfloat16_in_parallel(bfloat16_t *output_data, const float *acc_data,
        size_t parallel_work, size_t parallel_work_size, bool do_in_parallel) {
    parallel(do_in_parallel ? 0 : 1, [&](const int ithr, const int nthr) {
        size_t start = 0, end = 0;
        balance211(parallel_work, nthr, ithr, start, end);
        if (start < end)
            cvt_float_to_bfloat16(&output_data[start * parallel_work_size],
                    &acc_data[start * parallel_work_size],
                    (end - start) * parallel_work_size);
    });
}

template <data_type_t dst_data_type>
gemm_bf16_convolution_fwd_t<dst_data_type>::pp_ker_t::pp_ker_t(const pd_t *pd)
    : ker_(nullptr)
    , jcp_(pd->jcp_)
    , OC_(pd->jcp_.oc)
    , do_bias_(false)
    , do_eltwise_(false)
    , do_sum_(false)
    , max_data_reg_idx_(31)
    , max_unroll_(12)
    , compute_reg_step_(1)
    , data_reg_base_idx_(0)
    , bf16_emu_(nullptr)
    , eltwise_injector_(nullptr) {
    using namespace types;
    using namespace Xbyak;

    if (!mayiuse(avx512_core))
        // bf16 is not supported
        return;

    auto &post_ops = pd->attr()->post_ops_;
    const int eltwise_ind = post_ops.find(primitive_kind::eltwise);
    do_eltwise_ = eltwise_ind != -1;
    if (do_eltwise_)
        eltwise_injector_ = new jit_uni_eltwise_injector_f32<avx512_common>(
                this, post_ops.entry_[eltwise_ind].eltwise, true,
                reserved_eltwise_gpr, reserved_eltwise_maskr);

    do_sum_ = dst_data_type != data_type::f32
            && post_ops.contain(primitive_kind::sum, 0);
    if (do_sum_) {
        compute_reg_step_ = 2;
        vreg_sum_scale = Zmm(data_reg_base_idx_++);
    }

    do_bias_ = pd->with_bias();
    if (do_bias_) vreg_bias = Zmm(data_reg_base_idx_++);

    vlen_ = cpu_isa_traits<avx512_common>::vlen / sizeof(float);

    isa_ = mayiuse(avx512_core_bf16) ? avx512_core_bf16
                                     : bf16_emulation_t::get_isa();

    if (isa_ != avx512_core_bf16) {
        max_data_reg_idx_ = 26;
        bf16_emu_ = new bf16_emulation_t(this, bf16_emu_reserv_1,
                bf16_emu_reserv_2, bf16_emu_reserv_3, bf16_emu_reserv_4,
                bf16_emu_reserv_5, bf16_emu_reserv_6);
    }

    max_unroll_
            = (max_data_reg_idx_ - data_reg_base_idx_ + 1) / compute_reg_step_;

    generate();
}

template <data_type_t dst_data_type>
void gemm_bf16_convolution_fwd_t<dst_data_type>::pp_ker_t::generate() {
    using namespace Xbyak;
    using namespace utils;

    preamble();

#define PARAM_OFF(x) offsetof(ker_args, x)
    mov(reg_dst_base, ptr[reg_param + PARAM_OFF(dst)]);
    mov(reg_acc_base, ptr[reg_param + PARAM_OFF(acc)]);
    if (do_bias_) mov(reg_bias, ptr[reg_param + PARAM_OFF(bias)]);
    mov(reg_dst_str, ptr[reg_param + PARAM_OFF(dst_stride_in_bytes)]);
    mov(reg_acc_str, ptr[reg_param + PARAM_OFF(acc_stride_in_bytes)]);
    mov(reg_len, ptr[reg_param + PARAM_OFF(spatial_length)]);
    mov(reg_oc_iter, ptr[reg_param + PARAM_OFF(oc_work)]);

    if (do_sum_)
        vbroadcastss(vreg_sum_scale, ptr[reg_param + PARAM_OFF(sum_scale)]);
#undef PARAM_OFF

    // Load accumulated value, apply sum (if any), bias (if any)
    // and relu (if any); then convert to destination type and store
    auto compute = [&](size_t offset, int idx, bool apply_mask) {
        auto acc_addr = ptr[reg_acc + offset * sizeof(acc_data_t)];
        auto vreg_dst_ = vreg_dst(idx);

        if (dst_data_type == data_type::bf16 && isa_ != avx512_core_bf16)
            bf16_emu_->init_vcvtneps2bf16();

        if (apply_mask) vreg_dst_ = vreg_dst_ | kreg_rem_mask;
        vmovups(vreg_dst_, acc_addr);

        if (do_bias_) vaddps(vreg_dst(idx), vreg_dst(idx), vreg_bias);

        auto dst_addr = ptr[reg_dst + offset * sizeof(dst_data_t)];
        if (do_sum_) {
            auto vreg_prev_dst_ = vreg_prev_dst(idx);
            if (dst_data_type == data_type::f32) {
                if (apply_mask) vreg_prev_dst_ = vreg_prev_dst_ | kreg_rem_mask;

                vmovups(vreg_prev_dst_, dst_addr);
            } else if (dst_data_type == data_type::bf16) {
                auto vreg_prev_dst_ymm_ = vreg_prev_dst_ymm(idx);
                if (apply_mask)
                    vreg_prev_dst_ymm_ = vreg_prev_dst_ymm_ | kreg_rem_mask;

                vmovdqu16(vreg_prev_dst_ymm_, dst_addr);
                vpmovzxwd(vreg_prev_dst(idx), vreg_prev_dst_ymm_);
                vpslld(vreg_prev_dst(idx), vreg_prev_dst(idx), 0x10);
            } else
                assert(!"unsupported data type");

            vfmadd231ps(vreg_dst(idx), vreg_prev_dst(idx), vreg_sum_scale);
        }

        if (do_eltwise_) eltwise_injector_->compute_vector(vreg_dst_idx(idx));

        if (dst_data_type == data_type::bf16) {
            // TODO: implement store by zmm registers for bf16
            auto vreg_dst_ymm_ = vreg_dst_ymm(idx);
            if (isa_ == avx512_core_bf16)
                vcvtneps2bf16(vreg_dst_ymm_, vreg_dst(idx));
            else
                bf16_emu_->vcvtneps2bf16(vreg_dst_ymm_, vreg_dst(idx));

            if (apply_mask) vreg_dst_ymm_ = vreg_dst_ymm_ | kreg_rem_mask;

            vmovdqu16(dst_addr, vreg_dst_ymm_);
        } else if (dst_data_type == data_type::f32)
            vmovups(dst_addr, vreg_dst_);
        else
            assert(!"unimplemented");
    };

    // Advance all pointers by an immediate
    auto advance_ptrs_imm = [&](size_t offset) {
        add(reg_dst, offset * sizeof(dst_data_t));
        add(reg_acc, offset * sizeof(acc_data_t));
    };

    Xbyak::Label oc_loop, oc_loop_end;

    cmp(reg_oc_iter, 0);
    jle(oc_loop_end, T_NEAR);

    L(oc_loop);

    mov(reg_len_iter, reg_len);
    mov(reg_dst, reg_dst_base);
    mov(reg_acc, reg_acc_base);

    if (do_bias_) vbroadcastss(vreg_bias, ptr[reg_bias]);

    constexpr int n_unroll = default_unroll_2_pow_; // unroll by powers of 2
            // from 2^n to 2^0
    assert((1 << n_unroll) <= max_unroll_);

    Xbyak::Label l_simd_loop[n_unroll + 2], l_simd_notail;
    for (int i = n_unroll; i >= 0; i--) {
        const int unroll = 1 << i; // 4, 2, 1
        L(l_simd_loop[i + 1]);
        {
            const int loop_len = unroll * vlen_;
            cmp(reg_len_iter, loop_len);
            jl(l_simd_loop[i], T_NEAR);
            for (int j = 0; j < unroll; j++)
                compute(j * vlen_, j, false);

            advance_ptrs_imm(loop_len);
            sub(reg_len_iter, loop_len);
            jmp(l_simd_loop[i + 1], T_NEAR);
        }
    }
    L(l_simd_loop[0]);

    mov(reg_tmp, reg_len_iter); // reg_tmp is rcx, and we need cl for the shift
    mov(reg_rem_mask, 1);
    shl(reg_rem_mask, cl); // reg_tmp == rcx and reg_tail < vlen_ == 16
    sub(reg_rem_mask, 1);
    jz(l_simd_notail, T_NEAR);
    kmovq(kreg_rem_mask, reg_rem_mask);
    compute(0, 0, true);

    L(l_simd_notail);

    add(reg_dst_base, reg_dst_str);
    add(reg_acc_base, reg_acc_str);
    if (do_bias_) add(reg_bias, sizeof(acc_data_t));

    dec(reg_oc_iter);
    jnz(oc_loop, T_NEAR); // oc_loop end

    L(oc_loop_end);

    postamble();

    if (do_eltwise_) eltwise_injector_->prepare_table();

    ker_ = getCode<decltype(ker_)>();
}

template <data_type_t dst_data_type>
void gemm_bf16_convolution_fwd_t<dst_data_type>::pp_ker_t::operator()(
        dst_data_t *dst, const acc_data_t *acc, const acc_data_t *bias,
        float sum_scale, size_t dst_stride_in_elements,
        size_t acc_stride_in_elements, size_t len, bool do_parallel) {
    assert(ker_);
    if (len == 0) return;

    parallel(do_parallel ? 0 : 1, [&](const int ithr, const int nthr) {
        size_t start_oc = 0, end_oc = 0;
        balance211(OC_, nthr, ithr, start_oc, end_oc);
        if (end_oc > start_oc) {
            ker_args args;
            args.acc = acc + start_oc * acc_stride_in_elements;
            args.dst = dst + start_oc * dst_stride_in_elements;
            args.bias = bias + start_oc;
            args.sum_scale = sum_scale;
            args.dst_stride_in_bytes
                    = dst_stride_in_elements * sizeof(dst_data_t);
            args.acc_stride_in_bytes
                    = acc_stride_in_elements * sizeof(acc_data_t);
            args.spatial_length = len;
            args.oc_work = end_oc - start_oc;
            ker_(&args);
        }
    });
}

template <data_type_t dst_data_type>
void gemm_bf16_convolution_fwd_t<dst_data_type>::execute_forward(
        const exec_ctx_t &ctx) const {
    auto src = CTX_IN_MEM(const src_data_t *, DNNL_ARG_SRC);
    auto weights = CTX_IN_MEM(const wei_data_t *, DNNL_ARG_WEIGHTS);
    auto dst = CTX_OUT_MEM(dst_data_t *, DNNL_ARG_DST);

    bool is_bf16_dst = dst_data_type == data_type::bf16;

    auto col = ctx.get_scratchpad_grantor().template get<src_data_t>(
            key_conv_gemm_col);
    acc_data_t *acc_base = is_bf16_dst
            ? ctx.get_scratchpad_grantor().template get<acc_data_t>(
                    key_conv_int_dat_in_acc_dt)
            : nullptr;

    const jit_gemm_conv_conf_t &jcp = this->pd()->jcp_;

    float *bias = nullptr;
    if (jcp.with_bias) {
        if (pd()->desc()->bias_desc.data_type == data_type::bf16) {
            auto bias_in = CTX_IN_MEM(const bfloat16_t *, DNNL_ARG_BIAS);
            bias = ctx.get_scratchpad_grantor().template get<float>(
                    key_conv_bias_bf16_convert_wsp);
            cvt_bfloat16_to_float(bias, bias_in, jcp.ngroups * jcp.oc);
        } else {
            auto bias_in = CTX_IN_MEM(const float *, DNNL_ARG_BIAS);
            bias = const_cast<float *>(bias_in);
        }
    }

    const auto &post_ops = pd()->attr()->post_ops_;
    const bool do_sum = post_ops.contain(primitive_kind::sum, 0);
    const float sum_scale = do_sum ? post_ops.entry_[0].sum.scale : 0;

    const int M = jcp.os * jcp.od;
    const size_t src_step = (size_t)jcp.ic * jcp.ih * jcp.iw * jcp.id;
    const size_t dst_step = (size_t)jcp.oc * M;
    const size_t weights_g_size = (size_t)jcp.ic * jcp.oc * jcp.ks;
    const bool is_problem_3d = pd()->ndims() == 5;

    assert(IMPLICATION(
            is_problem_3d, jcp.oh_block == jcp.oh && jcp.ow_block == jcp.ow));
    assert(IMPLICATION(jcp.ow_block != jcp.ow, jcp.oh_block == 1));

    const int64_t K = jcp.ic * jcp.ks;
    const int64_t N = jcp.oc;

    const int nb_oh = div_up(jcp.oh, jcp.oh_block);
    const int nb_ow = div_up(jcp.ow, jcp.ow_block);
    const size_t work_amount
            = (size_t)jcp.ngroups * jcp.mb * jcp.od * nb_oh * nb_ow;
    parallel(jcp.nthr, [&](const int ithr, const int nthr) {
        src_data_t *_col = col + (ptrdiff_t)ithr * jcp.im2col_sz;
        if (is_problem_3d) {
            // jit_gemm_convolution_utils::im2col_3d() requires external
            // data initialization by zeroes
            for (ptrdiff_t i = 0; i < jcp.im2col_sz; i++)
                _col[i] = (src_data_t)0;
        }

        int g {0}, n {0}, od {0}, ohb {0}, owb {0};
        size_t start = 0, end = 0;

        balance211(work_amount, nthr, ithr, start, end);
        nd_iterator_init(start, g, jcp.ngroups, n, jcp.mb, od, jcp.od, ohb,
                nb_oh, owb, nb_ow);
        for (size_t iwork = start; iwork < end; ++iwork) {
            int oh = ohb * jcp.oh_block;
            int ow = owb * jcp.ow_block;
            const src_data_t *_src = src + (n * jcp.ngroups + g) * src_step;
            const wei_data_t *_weights = weights + g * weights_g_size;
            dst_data_t *_dst_im = dst + (n * jcp.ngroups + g) * dst_step;
            const int h_step = nstl::min(jcp.oh_block, jcp.oh - oh);
            const int w_step = nstl::min(jcp.ow_block, jcp.ow - ow);
            if (jcp.im2col_sz) {
                if (!is_problem_3d)
                    jit_gemm_convolution_utils::im2col<src_data_t>(
                            jcp, _src, _col, 0, jcp.os, 0, jcp.ic);
                else
                    jit_gemm_convolution_utils::im2col_3d<src_data_t>(
                            jcp, _src, _col, od);
            }

            const acc_data_t one = 1.0;

            const int64_t m = h_step * w_step;
            const int64_t LDA = jcp.im2col_sz ? m : M;
            const int64_t LDC = is_bf16_dst ? m : M;
            dst_data_t *dst_local = _dst_im + od * jcp.os + oh * jcp.ow + ow;
            acc_data_t *_acc = is_bf16_dst ? acc_base
                            + ithr
                                    * rnd_up(jcp.oc * jcp.oh_block
                                                    * jcp.ow_block,
                                            16)
                                           : (acc_data_t *)dst_local;

            gemm_bf16bf16f32("N", "N", &m, &N, &K, &one,
                    jcp.im2col_sz ? _col : _src + od * m, &LDA, _weights, &K,
                    &this->beta_, _acc, &LDC);

            if (this->pd()->is_postprocess_required()) {
                size_t acc_str = LDC;
                size_t dst_str = M;
                (*pp_ker_)(dst_local, _acc, bias + g * jcp.oc, sum_scale,
                        dst_str, acc_str, m, jcp.nthr == 1);
            }

            nd_iterator_step(g, jcp.ngroups, n, jcp.mb, od, jcp.od, ohb, nb_oh,
                    owb, nb_ow);
        }
    });
}

template <data_type_t diff_src_data_type>
void gemm_bf16_convolution_bwd_data_t<diff_src_data_type>::
        execute_backward_data(const exec_ctx_t &ctx) const {
    auto diff_dst = CTX_IN_MEM(const diff_dst_data_t *, DNNL_ARG_DIFF_DST);
    auto weights = CTX_IN_MEM(const wei_data_t *, DNNL_ARG_WEIGHTS);
    auto diff_src = CTX_OUT_MEM(diff_src_data_t *, DNNL_ARG_DIFF_SRC);

    auto col = ctx.get_scratchpad_grantor().template get<acc_data_t>(
            key_conv_gemm_col);
    acc_data_t *acc_base = diff_src_data_type == data_type::bf16
            ? ctx.get_scratchpad_grantor().template get<acc_data_t>(
                    key_conv_int_dat_in_acc_dt)
            : nullptr;

    const jit_gemm_conv_conf_t &jcp = this->pd()->jcp_;

    const int64_t M = jcp.os * jcp.od;
    const size_t src_step = (size_t)jcp.ic * jcp.ih * jcp.iw * jcp.id;
    const size_t dst_step = (size_t)jcp.oc * M;
    const size_t weights_g_size = (size_t)jcp.ic * jcp.oc * jcp.ks;

    const int64_t m = jcp.os;
    const int64_t K = jcp.oc;
    const int64_t N = jcp.ic * jcp.ks;
    const int64_t LDC = jcp.im2col_sz ? m : M;

    const size_t work_amount = (size_t)jcp.ngroups * jcp.mb;
    const bool is_problem_3d = pd()->ndims() == 5;

    parallel(jcp.nthr, [&](const int ithr, const int nthr) {
        acc_data_t *_col = col + (ptrdiff_t)ithr * jcp.im2col_sz;

        int g {0}, n {0};
        size_t start = 0, end = 0;
        balance211(work_amount, nthr, ithr, start, end);
        nd_iterator_init(start, g, jcp.ngroups, n, jcp.mb);
        for (size_t iwork = start; iwork < end; ++iwork) {

            diff_src_data_t *diff_src_local
                    = diff_src + (n * jcp.ngroups + g) * src_step;
            acc_data_t *acc = diff_src_data_type == data_type::bf16
                    ? acc_base + ithr * rnd_up(src_step, 16)
                    : (acc_data_t *)diff_src_local;

            if (is_problem_3d && jcp.im2col_sz > 0) {
                // jit_gemm_convolution_utils::col2im_3d() assumes that the
                // accumulator is initialized by zeroes
                for (size_t i = 0; i < src_step; i++)
                    acc[i] = (acc_data_t)0;
            }

            const wei_data_t *_weights = weights + g * weights_g_size;
            for (int od = 0; od < jcp.od; ++od) {
                const diff_dst_data_t *_diff_dst
                        = diff_dst + (n * jcp.ngroups + g) * dst_step + od * m;

                const acc_data_t zero = 0.0, one = 1.0;
                gemm_bf16bf16f32("N", "T", &m, &N, &K, &one, _diff_dst, &M,
                        _weights, &N, &zero,
                        jcp.im2col_sz ? _col : acc + od * m, &LDC);

                if (jcp.im2col_sz) {
                    if (!is_problem_3d)
                        jit_gemm_convolution_utils::col2im(jcp, _col, acc);
                    else
                        jit_gemm_convolution_utils::col2im_3d(
                                jcp, _col, acc, od);
                }
            }
            if (diff_src_data_type == data_type::bf16) {
                size_t spatial_size = (size_t)jcp.ih * jcp.iw * jcp.id;
                store_bfloat16_in_parallel((bfloat16_t *)diff_src_local,
                        (const float *)acc, jcp.ic, spatial_size,
                        jcp.nthr == 1);
            }
            nd_iterator_step(g, jcp.ngroups, n, jcp.mb);
        }
    });
}

template <data_type_t diff_wei_data_type>
void gemm_bf16_convolution_bwd_weights_t<
        diff_wei_data_type>::bf16_bwd_weights_reduction_par(int ithr_mb,
        int nthr_mb, const jit_gemm_conv_conf_t &jcp,
        const acc_data_t *weights_reduce_base,
        diff_wei_data_t *weights_base) const {
    assert(nthr_mb > 1); // no reduction for nthr_mb == 1

    const bool is_bf16_out = diff_wei_data_type == data_type::bf16;
    const size_t weights_g_size = (size_t)jcp.ic * jcp.oc * jcp.ks;
    size_t weights_start {0}, weights_end {0};
    balance211(weights_g_size, nthr_mb, ithr_mb, weights_start, weights_end);

    if (weights_start >= weights_end) return; // nothing to do

    size_t acc_size = weights_end - weights_start;
    float *wei_reduced = is_bf16_out
            ? (float *)weights_reduce_base + weights_start
            : (float *)weights_base + weights_start;
    if (!is_bf16_out) {
        // f32 diff_weights require initialization by weights_reduce
        // for thr_mb = 0
        for (size_t i = 0; i < acc_size; i++)
            wei_reduced[i] = ((float *)weights_reduce_base + weights_start)[i];
    }

    for (int thr_mb = 1; thr_mb < nthr_mb; ++thr_mb) {
        float *wei_to_reduce = (float *)weights_reduce_base
                + thr_mb * weights_g_size + weights_start;

        if (is_bf16_out && thr_mb == nthr_mb - 1)
            // the last iteration for bfloat16 requires conversion
            // and store to diff_weights array
            add_floats_and_cvt_to_bfloat16(
                    (bfloat16_t *)(weights_base + weights_start), wei_reduced,
                    wei_to_reduce, acc_size);
        else
            acc_ker_->accumulate(wei_reduced, wei_to_reduce, acc_size);
    }
}

template <data_type_t diff_wei_data_type>
void gemm_bf16_convolution_bwd_weights_t<diff_wei_data_type>::
        execute_backward_weights(const exec_ctx_t &ctx) const {
    auto diff_dst = CTX_IN_MEM(const diff_dst_data_t *, DNNL_ARG_DIFF_DST);
    auto src = CTX_IN_MEM(const src_data_t *, DNNL_ARG_SRC);
    auto diff_weights = CTX_OUT_MEM(diff_wei_data_t *, DNNL_ARG_DIFF_WEIGHTS);

    auto col = ctx.get_scratchpad_grantor().template get<src_data_t>(
            key_conv_gemm_col);
    auto wei_reduction = ctx.get_scratchpad_grantor().template get<acc_data_t>(
            key_conv_wei_reduction);

    const jit_gemm_conv_conf_t &jcp = this->pd()->jcp_;

    acc_data_t *acc_base = diff_wei_data_type == data_type::bf16
            ? ctx.get_scratchpad_grantor().template get<acc_data_t>(
                    key_conv_int_dat_in_acc_dt)
            : (acc_data_t *)diff_weights;

    float *diff_bias = nullptr;
    if (jcp.with_bias) {
        if (pd()->desc()->diff_bias_desc.data_type == data_type::bf16)
            diff_bias = ctx.get_scratchpad_grantor().template get<float>(
                    key_conv_bias_bf16_convert_wsp);
        else
            diff_bias = CTX_OUT_MEM(float *, DNNL_ARG_DIFF_BIAS);
    }

    const int64_t K = jcp.os * jcp.od;
    const size_t src_step = (size_t)jcp.ic * jcp.ih * jcp.iw * jcp.id;
    const size_t dst_step = (size_t)jcp.oc * K;
    const size_t weights_g_size = (size_t)jcp.ic * jcp.oc * jcp.ks;

    const int64_t k = jcp.os;
    const int64_t N = jcp.oc;
    const int64_t M = jcp.ic * jcp.ks;
    const int64_t LDA = jcp.im2col_sz ? k : K;
    const bool is_problem_3d = pd()->ndims() == 5;

    parallel(jcp.nthr, [&](const int ithr, const int nthr) {
        int ithr_g, nthr_g, ithr_mb, nthr_mb;
        size_t g_start {0}, g_end {0}, mb_start {0}, mb_end {0};

        const int mb_for_balance = jcp.need_wei_reduction ? jcp.mb : 1;
        jit_gemm_convolution_utils::bwd_weights_balance(ithr, nthr, jcp.ngroups,
                mb_for_balance, ithr_g, nthr_g, ithr_mb, nthr_mb);

        assert(IMPLICATION(!jcp.need_wei_reduction, nthr_mb == 1));
        const int need_reduction = nthr_mb != 1;

        if (ithr_g != -1 && ithr_mb != -1) {
            balance211((size_t)jcp.ngroups, nthr_g, ithr_g, g_start, g_end);
            balance211((size_t)jcp.mb, nthr_mb, ithr_mb, mb_start, mb_end);

            assert(IMPLICATION((g_end - g_start) > 1, need_reduction == 0));

            src_data_t *_col = col + (ptrdiff_t)ithr * jcp.im2col_sz;
            if (is_problem_3d) {
                // jit_gemm_convolution_utils::im2col_3d() requires external
                // data initialization by zeroes
                for (ptrdiff_t i = 0; i < jcp.im2col_sz; i++)
                    _col[i] = (src_data_t)0;
            }

            acc_data_t *weights_reduce_base
                    = wei_reduction + ithr_g * nthr_mb * weights_g_size;
            acc_data_t *weights_reduce
                    = weights_reduce_base + ithr_mb * weights_g_size;

            for (size_t g = g_start; g < g_end; ++g) {
                acc_data_t *acc = need_reduction
                        ? weights_reduce
                        : (acc_base + g * weights_g_size);
                for (size_t mb = mb_start; mb < mb_end; ++mb) {
                    const src_data_t *_src
                            = src + (mb * jcp.ngroups + g) * src_step;
                    for (int od = 0; od < jcp.od; ++od) {
                        const diff_dst_data_t *_diff_dst = diff_dst
                                + (mb * jcp.ngroups + g) * dst_step + od * k;

                        if (jcp.im2col_sz) {
                            if (!is_problem_3d)
                                jit_gemm_convolution_utils::im2col<src_data_t>(
                                        jcp, _src, _col, 0, jcp.os, 0, jcp.ic);
                            else
                                jit_gemm_convolution_utils::im2col_3d<
                                        src_data_t>(jcp, _src, _col, od);
                        }

                        const acc_data_t zero = 0.0, one = 1.0;
                        gemm_bf16bf16f32("T", "N", &M, &N, &k, &one,
                                jcp.im2col_sz ? _col : _src + od * k, &LDA,
                                _diff_dst, &K,
                                mb == mb_start && od == 0 ? &zero : &one, acc,
                                &M);
                    }
                }
            }
            if (need_reduction && dnnl_thr_syncable()) {
                dnnl_thr_barrier();
                diff_wei_data_t *weights_base
                        = diff_weights + g_start * weights_g_size;
                bf16_bwd_weights_reduction_par(ithr_mb, nthr_mb, jcp,
                        weights_reduce_base, weights_base);
            } else if (diff_wei_data_type == data_type::bf16
                    && g_end > g_start) {
                const size_t weights_g_size = (size_t)jcp.ic * jcp.oc * jcp.ks;
                const size_t work_size = (g_end - g_start) * weights_g_size;
                bfloat16_t *diff_weights_local
                        = (bfloat16_t *)diff_weights + g_start * weights_g_size;
                const float *acc_local
                        = (const float *)acc_base + g_start * weights_g_size;
                store_bfloat16_in_parallel(diff_weights_local, acc_local,
                        work_size, 1, jcp.nthr == 1);
            }
        } else {
            if (need_reduction && dnnl_thr_syncable()) dnnl_thr_barrier();
        }
    });

    if (jcp.need_wei_reduction && !dnnl_thr_syncable()) {
        parallel(jcp.nthr, [&](const int ithr, const int nthr) {
            int ithr_g, nthr_g, ithr_mb, nthr_mb;
            size_t g_start {0}, g_end {0}, mb_start {0}, mb_end {0};

            const int mb_for_balance = jcp.need_wei_reduction ? jcp.mb : 1;
            jit_gemm_convolution_utils::bwd_weights_balance(ithr, nthr,
                    jcp.ngroups, mb_for_balance, ithr_g, nthr_g, ithr_mb,
                    nthr_mb);

            assert(IMPLICATION(!jcp.need_wei_reduction, nthr_mb == 1));
            const int need_reduction = nthr_mb != 1;

            if (need_reduction && ithr_g != -1 && ithr_mb != -1) {
                balance211((size_t)jcp.ngroups, nthr_g, ithr_g, g_start, g_end);
                balance211((size_t)jcp.mb, nthr_mb, ithr_mb, mb_start, mb_end);

                assert(IMPLICATION((g_end - g_start) > 1, need_reduction == 0));

                acc_data_t *weights_reduce_base
                        = wei_reduction + ithr_g * nthr_mb * weights_g_size;

                diff_wei_data_t *weights_base
                        = diff_weights + g_start * weights_g_size;
                bf16_bwd_weights_reduction_par(ithr_mb, nthr_mb, jcp,
                        weights_reduce_base, weights_base);
            }
        });
    }

    if (jcp.with_bias) {
        parallel_nd(jcp.ngroups, jcp.oc, [&](int g, int oc) {
            acc_data_t db = 0;
            size_t offset_ = (size_t)g * dst_step + (size_t)oc * K;
            for (int mb = 0; mb < jcp.mb; ++mb) {
                size_t offset = offset_ + (size_t)mb * jcp.ngroups * dst_step;
                for_(int od = 0; od < jcp.od; ++od)
                for (int oh = 0; oh < jcp.oh; ++oh)
                    PRAGMA_OMP_SIMD(reduction(+ : db))
                for (int ow = 0; ow < jcp.ow; ++ow) {
                    db += diff_dst[offset];
                    offset++;
                }
            }
            diff_bias[g * jcp.oc + oc] = db;
        });

        if (pd()->desc()->diff_bias_desc.data_type == data_type::bf16) {
            auto diff_bias_in = CTX_OUT_MEM(bfloat16_t *, DNNL_ARG_DIFF_BIAS);
            cvt_float_to_bfloat16(
                    diff_bias_in, diff_bias, jcp.ngroups * jcp.oc);
        }
    }
}

template struct gemm_bf16_convolution_fwd_t<data_type::f32>;
template struct gemm_bf16_convolution_fwd_t<data_type::bf16>;
template struct gemm_bf16_convolution_bwd_data_t<data_type::f32>;
template struct gemm_bf16_convolution_bwd_data_t<data_type::bf16>;
template struct gemm_bf16_convolution_bwd_weights_t<data_type::f32>;
template struct gemm_bf16_convolution_bwd_weights_t<data_type::bf16>;

} // namespace cpu
} // namespace impl
} // namespace dnnl

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

#include "gemm_inner_product_utils.hpp"
#include "dnnl_thread.hpp"
#include "jit_uni_eltwise.hpp"
#include "math_utils.hpp"
#include "simple_q10n.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

namespace inner_product_utils {

using namespace alg_kind;
using namespace math;

template <data_type_t acc_type, data_type_t dst_type>
pp_kernel_t<acc_type, dst_type>::pp_kernel_t(
        const cpu_inner_product_fwd_pd_t *pd, bool skip_sum)
    : ker_(nullptr)
    , eltwise_injector_(nullptr)
    , ref_eltwise_(nullptr)
    , bf16_emu_(nullptr)
    , OC_(pd->OC())
    , do_bias_(pd->with_bias())
    , bias_data_type_(data_type::undef)
    , bias_data_type_size_(0)
    , do_scale_(false)
    , scale_idx_mult_(0)
    , do_eltwise_(false)
    , do_sum_(false)
    , sum_scale_(0)
    , isa_(isa_any)
    , max_OC_loop_unroll_(13)
    , idx_compute_vreg_start_(0)
    , idx_compute_vreg_max_(31)
    , compute_vregs_per_iter_(1)
    , compute_vreg_bias_shift_(0)
    , compute_vreg_prev_dst_shift_(0) {
    using namespace types;
    using namespace Xbyak;

    do_scale_ = !pd->attr()->output_scales_.has_default_values();
    if (do_scale_) {
        scale_idx_mult_ = (pd->attr()->output_scales_.mask_ == (1 << 1));
        vreg_scale = Zmm(idx_compute_vreg_start_++);
    }
    if (dst_type == data_type::u8) vreg_zero = Zmm(idx_compute_vreg_start_++);

    auto &p = pd->attr()->post_ops_;
    const int eltwise_ind = p.find(primitive_kind::eltwise);
    do_eltwise_ = eltwise_ind != -1;
    if (do_eltwise_) eltwise_ = p.entry_[eltwise_ind].eltwise;

    const int sum_ind = p.find(primitive_kind::sum);
    do_sum_ = sum_ind != -1 && !skip_sum;
    if (do_sum_) {
        sum_scale_ = p.entry_[sum_ind].sum.scale;
        vreg_sum_scale = Zmm(idx_compute_vreg_start_++);
        compute_vreg_prev_dst_shift_ = compute_vregs_per_iter_++;
    }

    if (do_bias_) {
        bias_data_type_ = pd->desc()->bias_desc.data_type;
        assert(bias_data_type_ != data_type::undef);
        bias_data_type_size_ = data_type_size(bias_data_type_);
        compute_vreg_bias_shift_ = compute_vregs_per_iter_++;
    }

    if (!mayiuse(avx512_core)) {
        // use fallback code for older CPUs since they do not have optimized
        // x8s8s32 GEMM anyways. The configuration variables above are used by
        // the fallback code.
        if (do_eltwise_)
            ref_eltwise_ = new ref_eltwise_scalar_fwd_t(
                    eltwise_.alg, eltwise_.alpha, eltwise_.beta);
        return;
    } else {
        isa_ = mayiuse(avx512_core_bf16) ? avx512_core_bf16
                                         : bf16_emulation_t::get_isa();
        if (dst_type == data_type::bf16 && isa_ != avx512_core_bf16) {
            idx_compute_vreg_max_ = 27;
            bf16_emu_ = new bf16_emulation_t(this, bf16_emu_reserv_1,
                    bf16_emu_reserv_2, bf16_emu_reserv_3, bf16_emu_reserv_4,
                    bf16_emu_reserv_5);
        }

        int max_unroll = (idx_compute_vreg_max_ - idx_compute_vreg_start_ + 1)
                / compute_vregs_per_iter_;
        max_OC_loop_unroll_ = nstl::min(max_OC_loop_unroll_, max_unroll);

        if (do_eltwise_)
            eltwise_injector_ = new jit_uni_eltwise_injector_f32<avx512_core>(
                    this, eltwise_, true, eltwise_reserved_1_,
                    eltwise_reserved_2_);
        generate();
    }
}

template <data_type_t acc_type, data_type_t dst_type>
void pp_kernel_t<acc_type, dst_type>::generate() {
    using namespace Xbyak;
    using namespace utils;

    const size_t vlen = cpu_isa_traits<avx512_core>::vlen / sizeof(float);

    preamble();

#define PARAM_OFF(x) offsetof(ker_args, x)
    mov(reg_dst, ptr[reg_param + PARAM_OFF(dst)]);
    mov(reg_acc, ptr[reg_param + PARAM_OFF(acc)]);
    mov(reg_bias, ptr[reg_param + PARAM_OFF(bias)]);
    if (do_scale_) mov(reg_scales, ptr[reg_param + PARAM_OFF(scales)]);
    mov(reg_len, ptr[reg_param + PARAM_OFF(len)]);
    mov(reg_oc_offset, ptr[reg_param + PARAM_OFF(oc_offset)]);
    if (do_scale_ && scale_idx_mult_ == 0)
        vbroadcastss(vreg_scale, dword[reg_scales]);
#undef PARAM_OFF

    if (do_sum_) {
        mov(reg_tmp, float2int(sum_scale_));
        auto xreg_sum_scale = Xmm(vreg_sum_scale.getIdx());
        vmovq(xreg_sum_scale, reg_tmp);
        vbroadcastss(vreg_sum_scale, xreg_sum_scale);
    }

    if (dst_type == data_type::u8) vxorps(vreg_zero, vreg_zero, vreg_zero);

    // Load accumulated value, convert to float, apply bias (if any), scaling,
    // and eltwise (if any); then convert to destination type and store
    auto compute = [&](size_t offset, int idx, bool apply_mask) {
        auto acc_addr = ptr[reg_acc + offset * sizeof(acc_data_t)];
        if (dst_type == data_type::bf16 && isa_ != avx512_core_bf16)
            bf16_emu_->init_vcvtneps2bf16();

        if (do_scale_ && scale_idx_mult_ == 1) {
            auto scale_addr = ptr[reg_scales + offset * sizeof(float)];
            auto vreg_scale_msk_ = vreg_scale;
            if (apply_mask) vreg_scale_msk_ = vreg_scale_msk_ | kreg_rem_mask;
            vmovups(vreg_scale_msk_, scale_addr);
        }

        auto vreg_dst_ = vreg_dst(idx);
        auto vreg_dst_msk_ = apply_mask ? vreg_dst_ | kreg_rem_mask : vreg_dst_;

        switch (acc_type) {
            case data_type::s32: vcvtdq2ps(vreg_dst_msk_, acc_addr); break;
            case data_type::f32: vmovups(vreg_dst_msk_, acc_addr); break;
        }

        if (do_bias_) {
            auto bias_addr = ptr[reg_bias + offset * bias_data_type_size_];
            auto vreg_bias_ = vreg_bias(idx);
            auto vreg_bias_msk_
                    = apply_mask ? vreg_bias_ | kreg_rem_mask : vreg_bias_;

            switch (bias_data_type_) {
                case data_type::s8: vpmovsxbd(vreg_bias_msk_, bias_addr); break;
                case data_type::u8: vpmovzxbd(vreg_bias_msk_, bias_addr); break;
                case data_type::s32:
                case data_type::f32: vmovups(vreg_bias_msk_, bias_addr); break;
                case data_type::bf16:
                    vpmovzxwd(vreg_bias_msk_, bias_addr);
                    vpslld(vreg_bias_, vreg_bias_, 0x10);
                    break;
                default: assert(!"unimplemented");
            }
            if (utils::one_of(bias_data_type_, data_type::u8, data_type::s8,
                        data_type::s32))
                vcvtdq2ps(vreg_bias_, vreg_bias_);
            vaddps(vreg_dst_, vreg_dst_, vreg_bias_);
        }

        if (do_scale_) vmulps(vreg_dst_, vreg_dst_, vreg_scale);

        auto dst_addr = ptr[reg_dst + offset * sizeof(dst_data_t)];
        if (do_sum_) {
            auto vreg_prev_dst_ = vreg_prev_dst(idx);
            auto vreg_prev_dst_msk_ = apply_mask
                    ? vreg_prev_dst_ | kreg_rem_mask
                    : vreg_prev_dst_;

            switch (dst_type) {
                case data_type::f32:
                case data_type::s32:
                    vmovups(vreg_prev_dst_msk_, dst_addr);
                    break;
                case data_type::s8:
                    vpmovsxbd(vreg_prev_dst_msk_, dst_addr);
                    break;
                case data_type::u8:
                    vpmovzxbd(vreg_prev_dst_msk_, dst_addr);
                    break;
                case data_type::bf16:
                    vpmovzxwd(vreg_prev_dst_msk_, dst_addr);
                    vpslld(vreg_prev_dst_, vreg_prev_dst_, 0x10);
                    break;
                default: assert(!"unsupported data type");
            }
            if (utils::one_of(
                        dst_type, data_type::u8, data_type::s8, data_type::s32))
                vcvtdq2ps(vreg_prev_dst_, vreg_prev_dst_);

            vfmadd231ps(vreg_dst_, vreg_prev_dst_, vreg_sum_scale);
        }

        if (do_eltwise_) eltwise_injector_->compute_vector(vreg_dst_.getIdx());

        if (dst_type == data_type::u8) vmaxps(vreg_dst_, vreg_dst_, vreg_zero);

        if (utils::one_of(
                    dst_type, data_type::s8, data_type::u8, data_type::s32)) {
            vcvtps2dq(vreg_dst_, vreg_dst_);
        } else if (dst_type == data_type::bf16) {
            if (isa_ == avx512_core_bf16)
                vcvtneps2bf16(Ymm(vreg_dst_.getIdx()), vreg_dst_);
            else
                bf16_emu_->vcvtneps2bf16(Ymm(vreg_dst_.getIdx()), vreg_dst_);
        }

        switch (dst_type) {
            case data_type::s8: vpmovsdb(dst_addr, vreg_dst_msk_); break;
            case data_type::u8: vpmovusdb(dst_addr, vreg_dst_msk_); break;
            case data_type::f32:
            case data_type::s32: vmovups(dst_addr, vreg_dst_msk_); break;
            case data_type::bf16:
                vmovdqu16(dst_addr,
                        apply_mask ? Ymm(vreg_dst_.getIdx()) | kreg_rem_mask
                                   : Ymm(vreg_dst_.getIdx()));
                break;
            default: assert(!"unimplemented");
        }
    };

    // Advance all pointers by an immediate
    auto advance_ptrs_imm = [&](size_t offset) {
        add(reg_dst, offset * sizeof(dst_data_t));
        add(reg_acc, offset * sizeof(acc_data_t));
        if (do_scale_ && scale_idx_mult_ == 1)
            add(reg_scales, offset * sizeof(float));
        if (do_bias_) add(reg_bias, offset * bias_data_type_size_);
    };

    // Advance all pointers by a value stored in a register
    auto advance_ptrs_reg = [&](Reg64 offset) {
        lea(reg_dst, ptr[reg_dst + offset * sizeof(dst_data_t)]);
        lea(reg_acc, ptr[reg_acc + offset * sizeof(acc_data_t)]);
        if (do_scale_ && scale_idx_mult_ == 1)
            lea(reg_scales, ptr[reg_scales + offset * sizeof(float)]);
        if (do_bias_)
            lea(reg_bias, ptr[reg_bias + offset * bias_data_type_size_]);
    };

    // Rewind pointers that point to data that is indixed by output channel
    // (bias or per-oc scaling factors)
    auto rewind_ptrs = [&]() {
        if (do_bias_) sub(reg_bias, OC_ * bias_data_type_size_);
        if (do_scale_ && scale_idx_mult_ == 1)
            sub(reg_scales, OC_ * sizeof(float));
    };

    //      <-------------------- OC ------------------------------->
    //
    // ^    +....................+----------------------------------+
    // |    :   not accessed     |          Prologue loop           |
    // |    +--------------------+----------------------------------+
    //      |                                                       |
    // M    |                 Main loop (unrolled)                  |
    // B    |                                                       |
    //      +--------------------------------+----------------------+
    // |    |       Epilogue loop            |      not accessed    :
    // v    +--------------------------------+......................+

    Label prologue_end;
    cmp(reg_oc_offset, 0);
    je(prologue_end, T_NEAR);

    // Prologue loop
    {
        mov(reg_tmp, OC_);
        sub(reg_tmp, reg_oc_offset);
        cmp(reg_tmp, reg_len);
        cmovg(reg_tmp, reg_len);
        sub(reg_len, reg_tmp);

        Label prologue_loop, prologue_loop_tail, prologue_loop_end;
        cmp(reg_tmp, vlen);
        jle(prologue_loop_tail, T_NEAR); // Skips for reg_tmp == 16 too (?)
        L(prologue_loop);
        {
            compute(0, 0, false);
            advance_ptrs_imm(vlen);
            sub(reg_tmp, vlen);
            cmp(reg_tmp, vlen);
            jge(prologue_loop, T_NEAR);
        }

        L(prologue_loop_tail);
        mov(reg_rem_mask, 1);
        shl(reg_rem_mask, cl); // cl == reg_tmp because reg_tmp <= vlen here
        sub(reg_rem_mask, 1);
        jz(prologue_loop_end, T_NEAR);

        kmovq(kreg_rem_mask, reg_rem_mask);
        compute(0, 0, true);
        advance_ptrs_reg(reg_tmp);

        L(prologue_loop_end);
        rewind_ptrs();
    }
    L(prologue_end);

    // Main loop
    Label main_loop_end;
    {
        cmp(reg_len, OC_);
        jle(main_loop_end, T_NEAR);

        Label main_loop;
        L(main_loop);
        {
            size_t OC_loop, OC_tail;
            if (OC_ < max_OC_loop_unroll_ * vlen) {
                // Fully unroll small loops
                OC_loop = 0;
                OC_tail = OC_;
            } else {
                OC_loop = vlen * default_OC_loop_unroll_;
                OC_tail = OC_ % OC_loop;
            }

            assert(!!OC_loop || !!OC_tail);

            if (OC_tail % vlen) {
                int vlen_tail = OC_tail % vlen;
                unsigned tail_mask = (1 << vlen_tail) - 1;
                mov(reg_tmp, tail_mask);
                kmovq(kreg_rem_mask, reg_tmp);
            }

            if (OC_loop) {
                mov(reg_tmp, rnd_dn(OC_, OC_loop));
                Label oc_loop;
                L(oc_loop);
                {
                    for (size_t offset = 0; offset < OC_loop; offset += vlen)
                        compute(offset, offset / vlen, false);
                    advance_ptrs_imm(OC_loop);
                    sub(reg_tmp, OC_loop);
                    jnz(oc_loop);
                }
            }

            if (OC_tail) {
                for (size_t offset = 0; offset < OC_tail; offset += vlen) {
                    bool use_mask = (offset + vlen) > OC_tail;
                    compute(offset, offset / vlen, use_mask);
                }
                advance_ptrs_imm(OC_tail);
            }

            rewind_ptrs();
            sub(reg_len, OC_);
            cmp(reg_len, OC_);
            jge(main_loop, T_NEAR);
        }
    }
    L(main_loop_end);

    // Epilogue loop
    Label epilogue_end;
    {
        cmp(reg_len, 0);
        je(epilogue_end, T_NEAR);

        Label epilogue_loop, epilogue_loop_tail;
        cmp(reg_len, vlen);
        jle(epilogue_loop_tail, T_NEAR); // Skips for reg_len == 16 (?)
        L(epilogue_loop);
        {
            compute(0, 0, false);
            sub(reg_len, vlen);
            advance_ptrs_imm(vlen);
            cmp(reg_len, vlen);
            jge(epilogue_loop, T_NEAR);
        }

        L(epilogue_loop_tail);
        mov(reg_tmp, reg_len); // reg_tmp is rcx, and we need cl for the shift
        mov(reg_rem_mask, 1);
        shl(reg_rem_mask, cl); // reg_tmp == rcx and reg_tail < vlen == 16
        sub(reg_rem_mask, 1);
        jz(epilogue_end, T_NEAR);
        kmovq(kreg_rem_mask, reg_rem_mask);
        compute(0, 0, true);
    }

    L(epilogue_end);

    postamble();

    if (do_eltwise_) eltwise_injector_->prepare_table();

    ker_ = getCode<decltype(ker_)>();
}

template <data_type_t acc_type, data_type_t dst_type>
void pp_kernel_t<acc_type, dst_type>::operator()(dst_data_t *dst,
        const acc_data_t *acc, const char *bias, const float *scales,
        size_t start, size_t end) {
    using math::get_bias;

    if (end <= start) return;

    if (ker_) {
        // JIT
        ker_args args;
        size_t oc_offset = start % OC_;
        args.dst = dst + start;
        args.acc = acc + start;
        args.bias = bias + oc_offset * bias_data_type_size_;
        args.scales = scales + scale_idx_mult_ * oc_offset;
        args.len = end - start;
        args.oc_offset = oc_offset;
        ker_(&args);
    } else {
        // Fallback
        size_t oc = start % OC_;
        for (size_t i = start; i < end; i++) {
            float d = (float)acc[i];
            if (do_bias_) d += get_bias(bias, oc, bias_data_type_);
            if (do_scale_) d *= scales[oc * scale_idx_mult_];
            if (do_sum_) d += sum_scale_ * dst[i];
            if (do_eltwise_) d = ref_eltwise_->compute_scalar(d);
            dst[i] = qz_a1b0<float, dst_data_t>()(d);
            oc = (oc == OC_ - 1) ? 0 : oc + 1;
        }
    }
};

using namespace data_type;
template class pp_kernel_t<f32, f32>;
template class pp_kernel_t<s32, f32>;
template class pp_kernel_t<s32, s32>;
template class pp_kernel_t<s32, s8>;
template class pp_kernel_t<s32, u8>;
template class pp_kernel_t<f32, bf16>;
} // namespace inner_product_utils

} // namespace cpu
} // namespace impl
} // namespace dnnl

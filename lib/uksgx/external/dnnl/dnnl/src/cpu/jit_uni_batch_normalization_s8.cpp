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

#include <assert.h>

#include "c_types_map.hpp"
#include "dnnl_thread.hpp"
#include "math_utils.hpp"
#include "nstl.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

#include "jit_generator.hpp"

#include "jit_uni_batch_normalization_s8.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

namespace {

using namespace Xbyak;

typedef int8_t data_t;

template <cpu_isa_t isa>
struct jit_bnorm_t : public jit_generator {
    struct call_params_t {
        // keep int sizes at 8 bytes -- jit code expects this
        size_t coff_max, soff_max;
        float eps;
        const float *scale_shift, *mean, *var;
        const data_t *src, *dst;
    };

    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_bnorm_t)

    using Vmm = typename cpu_isa_traits<isa>::Vmm;
    const AddressFrame &vmmword = (isa == avx2) ? yword : zword;
    const int vlen = cpu_isa_traits<isa>::vlen;

    const batch_normalization_pd_t *bdesc_;

    void (*ker)(const call_params_t *);
    void operator()(const call_params_t *p) { (*ker)(p); }

    Reg64 reg_param = abi_param1;

    Reg64 reg_scale_shift = rbx;
    Reg64 reg_mean = rbp;

    Reg64 reg_coff_max = r8;
    Reg64 reg_soff = r9;
    Reg64 reg_soff_max = r10;
    Reg64 reg_tmp = r11;
    Reg64 reg_src = r12;
    Reg64 reg_dst = r13;
    Reg64 reg_var = r14;
    Reg64 reg_coff_1byte = r15;
    Reg64 reg_coff_4byte = rax;

    // channel tail processing
    Opmask ktail_mask = Opmask(1); // f32 mask for channel math

    Vmm vtail_mask = Vmm(isa == avx512_core ? 27 : 11);
    Vmm vbody_mask = Vmm(isa == avx512_core ? 28 : 12);
    Vmm vzero = Vmm(isa == avx512_core ? 29 : 13);
    Xmm xone = Xmm(14);
    Vmm vone = Vmm(isa == avx512_core ? 30 : 14);
    Vmm veps = Vmm(isa == avx512_core ? 31 : 15);

    bool with_relu_;
    size_t simd_w_;
    size_t c_in_xmm_;
    size_t chan_data_offt_;
    size_t num_c16_blocks_;
    size_t c_tail_;

    void compute_predefined_variables() {
        chan_data_offt_ = bdesc_->C() * sizeof(float);
        c_in_xmm_ = 16;
        num_c16_blocks_ = bdesc_->C() / c_in_xmm_;
        c_tail_ = bdesc_->C() % c_in_xmm_;
        with_relu_ = (bdesc_->with_relu_post_op() || bdesc_->fuse_norm_relu())
                && bdesc_->is_fwd();
    }

    void load_common_params() {
        mov(reg_tmp, float2int(1.0f));
        movq(xone, reg_tmp);
        uni_vbroadcastss(vone, xone);

#define PARAM_OFF(x) offsetof(call_params_t, x)
        uni_vbroadcastss(veps, vmmword[reg_param + PARAM_OFF(eps)]);
        uni_vpxor(vzero, vzero, vzero);

        mov(reg_coff_max, ptr[reg_param + PARAM_OFF(coff_max)]);
        mov(reg_soff_max, ptr[reg_param + PARAM_OFF(soff_max)]);
        mov(reg_src, ptr[reg_param + PARAM_OFF(src)]);
        mov(reg_dst, ptr[reg_param + PARAM_OFF(dst)]);
        mov(reg_mean, ptr[reg_param + PARAM_OFF(mean)]);
        mov(reg_scale_shift, ptr[reg_param + PARAM_OFF(scale_shift)]);
        mov(reg_var, ptr[reg_param + PARAM_OFF(var)]);
#undef PARAM_OFF
    }

    void prepare_tail_mask_avx512() {
        if (!c_tail_) return;

        const int mask_f32 = (1 << c_tail_) - 1;

        Reg32 regw_tmp = reg_tmp.cvt32();
        mov(regw_tmp, mask_f32);
        kmovw(ktail_mask, regw_tmp);
    }

    void prepare_tail_mask_avx2() {
        static const uint32_t mask_half_ymm[8]
                = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0, 0, 0, 0};
        mov(reg_tmp, reinterpret_cast<size_t>(&mask_half_ymm[0]));
        vmovups(vbody_mask, ptr[reg_tmp]);

        if (!c_tail_) return;

        static const uint32_t mask_f32[16] = {0xffffffff, 0xffffffff,
                0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                0xffffffff, 0, 0, 0, 0, 0, 0, 0, 0};

        mov(reg_tmp,
                reinterpret_cast<size_t>(&mask_f32[8 - c_tail_ % simd_w_]));
        vmovups(vtail_mask, ptr[reg_tmp]);
    }

    void uni_vmovups_tail_avx2(const Operand &dst, const Operand &src) {
        if (dst.isMEM())
            vmaskmovps(dst.getAddress(), vtail_mask, Vmm(src.getIdx()));
        else
            vmaskmovps(Vmm(dst.getIdx()), vtail_mask, src.getAddress());
    }

    void uni_vmovups_tail_avx512(const Operand &dst, const Operand &src) {
        if (dst.isMEM())
            vmovups(dst.getAddress() | ktail_mask, Vmm(src.getIdx()));
        else
            vmovups(Vmm(dst.getIdx()) | ktail_mask | T_z, src.getAddress());
    }

    void uni_vmovups_tail(const Operand &dst, const Operand &src) {
        if (isa == avx512_core)
            uni_vmovups_tail_avx512(dst, src);
        else if (isa == avx2)
            uni_vmovups_tail_avx2(dst, src);
    }

    Address mean_ptr(size_t offt = 0) {
        return vmmword[reg_mean + reg_coff_4byte + offt];
    }

    Address var_ptr(size_t offt = 0) {
        return vmmword[reg_var + reg_coff_4byte + offt];
    }

    Address scale_ptr(size_t offt = 0) {
        return vmmword[reg_scale_shift + reg_coff_4byte + offt
                + 0 * chan_data_offt_];
    }

    Address shift_ptr(size_t offt = 0) {
        return vmmword[reg_scale_shift + reg_coff_4byte + offt
                + 1 * chan_data_offt_];
    }

    Address src_ptr(size_t offt = 0) {
        return vmmword[reg_src + reg_soff + offt];
    }

    Address dst_ptr(size_t offt = 0) {
        return vmmword[reg_dst + reg_soff + offt];
    }

    // Precomputes vscale and vshift for following
    // `vdst = vscale * vsrc + vshift`
    void compute_vscaleshift(const Vmm &vscale, const Vmm &vshift,
            const Vmm &vmean, const Vmm &vsqrtvar, size_t offt = 0,
            bool need_tail = false) {
        if (need_tail) {
            uni_vmovups_tail(vmean, mean_ptr(offt));
            uni_vmovups_tail(vsqrtvar, var_ptr(offt));
        } else {
            uni_vmovups(vmean, mean_ptr(offt));
            uni_vmovups(vsqrtvar, var_ptr(offt));
        }
        uni_vaddps(vsqrtvar, vsqrtvar, veps);
        uni_vsqrtps(vsqrtvar, vsqrtvar);

        if (bdesc_->use_scaleshift()) {
            if (need_tail) {
                uni_vmovups_tail(vscale, scale_ptr(offt));
                uni_vmovups_tail(vshift, shift_ptr(offt));
            } else {
                uni_vmovups(vscale, scale_ptr(offt));
                uni_vmovups(vshift, shift_ptr(offt));
            }
            vdivps(vscale, vscale, vsqrtvar);
            uni_vfnmadd231ps(vshift, vmean, vscale);
        } else {
            vdivps(vscale, vone, vsqrtvar);
            uni_vmulps(vmean, vmean, vscale);
            uni_vsubps(vshift, vzero, vmean);
        }
    };

    void working_cycle_avx512(bool need_tail = false) {
        Label c_loop;
        L(c_loop);
        {

            Xmm x = Xmm(0);
            Vmm v = Vmm(0);
            Vmm vscale = Vmm(1);
            Vmm vshift = Vmm(2);
            Vmm vmean = Vmm(3);
            Vmm vsqrtvar = Vmm(4);

            // compute single vscale and vshift vectors...
            compute_vscaleshift(vscale, vshift, vmean, vsqrtvar, 0, need_tail);

            // ... then process all spatial loop with it and move to the
            // next channel chunk
            mov(reg_soff, reg_coff_1byte);
            Label mb_sp_loop;
            L(mb_sp_loop);
            {

                if (need_tail) {
                    for (size_t tl = 0; tl < c_tail_; tl++)
                        vpinsrb(x, x, src_ptr(tl), tl);
                    vpmovsxbd(v, x);
                } else
                    vpmovsxbd(v, src_ptr());

                vcvtdq2ps(v, v);

                uni_vfmadd213ps(v, vscale, vshift);
                if (with_relu_) uni_vmaxps(v, v, vzero);

                vcvtps2dq(v, v);
                if (need_tail) {
                    vpmovsdb(x, v);
                    for (size_t tl = 0; tl < c_tail_; tl++)
                        vpextrb(dst_ptr(tl), x, tl);
                } else
                    vpmovsdb(dst_ptr(), v);

                add(reg_soff, reg_coff_max);
                cmp(reg_soff, reg_soff_max);
                jl(mb_sp_loop);
            }

            // reg_tmp checks c_in_xmm_ channels ahead for further tail process
            add(reg_tmp, sizeof(data_t) * c_in_xmm_);
            add(reg_coff_1byte, sizeof(data_t) * c_in_xmm_);
            add(reg_coff_4byte, sizeof(float) * c_in_xmm_);
            cmp(reg_tmp, reg_coff_max);
            jle(c_loop);
        }
    }

    void forward_avx512() {
        xor_(reg_coff_1byte, reg_coff_1byte);
        xor_(reg_coff_4byte, reg_coff_4byte);
        mov(reg_tmp, sizeof(data_t) * c_in_xmm_);

        if (num_c16_blocks_) working_cycle_avx512();
        if (c_tail_) working_cycle_avx512(true);
    }

    void working_cycle_avx2(bool need_tail = false) {
        Label c_loop;
        L(c_loop);
        {

            Xmm x0 = Xmm(0);
            Vmm v0 = Vmm(0);
            Xmm x1 = Xmm(1);
            Vmm v1 = Vmm(1);
            Vmm vscale0 = Vmm(2);
            Vmm vshift0 = Vmm(3);
            Vmm vmean0 = Vmm(4);
            Vmm vsqrtvar0 = Vmm(5);
            Vmm vscale1 = Vmm(6);
            Vmm vshift1 = Vmm(7);
            Vmm vmean1 = Vmm(8);
            Vmm vsqrtvar1 = Vmm(9);

            // compute couple vscale and vshift vectors each of 8 channels...
            compute_vscaleshift(vscale0, vshift0, vmean0, vsqrtvar0, 0,
                    (c_tail_ < simd_w_ && need_tail) ? true : false);
            if (!need_tail || c_tail_ > simd_w_) {
                compute_vscaleshift(vscale1, vshift1, vmean1, vsqrtvar1,
                        simd_w_ * sizeof(float), need_tail);
            }

            // ... then process all spatial loop with it and move to the
            // next channel chunk
            mov(reg_soff, reg_coff_1byte);
            Label mb_sp_loop;
            L(mb_sp_loop);
            {

                if (need_tail) {
                    for (size_t tl = 0; tl < c_tail_; tl++) {
                        if (tl < simd_w_) {
                            vpinsrb(x0, x0, src_ptr(tl), tl);
                        } else {
                            vpinsrb(x1, x1, src_ptr(tl), tl - simd_w_);
                        }
                    }
                    vpmovsxbd(v0, x0);
                    vpmovsxbd(v1, x1);
                } else {
                    vpmovsxbd(v0, src_ptr());
                    vpmovsxbd(v1, src_ptr(simd_w_));
                }
                vcvtdq2ps(v0, v0);
                vcvtdq2ps(v1, v1);

                uni_vfmadd213ps(v0, vscale0, vshift0);
                uni_vfmadd213ps(v1, vscale1, vshift1);
                if (with_relu_) {
                    uni_vmaxps(v0, v0, vzero);
                    uni_vmaxps(v1, v1, vzero);
                }

                vcvtps2dq(v0, v0); // BA
                vcvtps2dq(v1, v1); // DC
                vpackssdw(v0, v0, v1); // BA + DC -> DBCA
                vpermq(v0, v0, 0xD8); // DBCA -> DCBA
                vperm2i128(v1, v0, v0, 0x1); // DCBA -> BADC
                vpacksswb(v0, v0, v1); // DCBA + BADC -> badcDCBA

                if (need_tail) {
                    for (size_t tl = 0; tl < c_tail_; tl++) {
                        vpextrb(dst_ptr(tl), x0, tl);
                    }
                } else {
                    // due to vpacksswb produces 32 integers in ymm, and top
                    // half of them are garbage, do 128-b masked store
                    vmaskmovps(dst_ptr(), vbody_mask, v0);
                }

                add(reg_soff, reg_coff_max);
                cmp(reg_soff, reg_soff_max);
                jl(mb_sp_loop);
            }

            // reg_tmp checks c_in_xmm_ channels ahead for further tail process
            add(reg_tmp, sizeof(data_t) * c_in_xmm_);
            add(reg_coff_1byte, sizeof(data_t) * c_in_xmm_);
            add(reg_coff_4byte, sizeof(float) * c_in_xmm_);
            cmp(reg_tmp, reg_coff_max);
            jle(c_loop);
        }
    }

    void forward_avx2() {
        xor_(reg_coff_1byte, reg_coff_1byte);
        xor_(reg_coff_4byte, reg_coff_4byte);
        mov(reg_tmp, sizeof(data_t) * c_in_xmm_);

        if (num_c16_blocks_) working_cycle_avx2();
        if (c_tail_) working_cycle_avx2(true);
    }

    jit_bnorm_t(const batch_normalization_pd_t *bdesc) : bdesc_(bdesc) {
        static_assert(isa == avx2 || isa == avx512_core, "unsupported isa");

        simd_w_ = cpu_isa_traits<isa>::vlen / sizeof(float);

        preamble();
        compute_predefined_variables();
        load_common_params();

        if (isa == avx512_core) {
            prepare_tail_mask_avx512();
            forward_avx512();
        } else if (isa == avx2) {
            prepare_tail_mask_avx2();
            forward_avx2();
        }

        postamble();

        ker = reinterpret_cast<decltype(ker)>(
                const_cast<uint8_t *>(this->getCode()));
    }
};

} // namespace

namespace bnorm_s8_impl {

template <cpu_isa_t isa>
struct driver_t : public c_compatible {
    driver_t(const batch_normalization_pd_t *bdesc)
        : bdesc_(bdesc), ker_(bdesc_) {}
    ~driver_t() {}

    // TODO: for problems where thread pieces don't fit L2 cache, add spatial
    // re-balance using less pieces.
    void exec(int ithr, int nthr, const data_t *src, data_t *dst,
            const float *scale_shift, const float *mean, const float *var) {
        dim_t N = bdesc_->MB();
        dim_t C = bdesc_->C();
        dim_t D = bdesc_->D();
        dim_t H = bdesc_->H();
        dim_t W = bdesc_->W();
        dim_t SP = D * H * W;

        typename jit_bnorm_t<isa>::call_params_t p;

        p.eps = bdesc_->desc()->batch_norm_epsilon;

        p.scale_shift = scale_shift;
        p.mean = mean;
        p.var = var;

        dim_t work_amount {N * SP}, start {0}, end {0};
        balance211(work_amount, nthr, ithr, start, end);

        p.coff_max = C;
        p.soff_max = (end - start) * p.coff_max;
        p.src = src + start * p.coff_max;
        p.dst = dst + start * p.coff_max;

        if (p.soff_max != 0) ker_(&p);
    }

private:
    const batch_normalization_pd_t *bdesc_;

    jit_bnorm_t<isa> ker_;
};

} // namespace bnorm_s8_impl

using namespace data_type;
using namespace format_tag;
using namespace utils;

/* fwd */

template <cpu_isa_t isa>
status_t jit_uni_batch_normalization_s8_fwd_t<isa>::pd_t::init() {
    auto desired_fmt_tag = (ndims() == 4) ? nhwc : ndhwc;

    bool ok = true && mayiuse(isa) && is_fwd() && !has_zero_dim_memory()
            && one_of(ndims(), 4, 5) && stats_is_src()
            && src_md()->data_type == s8
            && IMPLICATION(use_scaleshift(), weights_md()->data_type == f32)
            && memory_desc_matches_tag(*src_md(), desired_fmt_tag)
            && (attr()->has_default_values() || this->with_relu_post_op());
    if (!ok) return status::unimplemented;

    return status::success;
}

template <cpu_isa_t isa>
jit_uni_batch_normalization_s8_fwd_t<isa>::jit_uni_batch_normalization_s8_fwd_t(
        const pd_t *apd)
    : primitive_impl_t(apd) {
    bnorm_driver_ = new bnorm_s8_impl::driver_t<isa>(pd());
}

template <cpu_isa_t isa>
status_t jit_uni_batch_normalization_s8_fwd_t<isa>::execute(
        const exec_ctx_t &ctx) const {
    auto src = CTX_IN_MEM(const data_t *, DNNL_ARG_SRC);
    auto scale_shift = CTX_IN_MEM(const float *, DNNL_ARG_SCALE_SHIFT);
    auto mean = const_cast<float *>(CTX_IN_MEM(const float *, DNNL_ARG_MEAN));
    auto var
            = const_cast<float *>(CTX_IN_MEM(const float *, DNNL_ARG_VARIANCE));
    auto dst = CTX_OUT_MEM(data_t *, DNNL_ARG_DST);

    // do sequential if the problem is less than one 4K memory page
    const bool force_sequential
            = pd()->MB() * pd()->C() * pd()->D() * pd()->H() * pd()->W()
            <= 4096;

    parallel(force_sequential ? 1 : 0, [&](const int ithr, const int nthr) {
        bnorm_driver_->exec(ithr, nthr, src, dst, scale_shift, mean, var);
    });

    return status::success;
}

template <cpu_isa_t isa>
jit_uni_batch_normalization_s8_fwd_t<
        isa>::~jit_uni_batch_normalization_s8_fwd_t() {
    delete bnorm_driver_;
}

/* struct instantiation */
template struct jit_uni_batch_normalization_s8_fwd_t<avx512_core>;
template struct jit_uni_batch_normalization_s8_fwd_t<avx2>;

} // namespace cpu
} // namespace impl
} // namespace dnnl

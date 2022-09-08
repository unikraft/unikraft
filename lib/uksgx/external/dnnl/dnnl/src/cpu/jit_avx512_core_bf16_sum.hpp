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

#ifndef JIT_AVX512_CORE_BF16_SUM_HPP
#define JIT_AVX512_CORE_BF16_SUM_HPP

#include "c_types_map.hpp" // common

#include "cpu_sum_pd.hpp" // cpu
#include "jit_avx512_core_bf16cvt.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

struct jit_sum_conf_t {
    int num_srcs;
    cpu_isa_t isa;
    int is_bf16_dst;
    int typesize_in;
    int typesize_out;
    int loop_unroll;
    int size_blocking; /* minimum recommended data blocking size as this
                          number of elements computes main unrolled loop
                          in jit kernel per iteration */
};

struct jit_sum_call_s {
    const void **srcs;
    const void *dst;
    const void *scales;
    dim_t size;
};

struct jit_avx512_core_bf16_sum_kernel : public jit_generator {
    jit_avx512_core_bf16_sum_kernel(jit_sum_conf_t ajsp)
        : jsp(ajsp), bf16_emu_(nullptr) {
        if (!mayiuse(avx512_core_bf16))
            bf16_emu_ = new bf16_emulation_t(this, bf16_emu_reserved_1,
                    bf16_emu_reserved_2, bf16_emu_reserved_3, bf16_emu_scratch,
                    bf16_emu_reserved_4, bf16_emu_reserved_5);

        this->generate();
        jit_ker = (void (*)(jit_sum_call_s *))this->getCode();
    }

    ~jit_avx512_core_bf16_sum_kernel() { delete bf16_emu_; }

    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_bf16_sum_kernel)

    static status_t init_conf(jit_sum_conf_t &jsp, const int num_srcs,
            const memory_desc_t &dst_d);

    static constexpr int max_num_arrs = 8;
    jit_sum_conf_t jsp;
    void (*jit_ker)(jit_sum_call_s *);

private:
    using reg64_t = const Xbyak::Reg64;
    using reg32_t = const Xbyak::Reg32;
    using reg8_t = const Xbyak::Reg8;
    using zmm_t = const Xbyak::Zmm;
    using ymm_t = const Xbyak::Ymm;
    using mask_t = const Xbyak::Opmask;

    enum { f32_simd_w = 16, bf16_simd_w = 32 };

    reg64_t param = abi_param1; /* may be rcx, note that cl is required
                                    for mask computation */

    reg64_t reg_srcs = abi_not_param1; /* may be rcx, note that cl is required
                                          for mask computation */
    reg64_t reg_idx_table = abi_not_param1; /* may be rcx, note that cl is
                                               required for mask computation */
    reg64_t reg_mask = rsi;
    reg32_t reg32_mask = esi;

    reg64_t reg_dst = rax;
    reg64_t reg_scales = rbx;
    reg64_t reg_sz = rdx;

    reg64_t reg_src[max_num_arrs] = {r8, r9, r10, r11, r12, r13, r14, r15};

    static int max_vregs_available(bool bf16_isa) {
        // one vector registers are reserved for vperm index and zero values
        // additional 5 registers are reserved for bf16 emulation on non-cpx
        return bf16_isa ? 31 : 26;
    }

    int acc_vreg_idx(int i_unroll, int i_acc) {
        // 2 accumulation registers per unroll iteration
        int idx = 2 * i_unroll + i_acc;
        assert(idx < max_vregs_available(isa_has_bf16(jsp.isa)));
        return idx;
    }

    int scale_vreg_idx(int i_acc_iter) {
        int scale_idx_start = 2 * jsp.loop_unroll; // reserved for acc registers
        int idx = scale_idx_start + i_acc_iter;
        assert(idx < max_vregs_available(isa_has_bf16(jsp.isa)));
        return idx;
    }

    int src_vreg_idx(int i_unroll, int i_inp) {
        // reserved for acc and scale registers
        int inp_idx_start
                = 2 * jsp.loop_unroll + utils::div_up(jsp.num_srcs, 2);
        int idx = inp_idx_start + utils::rnd_up(jsp.num_srcs, 2) * i_unroll
                + i_inp;
        assert(idx < max_vregs_available(isa_has_bf16(jsp.isa)));
        return idx;
    }

    int tmp_vreg_idx(int i_unroll, int i_acc_iter) {
        int num_acc_iters = utils::div_up(jsp.num_srcs, 2);
        // reserved for acc, scale and src registers
        int tmp_idx_start = utils::div_up(jsp.num_srcs, 2)
                + (2 + utils::rnd_up(jsp.num_srcs, 2)) * jsp.loop_unroll;
        int idx = tmp_idx_start + num_acc_iters * i_unroll + i_acc_iter;
        assert(idx < max_vregs_available(isa_has_bf16(jsp.isa)));
        return idx;
    }

    static int num_vregs_required(int unroll, int num_srcs) {
        int num_acc_iters = utils::div_up(num_srcs, 2);
        // reserved for acc, scale and src registers
        int num_regs = utils::div_up(num_srcs, 2)
                + (2 + utils::rnd_up(num_srcs, 2)) * unroll;
        // tmp registers
        num_regs += num_acc_iters * unroll;
        return num_regs;
    }

    Xbyak::Zmm bf16_emu_reserved_1 = Xbyak::Zmm(26);
    Xbyak::Zmm bf16_emu_reserved_2 = Xbyak::Zmm(27);
    Xbyak::Zmm bf16_emu_reserved_3 = Xbyak::Zmm(28);
    Xbyak::Zmm bf16_emu_reserved_4 = Xbyak::Zmm(29);
    Xbyak::Zmm bf16_emu_reserved_5 = Xbyak::Zmm(30);
    Xbyak::Reg64 bf16_emu_scratch = abi_not_param1;

    Xbyak::Zmm zmm_idx = Xbyak::Zmm(31);

    Xbyak::Label idx_table;
    const Xbyak::Opmask k_mask = k1;

    void generate();
    void loop_iteration(int current_unroll);
    bf16_emulation_t *bf16_emu_;
};

template <data_type_t src_data_type, data_type_t dst_data_type>
struct jit_bf16_sum_t : public primitive_impl_t {
    struct pd_t : public cpu_sum_pd_t {
        using cpu_sum_pd_t::cpu_sum_pd_t;

        DECLARE_SUM_PD_T(JIT_IMPL_NAME_HELPER("jit_bf16_", jsp_.isa, ""),
                jit_bf16_sum_t);

        virtual status_t init() {
            bool ok = true && mayiuse(avx512_core)
                    && cpu_sum_pd_t::init() == status::success
                    && src_mds_.size()
                            <= jit_avx512_core_bf16_sum_kernel::max_num_arrs;
            if (!ok) return status::unimplemented;

            const memory_desc_wrapper o_d(&dst_md_);
            ok = true && o_d.data_type() == dst_data_type && o_d.is_dense();
            if (!ok) return status::unimplemented;

            const auto n = src_mds_.size();

            if (n > jit_avx512_core_bf16_sum_kernel::max_num_arrs)
                return status::unimplemented;

            for (size_t i = 0; i < n; ++i) {
                const memory_desc_wrapper i_d(&src_mds_[i]);
                ok = true && src_data_type == i_d.data_type()
                        && o_d.similar_to(i_d, true, false, 0)
                        && i_d.is_dense()
                        // is scales representable in bfloat16: scales will be down
                        // converted to bf16 in order to use bf16 vnni instruction
                        && scales_[i] == float(bfloat16_t(scales_[i]));
                if (!ok) return status::unimplemented;
            }

            return jit_avx512_core_bf16_sum_kernel::init_conf(
                    jsp_, src_mds_.size(), dst_md_);
        }
        jit_sum_conf_t jsp_;
    };

    jit_bf16_sum_t(const pd_t *apd) : primitive_impl_t(apd) {
        kernel_ = new jit_avx512_core_bf16_sum_kernel(pd()->jsp_);
    }

    ~jit_bf16_sum_t() { delete kernel_; }

    virtual status_t execute(const exec_ctx_t &ctx) const override;

    typedef typename prec_traits<src_data_type>::type src_data_t;
    typedef typename prec_traits<dst_data_type>::type dst_data_t;
    typedef typename prec_traits<data_type::f32>::type acc_data_t;

private:
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_avx512_core_bf16_sum_kernel *kernel_;
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

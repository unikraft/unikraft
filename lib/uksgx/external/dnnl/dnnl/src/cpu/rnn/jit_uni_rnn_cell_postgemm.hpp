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

#ifndef CPU_JIT_RNN_CELL_POSTGEMM
#define CPU_JIT_RNN_CELL_POSTGEMM

#include "jit_uni_rnn_common_postgemm.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

template <cpu_isa_t isa, impl::data_type_t src_data_t,
        impl::data_type_t scratch_data_t>
struct jit_uni_rnn_cell_postgemm_fwd : public jit_uni_rnn_postgemm {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_uni_rnn_cell_postgemm_fwd)

    typedef typename utils::conditional<isa == avx512_core,
            jit_uni_eltwise_injector_f32<avx512_common>,
            jit_uni_eltwise_injector_f32<isa>>::type injector_t;

    jit_uni_rnn_cell_postgemm_fwd(
            const rnn_utils::rnn_conf_t &rnn, const rnn_pd_t *pd)
        : jit_uni_rnn_postgemm(rnn, pd) {}

    ~jit_uni_rnn_cell_postgemm_fwd() { delete injector_; }

    void init(data_type_t sdt) override {
        jit_uni_rnn_postgemm::init(src_data_t);
        // we use rax for constant tables
        injector_ = new injector_t(
                this, pd_->activation_kind(), 0.0f, 0.0f, true, rax);
        generate();
        kernel_ = (kernel_t)this->getCode();
    }

protected:
    injector_t *injector_;

    // register size in bytes
    using Vmm = typename jit_uni_eltwise_injector_f32<isa>::Vmm;
    size_t vlen = cpu_isa_traits<isa>::vlen;
    size_t vlen_dst
            = vlen / (sizeof(float) / types::data_type_size(src_data_t));
    size_t cstate_dt_size = sizeof(float);
    size_t hstate_dt_size = types::data_type_size(src_data_t);
    size_t gate_dt_size = types::data_type_size(src_data_t);
    size_t scratch_dt_size = types::data_type_size(scratch_data_t);
    size_t qscale_dt_size = sizeof(float);
    size_t bias_dt_size = sizeof(float);

    void generate() {
        using namespace Xbyak;

        // Labels declaration
        Label vector_loop_start_label, vector_loop_end_label;
        Label rem_loop_start_label, rem_loop_end_label;
        Label table_label;

        // Register map
        Reg64 loop_cnt(r11); // loop counter

        // Here we do no unrolling, loop overhead should not be that dramatic
        // We skip vmm0 as it can be used by the injector for masks on sse4.1
        Vmm G(1), tmp1_vmm(5), tmp2_vmm(6), zero_vmm(7);

        auto is_training
                = pd_->desc()->prop_kind == prop_kind::forward_training;

        // We start code generations here
        preamble();

        // extract addresses passed as parameter
        auto addr_ws_gates_reg = abi_param1;
        auto addr_scratch_gates_reg = abi_param2;
        auto addr_bias_reg = abi_param3;
        auto addr_states_t_l_reg = abi_param4;

        auto sg_addr
                = ptr[addr_scratch_gates_reg + 0 * rnn_.dic * scratch_dt_size];
        auto wg_addr = ptr[addr_ws_gates_reg + 0 * rnn_.dic * gate_dt_size];
        auto B_addr = ptr[addr_bias_reg + 0 * rnn_.dic * bias_dt_size];

        // initialize registers with addresses and constants
        init_regs(vlen);
        injector_->load_table_addr();

        mov(loop_cnt, rnn_.dic * scratch_dt_size);
        cmp(loop_cnt, vlen);
        jl(vector_loop_end_label, Xbyak::CodeGenerator::T_NEAR);

        L(vector_loop_start_label);
        {
            // load G
            uni_vmovups(G, sg_addr);

            // dequantize the gates from s32 to f32 if needed
            if (src_data_t == data_type::u8) {
                deq_w(G, tmp1_vmm, tmp2_vmm, 0, true);
            }

            // add biases
            uni_vmovups(tmp1_vmm, B_addr);
            uni_vaddps(G, G, tmp1_vmm);

            // inject eltwise code
            injector_->compute_vector(G.getIdx());

            // if training we write back the gates
            if (is_training) to_src<src_data_t>(wg_addr, G, vlen);

            to_src<src_data_t>(ptr[addr_states_t_l_reg], G, vlen);

            // increment address pointers
            add(addr_scratch_gates_reg, vlen);
            add(addr_bias_reg, vlen);
            add(addr_states_t_l_reg, vlen_dst);
            if (is_training) add(addr_ws_gates_reg, vlen_dst);
            inc_regs(vlen);

            // increment loop counter
            sub(loop_cnt, vlen);
            cmp(loop_cnt, vlen);
            jge(vector_loop_start_label);
        }
        L(vector_loop_end_label);

        cmp(loop_cnt, 0);
        je(rem_loop_end_label, Xbyak::CodeGenerator::T_NEAR);
        // Same code as above, we just use movuss for accessing inputs
        // TODO: smarter handling of tails with Zmm -> Ymm -> Xmm -> scalar
        L(rem_loop_start_label);
        {
            // remaping registers to Xmms
            Xmm Gs(G.getIdx());
            Xmm tmp1s_vmm(tmp1_vmm.getIdx());

            // load G
            uni_vmovss(Gs, sg_addr);

            // dequantize the gates from s32 to f32 if needed
            if (src_data_t == data_type::u8) {
                deq_w(G, tmp1_vmm, tmp2_vmm, 0, false);
            }

            // add biases
            uni_vmovss(tmp1s_vmm, B_addr);
            uni_vaddps(Gs, Gs, tmp1s_vmm);

            // inject eltwise code
            injector_->compute_vector(Gs.getIdx());

            // if training we write back the gates
            if (is_training) to_src<src_data_t>(wg_addr, G, scratch_dt_size);

            to_src<src_data_t>(ptr[addr_states_t_l_reg], G, scratch_dt_size);

            // increment address pointers
            add(addr_scratch_gates_reg, scratch_dt_size);
            add(addr_bias_reg, bias_dt_size);
            add(addr_states_t_l_reg, hstate_dt_size);
            if (is_training) add(addr_ws_gates_reg, gate_dt_size);
            inc_regs(qscale_dt_size);

            // increment loop counter
            sub(loop_cnt, scratch_dt_size);
            cmp(loop_cnt, 0);
            jg(rem_loop_start_label);
        }
        L(rem_loop_end_label);

        postamble();

        // inject the constant table for the activation
        injector_->prepare_table();
        init_table(vlen);
    }
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

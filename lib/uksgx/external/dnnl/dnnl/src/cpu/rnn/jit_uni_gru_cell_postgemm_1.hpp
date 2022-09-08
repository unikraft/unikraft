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

#ifndef CPU_JIT_GRU_CELL_POSTGEMM_PART1
#define CPU_JIT_GRU_CELL_POSTGEMM_PART1

#include "jit_uni_rnn_common_postgemm.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

template <cpu_isa_t isa, impl::data_type_t src_data_t,
        impl::data_type_t scratch_data_t>
struct jit_uni_gru_cell_postgemm_part1_fwd : public jit_uni_rnn_postgemm {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_uni_gru_cell_postgemm_part1_fwd)

    typedef typename utils::conditional<isa == avx512_core,
            jit_uni_eltwise_injector_f32<avx512_common>,
            jit_uni_eltwise_injector_f32<isa>>::type injector_t;

    jit_uni_gru_cell_postgemm_part1_fwd(
            const rnn_utils::rnn_conf_t &rnn, const rnn_pd_t *pd)
        : jit_uni_rnn_postgemm(rnn, pd) {}

    ~jit_uni_gru_cell_postgemm_part1_fwd() { delete sigmoid_injector_; }

    void init(data_type_t sdt) override {
        jit_uni_rnn_postgemm::init(src_data_t);
        // we use rax for both constant tables as they use the same table
        sigmoid_injector_ = new injector_t(
                this, alg_kind::eltwise_logistic, 0.0f, 0.0f, true, rax);
        generate();
        kernel_ = (kernel_t)this->getCode();
    }

protected:
    injector_t *sigmoid_injector_;

    // register size in bytes
    using Vmm = typename jit_uni_eltwise_injector_f32<isa>::Vmm;
    size_t vlen = cpu_isa_traits<isa>::vlen;
    size_t vlen_dst
            = vlen / (sizeof(float) / types::data_type_size(src_data_t));
    size_t hstate_dt_size = types::data_type_size(src_data_t);
    size_t gate_dt_size = types::data_type_size(src_data_t);
    size_t scratch_dt_size = types::data_type_size(scratch_data_t);
    size_t bias_dt_size = sizeof(float);
    size_t qscale_dt_size = sizeof(float);

    void generate() {
        using namespace Xbyak;
        auto is_training
                = pd_->desc()->prop_kind == prop_kind::forward_training;
        // Labels declaration
        Label vector_loop_start_label, vector_loop_end_label;
        Label rem_loop_start_label, rem_loop_end_label;

        // Register map
        Reg64 loop_cnt(r11); // loop counter

        // We skip vmm0 as it can be used by the injector for masks on sse4.1
        Vmm G0(1), G1(2), tmp1_vmm(3);

        // We start code generations here
        preamble();

        // extract addresses passed as parameter
        auto addr_ws_gates_reg = abi_param1;
        auto addr_scratch_gates_reg = abi_param2;
        auto addr_bias_reg = abi_param3;
        auto addr_states_t_l_reg = abi_param4;
#if _WIN32
        auto addr_states_tm1_l_reg = r10;
        mov(addr_states_tm1_l_reg, ptr[rsp + get_size_of_abi_save_regs() + 40]);
#else
        auto addr_states_tm1_l_reg = abi_param5;
#endif
        // helper lambda to address the gates and biases
        auto sg_addr = [&](int i) {
            return ptr[addr_scratch_gates_reg + i * rnn_.dic * scratch_dt_size];
        };
        auto wg_addr = [&](int i) {
            return ptr[addr_ws_gates_reg + i * rnn_.dic * gate_dt_size];
        };
        auto B_addr = [&](int i) {
            return ptr[addr_bias_reg + i * rnn_.dic * bias_dt_size];
        };

        // initialize registers with addresses and constants
        init_regs(vlen);

        // both sigmoid and tanh use the same table so load address just once in rax
        sigmoid_injector_->load_table_addr();

        mov(loop_cnt, rnn_.dic * scratch_dt_size);
        cmp(loop_cnt, vlen);
        jl(vector_loop_end_label, Xbyak::CodeGenerator::T_NEAR);

        L(vector_loop_start_label);
        {
            // Compute gate 0: G0 = sigmoid(G0 + b0)
            uni_vmovups(G0, sg_addr(0));
            uni_vmovups(tmp1_vmm, B_addr(0));
            uni_vaddps(G0, G0, tmp1_vmm);
            sigmoid_injector_->compute_vector(G0.getIdx());
            // we store it for use in postgemm_part2
            uni_vmovups(sg_addr(0), G0);
            if (is_training) to_src<src_data_t>(wg_addr(0), G0, vlen);

            // Compute gate 1:  G1 = sigmoid(G1 + b1)
            uni_vmovups(G1, sg_addr(1));
            uni_vmovups(tmp1_vmm, B_addr(1));
            uni_vaddps(G1, G1, tmp1_vmm);
            sigmoid_injector_->compute_vector(G1.getIdx());
            uni_vmovups(sg_addr(1), G1);
            // if training we write back the gates
            if (is_training) to_src<src_data_t>(wg_addr(1), G1, vlen);

            // states_t_l = states_tm1_l * G1
            to_scratch<src_data_t>(tmp1_vmm, ptr[addr_states_tm1_l_reg], vlen);
            uni_vmulps(G1, G1, tmp1_vmm);
            to_src<src_data_t>(ptr[addr_states_t_l_reg], G1, vlen);

            // increment address pointers
            add(addr_scratch_gates_reg, vlen);
            add(addr_bias_reg, vlen);
            add(addr_states_t_l_reg, vlen_dst);
            add(addr_states_tm1_l_reg, vlen_dst);
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
            Xmm G0s(G0.getIdx()), G1s(G1.getIdx()),
                    tmp1s_vmm(tmp1_vmm.getIdx());

            // Compute gate 0:  G0 = sigmoid(G0 + b0)
            uni_vmovss(G0s, sg_addr(0));
            uni_vaddss(G0s, G0s, B_addr(0));
            sigmoid_injector_->compute_vector(G0s.getIdx());
            // we store it for use in postgemm_part2
            uni_vmovss(sg_addr(0), G0s);
            if (is_training)
                to_src<src_data_t>(wg_addr(0), G0, scratch_dt_size);

            // Compute gate 1: G1 = sigmoid(G1 + b1)
            uni_vmovss(G1s, sg_addr(1));
            uni_vaddss(G1s, G1s, B_addr(1));
            sigmoid_injector_->compute_vector(G1s.getIdx());
            uni_vmovss(sg_addr(1), G1);
            // if training we write back the gates
            if (is_training)
                to_src<src_data_t>(wg_addr(1), G1, scratch_dt_size);

            // states_t_l = states_tm1_l * G1
            to_scratch<src_data_t>(
                    tmp1s_vmm, ptr[addr_states_tm1_l_reg], scratch_dt_size);
            uni_vmulss(G1s, G1s, tmp1s_vmm);
            to_src<src_data_t>(ptr[addr_states_t_l_reg], G1s, scratch_dt_size);

            // increment address pointers
            add(addr_scratch_gates_reg, scratch_dt_size);
            add(addr_bias_reg, bias_dt_size);
            add(addr_states_t_l_reg, hstate_dt_size);
            add(addr_states_tm1_l_reg, hstate_dt_size);
            if (is_training) add(addr_ws_gates_reg, gate_dt_size);
            inc_regs(qscale_dt_size);

            // increment loop counter
            sub(loop_cnt, scratch_dt_size);
            cmp(loop_cnt, 0);
            jg(rem_loop_start_label);
        }
        L(rem_loop_end_label);

        postamble();

        // Again, only one table is needed and shared between sigmoid and tanh
        sigmoid_injector_->prepare_table(true);
        init_table(vlen);
    }
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
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

#ifndef SIMPLE_SUM_HPP
#define SIMPLE_SUM_HPP

#include "dnnl_thread.hpp"
#include "type_helpers.hpp"

#include "cpu_isa_traits.hpp"
#include "cpu_sum_pd.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

namespace {
struct sum_bf16_params_t {
    dim_t ws_cvt_elements_per_thread_;
    dim_t ws_acc_elements_per_thread_;
    dim_t ws_elements_per_thread_;
    dim_t acc_loop_step_;
};
} // namespace

template <data_type_t src_data_type, data_type_t dst_data_type = src_data_type>
struct simple_sum_t : public primitive_impl_t {
    struct pd_t : public cpu_sum_pd_t {
        using cpu_sum_pd_t::cpu_sum_pd_t;

        DECLARE_SUM_PD_T("simple:any", simple_sum_t);

        status_t init() {
            const int n = n_inputs();

            bool ok = true
                    && IMPLICATION(utils::one_of(data_type::bf16, src_data_type,
                                           dst_data_type),
                            mayiuse(avx512_core))
                    && cpu_sum_pd_t::init() == status::success
                    && n <= max_num_arrs;
            if (!ok) return status::unimplemented;

            const memory_desc_wrapper o_d(dst_md());
            ok = ok && o_d.data_type() == dst_data_type && o_d.is_dense();
            if (!ok) return status::unimplemented;

            for (int i = 0; i < n; ++i) {
                const memory_desc_wrapper i_d(src_md(i));
                ok = true && utils::everyone_is(src_data_type, i_d.data_type())
                        && o_d.similar_to(i_d, true, false, 0)
                        && i_d.is_dense();
                if (!ok) return status::unimplemented;
            }

            compute_blocking();
            init_scratchpad();
            return status::success;
        }

        sum_bf16_params_t bf16_p_;
        dim_t block_size_ = 0, nelems_ = 0, blocks_number_ = 0, tail_ = 0;

    private:
        const dim_t cacheline_size_ = 64; // bytes
        const dim_t half_L1_size_ = 16 * 1024; // bytes

        void compute_blocking() {
            block_size_
                    = (src_data_type == data_type::bf16 ? 16 * cacheline_size_
                                                        : half_L1_size_)
                    / sizeof(src_data_type);
            const memory_desc_wrapper o_d(dst_md());
            nelems_ = o_d.nelems();
            blocks_number_ = nelems_ / block_size_;
            tail_ = nelems_ % block_size_;
        }

        void init_scratchpad() {
            if (src_data_type == data_type::bf16) {
                bool is_dst_bf16_ = dst_data_type == data_type::bf16;
                bf16_p_.ws_cvt_elements_per_thread_
                        = cacheline_size_ / sizeof(acc_data_t);

                bf16_p_.ws_acc_elements_per_thread_ = is_dst_bf16_
                        ? bf16_p_.ws_cvt_elements_per_thread_
                        : 0;

                bf16_p_.acc_loop_step_ = is_dst_bf16_
                        ? bf16_p_.ws_cvt_elements_per_thread_
                        : 1;

                bf16_p_.ws_elements_per_thread_
                        = bf16_p_.ws_cvt_elements_per_thread_
                        + bf16_p_.ws_acc_elements_per_thread_;
                dim_t bf16cvt_buf_sz_ = sizeof(acc_data_t)
                        * bf16_p_.ws_elements_per_thread_
                        * dnnl_get_max_threads();
                auto scratchpad = scratchpad_registry().registrar();
                scratchpad.book(memory_tracking::names::key_sum_srcs_cvt,
                        bf16cvt_buf_sz_);
            }
        }
    };

    simple_sum_t(const pd_t *apd) : primitive_impl_t(apd) {}

    virtual status_t execute(const exec_ctx_t &ctx) const override;

    enum { max_num_arrs = 16 };
    typedef typename prec_traits<src_data_type>::type src_data_t;
    typedef typename prec_traits<dst_data_type>::type dst_data_t;
    typedef typename prec_traits<data_type::f32>::type acc_data_t;

private:
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

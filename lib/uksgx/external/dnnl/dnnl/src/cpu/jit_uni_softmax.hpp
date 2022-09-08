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

#ifndef JIT_UNI_SOFTMAX_HPP
#define JIT_UNI_SOFTMAX_HPP

#include <assert.h>

#include "c_types_map.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

#include "cpu_isa_traits.hpp"
#include "cpu_softmax_pd.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

namespace softmax_impl {
template <cpu_isa_t isa>
struct driver_t;
}

template <cpu_isa_t isa>
struct jit_uni_softmax_fwd_t : public primitive_impl_t {
    struct pd_t : public cpu_softmax_fwd_pd_t {
        pd_t(engine_t *engine, const softmax_desc_t *adesc,
                const primitive_attr_t *attr,
                const softmax_fwd_pd_t *hint_fwd_pd)
            : cpu_softmax_fwd_pd_t(engine, adesc, attr, hint_fwd_pd) {}

        DECLARE_COMMON_PD_T(
                JIT_IMPL_NAME_HELPER("jit:", isa, ""), jit_uni_softmax_fwd_t);

        status_t init() {
            auto is_dense = [&]() {
                const memory_desc_wrapper data_d(src_md());
                const auto &bd = data_d.blocking_desc();

                if (!data_d.is_dense(true) || !data_d.only_padded_dim(axis()))
                    return false;

                const auto blk_size = cpu_isa_traits<isa>::vlen / sizeof(float);
                if (data_d.is_plain())
                    return bd.strides[axis()] == 1;
                else {
                    // 31 is a general limit, 2 is for unroll_regs_ = 4;
                    const size_t max_stride = (1LL << (31 - 2)) - 1;
                    const int last_blk = bd.inner_nblks - 1;
                    return true && bd.inner_blks[last_blk] == blk_size
                            && bd.inner_idxs[last_blk] == axis()
                            && sizeof(float) * bd.strides[axis()] < max_stride;
                }
            };

            bool ok = true && mayiuse(isa) && is_fwd() && !has_zero_dim_memory()
                    && src_md()->data_type == data_type::f32
                    && is_dense() // not dense impl can be easily done
                    && attr()->has_default_values();
            if (!ok) return status::unimplemented;

            return status::success;
        };
    };

    jit_uni_softmax_fwd_t(const pd_t *apd);
    ~jit_uni_softmax_fwd_t();

    typedef float data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override;

private:
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }

    softmax_impl::driver_t<isa> *softmax_driver_;
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

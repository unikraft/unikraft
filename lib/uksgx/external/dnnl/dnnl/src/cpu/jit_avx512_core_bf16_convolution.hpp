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

#ifndef CPU_JIT_AVX512_CORE_BF16_CONVOLUTION_HPP
#define CPU_JIT_AVX512_CORE_BF16_CONVOLUTION_HPP

#include "c_types_map.hpp"
#include "dnnl_thread.hpp"
#include "memory_tracking.hpp"
#include "utils.hpp"

#include "cpu_barrier.hpp"
#include "cpu_convolution_pd.hpp"
#include "cpu_reducer.hpp"

#include "jit_avx512_core_bf16_conv_kernel.hpp"
#include "jit_transpose_src_utils.hpp"

namespace dnnl {
namespace impl {
namespace cpu {

struct jit_avx512_core_bf16_convolution_fwd_t : public primitive_impl_t {
    struct pd_t : public cpu_convolution_fwd_pd_t {
        pd_t(engine_t *engine, const convolution_desc_t *adesc,
                const primitive_attr_t *attr,
                const typename pd_t::base_class *hint_fwd_pd)
            : cpu_convolution_fwd_pd_t(engine, adesc, attr, hint_fwd_pd)
            , jcp_() {}

        DECLARE_COMMON_PD_T(JIT_IMPL_NAME_HELPER("jit_bf16:", jcp_.isa, ""),
                jit_avx512_core_bf16_convolution_fwd_t);

        status_t init() {
            bool ok = true && mayiuse(avx512_core) && is_fwd()
                    && set_default_alg_kind(alg_kind::convolution_direct)
                    && (expect_data_types(data_type::bf16, data_type::bf16,
                                data_type::undef, data_type::bf16,
                                data_type::undef)
                            || expect_data_types(data_type::bf16,
                                    data_type::bf16, data_type::undef,
                                    data_type::f32, data_type::undef))
                    && IMPLICATION(with_bias(),
                            utils::one_of(weights_md(1)->data_type,
                                    data_type::f32, data_type::bf16))
                    && !has_zero_dim_memory() && set_default_formats();
            if (!ok) return status::unimplemented;

            status_t status = jit_avx512_core_bf16_fwd_kernel::init_conf(jcp_,
                    *desc(), *src_md(), *weights_md(0), *dst_md(),
                    *weights_md(1), *attr(), dnnl_get_max_threads());
            if (status != status::success) return status::unimplemented;

            auto scratchpad = scratchpad_registry().registrar();
            jit_avx512_core_bf16_fwd_kernel::init_scratchpad(scratchpad, jcp_);

            return status::success;
        }

        jit_conv_conf_t jcp_;

    protected:
        bool set_default_formats() {
            using namespace format_tag;

            auto dat_tag = utils::pick(ndims() - 3, nCw16c, nChw16c, nCdhw16c);
            auto wei_tag = utils::pick(2 * ndims() - 6 + with_groups(),
                    OIw8i16o2i, gOIw8i16o2i, OIhw8i16o2i, gOIhw8i16o2i,
                    OIdhw8i16o2i, gOIdhw8i16o2i);

            return set_default_formats_common(dat_tag, wei_tag, dat_tag);
        }
    };

    jit_avx512_core_bf16_convolution_fwd_t(const pd_t *apd)
        : primitive_impl_t(apd) {
        kernel_ = new jit_avx512_core_bf16_fwd_kernel(
                pd()->jcp_, *pd()->attr());
    }
    ~jit_avx512_core_bf16_convolution_fwd_t() { delete kernel_; }

    typedef typename prec_traits<data_type::bf16>::type src_data_t;
    typedef typename prec_traits<data_type::bf16>::type wei_data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        if (pd()->ndims() == 3)
            execute_forward_1d(ctx);
        else if (pd()->ndims() == 4)
            execute_forward_2d(ctx);
        else if (pd()->ndims() == 5)
            execute_forward_3d(ctx);
        else
            return status::unimplemented;

        if (pd()->wants_zero_pad_dst()) ctx.memory(DNNL_ARG_DST)->zero_pad();

        return status::success;
    }

private:
    void prepare_padded_bias(const char *&bias,
            const memory_tracking::grantor_t &scratchpad) const;
    void execute_forward_1d(const exec_ctx_t &ctx) const;
    void execute_forward_2d(const exec_ctx_t &ctx) const;
    void execute_forward_3d(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }

    jit_avx512_core_bf16_fwd_kernel *kernel_;
};

struct jit_avx512_core_bf16_convolution_bwd_data_t : public primitive_impl_t {
    struct pd_t : public cpu_convolution_bwd_data_pd_t {
        pd_t(engine_t *engine, const convolution_desc_t *adesc,
                const primitive_attr_t *attr,
                const convolution_fwd_pd_t *hint_fwd_pd)
            : cpu_convolution_bwd_data_pd_t(engine, adesc, attr, hint_fwd_pd)
            , jcp_() {}

        DECLARE_COMMON_PD_T(JIT_IMPL_NAME_HELPER("jit_bf16:", jcp_.isa, ""),
                jit_avx512_core_bf16_convolution_bwd_data_t);

        status_t init() {
            using namespace prop_kind;
            bool ok = true && mayiuse(avx512_core) && is_bwd_d()
                    && set_default_alg_kind(alg_kind::convolution_direct)
                    && (expect_data_types(data_type::f32, data_type::bf16,
                                data_type::undef, data_type::bf16,
                                data_type::undef)
                            || expect_data_types(data_type::bf16,
                                    data_type::bf16, data_type::undef,
                                    data_type::bf16, data_type::undef))
                    && !has_zero_dim_memory() && set_default_formats();
            if (!ok) return status::unimplemented;

            status_t status = jit_avx512_core_bf16_bwd_data_kernel::init_conf(
                    jcp_, *desc(), *diff_src_md(), *weights_md(),
                    *diff_dst_md());
            return status;
        }

        jit_conv_conf_t jcp_;

    protected:
        bool set_default_formats() {
            using namespace format_tag;

            auto dat_tag = utils::pick(ndims() - 3, nCw16c, nChw16c, nCdhw16c);
            auto wei_tag = utils::pick(2 * ndims() - 6 + with_groups(),
                    OIw8o16i2o, gOIw8o16i2o, OIhw8o16i2o, gOIhw8o16i2o,
                    OIdhw8o16i2o, gOIdhw8o16i2o);

            return set_default_formats_common(dat_tag, wei_tag, dat_tag);
        }
    };

    jit_avx512_core_bf16_convolution_bwd_data_t(const pd_t *apd)
        : primitive_impl_t(apd) {
        kernel_ = new jit_avx512_core_bf16_bwd_data_kernel(pd()->jcp_);
    }
    ~jit_avx512_core_bf16_convolution_bwd_data_t() { delete kernel_; };

    typedef typename prec_traits<data_type::bf16>::type diff_dst_data_t;
    typedef typename prec_traits<data_type::bf16>::type wei_data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        if (pd()->ndims() < 5)
            execute_backward_data(ctx);
        else if (pd()->ndims() == 5)
            execute_backward_data_3d(ctx);
        else
            assert(!"invalid dimension");

        return status::success;
    }

private:
    void execute_backward_data(const exec_ctx_t &ctx) const;
    void execute_backward_data_3d(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }
    jit_avx512_core_bf16_bwd_data_kernel *kernel_;
};

struct jit_avx512_core_bf16_convolution_bwd_weights_t
    : public primitive_impl_t {
    struct pd_t : public cpu_convolution_bwd_weights_pd_t {
        pd_t(engine_t *engine, const convolution_desc_t *adesc,
                const primitive_attr_t *attr,
                const convolution_fwd_pd_t *hint_fwd_pd)
            : cpu_convolution_bwd_weights_pd_t(engine, adesc, attr, hint_fwd_pd)
            , jcp_() {}

        DECLARE_COMMON_PD_T(JIT_IMPL_NAME_HELPER("jit_bf16:", jcp_.isa, ""),
                jit_avx512_core_bf16_convolution_bwd_weights_t);

        status_t init() {
            bool ok = true && mayiuse(avx512_core) && is_bwd_w()
                    && set_default_alg_kind(alg_kind::convolution_direct)
                    && (expect_data_types(data_type::bf16, data_type::bf16,
                                data_type::undef, data_type::bf16,
                                data_type::undef)
                            || expect_data_types(data_type::bf16,
                                    data_type::f32, data_type::undef,
                                    data_type::bf16, data_type::undef))
                    && IMPLICATION(with_bias(),
                            utils::one_of(diff_weights_md(1)->data_type,
                                    data_type::f32, data_type::bf16))
                    && !has_zero_dim_memory() && set_default_formats();
            if (!ok) return status::unimplemented;

            status_t status = jit_avx512_core_bf16_conv_bwd_weights_kernel_f32::
                    init_conf(jcp_, *desc(), *src_md(), *diff_weights_md(0),
                            *diff_weights_md(1), *diff_dst_md());
            if (status != status::success) return status;

            init_balancers();

            auto scratchpad = scratchpad_registry().registrar();
            jit_avx512_core_bf16_conv_bwd_weights_kernel_f32::init_scratchpad(
                    scratchpad, jcp_);

            auto reducer_bia_scratchpad = memory_tracking::registrar_t(
                    scratchpad, memory_tracking::names::prefix_reducer_bia);
            reducer_bia_conf_.init_scratchpad(reducer_bia_scratchpad);
            return status;
        }

        jit_conv_conf_t jcp_;
        typename cpu_reducer_t<data_type::f32>::conf_t reducer_bia_conf_;

    protected:
        bool set_default_formats() {
            using namespace format_tag;

            auto dat_tag = utils::pick(ndims() - 3, nCw16c, nChw16c, nCdhw16c);
            auto wei_tag = utils::pick(2 * ndims() - 6 + with_groups(),
                    OIw16i16o, gOIw16i16o, OIhw16i16o, gOIhw16i16o, OIdhw16i16o,
                    gOIdhw16i16o);

            return set_default_formats_common(dat_tag, wei_tag, dat_tag);
        }

    private:
        void init_balancers() {
            const size_t max_buffer_size = jcp_.nthr * 3 * 5 * 5 * 16 * 16;
            if (with_bias()) {
                reducer_bia_conf_.init(reduce_balancer_t(jcp_.nthr,
                        jcp_.oc_block, jcp_.ngroups * jcp_.nb_oc, jcp_.mb,
                        max_buffer_size, true));
            }
        }
    };

    jit_avx512_core_bf16_convolution_bwd_weights_t(const pd_t *apd);
    ~jit_avx512_core_bf16_convolution_bwd_weights_t() {
        delete kernel_;
#ifndef BF16_CONV_BWD_W_JIT_KER_USES_PERMW_TRANSPOSITION
        delete trans_kernel_;
        delete trans_dst_kernel_;
#endif
        delete acc_ker_;
        delete reducer_bias_;
    }

    typedef typename prec_traits<data_type::bf16>::type src_data_t;
    typedef typename prec_traits<data_type::bf16>::type diff_dst_data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_backward_weights(ctx);
        return status::success;
    }

private:
    void execute_backward_weights(const exec_ctx_t &ctx) const;
    void prepare_scratchpad_data(const exec_ctx_t &ctx) const;
    struct thread_info_t;
    void compute_diff_weights(const thread_info_t *) const;
    void reduce_and_convert_diff_weights(const thread_info_t *) const;
    void compute_diff_bias(const thread_info_t *, const exec_ctx_t &ctx) const;

    const pd_t *pd() const { return (const pd_t *)primitive_impl_t::pd(); }

    int nthr_, nthr_mb_, nthr_g_, nthr_oc_b_, nthr_ic_b_;

    jit_avx512_core_bf16_conv_bwd_weights_kernel_f32 *kernel_;

    cpu_accumulator_1d_t<data_type::f32> *acc_ker_;
    cpu_reducer_t<data_type::f32> *reducer_bias_;

#ifndef BF16_CONV_BWD_W_JIT_KER_USES_PERMW_TRANSPOSITION
    jit_trans_src_t *trans_kernel_;
    jit_trans_dst_t *trans_dst_kernel_;
#endif
};

} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s

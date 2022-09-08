/*******************************************************************************
 * Copyright 2018 Intel Corporation
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

#ifndef RNN_HPP
#define RNN_HPP

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "common.hpp"
#include "dnn_types.hpp"
#include "dnnl_common.hpp"
#include "dnnl_debug.hpp"
#include "dnnl_memory.hpp"
#include "perf_report.hpp"

#define AOC array_offset_calculator

namespace rnn {

enum alg_t { VANILLA_RNN, VANILLA_LSTM, VANILLA_GRU, LBR_GRU };
alg_t str2alg(const char *str);
const char *alg2str(alg_t alg);
dnnl_alg_kind_t alg2kind(alg_t alg);

enum activation_t { UNDEF, RELU, LOGISTIC, TANH };
activation_t str2activation(const char *str);
const char *activation2str(activation_t alg);
dnnl_alg_kind_t activation2kind(activation_t alg);

dnnl_rnn_direction_t str2direction(const char *str);
const char *direction2str(dnnl_rnn_direction_t direction);

const int H = 0;
const int C = 1;

template <typename Telem>
struct array_offset_calculator {
    template <typename... Targs>
    array_offset_calculator(Telem *base, Targs... Fargs)
        : _size(sizeof...(Fargs)) {
        const int64_t init_list[] = {Fargs...};
        _dims = new int64_t[_size];
        for (int64_t i = 0; i < _size; ++i)
            _dims[i] = init_list[i];

        _base_ptr = base;
    }
    ~array_offset_calculator() { delete[] _dims; }
    template <typename... Targs>
    inline Telem &operator()(Targs... Fargs) {
        return *(_base_ptr + _offset(1, Fargs...));
    }

private:
    template <typename... Targs>
    inline int64_t _offset(int64_t const dimension, int64_t element) {
        return element;
    }

    template <typename... Targs>
    inline int64_t _offset(
            int64_t const dimension, int64_t theta, int64_t element) {
        return element + (_dims[dimension] * theta);
    }

    template <typename... Targs>
    inline int64_t _offset(int64_t const dimension, int64_t theta,
            int64_t element, Targs... Fargs) {
        int64_t t_prime = element + (_dims[dimension] * theta);
        return _offset(dimension + 1, t_prime, Fargs...);
    }

    Telem *_base_ptr;
    int64_t _size;
    int64_t *_dims;

    BENCHDNN_DISALLOW_COPY_AND_ASSIGN(array_offset_calculator);
};

struct desc_t {
    int64_t sic;
    int64_t slc;
    int64_t dic;
    int64_t dlc;
    int64_t wc;
    int64_t mb;
    int64_t n_layer;
    int64_t n_iter;
    const char *name;
};
int str2desc(desc_t *desc, const char *str);

enum rnn_data_kind_t {
    input,
    states,
    c_states,
    weights_input,
    weights_states,
    bias,
    dst_last_iteration,
    dst_c_last_iteration,
    dst_last_layer,

    dst_diff_input,
    dst_diff_states,
    dst_diff_c_states,
    dst_diff_weights_input,
    dst_diff_weights_states,
    dst_diff_bias,
    diff_last_iteration,
    diff_c_last_iteration,
    diff_last_layer,
    data_kind_total // should be last to provide the total number of data kinds
};

inline const char *rnn_data_kind2str(rnn_data_kind_t kind) {
    switch (kind) {
        case input: return "INPUT";
        case states: return "STATES";
        case c_states: return "STATES";
        case weights_input: return "WEIGHTS_INPUT";
        case weights_states: return "WEIGHTS_STATES";
        case bias: return "BIAS";
        case dst_last_layer: return "DST_LAST_LAYER";
        case dst_last_iteration: return "DST_LAST_ITERATION";
        case dst_c_last_iteration: return "DST_C_LAST_ITERATION";

        case dst_diff_input: return "DST_DIFF_INPUT";
        case dst_diff_states: return "DST_DIFF_STATES";
        case dst_diff_c_states: return "DST_DIFF_C_STATES";
        case dst_diff_weights_input: return "DST_DIFF_WEIGHTS_INPUT";
        case dst_diff_weights_states: return "DST_DIFF_WEIGHTS_STATES";
        case dst_diff_bias: return "DST_DIFF_BIAS";
        case diff_last_layer: return "DIFF_LAST_LAYER";
        case diff_last_iteration: return "DIFF_LAST_ITERATION";
        case diff_c_last_iteration: return "DIFF_C_LAST_ITERATION";
        default:
            assert(!"incorrect rnn data kind");
            return "incorrect rnn data kind";
    }
}

/** configuration structure, that controls initial data filling + error check
*
* dt defines precision
*
* for each lst data kind the values are filled as follows:
* if (rand() > f_sparsity) then:
*     v <-- f_base
* else:
*     v <-- f_min + rand() * f_step % (f_max - f_min)
*
* on final check the resulting values should be in [min .. max] range, the
* relative difference should not exceed eps
*/

typedef struct dt_conf_t {
    dnnl_data_type_t dt;
    int min, max; /* representative */
    float f_min, f_max; /* fill range */
    float f_mean,
            f_stddev; /* mean and std deviation of normally distributed data */
    double eps; /* acceptable error */
} _dt_conf_t[data_kind_total];

extern const _dt_conf_t conf_f32;
extern const _dt_conf_t conf_bf16;
extern const _dt_conf_t conf_f16;
extern const _dt_conf_t conf_u8u8u8u8;
extern const _dt_conf_t conf_u8u8u8f32;
extern const _dt_conf_t conf_f32u8f32f32;
extern const _dt_conf_t conf_f32u8f32u8;

const dt_conf_t *str2cfg(const char *str);
const char *cfg2str(const dt_conf_t *cfg);

inline bool is_cfg_u8(const dt_conf_t *cfg) {
    return cfg == conf_u8u8u8u8 || cfg == conf_u8u8u8f32
            || cfg == conf_f32u8f32f32 || cfg == conf_f32u8f32u8;
}

struct prb_t : public desc_t {
    prb_t(const desc_t &desc, const dt_conf_t *cfg, dnnl_prop_kind_t prop,
            alg_t alg, dnnl_rnn_direction_t direction, const attr_t &attr,
            policy_t scale_policy, unsigned int flags, activation_t activation,
            float alpha, float beta, bool skip_nonlinear, int mb = 0)
        : desc_t(desc)
        , cfg(cfg)
        , prop(prop)
        , alg(alg)
        , direction(direction)
        , flags(flags)
        , activation(activation)
        , alpha(alpha)
        , beta(beta)
        , attr(attr)
        , scale_policy(scale_policy)
        , ops(0.0)
        , skip_nonlinear(skip_nonlinear)
        , linear_cscale(0.0f) {

        if (mb) this->mb = mb;
        count_ops();
        wc = MAX2(sic, MAX2(slc, dic));

        wei_oc_scales = nullptr;
        linear_scales = nullptr;

        // We always allocate linear scales. Even if they are not
        // used, they get dereferenced when built in debug mode.
        linear_scales = (float *)zmalloc(sizeof(float) * n_gates(), 64);
        // Here we use the range of INPUT to set the scales
        set_tparams(cfg[input].f_min, cfg[input].f_max);

        if (scale_policy == policy_t::PER_OC)
            wei_oc_scales
                    = (float *)zmalloc(sizeof(float) * dic * n_gates(), 64);
        set_qparams(-1., 1.);
    }
    ~prb_t() {
        if (wei_oc_scales) zfree(wei_oc_scales);
        if (linear_scales) zfree(linear_scales);
    }

    void count_ops() {
        // Here, we count only the ops in GEMM portion as there is no
        // theoretical number of ops for the post-gemm operations
        int64_t num_cells = (int64_t)n_dir() * n_layer * n_iter;
        int64_t cell_ops = (int64_t)2 * (n_gates() * dic) * mb * (sic + slc);
        int64_t prop_multiplier = prop == dnnl_backward ? 2 : 1;
        ops = prop_multiplier * num_cells * cell_ops;
    }

    int64_t n_dir() const {
        return (direction == dnnl_bidirectional_concat
                       || direction == dnnl_bidirectional_sum)
                ? 2
                : 1;
    }
    int64_t n_weights() const { return 1; }
    int64_t n_states() const { return alg == VANILLA_LSTM ? 2 : 1; }
    int64_t n_gates() const {
        return alg == VANILLA_LSTM
                ? 4
                : (alg == VANILLA_GRU || alg == LBR_GRU ? 3 : 1);
    }
    int64_t n_bias() const {
        return alg == LBR_GRU ? n_gates() + 1 : n_gates();
    }

    const dt_conf_t *cfg;
    dnnl_prop_kind_t prop;
    alg_t alg;
    dnnl_rnn_direction_t direction;
    unsigned int flags;
    activation_t activation;
    float alpha;
    float beta;
    attr_t attr;
    policy_t scale_policy;

    double ops;

    float data_scale, data_shift;
    float wei_scale;
    float *wei_oc_scales;

    bool skip_nonlinear;
    float *linear_scales;
    float linear_cscale;

private:
    /* Todo: fused the two functions in set_shifts_scales */
    void set_qparams(float fp_min, float fp_max);
    void set_tparams(float fp_min, float fp_max);
    prb_t(const prb_t &) = delete;
    prb_t &operator=(const prb_t &) = delete;
};
std::ostream &operator<<(std::ostream &s, const prb_t &p);

struct perf_report_t : public base_perf_report_t {
    using base_perf_report_t::base_perf_report_t;

    void report(const prb_t *p, const res_t *r, const char *prb_str) {
        p_ = p;
        base_report(r, prb_str);
    }

    virtual void dump_cfg(std::ostream &s) const override {
        s << cfg2str(p_->cfg);
    }

    virtual void dump_desc_csv(std::ostream &s) const override {
        s << alg2str(p_->alg) << "_" << activation2str(p_->activation) << "_"
          << direction2str(p_->direction);
        s << "l" << p_->n_layer << "d" << p_->n_dir() << "t" << p_->n_iter
          << "mb" << p_->mb << "_"
          << "slc" << p_->slc << "sic" << p_->sic << "dic" << p_->dic;
    }

    virtual double ops() const override { return p_->ops; }
    virtual const char *name() const override { return p_->name; }
    virtual const dnnl_prop_kind_t *prop() const override { return &p_->prop; }

private:
    const prb_t *p_ = nullptr;
};

void rnn_linear_fwd(const prb_t &p, const float *src_iter_,
        const float *src_iter_c_, const float *src_layer_,
        const float *weights_layer_, const float *weights_iter_h_,
        const float *bias_, float *dst_iter_, float *dst_iter_c_,
        float *dst_layer_, float *ws_, float *gates_);

void compute_ref_fwd(const prb_t &p, dnn_mem_t &input_m, dnn_mem_t &states_m,
        dnn_mem_t &c_states_m, dnn_mem_t &weights_input_m,
        dnn_mem_t &weights_states_m, dnn_mem_t &bias_m,
        dnn_mem_t &dst_last_layer_m, dnn_mem_t &dst_last_iteration_m,
        dnn_mem_t &dst_c_last_iteration_m);

void compute_ref_bwd(const prb_t &p, dnn_mem_t &input_m, dnn_mem_t &states_m,
        dnn_mem_t &c_states_m, dnn_mem_t &diff_last_layer_m,
        dnn_mem_t &diff_last_iteration_m, dnn_mem_t &diff_c_last_iteration_m,
        dnn_mem_t &weights_input_m, dnn_mem_t &weights_states_m,
        dnn_mem_t &bias_m, dnn_mem_t &dst_last_layer_m,
        dnn_mem_t &dst_last_iteration_m, dnn_mem_t &dst_c_last_iteration_m,
        dnn_mem_t &dst_diff_input_m, dnn_mem_t &dst_diff_states_m,
        dnn_mem_t &dst_diff_c_states_m, dnn_mem_t &dst_diff_weights_input_m,
        dnn_mem_t &dst_diff_weights_states_m, dnn_mem_t &dst_diff_bias_m);

// dnnl_ntc
inline size_t ntc_off_f(const prb_t &p, int64_t n, int64_t t, int64_t c) {
    return (n * p.n_iter + t) * p.slc + c;
}

inline void inv_ntc_off_f(
        const prb_t &p, size_t off, int64_t &n, int64_t &t, int64_t &c) {
    c = off % p.slc;
    off /= p.slc;
    t = off % p.n_iter;
    off /= p.n_iter;
    n = off % p.mb;
    off /= p.mb;
    assert(off == 0);
}

// dnnl_ldnc
inline size_t ldnc_off_f(
        const prb_t &p, int64_t l, int64_t d, int64_t n, int64_t c) {
    return ((l * p.n_dir() + d) * p.mb + n) * p.sic + c;
}

inline void inv_ldnc_off_f(const prb_t &p, size_t off, int64_t &l, int64_t &d,
        int64_t &n, int64_t &c) {
    c = off % p.sic;
    off /= p.sic;
    n = off % p.mb;
    off /= p.mb;
    d = off % p.n_dir();
    off /= p.n_dir();
    l = off % p.n_layer;
    off /= p.n_layer;
    assert(off == 0);
}

// dnnl_ldigo
inline size_t ldigo_off_f(const prb_t &p, int64_t l, int64_t d, int64_t w,
        int64_t ic, int64_t oc) {
    return (((l * p.n_dir() + d) * p.n_weights() + w) * (4 * p.slc) + ic)
            * p.sic
            + oc;
}

inline void inv_ldigo_off_f(const prb_t &p, size_t off, int64_t &l, int64_t &d,
        int64_t &w, int64_t &ic, int64_t &oc) {
    oc = off % p.sic;
    off /= p.sic;
    ic = off % (4 * p.slc);
    off /= (4 * p.slc);
    w = off % p.n_weights();
    off /= p.n_weights();
    d = off % p.n_dir();
    off /= p.n_dir();
    l = off % p.n_layer;
    off /= p.n_layer;
    assert(off == 0);
}

// dnnl_ldwOcIc
inline size_t ldwOcIc_off_f(const prb_t &p, int64_t l, int64_t d, int64_t w,
        int64_t oc, int64_t ic) {
    return (((l * p.n_dir() + d) * p.n_weights() + w) * (4 * p.sic) + oc)
            * p.slc
            + ic;
}

inline void inv_ldwOcIc_off_f(const prb_t &p, size_t off, int64_t &l,
        int64_t &d, int64_t &w, int64_t &oc, int64_t &ic) {
    ic = off % p.slc;
    off /= p.slc;
    oc = off % (4 * p.sic);
    off /= (4 * p.sic);
    w = off % p.n_weights();
    off /= p.n_weights();
    d = off % p.n_dir();
    off /= p.n_dir();
    l = off % p.n_layer;
    off /= p.n_layer;
    assert(off == 0);
}

// bias: dnnl_ldgo
inline size_t ldgo_off_f(
        const prb_t &p, int64_t l, int64_t d, int64_t b, int64_t c) {
    return ((l * p.n_dir() + d) * p.n_bias() + b) * p.sic + c;
}

inline void inv_ldgo_off_f(const prb_t &p, size_t off, int64_t &l, int64_t &d,
        int64_t &b, int64_t &c) {
    c = off % p.dic;
    off /= p.dic;
    b = off % p.n_bias();
    off /= p.n_bias();
    d = off % p.n_dir();
    off /= p.n_dir();
    l = off % p.n_layer;
    off /= p.n_layer;
    assert(off == 0);
}

// dst_last_layer: dnnl_tnc
inline size_t tnc_off_f(const prb_t &p, int64_t t, int64_t n, int64_t c) {
    return (t * p.mb + n) * p.sic + c;
}

inline void inv_tnc_off_f(
        const prb_t &p, size_t off, int64_t &t, int64_t &n, int64_t &c) {
    auto cout = p.sic * (1 + (p.direction == dnnl_bidirectional_concat));
    c = off % cout;
    off /= cout;
    n = off % p.mb;
    off /= p.mb;
    t = off % p.n_iter;
    off /= p.n_iter;
    assert(off == 0);
}

void check_case_validity(const dt_conf_t *cfg, policy_t policy);

int doit(const prb_t &p, res_t *res);
int bench(int argc, char **argv);

} // namespace rnn

#endif

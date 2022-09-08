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

#include "dnnl_debug.hpp"

#include "binary/binary.hpp"

namespace binary {

alg_t str2alg(const char *str) {
#define CASE(_alg) \
    if (!strcasecmp(STRINGIFY(_alg), str)) return _alg
    CASE(ADD);
    CASE(MUL);
#undef CASE
    assert(!"unknown algorithm");
    return ADD;
}

const char *alg2str(alg_t alg) {
    if (alg == ADD) return "ADD";
    if (alg == MUL) return "MUL";
    assert(!"unknown algorithm");
    return "unknown algorithm";
}

dnnl_alg_kind_t alg2alg_kind(alg_t alg) {
    if (alg == ADD) return dnnl_binary_add;
    if (alg == MUL) return dnnl_binary_mul;
    assert(!"unknown algorithm");
    return dnnl_alg_kind_undef;
}

std::ostream &operator<<(std::ostream &s, const prb_t &p) {
    dump_global_params(s);

    if (!(p.sdt[0] == dnnl_f32 && p.sdt[1] == dnnl_f32))
        s << "--sdt=" << p.sdt << " ";
    if (p.ddt != dnnl_f32) s << "--ddt=" << dt2str(p.ddt) << " ";
    if (!(p.stag[0] == dnnl_nchw && p.stag[1] == dnnl_nchw))
        s << "--stag=" << p.stag << " ";
    if (p.alg != ADD) s << "--alg=" << alg2str(p.alg) << " ";
    if (p.scale_policy != policy_t::NONE)
        s << "--scaling=" << attr_t::scale_t::policy2str(p.scale_policy) << " ";
    if (p.inplace != true) s << "--inplace=" << bool2str(p.inplace) << " ";

    s << p.sdims;

    return s;
}

} // namespace binary

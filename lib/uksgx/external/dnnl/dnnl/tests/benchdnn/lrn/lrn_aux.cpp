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

#include <assert.h>
#include <stdlib.h>

#include "lrn/lrn.hpp"

namespace lrn {

alg_t str2alg(const char *str) {
#define CASE(_alg) \
    if (!strcasecmp(STRINGIFY(_alg), str)) return _alg
    CASE(ACROSS);
    CASE(WITHIN);
#undef CASE
    assert(!"unknown algorithm");
    return ACROSS;
}

const char *alg2str(alg_t alg) {
    if (alg == ACROSS) return "ACROSS";
    if (alg == WITHIN) return "WITHIN";
    assert(!"unknown algorithm");
    return "unknown algorithm";
}

dnnl_alg_kind_t alg2alg_kind(alg_t alg) {
    if (alg == ACROSS) return dnnl_lrn_across_channels;
    if (alg == WITHIN) return dnnl_lrn_within_channel;
    assert(!"unknown algorithm");
    return dnnl_alg_kind_undef;
}

int str2desc(desc_t *desc, const char *str) {
    /* canonical form:
     * mbXicXidXihXiwX_lsXalphaYbetaYkY_nS
     *
     * where:
     *  X is number (integer)
     *  Y is real (float)
     *  S - string
     * note: symbol `_` is ignored
     *
     * implicit rules:
     *  alpha = 1 / 8192 = 0.000122 ~~ 0.0001, but has exact representation
     *  beta = 0.75
     *  k = 1
     *  ls = 5
     *  S = "wip"
     *  if iw is unset iw <-- ih
     *  if ih is unset ih <-- 1
     *  if id is unset id <-- 1
     */

    desc_t d {0};
    d.mb = 2;
    d.ls = 5;
    d.alpha = 1. / 8192;
    d.beta = 0.75;
    d.k = 1;
    d.name = "\"wip\"";

    const char *s = str;
    assert(s);

    auto mstrtol = [](const char *nptr, char **endptr) {
        return strtol(nptr, endptr, 10);
    };

#define CASE_NN(p, c, cvfunc) \
    do { \
        if (!strncmp(p, s, strlen(p))) { \
            ok = 1; \
            s += strlen(p); \
            char *end_s; \
            d.c = cvfunc(s, &end_s); \
            s += (end_s - s); \
            if (d.c < 0) return FAIL; \
            /* printf("@@@debug: %s: " IFMT "\n", p, d. c); */ \
        } \
    } while (0)
#define CASE_N(c, cvfunc) CASE_NN(#c, c, cvfunc)
    while (*s) {
        int ok = 0;
        CASE_N(mb, mstrtol);
        CASE_N(ic, mstrtol);
        CASE_N(id, mstrtol);
        CASE_N(ih, mstrtol);
        CASE_N(iw, mstrtol);
        CASE_N(ls, mstrtol);
        CASE_N(alpha, strtof);
        CASE_N(beta, strtof);
        CASE_N(k, strtof);
        if (*s == 'n') {
            d.name = s + 1;
            break;
        }
        if (*s == '_') ++s;
        if (!ok) return FAIL;
    }
#undef CASE_NN
#undef CASE_N

    if (d.ic == 0 || (d.id == 0 && d.ih == 0 && d.iw == 0)) return FAIL;

    if (d.id == 0) d.id = 1;
    if (d.ih == 0) d.ih = 1;
    if (d.iw == 0) d.iw = d.ih;

    *desc = d;

    return OK;
}

std::ostream &operator<<(std::ostream &s, const desc_t &d) {
    const bool canonical = s.flags() & std::ios_base::fixed;

    if (canonical || d.mb != 2) s << "mb" << d.mb;

    s << "ic" << d.ic;

    if (d.id > 1) s << "id" << d.id;
    s << "ih" << d.ih;
    if (canonical || d.iw != d.ih || d.id > 1) s << "iw" << d.iw;

    if (canonical || d.ls != 5) s << "ls" << d.ls;
    if (canonical || d.alpha != 1. / 8192) s << "alpha" << d.alpha;
    if (canonical || d.beta != 0.75) s << "beta" << d.beta;
    if (canonical || d.k != 1) s << "k" << d.k;

    s << "n" << d.name;

    return s;
}

std::ostream &operator<<(std::ostream &s, const prb_t &p) {
    dump_global_params(s);

    if (p.dir != FWD_D) s << "--dir=" << dir2str(p.dir) << " ";
    if (p.dt != dnnl_f32) s << "--dt=" << dt2str(p.dt) << " ";
    if (p.tag != dnnl_nchw) s << "--tag=" << fmt_tag2str(p.tag) << " ";
    if (p.alg != ACROSS) s << "--alg=" << alg2str(p.alg) << " ";

    s << static_cast<const desc_t &>(p);

    return s;
}

} // namespace lrn

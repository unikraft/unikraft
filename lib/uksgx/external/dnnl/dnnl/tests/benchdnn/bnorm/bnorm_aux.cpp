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
#include "bnorm/bnorm.hpp"

namespace bnorm {

check_alg_t str2check_alg(const char *str) {
    if (!strcasecmp("alg_0", str)) return ALG_0;
    if (!strcasecmp("alg_1", str)) return ALG_1;
    return ALG_AUTO;
}

const char *check_alg2str(check_alg_t alg) {
    switch (alg) {
        case ALG_0: return "alg_0";
        case ALG_1: return "alg_1";
        case ALG_AUTO: return "alg_auto";
    }
    return "alg_auto";
}

flags_t str2flags(const char *str) {
    flags_t flags = (flags_t)0;
    while (str && *str) {
        if (*str == 'G') flags |= GLOB_STATS;
        if (*str == 'S') flags |= USE_SCALESHIFT;
        if (*str == 'R') flags |= FUSE_NORM_RELU;
        str++;
    }
    return flags;
}

const char *flags2str(flags_t flags) {
    if (flags & GLOB_STATS) {
        if (flags & USE_SCALESHIFT)
            return flags & FUSE_NORM_RELU ? "GSR" : "GS";
        return flags & FUSE_NORM_RELU ? "GR" : "G";
    }

    if (flags & USE_SCALESHIFT) return flags & FUSE_NORM_RELU ? "SR" : "S";

    return flags & FUSE_NORM_RELU ? "R" : "";
}

int str2desc(desc_t *desc, const char *str) {
    /* canonical form:
     * mbXicXihXiwXidXepsYnS
     *
     * where:
     *  X is number (integer)
     *  Y is real (float)
     *  S - string
     * note: symbol `_` is ignored
     *
     * implicit rules:
     *  eps = 1./16
     *  S = "wip"
     *  if iw is unset iw <-- ih
     *  if ih is unset ih <-- iw
     *  if id is unset id <-- 1
     */

    desc_t d {0};
    d.mb = 2;
    d.eps = 1.f / 16;
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
        CASE_N(eps, strtof);
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

    if (canonical || d.eps != 1.f / 16) s << "eps" << d.eps;

    s << "n" << d.name;

    return s;
}

std::ostream &operator<<(std::ostream &s, const prb_t &p) {
    dump_global_params(s);

    if (p.dir != FWD_D) s << "--dir=" << dir2str(p.dir) << " ";
    if (p.dt != dnnl_f32) s << "--dt=" << dt2str(p.dt) << " ";
    if (p.tag != dnnl_nchw) s << "--tag=" << fmt_tag2str(p.tag) << " ";
    if (p.flags != (flags_t)0) s << "--flags=" << flags2str(p.flags) << " ";
    if (p.check_alg != ALG_AUTO)
        s << "--check-alg=" << check_alg2str(p.check_alg) << " ";
    if (!p.attr.is_def()) s << "--attr=\"" << p.attr << "\" ";
    if (p.inplace != true) s << "--inplace=" << bool2str(p.inplace) << " ";

    s << static_cast<const desc_t &>(p);

    return s;
}

} // namespace bnorm

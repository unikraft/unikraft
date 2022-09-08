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

#include <stdio.h>
#include <stdlib.h>

#include <sstream>

#include "dnnl.h"

#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"
#include "parser.hpp"

#include "concat/concat.hpp"

namespace concat {

std::vector<dnnl_data_type_t> sdt {dnnl_f32};
std::vector<dnnl_data_type_t> ddt {dnnl_f32};
std::vector<std::vector<dnnl_format_tag_t>> stag {{dnnl_nchw, dnnl_nchw}};
std::vector<dnnl_format_tag_t> dtag {dnnl_format_tag_undef};
std::vector<int> axis {1};

std::vector<dims_t> sdims;
bool allow_unimpl = false;
const char *perf_template_csv
        = "perf,%engine%,%sdt%,%ddt%,%stag%,%dtag%,%axis%,%DESC%,%-time%,%"
          "0time%";
const char *perf_template_def = "perf,%engine%,%desc%,%-time%,%0time%";
const char *perf_template = perf_template_def;

void reset_parameters() {
    sdt = {dnnl_f32};
    ddt = {dnnl_f32};
    stag = {{dnnl_nchw, dnnl_nchw}};
    dtag = {dnnl_format_tag_undef};
    axis = {1};
    allow_unimpl = false;
}

void check_correctness() {
    for_(const auto &i_sdt : sdt)
    for_(const auto &i_ddt : ddt)
    for_(const auto &i_stag : stag)
    for_(const auto &i_dtag : dtag)
    for (const auto &i_axis : axis) {
        if (sdims.size() != i_stag.size()) // expect 1:1 match of sdims and tag
            SAFE_V(FAIL);

        const prb_t p(sdims, i_sdt, i_ddt, i_stag, i_dtag, i_axis);
        std::stringstream ss;
        ss << p;
        const std::string cpp_pstr = ss.str();
        const char *pstr = cpp_pstr.c_str();
        print(1, "run: %s\n", pstr);

        res_t res {};
        int status = doit(&p, &res);

        bool want_perf_report = false;
        parse_result(res, want_perf_report, allow_unimpl, status, pstr);

        if (want_perf_report && bench_mode & PERF) {
            perf_report_t pr(perf_template);
            pr.report(&p, &res, pstr);
        }

        benchdnn_stat.tests++;
    }
}

int bench(int argc, char **argv) {
    driver_name = "concat";
    using namespace parser;
    for (; argc > 0; --argc, ++argv) {
        const bool parsed_options = false || parse_bench_settings(argv[0])
                || parse_batch(bench, argv[0]) || parse_dt(sdt, argv[0], "sdt")
                || parse_dt(ddt, argv[0], "ddt")
                || parse_multi_tag(stag, argv[0])
                || parse_tag(dtag, argv[0], "dtag") || parse_axis(axis, argv[0])
                || parse_allow_unimpl(allow_unimpl, argv[0])
                || parse_perf_template(perf_template, perf_template_def,
                        perf_template_csv, argv[0])
                || parse_reset(reset_parameters, argv[0]);
        if (!parsed_options) {
            catch_unknown_options(argv[0]);

            parse_multi_dims(sdims, argv[0]);
            check_correctness();
        }
    }

    return parse_last_argument();
}

} // namespace concat

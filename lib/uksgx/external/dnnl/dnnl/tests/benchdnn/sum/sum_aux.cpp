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
#include "sum/sum.hpp"

namespace sum {

std::ostream &operator<<(std::ostream &s, const std::vector<float> &scales) {
    s << scales[0];
    for (int i = 1; i < (int)scales.size(); ++i)
        s << ":" << scales[i];
    return s;
}

std::ostream &operator<<(std::ostream &s, const prb_t &p) {
    dump_global_params(s);

    if (!(p.n_inputs() == 2 && p.sdt[0] == dnnl_f32 && p.sdt[1] == dnnl_f32))
        s << "--sdt=" << p.sdt << " ";
    if (p.ddt != dnnl_f32) s << "--ddt=" << dt2str(p.ddt) << " ";
    if (!(p.n_inputs() == 2 && p.stag[0] == dnnl_nchw
                && p.stag[1] == dnnl_nchw))
        s << "--stag=" << p.stag << " ";
    if (p.dtag != dnnl_format_tag_undef)
        s << "--dtag=" << fmt_tag2str(p.dtag) << " ";
    s << "--scales=" << p.scales << " ";

    s << p.dims;

    return s;
}

} // namespace sum

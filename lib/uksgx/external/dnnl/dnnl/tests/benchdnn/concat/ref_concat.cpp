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

#include "src/common/dnnl_thread.hpp"

#include "concat/concat.hpp"

namespace concat {

void get_sizes(const prb_t *p, int64_t &outer_size, int64_t &inner_size,
        int64_t &axis_size) {
    outer_size = inner_size = 1;
    for (int i = 0; i < p->axis; i++)
        outer_size *= p->sdims[0][i];
    for (int i = p->axis + 1; i < (int)p->sdims[0].size(); i++)
        inner_size *= p->sdims[0][i];
    axis_size = p->axis_size();
}

void compute_ref(
        const prb_t *p, const std::vector<dnn_mem_t> &src, dnn_mem_t &dst) {
    int64_t outer_size {0}, inner_size {0}, axis_size {0};
    get_sizes(p, outer_size, inner_size, axis_size);

    float *dst_ptr = (float *)dst;

    dnnl::impl::parallel_nd(
            outer_size, inner_size, [&](int64_t ou, int64_t in) {
                int64_t off_dst = ou * axis_size * inner_size;
                for (int i_input = 0; i_input < p->n_inputs(); ++i_input) {
                    const float *src_ptr = (const float *)src[i_input];
                    int64_t i_axis_size = p->sdims[i_input][p->axis];
                    int64_t off_src = ou * i_axis_size * inner_size;

                    for (int64_t as = 0; as < i_axis_size; ++as) {
                        int64_t idx = as * inner_size + in;
                        dst_ptr[off_dst + idx] = src_ptr[off_src + idx];
                    }
                    off_dst += i_axis_size
                            * inner_size; // the next input start point
                }
            });
}

} // namespace concat

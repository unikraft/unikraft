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

#include "binary/binary.hpp"

namespace binary {

void perform_op(const prb_t *p, float *d, float x, float y) {
    if (p->alg == ADD) {
        *d = x + y;
    } else if (p->alg == MUL) {
        *d = x * y;
    } else {
        assert(!"operation not supported!");
    }
};

int64_t map_idx_B(const prb_t *p, int64_t idx) {
    dims_t dims = off2dims_idx(p->sdims[0], idx);
    for (size_t d = 0; d < dims.size(); ++d)
        dims[d] *= (!p->broadcast_dims[d]);
    return dims_off(p->sdims[1], dims);
}

void compute_ref(const prb_t *p, const std::vector<dnn_mem_t> &src,
        const std::vector<dnn_mem_t> &scale, dnn_mem_t &dst) {
    float *dst_ptr = (float *)dst;
    const float *A = (const float *)src[0];
    const float *B = (const float *)src[1];
    const float alpha = p->scale_policy == policy_t::NONE
            ? 1
            : ((const float *)scale[0])[0];
    const float beta = p->scale_policy == policy_t::NONE
            ? 1
            : ((const float *)scale[1])[0];
    const auto nelems_A = src[0].nelems();
    const auto nelems_B = src[1].nelems();

    dnnl::impl::parallel_nd(nelems_A, [&](int64_t i) {
        int64_t idx_B = nelems_B == nelems_A ? i : map_idx_B(p, i);
        perform_op(p, &dst_ptr[i], alpha * A[i], beta * B[idx_B]);
    });
}

} // namespace binary

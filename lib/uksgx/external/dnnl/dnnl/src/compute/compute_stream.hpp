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

#ifndef COMPUTE_STREAM_HPP
#define COMPUTE_STREAM_HPP

#include <memory>

#include "common/stream.hpp"
#include "compute/kernel.hpp"

namespace dnnl {
namespace impl {
namespace compute {

class nd_range_t;
class kernel_arg_list_t;

class compute_stream_t : public stream_t {
public:
    using stream_t::stream_t;

    virtual status_t copy(const memory_storage_t &src,
            const memory_storage_t &dst, size_t size)
            = 0;
    virtual status_t parallel_for(const nd_range_t &range,
            const kernel_t &kernel, const kernel_arg_list_t &arg_list) {
        return kernel.parallel_for(*this, range, arg_list);
    }
};

} // namespace compute
} // namespace impl
} // namespace dnnl

#endif

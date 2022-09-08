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

#ifndef COMPUTE_KERNEL_HPP
#define COMPUTE_KERNEL_HPP

#include <memory>
#include <utility>

#include "common/stream.hpp"
#include "compute/kernel_arg_list.hpp"
#include "compute/utils.hpp"

namespace dnnl {
namespace impl {
namespace compute {

class kernel_impl_t;

class kernel_t {
public:
    kernel_t(kernel_impl_t *impl) : impl_(impl) {}

    kernel_t() = default;
    kernel_t(kernel_t &&other) = default;
    kernel_t(const kernel_t &other) = default;
    kernel_t &operator=(const kernel_t &other) = default;
    virtual ~kernel_t() = default;

    operator bool() const { return bool(impl_); }

    status_t parallel_for(stream_t &stream, const nd_range_t &range,
            const kernel_arg_list_t &arg_list) const;

private:
    std::shared_ptr<kernel_impl_t> impl_;
};

class kernel_impl_t {
public:
    kernel_impl_t() = default;

    kernel_impl_t(const kernel_impl_t &) = delete;
    kernel_impl_t &operator=(const kernel_impl_t &) = delete;
    virtual ~kernel_impl_t() = default;

    virtual status_t parallel_for(stream_t &stream, const nd_range_t &range,
            const kernel_arg_list_t &arg_list) const = 0;
};

inline status_t kernel_t::parallel_for(stream_t &stream,
        const nd_range_t &range, const kernel_arg_list_t &arg_list) const {
    return impl_->parallel_for(stream, range, arg_list);
}

} // namespace compute
} // namespace impl
} // namespace dnnl

#endif // COMPUTE_KERNEL_HPP

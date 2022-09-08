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

#ifndef BFLOAT16_HPP
#define BFLOAT16_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>

#include "dnnl.h"

namespace dnnl {
namespace impl {

struct bfloat16_t {
    uint16_t raw_bits_;
    bfloat16_t() = default;
    constexpr bfloat16_t(uint16_t r, bool) : raw_bits_(r) {}
    bfloat16_t(float f) { (*this) = f; }

    bfloat16_t DNNL_API &operator=(float f);

    DNNL_API operator float() const;

    bfloat16_t &operator+=(bfloat16_t a) {
        (*this) = (float)(*this) + (float)a;
        return *this;
    }
};

static_assert(sizeof(bfloat16_t) == 2, "bfloat16_t must be 2 bytes");

void cvt_float_to_bfloat16(bfloat16_t *out, const float *inp, size_t size);
void cvt_bfloat16_to_float(float *out, const bfloat16_t *inp, size_t size);

// performs element-by-element sum of inp and add float arrays and stores
// result to bfloat16 out array with downconversion
void add_floats_and_cvt_to_bfloat16(
        bfloat16_t *out, const float *inp0, const float *inp1, size_t size);

} // namespace impl
} // namespace dnnl

#endif

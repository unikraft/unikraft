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

#ifndef COMMON_U8_HPP
#define COMMON_U8_HPP

#include "jit_generator.hpp"

#define U8_COPY_KERNEL_CODE_SIZE (4096L * 4)

#define PADD_BYTESIZE_ONPAGE(x, size) \
    (((x) * (size) + PAGE_4K - 1) / PAGE_4K) * PAGE_4K
#define NEXT_THR_STRIDE(x, size) (PADD_BYTESIZE_ONPAGE(x, size)) / size

namespace dnnl {
namespace impl {
namespace cpu {

class jit_avx512_core_u8_copy_an_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_an_kern);

public:
    jit_avx512_core_u8_copy_an_kern();
};

class jit_avx512_core_u8_copy_at_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_at_kern);

public:
    jit_avx512_core_u8_copy_at_kern();
};

class jit_avx512_core_u8_copy_bn_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_bn_kern);

public:
    jit_avx512_core_u8_copy_bn_kern(bool s8 = false);
};

class jit_avx512_core_u8_copy_bt_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_bt_kern);

public:
    jit_avx512_core_u8_copy_bt_kern(bool s8 = false);
};

class jit_avx512_core_u8_copy_sum_an_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_sum_an_kern);

public:
    jit_avx512_core_u8_copy_sum_an_kern();
};

class jit_avx512_core_u8_copy_sum_at_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_sum_at_kern);

public:
    jit_avx512_core_u8_copy_sum_at_kern();
};

class jit_avx512_core_u8_copy_sum_bn_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_sum_bn_kern);

public:
    jit_avx512_core_u8_copy_sum_bn_kern(bool s8 = false);
};

class jit_avx512_core_u8_copy_sum_bt_kern : public jit_generator {
    DECLARE_CPU_JIT_AUX_FUNCTIONS(jit_avx512_core_u8_copy_sum_bt_kern);

public:
    jit_avx512_core_u8_copy_sum_bt_kern(bool s8 = false);
};

} // namespace cpu
} // namespace impl
} // namespace dnnl
#endif // COMMON_U8_HPP

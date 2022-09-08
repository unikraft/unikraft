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

#include <cstring>
#include <memory>
#include <tuple>

#include "dnnl_test_common.hpp"
#include "gtest/gtest.h"

#include "dnnl.hpp"

namespace dnnl {

typedef float data_t;

struct params_t {
    memory::dims dims;
    memory::format_tag fmt_tag;
    dnnl_status_t expected_status;
};

using params_w_engine_t = std::tuple<dnnl::engine::kind, params_t>;

class memory_creation_test
    : public ::testing::TestWithParam<params_w_engine_t> {
protected:
    virtual void SetUp() override {
        params_w_engine_t pwe
                = ::testing::TestWithParam<decltype(pwe)>::GetParam();

        auto engine_kind = std::get<0>(pwe);
        if (dnnl::engine::get_count(engine_kind) == 0) return;

        eng = dnnl::engine(engine_kind, 0);
        p = std::get<1>(pwe);
    }

    void Test() {
        dnnl::memory::desc md(p.dims, memory::data_type::f32, p.fmt_tag);
        dnnl::memory::dim phys_size = md.get_size() / sizeof(data_t);

        // mem0
        // Initially spoiled by putting non-zero values in padded area.
        // The test will manually fix it later.
        dnnl::memory mem0(md, eng);

        // `runtime`-aware buffer for future mem1
        dnnl::memory mem1_placeholder(md, eng);

        // Map-unmap section
        {
            // Put non-zeros even to the padded area
            auto mem0_ptr = map_memory<data_t>(mem0);
            fill_data<data_t>(phys_size, mem0_ptr);

            // mem1_placeholder = copy(mem0)
            auto mem1_ph_ptr = map_memory<data_t>(mem1_placeholder);
            for (dnnl::memory::dim i = 0; i < phys_size; ++i)
                mem1_ph_ptr[i] = mem0_ptr[i];
        }

        // mem1
        // A `corrected` version of mem0 (i.e. padded area should be filled with
        // zeros) and with a buffer taken from mem1_placeholder.
        dnnl::memory mem1(md, eng, mem1_placeholder.get_data_handle());

        check_zero_tail<data_t>(0, mem1); // Check, if mem1 is indeed corrected
        check_zero_tail<data_t>(1, mem0); // Manually correct mem0

        // Map-unmap section
        {
            auto mem0_ptr = map_memory<data_t>(mem0);
            auto mem1_ptr = map_memory<data_t>(mem1);

            // Check if mem0 == mem1
            for (dnnl::memory::dim i = 0; i < phys_size; ++i)
                ASSERT_EQ(mem0_ptr[i], mem1_ptr[i]) << i;
        }
    }

    dnnl::engine eng;
    params_t p;
};

TEST_P(memory_creation_test, TestsMemoryCreation) {
    SKIP_IF(eng.get(true) == nullptr, "Engine is not supported");
    catch_expected_failures([=]() { Test(); },
            p.expected_status != dnnl_success, p.expected_status);
}

namespace {
auto all_engine_kinds
        = ::testing::Values(dnnl::engine::kind::cpu, dnnl::engine::kind::gpu);

using fmt = dnnl::memory::format_tag;

auto cases_expect_to_fail = ::testing::Values(
        params_t {{2, 2, -1, 1}, fmt::nchw, dnnl_invalid_arguments},
        params_t {{1, 2, 3, 4}, fmt::any, dnnl_invalid_arguments});

auto cases_zero_dim = ::testing::Values(params_t {{2, 0, 1, 1}, fmt::nChw16c},
        params_t {{0, 1, 0, 1}, fmt::nhwc}, params_t {{2, 1, 0, 1}, fmt::nchw});

auto cases_generic = ::testing::Values(params_t {{2, 15, 3, 2}, fmt::nChw16c},
        params_t {{2, 15, 3, 2, 4}, fmt::nCdhw8c},
        params_t {{2, 9, 3, 2}, fmt::OIhw8o8i},
        params_t {{2, 9, 3, 2}, fmt::OIhw8i16o2i},
        params_t {{2, 9, 3, 2}, fmt::OIhw8o16i2o},
        params_t {{2, 9, 3, 2}, fmt::OIhw16o16i},
        params_t {{2, 9, 3, 2}, fmt::OIhw16i16o},
        params_t {{2, 9, 3, 2}, fmt::OIhw4i16o4i},
        params_t {{2, 9, 4, 3, 2}, fmt::gOihw16o},
        params_t {{1, 2, 9, 3, 2}, fmt::gOIhw8o8i},
        params_t {{1, 2, 9, 3, 2}, fmt::gOIhw4o4i},
        params_t {{1, 2, 9, 3, 2}, fmt::gOIhw8i8o},
        params_t {{2, 17, 9, 3, 2}, fmt::gOIhw4i16o4i},
        params_t {{2, 17, 9, 3, 2}, fmt::gOIhw2i8o4i},
        params_t {{15, 16, 16, 3, 3}, fmt::Goihw8g});
} // namespace

INSTANTIATE_TEST_SUITE_P(TestMemoryCreationEF, memory_creation_test,
        ::testing::Combine(all_engine_kinds, cases_expect_to_fail));

INSTANTIATE_TEST_SUITE_P(TestMemoryCreationZeroDim, memory_creation_test,
        ::testing::Combine(all_engine_kinds, cases_zero_dim));

INSTANTIATE_TEST_SUITE_P(TestMemoryCreationOK, memory_creation_test,
        ::testing::Combine(all_engine_kinds, cases_generic));

class c_api_memory_test : public ::testing::Test {
    virtual void SetUp() {}
};

TEST_F(c_api_memory_test, TestZeroPadBoom) {
    dnnl_memory_desc_t md;
    memset(&md, 0xcc, sizeof(md));

    md.ndims = 2;
    md.data_type = dnnl_f32;
    md.offset0 = 0;
    md.dims[0] = 1;
    md.dims[1] = 1001;
    md.padded_dims[0] = 1;
    md.padded_dims[1] = 1008;
    md.padded_offsets[0] = 0;
    md.padded_offsets[1] = 0;

    md.extra.flags = dnnl_memory_extra_flag_none;

    md.format_kind = dnnl_blocked;
    md.format_desc.blocking.inner_nblks = 1;
    md.format_desc.blocking.inner_blks[0] = 16;
    md.format_desc.blocking.inner_idxs[0] = 1;
    md.format_desc.blocking.strides[0] = 1008;
    md.format_desc.blocking.strides[1] = 16;

    dnnl_engine_t e;
    ASSERT_TRUE(dnnl_success == dnnl_engine_create(&e, dnnl_cpu, 0));

    dnnl_memory_t m;
    ASSERT_TRUE(
            dnnl_success == dnnl_memory_create(&m, &md, e, DNNL_MEMORY_NONE));

    void *p = malloc(dnnl_memory_desc_get_size(&md));
    ASSERT_TRUE(p != NULL);
    ASSERT_TRUE(dnnl_success == dnnl_memory_set_data_handle(m, p)); // Boom

    ASSERT_TRUE(dnnl_success == dnnl_memory_destroy(m));
    free(p);

    ASSERT_TRUE(dnnl_success == dnnl_engine_destroy(e));
}

} // namespace dnnl

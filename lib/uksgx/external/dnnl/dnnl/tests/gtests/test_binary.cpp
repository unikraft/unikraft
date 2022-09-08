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

#include "dnnl_test_common.hpp"
#include "gtest/gtest.h"

#include "cpu_isa_traits.hpp"
#include "dnnl.hpp"

namespace dnnl {

using fmt = memory::format_tag;

struct binary_test_params {
    std::vector<fmt> srcs_format;
    fmt dst_format;
    algorithm aalgorithm;
    memory::dims dims;
    bool expect_to_fail;
    dnnl_status_t expected_status;
};

template <typename src_data_t, typename dst_data_t = src_data_t>
class binary_test : public ::testing::TestWithParam<binary_test_params> {
private:
    memory::data_type src_data_type;
    memory::data_type dst_data_type;

protected:
    virtual void SetUp() {
        src_data_type = data_traits<src_data_t>::data_type;
        dst_data_type = data_traits<dst_data_t>::data_type;
        binary_test_params p
                = ::testing::TestWithParam<binary_test_params>::GetParam();
        // TODO: remove me
        SKIP_IF(get_test_engine_kind() == engine::kind::gpu,
                "GPU does not support binary yet.");
        // TODO: remove me
        SKIP_IF(src_data_type != memory::data_type::f32
                        && src_data_type != memory::data_type::bf16,
                "Non-f32 data types are not supported yet.");

        SKIP_IF(get_test_engine_kind() == engine::kind::gpu
                        && src_data_type == memory::data_type::bf16,
                "GPU does not support bfloat16 data type.");
        SKIP_IF(src_data_type == memory::data_type::bf16
                        && !impl::cpu::mayiuse(impl::cpu::avx512_core),
                "current ISA doesn't support bfloat16 data type");

        catch_expected_failures(
                [=]() { Test(); }, p.expect_to_fail, p.expected_status);
    }

    void Test() {
        binary_test_params p
                = ::testing::TestWithParam<binary_test_params>::GetParam();

        auto eng = engine(get_test_engine_kind(), 0);
        auto strm = stream(eng);

        std::vector<memory::desc> srcs_md;
        std::vector<memory> srcs;

        memory::dims dims_B;
        for (int i_case = 0;; ++i_case) {
            if (i_case == 0) {
                dims_B = p.dims;
            } else if (i_case == 1) {
                dims_B[0] = p.dims[0];
            } else if (i_case == 2) {
                dims_B[0] = 1;
                dims_B[1] = p.dims[1];
            } else if (i_case == 3) {
                dims_B[1] = p.dims[1];
                dims_B[2] = p.dims[2];
            } else if (i_case == 4) {
                dims_B[0] = p.dims[0];
                dims_B[2] = p.dims[2];
                dims_B[3] = p.dims[3];
            } else if (i_case == 5) {
                dims_B[0] = 1;
            } else {
                break;
            }

            auto desc_A = memory::desc(p.dims, src_data_type, p.srcs_format[0]);
            auto mem_A = memory(desc_A, eng);

            auto desc_B = memory::desc(dims_B, src_data_type, p.srcs_format[1]);
            auto mem_B = memory(desc_B, eng);
            if (dims_B.size() < p.dims.size()) {
                for (size_t d = dims_B.size(); d < p.dims.size(); ++d)
                    dims_B[d] = 1;
                desc_B = desc_B.reshape(dims_B);
            }

            auto desc_C = memory::desc(p.dims, dst_data_type, p.dst_format);

#define ASSIGN_PD(desc, pd) \
    { \
        if (p.expect_to_fail) \
            (pd) = binary::primitive_desc((desc), eng); \
        else \
            ASSERT_NO_THROW((pd) = binary::primitive_desc((desc), eng)); \
    }
            binary::primitive_desc binary_pd;
            binary::desc binary_desc(p.aalgorithm, desc_A, desc_B, desc_C);
            ASSIGN_PD(binary_desc, binary_pd);

            desc_C = binary_pd.dst_desc();
            auto mem_C = memory(desc_C, eng);

            std::unordered_map<int, memory> args = {{DNNL_ARG_SRC_0, mem_A},
                    {DNNL_ARG_SRC_1, mem_B}, {DNNL_ARG_DST, mem_C}};

            binary prim(binary_pd);
            prim.execute(strm, args);
            strm.wait();
        }
    }
};

static auto expected_failures = []() {
    return ::testing::Values(
            // different src0 and dst format_tags
            binary_test_params {{fmt::nchw, fmt::nchw}, fmt::nhwc,
                    algorithm::binary_add, {1, 8, 4, 4}, true,
                    dnnl_invalid_arguments},
            // not supported alg_kind
            binary_test_params {{fmt::nchw, fmt::nchw}, fmt::nchw,
                    algorithm::eltwise_relu, {1, 8, 4, 4}, true,
                    dnnl_invalid_arguments},
            // negative dim
            binary_test_params {{fmt::nchw, fmt::nchw}, fmt::nchw,
                    algorithm::binary_add, {-1, 8, 4, 4}, true,
                    dnnl_invalid_arguments});
};

static auto zero_dim = []() {
    return ::testing::Values(
            binary_test_params {{fmt::nchw, fmt::nchw}, fmt::nchw,
                    algorithm::binary_add, {0, 7, 6, 5}},
            binary_test_params {{fmt::nChw8c, fmt::nhwc}, fmt::nChw8c,
                    algorithm::binary_mul, {5, 0, 7, 6}},
            binary_test_params {{fmt::nChw16c, fmt::nchw}, fmt::nChw16c,
                    algorithm::binary_add, {8, 15, 0, 5}},
            binary_test_params {{fmt::nhwc, fmt::nChw16c}, fmt::nhwc,
                    algorithm::binary_mul, {5, 16, 7, 0}});
};

static auto simple_cases = []() {
    return ::testing::Values(
            binary_test_params {{fmt::nchw, fmt::nchw}, fmt::nchw,
                    algorithm::binary_add, {8, 7, 6, 5}},
            binary_test_params {{fmt::nhwc, fmt::nhwc}, fmt::nhwc,
                    algorithm::binary_mul, {5, 8, 7, 6}},
            binary_test_params {{fmt::nChw8c, fmt::nchw}, fmt::nChw8c,
                    algorithm::binary_add, {8, 15, 6, 5}},
            binary_test_params {{fmt::nhwc, fmt::nChw16c}, fmt::any,
                    algorithm::binary_mul, {5, 16, 7, 6}});
};

#define CPU_INST_TEST_CASE(test) \
    CPU_TEST_P(test, Testsbinary) {} \
    CPU_INSTANTIATE_TEST_SUITE_P(TestbinaryEF, test, expected_failures()); \
    CPU_INSTANTIATE_TEST_SUITE_P(TestbinaryZero, test, zero_dim()); \
    CPU_INSTANTIATE_TEST_SUITE_P(TestbinarySimple, test, simple_cases());

#define INST_TEST_CASE(test) CPU_INST_TEST_CASE(test)

using binary_test_float = binary_test<float>;
using binary_test_bfloat16 = binary_test<bfloat16_t>;

INST_TEST_CASE(binary_test_float)
INST_TEST_CASE(binary_test_bfloat16)

#undef CPU_INST_TEST_CASE
} // namespace dnnl

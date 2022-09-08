/*******************************************************************************
* Copyright 2016-2018 Intel Corporation
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

#include "gtest/gtest.h"

#include <assert.h>
#include <atomic>
#include <string>

#include "dnnl_test_common.hpp"

#include "gtest/gtest.h"

using namespace testing;

static std::atomic<bool> g_is_current_test_failed(false);
bool is_current_test_failed() {
    return g_is_current_test_failed;
}

class assert_fail_handler_t : public EmptyTestEventListener {
protected:
    virtual void OnTestStart(const TestInfo &test_info) override {
        g_is_current_test_failed = false;
    }
    virtual void OnTestPartResult(
            const testing::TestPartResult &part_result) override {
        if (part_result.type() == testing::TestPartResult::kFatalFailure) {
            g_is_current_test_failed = true;
        }
    }
};

static void test_init(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    int result;
    {
        ::testing::InitGoogleTest(&argc, argv);

        // Parse DNNL command line arguments
        test_init(argc, argv);

        TestEventListeners &listeners = UnitTest::GetInstance()->listeners();

        auto *fail_handler = new assert_fail_handler_t();
        listeners.Append(fail_handler);

#if _WIN32
        // Safety cleanup.
        system("where /q umdh && del pre_cpu.txt");
        system("where /q umdh && del post_cpu.txt");
        system("where /q umdh && del memdiff_cpu.txt");

        // Get first snapshot.
        system("where /q umdh && umdh -pn:tests.exe -f:pre_cpu.txt");
#endif

        result = RUN_ALL_TESTS();
    }

#if _WIN32
    // Get second snapshot.
    system("where /q umdh && umdh -pn:tests.exe -f:post_cpu.txt");

    // Prepare memory diff.
    system("where /q umdh && umdh pre_cpu.txt post_cpu.txt -f:memdiff_cpu.txt");

    // Cleanup.
    system("where /q umdh && del pre_cpu.txt");
    system("where /q umdh && del post_cpu.txt");
#endif

    return result;
}

static std::string find_cmd_option(
        char **argv_beg, char **argv_end, const std::string &option) {
    for (auto arg = argv_beg; arg != argv_end; arg++) {
        std::string s(*arg);
        auto pos = s.find(option);
        if (pos != std::string::npos) return s.substr(pos + option.length());
    }
    return {};
}

static dnnl::engine::kind to_engine_kind(const std::string &str) {
    if (str.empty() || str == "cpu") return dnnl::engine::kind::cpu;

    if (str == "gpu") return dnnl::engine::kind::gpu;

    assert(!"not expected");
    return dnnl::engine::kind::cpu;
}

static dnnl::engine::kind test_engine_kind;
dnnl::engine::kind get_test_engine_kind() {
    return test_engine_kind;
}

void test_init(int argc, char *argv[]) {
    auto engine_str = find_cmd_option(argv, argv + argc, "--engine=");
#ifndef DNNL_TEST_WITH_ENGINE_PARAM
    assert(engine_str.empty()
            && "--engine parameter is not supported by this test");
#endif
    test_engine_kind = to_engine_kind(engine_str);

#ifdef DNNL_TEST_WITH_ENGINE_PARAM
    std::string filter_str = ::testing::GTEST_FLAG(filter);
    if (test_engine_kind == dnnl::engine::kind::cpu) {
        // Exclude non-CPU tests
        ::testing::GTEST_FLAG(filter) = filter_str + ":-*_GPU*";
    } else if (test_engine_kind == dnnl::engine::kind::gpu) {
        // Exclude non-GPU tests
        ::testing::GTEST_FLAG(filter) = filter_str + ":-*_CPU*";
    }
#endif
}

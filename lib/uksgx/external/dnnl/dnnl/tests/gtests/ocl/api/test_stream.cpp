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

#include <memory>
#include "dnnl.h"
#include <CL/cl.h>

namespace dnnl {
class ocl_stream_test_c : public ::testing::Test {
protected:
    virtual void SetUp() {
        if (!find_ocl_device(CL_DEVICE_TYPE_GPU)) { return; }

        DNNL_CHECK(dnnl_engine_create(&eng, dnnl_gpu, 0));

        DNNL_CHECK(dnnl_engine_get_ocl_context(eng, &ocl_ctx));
        DNNL_CHECK(dnnl_engine_get_ocl_device(eng, &ocl_dev));
    }

    virtual void TearDown() {
        if (eng) { DNNL_CHECK(dnnl_engine_destroy(eng)); }
    }

    dnnl_engine_t eng = nullptr;
    cl_context ocl_ctx = nullptr;
    cl_device_id ocl_dev = nullptr;
};

class ocl_stream_test_cpp : public ::testing::Test {
protected:
    virtual void SetUp() {
        if (!find_ocl_device(CL_DEVICE_TYPE_GPU)) { return; }

        eng = engine(engine::kind::gpu, 0);

        ocl_ctx = eng.get_ocl_context();
        ocl_dev = eng.get_ocl_device();
    }

    engine eng;
    cl_context ocl_ctx = nullptr;
    cl_device_id ocl_dev = nullptr;
};

TEST_F(ocl_stream_test_c, CreateC) {
    SKIP_IF(!find_ocl_device(CL_DEVICE_TYPE_GPU),
            "OpenCL GPU devices not found.");

    dnnl_stream_t stream;
    DNNL_CHECK(dnnl_stream_create(&stream, eng, dnnl_stream_default_flags));

    cl_command_queue ocl_queue;
    DNNL_CHECK(dnnl_stream_get_ocl_command_queue(stream, &ocl_queue));

    cl_device_id ocl_queue_dev;
    cl_context ocl_queue_ctx;
    OCL_CHECK(clGetCommandQueueInfo(ocl_queue, CL_QUEUE_DEVICE,
            sizeof(ocl_queue_dev), &ocl_queue_dev, nullptr));
    OCL_CHECK(clGetCommandQueueInfo(ocl_queue, CL_QUEUE_CONTEXT,
            sizeof(ocl_queue_ctx), &ocl_queue_ctx, nullptr));

    ASSERT_EQ(ocl_dev, ocl_queue_dev);
    ASSERT_EQ(ocl_ctx, ocl_queue_ctx);

    DNNL_CHECK(dnnl_stream_destroy(stream));
}

TEST_F(ocl_stream_test_cpp, CreateCpp) {
    SKIP_IF(!find_ocl_device(CL_DEVICE_TYPE_GPU),
            "OpenCL GPU devices not found.");

    stream s(eng);
    cl_command_queue ocl_queue = s.get_ocl_command_queue();

    cl_device_id ocl_queue_dev;
    cl_context ocl_queue_ctx;
    OCL_CHECK(clGetCommandQueueInfo(ocl_queue, CL_QUEUE_DEVICE,
            sizeof(ocl_queue_dev), &ocl_queue_dev, nullptr));
    OCL_CHECK(clGetCommandQueueInfo(ocl_queue, CL_QUEUE_CONTEXT,
            sizeof(ocl_queue_ctx), &ocl_queue_ctx, nullptr));

    ASSERT_EQ(ocl_dev, ocl_queue_dev);
    ASSERT_EQ(ocl_ctx, ocl_queue_ctx);
}

TEST_F(ocl_stream_test_c, BasicInteropC) {
    SKIP_IF(!find_ocl_device(CL_DEVICE_TYPE_GPU),
            "OpenCL GPU devices not found.");

    cl_int err;
    cl_command_queue interop_ocl_queue
            = clCreateCommandQueue(ocl_ctx, ocl_dev, 0, &err);
    OCL_CHECK(err);

    dnnl_stream_t stream;
    DNNL_CHECK(dnnl_stream_create_ocl(&stream, eng, interop_ocl_queue));

    cl_command_queue ocl_queue;
    DNNL_CHECK(dnnl_stream_get_ocl_command_queue(stream, &ocl_queue));
    ASSERT_EQ(ocl_queue, interop_ocl_queue);

    cl_uint ref_count;
    OCL_CHECK(clGetCommandQueueInfo(interop_ocl_queue, CL_QUEUE_REFERENCE_COUNT,
            sizeof(cl_uint), &ref_count, nullptr));
    int i_ref_count = int(ref_count);
    ASSERT_EQ(i_ref_count, 2);

    DNNL_CHECK(dnnl_stream_destroy(stream));

    OCL_CHECK(clGetCommandQueueInfo(interop_ocl_queue, CL_QUEUE_REFERENCE_COUNT,
            sizeof(cl_uint), &ref_count, nullptr));
    i_ref_count = int(ref_count);
    ASSERT_EQ(i_ref_count, 1);

    OCL_CHECK(clReleaseCommandQueue(interop_ocl_queue));
}

TEST_F(ocl_stream_test_cpp, BasicInteropC) {
    SKIP_IF(!find_ocl_device(CL_DEVICE_TYPE_GPU),
            "OpenCL GPU devices not found.");

    cl_int err;
    cl_command_queue interop_ocl_queue
            = clCreateCommandQueue(ocl_ctx, ocl_dev, 0, &err);
    OCL_CHECK(err);

    {
        stream s(eng, interop_ocl_queue);

        cl_uint ref_count;
        OCL_CHECK(clGetCommandQueueInfo(interop_ocl_queue,
                CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &ref_count,
                nullptr));
        int i_ref_count = int(ref_count);
        ASSERT_EQ(i_ref_count, 2);

        cl_command_queue ocl_queue = s.get_ocl_command_queue();
        ASSERT_EQ(ocl_queue, interop_ocl_queue);
    }

    cl_uint ref_count;
    OCL_CHECK(clGetCommandQueueInfo(interop_ocl_queue, CL_QUEUE_REFERENCE_COUNT,
            sizeof(cl_uint), &ref_count, nullptr));
    int i_ref_count = int(ref_count);
    ASSERT_EQ(i_ref_count, 1);

    OCL_CHECK(clReleaseCommandQueue(interop_ocl_queue));
}

TEST_F(ocl_stream_test_c, InteropIncompatibleQueueC) {
    SKIP_IF(!find_ocl_device(CL_DEVICE_TYPE_GPU),
            "OpenCL GPU devices not found.");

    cl_device_id cpu_ocl_dev = find_ocl_device(CL_DEVICE_TYPE_CPU);
    SKIP_IF(!cpu_ocl_dev, "OpenCL CPU devices not found.");

    cl_int err;
    cl_context cpu_ocl_ctx
            = clCreateContext(nullptr, 1, &cpu_ocl_dev, nullptr, nullptr, &err);
    OCL_CHECK(err);

    cl_command_queue cpu_ocl_queue
            = clCreateCommandQueue(cpu_ocl_ctx, cpu_ocl_dev, 0, &err);
    OCL_CHECK(err);

    dnnl_stream_t stream;
    dnnl_status_t status = dnnl_stream_create_ocl(&stream, eng, cpu_ocl_queue);
    ASSERT_EQ(status, dnnl_invalid_arguments);

    OCL_CHECK(clReleaseCommandQueue(cpu_ocl_queue));
}

TEST_F(ocl_stream_test_cpp, InteropIncompatibleQueueCpp) {
    SKIP_IF(!find_ocl_device(CL_DEVICE_TYPE_GPU),
            "OpenCL GPU devices not found.");

    cl_device_id cpu_ocl_dev = find_ocl_device(CL_DEVICE_TYPE_CPU);
    SKIP_IF(!cpu_ocl_dev, "OpenCL CPU devices not found.");

    cl_int err;
    cl_context cpu_ocl_ctx
            = clCreateContext(nullptr, 1, &cpu_ocl_dev, nullptr, nullptr, &err);
    OCL_CHECK(err);

    cl_command_queue cpu_ocl_queue
            = clCreateCommandQueue(cpu_ocl_ctx, cpu_ocl_dev, 0, &err);
    OCL_CHECK(err);

    catch_expected_failures([&] { stream s(eng, cpu_ocl_queue); }, true,
            dnnl_invalid_arguments);

    OCL_CHECK(clReleaseCommandQueue(cpu_ocl_queue));
}

} // namespace dnnl

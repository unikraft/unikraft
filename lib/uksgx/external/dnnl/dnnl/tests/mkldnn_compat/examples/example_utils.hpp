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

#ifndef EXAMPLE_UTILS_HPP
#define EXAMPLE_UTILS_HPP

#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <string>

#include "mkldnn.hpp"

static mkldnn::engine::kind parse_engine_kind(
        int argc, char **argv, int extra_args = 0) {
    // Returns default engine kind, i.e. CPU, if none given
    if (argc == 1) {
        return mkldnn::engine::kind::cpu;
    } else if (argc <= extra_args + 2) {
        std::string engine_kind_str = argv[1];
        // Checking the engine type, i.e. CPU or GPU
        if (engine_kind_str == "cpu") {
            return mkldnn::engine::kind::cpu;
        } else if (engine_kind_str == "gpu") {
            // Checking if a GPU exists on the machine
            if (mkldnn::engine::get_count(mkldnn::engine::kind::gpu) == 0) {
                std::cerr << "Application couldn't find GPU, please run with "
                             "CPU instead. Thanks!\n";
                exit(1);
            }
            return mkldnn::engine::kind::gpu;
        }
    }

    // If all above fails, the example should be ran properly
    std::cerr << "Please run example like this" << argv[0] << " cpu|gpu";
    if (extra_args) { std::cerr << " [extra arguments]"; }
    std::cerr << "\n";
    exit(1);
}

// Read from memory, write to handle
inline void read_from_mkldnn_memory(void *handle, mkldnn::memory &mem) {
    mkldnn::engine eng = mem.get_engine();
    size_t bytes = mem.get_desc().get_size();

    if (eng.get_kind() == mkldnn::engine::kind::cpu) {
        uint8_t *src = static_cast<uint8_t *>(mem.get_data_handle());
        std::copy(src, src + bytes, (uint8_t *)handle);
    }
#if MKLDNN_GPU_RUNTIME == MKLDNN_RUNTIME_OCL
    else if (eng.get_kind() == mkldnn::engine::kind::gpu) {
        mkldnn::stream s(eng);
        cl_command_queue q = s.get_ocl_command_queue();
        cl_mem m = mem.get_ocl_mem_object();

        cl_int ret = clEnqueueReadBuffer(
                q, m, CL_TRUE, 0, bytes, handle, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("clEnqueueReadBuffer failed. Status Code: "
                    + std::to_string(ret) + "\n");
    }
#endif
}

// Read from handle, write to memory
inline void write_to_mkldnn_memory(void *handle, mkldnn::memory &mem) {
    mkldnn::engine eng = mem.get_engine();
    size_t bytes = mem.get_desc().get_size();

    if (eng.get_kind() == mkldnn::engine::kind::cpu) {
        uint8_t *dst = static_cast<uint8_t *>(mem.get_data_handle());
        std::copy((uint8_t *)handle, (uint8_t *)handle + bytes, dst);
    }
#if MKLDNN_GPU_RUNTIME == MKLDNN_RUNTIME_OCL
    else if (eng.get_kind() == mkldnn::engine::kind::gpu) {
        mkldnn::stream s(eng);
        cl_command_queue q = s.get_ocl_command_queue();
        cl_mem m = mem.get_ocl_mem_object();
        size_t bytes = mem.get_desc().get_size();

        cl_int ret = clEnqueueWriteBuffer(
                q, m, CL_TRUE, 0, bytes, handle, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error(
                    "clEnqueueWriteBuffer failed. Status Code: "
                    + std::to_string(ret) + "\n");
    }
#endif
}

#endif

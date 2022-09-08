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

#ifndef OCL_GPU_DEVICE_INFO_HPP
#define OCL_GPU_DEVICE_INFO_HPP

#include <string>
#include <vector>
#include <CL/cl.h>

#include "common/z_magic.hpp"
#include "compute/device_info.hpp"
#include "ocl/ocl_utils.hpp"

namespace dnnl {
namespace impl {
namespace ocl {

enum class gpu_arch_t {
    unknown,
    gen9,
};

inline gpu_arch_t str2gpu_arch(const char *str) {
#define CASE(_case) \
    if (!strcmp(STRINGIFY(_case), str)) return gpu_arch_t::_case

    CASE(gen9);
    return gpu_arch_t::unknown;
#undef CASE
}

inline const char *gpu_arch2str(gpu_arch_t arch) {
#define CASE(_case) \
    case gpu_arch_t::_case: return STRINGIFY(_case)

    switch (arch) {
        CASE(gen9);
        CASE(unknown);
    }
    return "unknown";
#undef CASE
}

class ocl_gpu_device_info_t : public compute::device_info_t {
public:
    ocl_gpu_device_info_t(cl_device_id device) : device_(device) {}

    virtual status_t init() override {
        CHECK(init_arch());
        CHECK(init_extensions());
        CHECK(init_attributes());

        // OpenCL runtime version
        size_t size_driver_version {0};
        cl_int err = clGetDeviceInfo(
                device_, CL_DRIVER_VERSION, 0, nullptr, &size_driver_version);
        OCL_CHECK(err);
        std::string driver_version;
        driver_version.resize(size_driver_version);
        err = clGetDeviceInfo(device_, CL_DRIVER_VERSION, size_driver_version,
                &driver_version[0], nullptr);
        OCL_CHECK(err);

        driver_version[size_driver_version - 1] = '\0';
        if (runtime_version_.set_from_string(&driver_version[0])
                != status::success) {
            runtime_version_.major = 0;
            runtime_version_.minor = 0;
            runtime_version_.build = 0;
        }

        return status::success;
    }

    virtual bool has(compute::device_ext_t ext) const override {
        return has(extensions_, ext);
    }

    gpu_arch_t gpu_arch() const { return gpu_arch_; }

    int eu_count() const { return eu_count_; }
    int hw_threads() const { return hw_threads_; }

    virtual const compute::runtime_version_t &runtime_version() const override {
        return runtime_version_;
    }

private:
    status_t init_arch() {
        // Device name
        size_t size_name {0};
        cl_int err = clGetDeviceInfo(
                device_, CL_DEVICE_NAME, 0, nullptr, &size_name);
        OCL_CHECK(err);

        std::string dev_name;
        dev_name.resize(size_name);
        err = clGetDeviceInfo(
                device_, CL_DEVICE_NAME, size_name, &dev_name[0], &size_name);
        OCL_CHECK(err);

        if (dev_name.find("Gen9") != std::string::npos)
            gpu_arch_ = gpu_arch_t::gen9;
        else
            gpu_arch_ = gpu_arch_t::unknown;

        return status::success;
    }

    status_t init_extensions() {
        // Handle extensions provided by the OpenCL runtime
        size_t size_ext {0};
        cl_int err = clGetDeviceInfo(
                device_, CL_DEVICE_EXTENSIONS, 0, nullptr, &size_ext);
        OCL_CHECK(err);

        std::string dev_ext;
        dev_ext.resize(size_ext);

        err = clGetDeviceInfo(device_, CL_DEVICE_EXTENSIONS, size_ext,
                &dev_ext[0], &size_ext);
        OCL_CHECK(err);

        for (uint64_t i_ext = 1; i_ext < (uint64_t)compute::device_ext_t::last;
                i_ext <<= 1) {
            const char *s_ext = ext2cl_str((compute::device_ext_t)i_ext);
            if (s_ext != nullptr && dev_ext.find(s_ext) != std::string::npos) {
                extensions_ |= i_ext;
            }
        }

        return status::success;
    }

    status_t init_attributes() {
        cl_uint eu_count;
        cl_int err = clGetDeviceInfo(device_, CL_DEVICE_MAX_COMPUTE_UNITS,
                sizeof(cl_uint), &eu_count, nullptr);
        eu_count_ = (err == CL_SUCCESS) ? eu_count : 0;

        // Assume 7 threads by for Gen9
        int32_t threads_per_eu = 7;

        switch (gpu_arch_) {
            case gpu_arch_t::gen9: threads_per_eu = 7; break;
            default: break;
        }

        hw_threads_ = eu_count_ * threads_per_eu;
        return status::success;
    }

    bool has(uint64_t extensions, compute::device_ext_t ext) const {
        return extensions & (uint64_t)ext;
    }

    cl_device_id device_ = nullptr;

    int32_t eu_count_ = 0;
    int32_t hw_threads_ = 0;

    // extensions_ and gpu_arch_ describe effective extensions and GPU
    // architecture.
    uint64_t extensions_ = 0;
    gpu_arch_t gpu_arch_ = gpu_arch_t::unknown;

    compute::runtime_version_t runtime_version_;
};

} // namespace ocl
} // namespace impl
} // namespace dnnl

#endif // OCL_GPU_DEVICE_INFO_HPP

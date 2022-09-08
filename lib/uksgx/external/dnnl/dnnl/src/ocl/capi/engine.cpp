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

#include <CL/cl.h>

#include "common/c_types_map.hpp"
#include "common/engine.hpp"
#include "ocl/ocl_engine.hpp"

using namespace dnnl::impl;
using namespace dnnl::impl::ocl;

status_t dnnl_engine_create_ocl(engine_t **engine, engine_kind_t kind,
        cl_device_id device, cl_context context) {
    bool args_ok = true && (kind == engine_kind::gpu)
            && !utils::any_null(engine, device, context);
    if (!args_ok) return status::invalid_arguments;

    ocl_engine_factory_t f(kind);
    return f.engine_create(engine, device, context);
}

status_t dnnl_engine_get_ocl_context(engine_t *engine, cl_context *context) {
    bool args_ok = true && !utils::any_null(engine, context)
            && (engine->runtime_kind() == runtime_kind::ocl);

    if (!args_ok) return status::invalid_arguments;

    auto *ocl_engine = utils::downcast<ocl_gpu_engine_t *>(engine);
    *context = ocl_engine->context();
    return status::success;
}

status_t dnnl_engine_get_ocl_device(engine_t *engine, cl_device_id *device) {
    bool args_ok = true && !utils::any_null(engine, device)
            && (engine->runtime_kind() == runtime_kind::ocl);

    if (!args_ok) return status::invalid_arguments;

    auto *ocl_engine = utils::downcast<ocl_gpu_engine_t *>(engine);
    *device = ocl_engine->device();
    return status::success;
}

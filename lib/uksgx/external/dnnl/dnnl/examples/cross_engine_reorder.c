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

/// @example cross_engine_reorder.c
/// @copybrief cross_engine_reorder_c
/// > Annotated version: @ref cross_engine_reorder_c

/// @page cross_engine_reorder_c Reorder between CPU and GPU engines
/// This C API example demonstrates programming flow when reordering memory
/// between CPU and GPU engines
///
/// > Example code: @ref cross_engine_reorder.c
///
/// - How to create DNNL memory objects.
///   - How to get data from user's buffer into an DNNL
///     memory object.
///   - How tensor's logical dimensions and memory object formats relate.
/// - How to create DNNL primitives.
/// - How to execute the primitives.
///
/// @include cross_engine_reorder.c
/// @page cross_engine_reorder_c

#include <stdio.h>
#include <stdlib.h>

#include "dnnl.h"

#define CHECK(f) \
    do { \
        dnnl_status_t s = f; \
        if (s != dnnl_success) { \
            printf("[%s:%d] error: %s returns %d\n", __FILE__, __LINE__, #f, \
                    s); \
            exit(2); \
        } \
    } while (0)

size_t product(int n_dims, const dnnl_dim_t dims[]) {
    size_t n_elems = 1;
    for (int d = 0; d < n_dims; ++d) {
        n_elems *= (size_t)dims[d];
    }
    return n_elems;
}

void fill(dnnl_memory_t mem, int n_dims, const dnnl_dim_t dims[]) {
    float *array;
    CHECK(dnnl_memory_map_data(mem, (void **)&array));

    const size_t n_elems = product(n_dims, dims);
    for (size_t e = 0; e < n_elems; ++e) {
        array[e] = e % 7 ? 1.0f : -1.0f;
    }

    CHECK(dnnl_memory_unmap_data(mem, array));
}

int find_negative(dnnl_memory_t mem, int n_dims, const dnnl_dim_t dims[]) {
    int negs = 0;

    float *array;
    CHECK(dnnl_memory_map_data(mem, (void **)&array));

    const size_t n_elems = product(n_dims, dims);
    for (size_t e = 0; e < n_elems; ++e) {
        negs += array[e] < 0.0f;
    }

    CHECK(dnnl_memory_unmap_data(mem, array));

    return negs;
}

int doit() {
    dnnl_engine_t engine_cpu, engine_gpu;
    CHECK(dnnl_engine_create(&engine_cpu, dnnl_cpu, 0));
    CHECK(dnnl_engine_create(&engine_gpu, dnnl_gpu, 0));

    dnnl_dim_t tz[4] = {2, 16, 1, 1};

    dnnl_memory_desc_t m_cpu_md, m_gpu_md;
    CHECK(dnnl_memory_desc_init_by_tag(&m_cpu_md, 4, tz, dnnl_f32, dnnl_nchw));
    CHECK(dnnl_memory_desc_init_by_tag(&m_gpu_md, 4, tz, dnnl_f32, dnnl_nchw));

    dnnl_memory_t m_cpu, m_gpu;
    CHECK(dnnl_memory_create(
            &m_cpu, &m_cpu_md, engine_cpu, DNNL_MEMORY_ALLOCATE));
    CHECK(dnnl_memory_create(
            &m_gpu, &m_gpu_md, engine_gpu, DNNL_MEMORY_ALLOCATE));

    fill(m_cpu, 4, tz);
    if (find_negative(m_cpu, 4, tz) == 0) {
        printf("Please fix filling of data\n");
        exit(2);
    }

    /* reorder cpu -> gpu */
    dnnl_primitive_desc_t r1_pd;
    CHECK(dnnl_reorder_primitive_desc_create(
            &r1_pd, &m_cpu_md, engine_cpu, &m_gpu_md, engine_gpu, NULL));
    dnnl_primitive_t r1;
    CHECK(dnnl_primitive_create(&r1, r1_pd));

    /* relu gpu */
    dnnl_eltwise_desc_t relu_d;
    CHECK(dnnl_eltwise_forward_desc_init(
            &relu_d, dnnl_forward, dnnl_eltwise_relu, &m_gpu_md, 0.0f, 0.0f));

    dnnl_primitive_desc_t relu_pd;
    CHECK(dnnl_primitive_desc_create(
            &relu_pd, &relu_d, NULL, engine_gpu, NULL));

    dnnl_primitive_t relu;
    CHECK(dnnl_primitive_create(&relu, relu_pd));

    /* reorder gpu -> cpu */
    dnnl_primitive_desc_t r2_pd;
    CHECK(dnnl_reorder_primitive_desc_create(
            &r2_pd, &m_gpu_md, engine_gpu, &m_cpu_md, engine_cpu, NULL));
    dnnl_primitive_t r2;
    CHECK(dnnl_primitive_create(&r2, r2_pd));

    dnnl_stream_t stream_gpu;
    CHECK(dnnl_stream_create(
            &stream_gpu, engine_gpu, dnnl_stream_default_flags));

    dnnl_exec_arg_t r1_args[] = {{DNNL_ARG_FROM, m_cpu}, {DNNL_ARG_TO, m_gpu}};
    CHECK(dnnl_primitive_execute(r1, stream_gpu, 2, r1_args));

    dnnl_exec_arg_t relu_args[]
            = {{DNNL_ARG_SRC, m_gpu}, {DNNL_ARG_DST, m_gpu}};
    CHECK(dnnl_primitive_execute(relu, stream_gpu, 2, relu_args));

    dnnl_exec_arg_t r2_args[] = {{DNNL_ARG_FROM, m_gpu}, {DNNL_ARG_TO, m_cpu}};
    CHECK(dnnl_primitive_execute(r2, stream_gpu, 2, r2_args));

    CHECK(dnnl_stream_wait(stream_gpu));

    if (find_negative(m_cpu, 4, tz) != 0) return 2;

    /* clean up */
    dnnl_primitive_desc_destroy(relu_pd);
    dnnl_primitive_desc_destroy(r1_pd);
    dnnl_primitive_desc_destroy(r2_pd);

    dnnl_primitive_destroy(relu);
    dnnl_primitive_destroy(r1);
    dnnl_primitive_destroy(r2);
    dnnl_memory_destroy(m_cpu);
    dnnl_memory_destroy(m_gpu);

    dnnl_stream_destroy(stream_gpu);

    dnnl_engine_destroy(engine_cpu);
    dnnl_engine_destroy(engine_gpu);

    return 0;
}

int main() {
    int result = doit();
    if (result)
        printf("failed\n");
    else
        printf("passed\n");

    return result;
}

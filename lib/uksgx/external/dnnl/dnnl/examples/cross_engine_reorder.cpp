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

/// @example cross_engine_reorder.cpp
/// @copybrief cross_engine_reorder_cpp
/// > Annotated version: @ref cross_engine_reorder_cpp

#include <iostream>
#include <sstream>

/// @page cross_engine_reorder_cpp Getting started on GPU
/// This C++ API example demonstrates programming for Intel(R) Processor
/// Graphics with DNNL.
///
/// > Example code: @ref cross_engine_reorder.cpp
///
///   - How to create DNNL memory objects for both GPU and CPU.
///   - How to get data from the user's buffer into an DNNL
///     GPU memory object.
///   - How to get results from an DNNL GPU memory object
///     into the user's buffer.
///   - How to create DNNL primitives on GPU.
///   - How to execute the primitives on GPU.
///
/// @section cross_engine_reorder_cpp_headers Public headers
///
/// To start using DNNL, we must first include the @ref dnnl.hpp
/// header file in the application. We also include @ref dnnl_debug.h, which
/// contains some debugging facilities such as returning a string representation
/// for common DNNL C types.
///
/// All C++ API types and functions reside in the `dnnl` namespace.
/// For simplicity of the example we import this namespace.
/// @page cross_engine_reorder_cpp
/// @snippet cross_engine_reorder.cpp Prologue
// [Prologue]
#include "dnnl.hpp"

// Optional header to access debug functions like `dnnl_status2str()`
#include "dnnl_debug.h"

using namespace dnnl;

using namespace std;
// [Prologue]

size_t product(const memory::dims adims) {
    size_t n_elems = 1;
    for (size_t d = 0; d < adims.size(); ++d) {
        n_elems *= (size_t)adims[d];
    }
    return n_elems;
}

void fill(const memory &mem, const memory::dims adims) {
    float *array = mem.map_data<float>();

    for (size_t e = 0; e < adims.size(); ++e) {
        array[e] = e % 7 ? 1.0f : -1.0f;
    }

    mem.unmap_data(array);
}

int find_negative(const memory &mem, const memory::dims adims) {
    int negs = 0;

    float *array = mem.map_data<float>();

    for (size_t e = 0; e < adims.size(); ++e) {
        negs += array[e] < 0.0f;
    }

    mem.unmap_data(array);
    return negs;
}

/// @page cross_engine_reorder_cpp
/// @section cross_engine_reorder_cpp_tutorial cross_engine_reorder_tutorial() function
/// @page cross_engine_reorder_cpp
void cross_engine_reorder_tutorial() {
    /// @page cross_engine_reorder_cpp
    /// @subsection cross_engine_reorder_cpp_sub1 Engine and stream
    ///
    /// All DNNL primitives and memory objects are attached to a
    /// particular @ref dnnl::engine, which is an abstraction of a
    /// computational device (see also @ref dev_guide_basic_concepts). The
    /// primitives are created and optimized for the device they are attached
    /// to, and the memory objects refer to memory residing on the
    /// corresponding device. In particular, that means neither memory objects
    /// nor primitives that were created for one engine can be used on
    /// another.
    ///
    /// To create engines, we must specify the @ref dnnl::engine::kind
    /// and the index of the device of the given kind. There is only one CPU
    /// engine and one GPU engine, so the index for both engines must be 0.
    ///
    /// @snippet cross_engine_reorder.cpp Initialize engine
    // [Initialize engine]
    auto cpu_engine = engine(engine::kind::cpu, 0);
    auto gpu_engine = engine(engine::kind::gpu, 0);
    // [Initialize engine]

    /// In addition to an engine, all primitives require a @ref dnnl::stream
    /// for the execution. The stream encapsulates an execution context and is
    /// tied to a particular engine.
    ///
    /// In this example, a GPU stream is created.
    ///
    /// @snippet cross_engine_reorder.cpp Initialize stream
    // [Initialize stream]
    auto stream_gpu = stream(gpu_engine);
    // [Initialize stream]

    /// @subsection cross_engine_reorder_cpp_sub2 Wrapping data into DNNL GPU memory object
    /// Fill the data in CPU memory first, and then move data from CPU to GPU
    /// memory by reorder.
    /// @snippet cross_engine_reorder.cpp reorder cpu2gpu
    //  [reorder cpu2gpu]
    const auto tz = memory::dims {2, 16, 1, 1};
    auto m_cpu
            = memory({{tz}, memory::data_type::f32, memory::format_tag::nchw},
                    cpu_engine);
    auto m_gpu
            = memory({{tz}, memory::data_type::f32, memory::format_tag::nchw},
                    gpu_engine);
    fill(m_cpu, tz);
    auto r1 = reorder(m_cpu, m_gpu);
    //  [reorder cpu2gpu]

    /// @subsection cross_engine_reorder_cpp_sub3 Creating a ReLU primitive
    ///
    /// Let's now create a ReLU primitive for GPU.
    ///
    /// The library implements the ReLU primitive as a particular algorithm of a
    /// more general @ref dev_guide_eltwise primitive, which applies a specified
    /// function to each element of the source tensor.
    ///
    /// Just as in the case of @ref dnnl::memory, a user should always go
    /// through (at least) three creation steps (which, however, can sometimes
    /// be combined thanks to C++11):
    /// 1. Initialize an operation descriptor (in the case of this example,
    ///    @ref dnnl::eltwise_forward::desc), which defines the operation
    ///    parameters including a GPU memory descriptor.
    /// 2. Create an operation primitive descriptor (here @ref
    ///    dnnl::eltwise_forward::primitive_desc) on a GPU engine, which is a
    ///    **lightweight** descriptor of the actual algorithm that
    ///    **implements** the given operation.
    /// 3. Create a primitive (here @ref dnnl::eltwise_forward) that can be
    ///    executed on GPU memory objects to compute the operation by a GPU
    ///    engine.
    ///
    ///@note
    ///    Primitive creation might be a very expensive operation, so consider
    ///    creating primitive objects once and executing them multiple times.
    ///
    /// The code:
    /// @snippet cross_engine_reorder.cpp Create a ReLU primitive
    // [Create a ReLU primitive]
    //  ReLU op descriptor (uses a GPU memory as source memory.
    //  no engine- or implementation-specific information)
    auto relu_d = eltwise_forward::desc(prop_kind::forward,
            algorithm::eltwise_relu, m_gpu.get_desc(), 0.0f);
    // ReLU primitive descriptor, which corresponds to a particular
    // implementation in the library. Specify engine type for the ReLU
    // primitive. Use a GPU engine here.
    auto relu_pd = eltwise_forward::primitive_desc(relu_d, gpu_engine);
    // ReLU primitive
    auto relu = eltwise_forward(relu_pd);
    // [Create a ReLU primitive]

    /// @subsection cross_engine_reorder_cpp_sub4 Getting results from an DNNL GPU memory object
    /// After the ReLU operation, users need to get data from GPU to CPU memory
    /// by reorder.
    /// @snippet cross_engine_reorder.cpp reorder gpu2cpu
    //  [reorder gpu2cpu]
    auto r2 = reorder(m_gpu, m_cpu);
    //  [reorder gpu2cpu]

    /// @subsection cross_engine_reorder_cpp_sub5 Executing all primitives
    ///
    /// Finally, let's execute all primitives and wait for their completion
    /// via the following sequence:
    ///
    /// Reorder(CPU,GPU) -> ReLU -> Reorder(GPU,CPU).
    ///
    /// 1. After execution of the first Reorder, ReLU has source data in GPU.
    ///
    /// 2. The input and output memory objects are passed to the ReLU
    /// `execute()` method using a <tag, memory> map. Each tag specifies what
    /// kind of tensor each memory object represents. All @ref dev_guide_eltwise
    /// primitives require the map to have two elements: a source memory
    /// object (input) and a destination memory (output). For executing
    /// on GPU engine, both source and destination memory object must use
    /// GPU memory.
    ///
    /// 3. After the execution of the ReLU on GPU, the second Reorder moves
    /// the results from GPU to CPU.
    ///
    /// @note
    ///    All primitives are executed in the SAME GPU stream (the first
    ///    parameter of the `execute()` method).
    ///
    /// Depending on the stream kind, an execution might be blocking or
    /// non-blocking. This means that we need to call @ref
    /// dnnl::stream::wait before accessing the results.
    ///
    /// @snippet cross_engine_reorder.cpp Execute primitives
    // [Execute primitives]
    // wrap source data from CPU to GPU
    r1.execute(stream_gpu, m_cpu, m_gpu);
    // Execute ReLU on a GPU stream
    relu.execute(stream_gpu, {{DNNL_ARG_SRC, m_gpu}, {DNNL_ARG_DST, m_gpu}});
    // Get result data from GPU to CPU
    r2.execute(stream_gpu, m_gpu, m_cpu);

    stream_gpu.wait();
    // [Execute primitives]

    /// @page cross_engine_reorder_cpp
    /// @subsection cross_engine_reorder_cpp_sub6 Validate the result
    ///
    /// Now that we have the computed the result on CPU memory, let's validate
    /// that it is actually correct.
    ///
    /// @snippet cross_engine_reorder.cpp Check the results
    // [Check the results]
    if (find_negative(m_cpu, tz) != 0) {
        std::stringstream ss;
        ss << "Unexpected output, find a negative value after the ReLU "
              "execution";
        throw ss.str();
    }
    // [Check the results]
}

/// @page cross_engine_reorder_cpp
/// @section cross_engine_reorder_cpp_main main() function
///
/// We now just call everything we prepared earlier.
///
/// Since we are using the DNNL C++ API, we use exceptions to handle
/// errors (see @ref dev_guide_c_and_cpp_apis).
/// The DNNL C++ API throws exceptions of type @ref dnnl::error,
/// which contains the error status (of type @ref dnnl_status_t) and a
/// human-readable error message accessible through regular `what()` method.
/// @snippet cross_engine_reorder.cpp Main

// [Main]
int main(int argc, char **argv) {
    try {
        cross_engine_reorder_tutorial();
    } catch (dnnl::error &e) {
        std::cerr << "DNNL error: " << e.what() << std::endl
                  << "Error status: " << dnnl_status2str(e.status) << std::endl;
        return 1;
    } catch (std::string &e) {
        std::cerr << "Error in the example: " << e << std::endl;
        return 2;
    }

    std::cout << "Example passes" << std::endl;
    return 0;
}
// [Main]

/// <b></b>
///
/// Upon compiling and running the example, the output should be just:
///
/// ~~~
/// Example passes
/// ~~~
///
/// @page cross_engine_reorder_cpp

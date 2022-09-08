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

#ifndef DNNL_TEST_COMMON_HPP
#define DNNL_TEST_COMMON_HPP

#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <type_traits>
#include <unordered_map>

#include "gtest/gtest.h"

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#define collapse(x)
#endif

#include "dnnl.hpp"
#include "dnnl_test_macros.hpp"

#if DNNL_GPU_RUNTIME == DNNL_RUNTIME_OCL
#include "dnnl_test_common_ocl.hpp"
#endif

#include "src/common/bfloat16.hpp"
#include "src/common/dnnl_thread.hpp"
#include "src/common/float16.hpp"
#include "src/common/memory_desc_wrapper.hpp"
#include "src/common/nstl.hpp"

#define for_ for

using dnnl::impl::bfloat16_t;
using dnnl::impl::f16_support::float16_t;

#define DNNL_CHECK(f) \
    do { \
        dnnl_status_t s = (f); \
        ASSERT_EQ(s, dnnl_success); \
    } while (0)

using memory = dnnl::memory;

bool is_current_test_failed();
dnnl::engine::kind get_test_engine_kind();

template <typename data_t>
struct data_traits {};
template <>
struct data_traits<float16_t> {
    static const auto data_type = memory::data_type::f16;

    using uint_type = uint16_t;
};
template <>
struct data_traits<bfloat16_t> {
    static const auto data_type = memory::data_type::bf16;

    using uint_type = uint16_t;
};
template <>
struct data_traits<float> {
    static const auto data_type = memory::data_type::f32;

    using uint_type = uint32_t;
};
template <>
struct data_traits<uint8_t> {
    static const auto data_type = memory::data_type::u8;

    using uint_type = uint8_t;
};
template <>
struct data_traits<int8_t> {
    static const auto data_type = memory::data_type::s8;

    using uint_type = uint8_t;
};
template <>
struct data_traits<int32_t> {
    static const auto data_type = memory::data_type::s32;

    using uint_type = uint32_t;
};

template <typename T>
inline void assert_eq(T a, T b);
template <>
inline void assert_eq<float>(float a, float b) {
    ASSERT_FLOAT_EQ(a, b);
}

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
inline int mxcsr_round(float f) {
    return _mm_cvtss_si32(_mm_load_ss(&f));
}
#else
inline int mxcsr_round(float f) {
    return (int)nearbyintf(f);
}
#endif

template <typename data_t>
data_t out_round(float x) {
    return (data_t)mxcsr_round(x);
}
template <>
inline float out_round<float>(float x) {
    return x;
}

template <typename data_t, typename out_t>
out_t saturate(const out_t &x) {
    out_t v = x;
    if (v <= std::numeric_limits<data_t>::min())
        v = std::numeric_limits<data_t>::min();
    if (v > std::numeric_limits<data_t>::max())
        v = std::numeric_limits<data_t>::max();
    return v;
}

inline memory::dim right_padding(memory::dim i, memory::dim o, memory::dim k,
        memory::dim p, memory::dim s, memory::dim d = 0) {
    return (o - 1) * s + (k - 1) * (d + 1) - (p + i - 1);
}

template <typename data_t>
struct acc_t {
    typedef data_t type;
};
template <>
struct acc_t<int8_t> {
    typedef int type;
};
template <>
struct acc_t<uint8_t> {
    typedef int type;
};

// Smart pointer for map/unmap operations with unique_ptr semantics
template <typename T>
struct mapped_ptr_t {
    using nonconst_type = typename std::remove_cv<T>::type;

    mapped_ptr_t(std::nullptr_t) : mem_(nullptr), ptr_(nullptr) {}
    mapped_ptr_t(const memory *mem) : mem_(mem) {
        ptr_ = mem->map_data<nonconst_type>();
    }
    mapped_ptr_t(mapped_ptr_t &&other) : mem_(other.mem_), ptr_(other.ptr_) {
        other.mem_ = nullptr;
        other.ptr_ = nullptr;
    }

    mapped_ptr_t(const mapped_ptr_t &) = delete;
    mapped_ptr_t &operator=(const mapped_ptr_t &) = delete;

    ~mapped_ptr_t() {
        if (mem_ && ptr_) mem_->unmap_data(ptr_);
    };

    operator T *() { return ptr_; }
    operator const T *() const { return ptr_; }

private:
    const memory *mem_;
    nonconst_type *ptr_;
};

template <typename T>
mapped_ptr_t<T> map_memory(const memory &mem) {
    return mapped_ptr_t<T>(&mem);
}

// check_zero_tail - check on zero or set to zero padded memory
template <typename data_t>
void check_zero_tail(int set_zero_flag, const memory &src) {

    auto src_data = map_memory<data_t>(src);

    const memory::desc src_d = src.get_desc();
    const int ndims = src_d.data.ndims;
    const auto *dims = src_d.data.dims;
    const auto *pdims = src_d.data.padded_dims;
    const dnnl::impl::memory_desc_wrapper mdw(src_d.data);

    memory::dim idx[DNNL_MAX_NDIMS] = {}, str[DNNL_MAX_NDIMS] = {};
    memory::dim nelems = 1;
    int tail_flag = 0;
    for (int i = 0; i < ndims; ++i) {
        if (dims[ndims - i - 1] != pdims[ndims - i - 1]) tail_flag = 1;
        nelems *= pdims[ndims - i - 1];
        idx[i] = 0;
        str[i] = (i == 0) ? 1 : str[i - 1] * pdims[ndims - i];
    }
    if (tail_flag == 0) return;

    for (memory::dim i = 0; i < nelems; ++i) {
        memory::dim off = 0;
        bool flag = 0;
        for (int j = 0; j < ndims; ++j) {
            off += idx[j] * str[j];
            if (idx[j] >= dims[ndims - j - 1]) flag = 1;
        }
        if (flag == 1) {
            memory::dim blk_off = mdw.off_l(off, true);
            if (set_zero_flag) {
                src_data[blk_off] = 0.0;
            } else {
                ASSERT_EQ(src_data[blk_off], 0.0)
                        << " blk_off = " << blk_off << "off = " << off;
            }
        }
        /*Update idx*/
        for (int j = 0; j < ndims; ++j) {
            idx[j]++;
            if (idx[j] < pdims[ndims - j - 1]) break;
            idx[j] = 0;
        }
    }
}

inline memory::desc create_md(memory::dims dims, memory::data_type data_type,
        memory::format_tag fmt_tag) {
    return memory::desc(dims, data_type, fmt_tag);
}

template <typename data_t>
static inline data_t set_value(
        memory::dim index, data_t mean, data_t deviation, double sparsity) {
    if (data_traits<data_t>::data_type == memory::data_type::f16
            || data_traits<data_t>::data_type == memory::data_type::bf16) {
        return data_t(set_value<float>(index, mean, deviation, sparsity));
    } else if (data_traits<data_t>::data_type == memory::data_type::f32) {
        const memory::dim group_size = (memory::dim)(1. / sparsity);
        const memory::dim group = index / group_size;
        const memory::dim in_group = index % group_size;
        const bool fill = in_group == ((group % 1637) % group_size);
        return fill ? static_cast<data_t>(
                       mean + deviation * sinf(float(index % 37)))
                    : data_t {0};
    } else if (data_traits<data_t>::data_type == memory::data_type::s32
            || data_traits<data_t>::data_type == memory::data_type::s8) {
        return data_t(index * 13 % 21 - 10);
    } else if (data_traits<data_t>::data_type == memory::data_type::u8) {
        return data_t(index * 13 % 17);
    }
    assert(!"not expected");
    return data_t(0);
}

template <typename data_t>
static void fill_data(const memory::dim size, data_t *data, data_t mean,
        data_t deviation, double sparsity = 1.) {
    dnnl::impl::parallel_nd(size, [&](memory::dim n) {
        data[n] = set_value<data_t>(n, mean, deviation, sparsity);
    });
}

template <typename data_t>
static void fill_data(const memory::dim size, const memory &mem, data_t mean,
        data_t deviation, double sparsity = 1.) {
    auto data_ptr = map_memory<data_t>(mem);
    fill_data<data_t>(size, data_ptr, mean, deviation, sparsity);
}

template <typename data_t>
static void fill_data(const memory::dim size, data_t *data,
        double sparsity = 1., bool init_negs = false) {
    dnnl::impl::parallel_nd(size, [&](memory::dim n) {
        data[n] = set_value<data_t>(n, data_t(1), data_t(2e-1), sparsity);

        if (init_negs && n % 4 == 0)
            data[n] = static_cast<data_t>(
                    -data[n]); // weird for unsigned types!
    });
}

template <typename data_t>
static void fill_data(const memory::dim size, const memory &mem,
        double sparsity = 1., bool init_negs = false) {
    auto data_ptr = map_memory<data_t>(mem);
    fill_data<data_t>(size, data_ptr, sparsity, init_negs);
}

template <typename data_t>
static void compare_data(
        const memory &ref, const memory &dst, data_t threshold = (data_t)1e-4) {
    using data_type = memory::data_type;

    ASSERT_TRUE(data_traits<data_t>::data_type == data_type::f32
            || data_traits<data_t>::data_type == data_type::f16
            || data_traits<data_t>::data_type == data_type::bf16
            || data_traits<data_t>::data_type == data_type::s32
            || data_traits<data_t>::data_type == data_type::s8);

    /* Note: size_t incompatible with MSVC++ */
    auto ref_desc = ref.get_desc();
    auto dst_desc = dst.get_desc();
    const dnnl::impl::memory_desc_wrapper mdw_ref(ref_desc.data);
    const dnnl::impl::memory_desc_wrapper mdw_dst(dst_desc.data);

    ASSERT_TRUE(ref_desc.data.ndims == dst_desc.data.ndims);

    auto ndims = ref_desc.data.ndims;

    for (auto d = 0; d < ndims; ++d) {
        ASSERT_TRUE(ref_desc.data.dims[d] == dst_desc.data.dims[d]);
    }

    auto dims = ref_desc.data.dims;

    memory::dim num = 1;
    for (auto d = 0; d < ndims; ++d) {
        num *= dims[d];
    }

    auto ref_data = map_memory<data_t>(ref);
    auto dst_data = map_memory<data_t>(dst);

    dnnl::impl::parallel_nd(num, [&](memory::dim i) {
        if (is_current_test_failed()) return;

        data_t ref = ref_data[mdw_ref.off_l(i, true)];
        data_t got = dst_data[mdw_dst.off_l(i, true)];

        if (data_traits<data_t>::data_type == data_type::f32
                || data_traits<data_t>::data_type == data_type::f16
                || data_traits<data_t>::data_type == data_type::bf16) {
            const float threshold_f32 = static_cast<float>(threshold);
            const float ref_f32 = static_cast<float>(ref);
            const float got_f32 = static_cast<float>(got);
            const float diff_f32
                    = (got_f32 == ref_f32) ? 0.0f : got_f32 - ref_f32;
            const float e = (std::abs(ref_f32) > threshold_f32)
                    ? diff_f32 / ref_f32
                    : diff_f32;
            ASSERT_NEAR(e, 0.0, threshold_f32)
                    << "Index: " << i << " Total: " << num;
        } else {
            ASSERT_EQ(ref, got) << "Index: " << i << " Total: " << num;
        }
    });
}

inline const char *query_impl_info(const_dnnl_primitive_desc_t pd) {
    const char *str;
    dnnl_primitive_desc_query(pd, dnnl_query_impl_info_str, 0, &str);
    return str;
};

inline dnnl_status_t get_conv_impl_status(
        const_dnnl_primitive_desc_t pd, const char *match_str) {
    const char *conv_str = query_impl_info(pd);

    if (strstr(conv_str, match_str) != NULL) return dnnl_status_t::dnnl_success;
    return dnnl_status_t::dnnl_unimplemented;
};

struct test_convolution_sizes_t {
    test_convolution_sizes_t(memory::dim mb, memory::dim ng, memory::dim ic,
            memory::dim ih, memory::dim iw, memory::dim oc, memory::dim oh,
            memory::dim ow, memory::dim kh, memory::dim kw, memory::dim padh,
            memory::dim padw, memory::dim strh, memory::dim strw,
            memory::dim dilh = 0, memory::dim dilw = 0)
        : mb(mb)
        , ng(ng)
        , ic(ic)
        , ih(ih)
        , iw(iw)
        , oc(oc)
        , oh(oh)
        , ow(ow)
        , kh(kh)
        , kw(kw)
        , padh(padh)
        , padw(padw)
        , strh(strh)
        , strw(strw)
        , dilh(dilh)
        , dilw(dilw) {}
    memory::dim mb;
    memory::dim ng;
    memory::dim ic, ih, iw;
    memory::dim oc, oh, ow;
    memory::dim kh, kw;
    memory::dim padh, padw;
    memory::dim strh, strw;
    memory::dim dilh, dilw;
};

struct test_convolution_attr_t {
    struct scale_t {
        enum policy_t { NONE = 0, COMMON };

        bool is_def() const { return policy != NONE; }

        scale_t(float s, policy_t p = NONE) : scale(s) { policy = p; }

        policy_t policy;
        float scale;
    };

    void dnnl_attr_recreate() {
        mkl_attr = dnnl::primitive_attr();
        if (oscale.is_def()) {
            const memory::dim count = 1;
            const int mask = 0;
            std::vector<float> s(count, oscale.scale);
            mkl_attr.set_output_scales(mask, s);
        }
    }

    test_convolution_attr_t(
            float s, scale_t::policy_t p = scale_t::policy_t::NONE)
        : oscale(s, p), mkl_attr() {}

    test_convolution_attr_t() : test_convolution_attr_t(1.f) {}

    scale_t oscale;
    dnnl::primitive_attr mkl_attr;
};

struct test_convolution_formats_t {
    memory::format_tag src_format;
    memory::format_tag weights_format;
    memory::format_tag bias_format;
    memory::format_tag dst_format;
};

struct test_convolution_params_t {
    dnnl::algorithm aalgorithm;
    test_convolution_formats_t formats;
    test_convolution_attr_t attr;
    test_convolution_sizes_t sizes;
    bool expect_to_fail;
    dnnl_status_t expected_status;
};

struct test_convolution_eltwise_params_t {
    const dnnl::algorithm alg;
    dnnl::algorithm aalgorithm;
    const float eltwise_alpha;
    const float eltwise_beta;
    test_convolution_formats_t formats;
    test_convolution_attr_t attr;
    test_convolution_sizes_t sizes;
    bool expect_to_fail;
    dnnl_status_t expected_status;
};

template <typename F>
bool catch_expected_failures(const F &f, bool expect_to_fail,
        dnnl_status_t expected_status, bool ignore_unimplemented = true) {
    try {
        f();
    } catch (const dnnl::error &e) {
        // Rethrow the exception if it is not expected or the error status did
        // not match.
        if (!(expect_to_fail) || e.status != (expected_status)) {
            // Ignore unimplemented
            if (ignore_unimplemented && (e.status == dnnl_unimplemented)) {
                // Print unimplemented but do not treat as error
                std::cout << "[  UNIMPL  ] "
                          << "Implementation not found" << std::endl;
                return true;
            } else {
                throw e;
            }
        }
        // Return normally if the failure is expected
        if (expect_to_fail) return true;
    }

    // Throw an exception if the failure is expected but did not happen
    if (expect_to_fail) throw std::exception();

    return false;
}

#define TEST_MALLOC_OFFSET 8
static char *test_malloc(size_t size) {
    void *ptr;
    const size_t align = 64;
    const size_t padded_size = TEST_MALLOC_OFFSET + size;
#ifdef _WIN32
    ptr = _aligned_malloc(padded_size, align);
    int rc = ((ptr) ? 0 : errno);
#else
    int rc = ::posix_memalign(&ptr, align, padded_size);
#endif /* _WIN32 */
    return rc == 0 ? (char *)ptr + TEST_MALLOC_OFFSET : 0;
}

static void test_free(char *ptr) {
    char *base_ptr = ptr - TEST_MALLOC_OFFSET;
#ifdef _WIN32
    _aligned_free(base_ptr);
#else
    return ::free(base_ptr);
#endif /* _WIN32 */
}
#undef TEST_MALLOC_OFFSET

class test_memory {
public:
    test_memory(const memory::desc &d, const dnnl::engine &e) {
        bool is_cpu_native = (e.get_kind() == dnnl::engine::kind::cpu);

        size_ = d.get_size();
        if (is_cpu_native) {
            data_.reset(test_malloc(size_), test_free);
            mem_ = memory(d, e, data_.get());
        } else {
            mem_ = memory(d, e);
        }
        // Fill with a magic number to catch possible uninitialized access
        mapped_ptr_t<char> ptr(&mem_);
        memset(ptr, 0xFF, size_);
    }
    size_t get_size() const { return size_; }
    const memory &get() const { return mem_; }

    operator bool() const { return mem_.get(true) != nullptr; }

private:
    memory mem_;
    std::shared_ptr<char> data_;
    size_t size_;
};

template <typename T>
mapped_ptr_t<T> map_memory(const test_memory &mem) {
    return mapped_ptr_t<T>(&mem.get());
}

inline std::string to_string(dnnl_engine_kind_t engine_kind) {
    std::stringstream ss;
    if (engine_kind == dnnl_cpu)
        ss << "cpu";
    else if (engine_kind == dnnl_gpu)
        ss << "gpu";
    else
        ss << "unknown";

    return ss.str();
}

#endif

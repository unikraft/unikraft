/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
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

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "dnnl.h"

#include "common.hpp"

// BENCHDNN_MEMORY_CHECK macro enables guarding mechanism for memory allocation:
// memory block is allocated on a page boundary and the page after the block is
// protected to catch possible invalid accesses.
//
// Note that the macro affects the correctness mode only.
#ifdef __unix__
#define BENCHDNN_MEMORY_CHECK
#endif

#ifdef BENCHDNN_MEMORY_CHECK
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

const char *bench_mode2str(bench_mode_t mode) {
    const char *modes[] = {"MODE_UNDEF", "CORR", "PERF", "CORR+PERF", "LIST"};
    assert((int)mode < sizeof(modes) / sizeof(*modes));
    return modes[(int)mode];
}

bench_mode_t str2bench_mode(const char *str) {
    bench_mode_t mode = MODE_UNDEF;
    if (strchr(str, 'c') || strchr(str, 'C'))
        mode = (bench_mode_t)((int)mode | (int)CORR);
    if (strchr(str, 'p') || strchr(str, 'P'))
        mode = (bench_mode_t)((int)mode | (int)PERF);
    if (strchr(str, 'l') || strchr(str, 'L')) {
        // list mode is exclusive
        assert(mode == MODE_UNDEF);
        mode = (bench_mode_t)((int)mode | (int)LIST);
    }
    if (mode == MODE_UNDEF)
        []() {
            SAFE(FAIL, CRIT);
            return 0;
        }();
    return mode;
}

/* perf */
#include <chrono>

static inline double ms_now() {
    auto timePointTmp
            = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration<double, std::milli>(timePointTmp).count();
}

#if !defined(BENCHDNN_USE_RDPMC) || defined(_WIN32)
unsigned long long ticks_now() {
    return (unsigned long long)0;
}
#else
unsigned long long ticks_now() {
    unsigned eax, edx, ecx;

    ecx = (1 << 30) + 1;
    __asm__ volatile("rdpmc" : "=a"(eax), "=d"(edx) : "c"(ecx));

    return (unsigned long long)eax | (unsigned long long)edx << 32;
}
#endif

void benchdnn_timer_t::reset() {
    times_ = 0;
    for (int i = 0; i < n_modes; ++i)
        ticks_[i] = 0;
    ticks_start_ = 0;
    for (int i = 0; i < n_modes; ++i)
        ms_[i] = 0;
    ms_start_ = 0;

    start();
}

void benchdnn_timer_t::start() {
    ticks_start_ = ticks_now();
    ms_start_ = ms_now();
}

void benchdnn_timer_t::stop(int add_times) {
    long long d_ticks = ticks_now() - ticks_start_; /* FIXME: overflow? */
    double d_ms = ms_now() - ms_start_;

    ticks_start_ += d_ticks;
    ms_start_ += d_ms;

    ms_[benchdnn_timer_t::avg] += d_ms;
    ticks_[benchdnn_timer_t::avg] += d_ticks;

    d_ticks /= add_times;
    d_ms /= add_times;

    ms_[benchdnn_timer_t::min]
            = times_ ? MIN2(ms_[benchdnn_timer_t::min], d_ms) : d_ms;
    ms_[benchdnn_timer_t::max]
            = times_ ? MAX2(ms_[benchdnn_timer_t::max], d_ms) : d_ms;

    ticks_[benchdnn_timer_t::min]
            = times_ ? MIN2(ticks_[benchdnn_timer_t::min], d_ticks) : d_ticks;
    ticks_[benchdnn_timer_t::max]
            = times_ ? MAX2(ticks_[benchdnn_timer_t::max], d_ticks) : d_ticks;

    times_ += add_times;
}

benchdnn_timer_t &benchdnn_timer_t::operator=(const benchdnn_timer_t &rhs) {
    if (this == &rhs) return *this;
    times_ = rhs.times_;
    for (int i = 0; i < n_modes; ++i)
        ticks_[i] = rhs.ticks_[i];
    ticks_start_ = rhs.ticks_start_;
    for (int i = 0; i < n_modes; ++i)
        ms_[i] = rhs.ms_[i];
    ms_start_ = rhs.ms_start_;
    return *this;
}

/* result structure */
const char *state2str(res_state_t state, bool allow_unimpl) {
    if (state == UNIMPLEMENTED && !allow_unimpl) return "UNIMPLEMENTED_FAILED";

#define CASE(x) \
    if (state == x) return STRINGIFY(x)
    CASE(UNTESTED);
    CASE(PASSED);
    CASE(SKIPPED);
    CASE(MISTRUSTED);
    CASE(UNIMPLEMENTED);
    CASE(FAILED);
    CASE(LISTED);
#undef CASE
    assert(!"unknown res state");
    return "STATE_UNDEF";
}

void parse_result(res_t &res, bool &want_perf_report, bool allow_unimpl,
        int status, const char *pstr) {
    auto &bs = benchdnn_stat;
    const char *state = state2str(res.state, allow_unimpl);

    switch (res.state) {
        case UNTESTED:
            if (!(bench_mode & CORR)) {
                want_perf_report = true;
                break;
            }
        case FAILED:
            assert(status == FAIL);
            bs.failed++;
            print(0, "%d:%s (errors:%lu total:%lu) __REPRO: %s\n", bs.tests,
                    state, (unsigned long)res.errors, (unsigned long)res.total,
                    pstr);
            break;
        case SKIPPED:
            assert(status == OK);
            print(0, "%d:%s __REPRO: %s\n", bs.tests, state, pstr);
            bs.skipped++;
            break;
        case UNIMPLEMENTED:
            assert(status == OK);
            print(0, "%d:%s __REPRO: %s\n", bs.tests, state, pstr);
            bs.unimplemented++;
            bs.failed += !allow_unimpl;
            break;
        case MISTRUSTED:
            assert(status == OK);
            bs.mistrusted++;
            print(0, "%d:%s __REPRO: %s\n", bs.tests, state, pstr);
            // bs.failed++; /* temporal workaround for some tests */
            break;
        case PASSED:
            assert(status == OK);
            print(0, "%d:%s __REPRO: %s\n", bs.tests, state, pstr);
            want_perf_report = true;
            bs.passed++;
            break;
        case LISTED:
            assert(status == OK);
            print(0, "%d:%s __REPRO: %s\n", bs.tests, state, pstr);
            want_perf_report = false;
            bs.listed++;
            break;
        default:
            assert(!"unknown state");
            {
                []() {
                    SAFE(FAIL, CRIT);
                    return 0;
                }();
            }
    }

    if (bench_mode & PERF) {
        using bt = benchdnn_timer_t;
        for (int mode = 0; mode < (int)bt::n_modes; ++mode)
            bs.ms[mode] += res.timer.ms((bt::mode_t)mode);
    }
}

/* misc */

#ifdef BENCHDNN_MEMORY_CHECK
static void *zmalloc_protect(size_t size) {
    const size_t page_sz = getpagesize();

    const size_t block_sz = size + 2 * sizeof(void *);
    const size_t total_sz = div_up(block_sz, page_sz) * page_sz + page_sz;

    void *mem_ptr;
    int rc = ::posix_memalign(&mem_ptr, page_sz, total_sz);
    if (rc != 0) return nullptr;

    uint8_t *ptr_start = (uint8_t *)mem_ptr;
    uint8_t *ptr = ptr_start + total_sz - page_sz - size;

    // Aligned on a page boundary
    void *ptr_protect = ptr + size;

    // Layout of the allocated region:
    // ptr_start   <- start of the allocated region
    // ptr[-16]    <- stores start address: ptr_start
    // ptr[-8]     <- stores protected address: ptr_protect
    // ptr         <- pointer to be returned from the function
    // ptr_protect <- pointer to the block to protect

    // Protect one page right after the block of size bytes
    int err = mprotect(ptr_protect, page_sz, PROT_NONE);
    if (err != 0) {
        ::free(ptr_start);
        return nullptr;
    }

    // Save pointers for zfree_protect
    ((void **)ptr)[-2] = ptr_start;
    ((void **)ptr)[-1] = ptr_protect;

    return ptr;
}

static void zfree_protect(void *ptr) {
    const size_t page_sz = getpagesize();

    // Restore read-write access for the protected region
    void *ptr_protect = ((void **)ptr)[-1];
    mprotect(ptr_protect, page_sz, PROT_READ | PROT_WRITE);

    // Deallocate the whole region
    void *ptr_start = ((void **)ptr)[-2];
    ::free(ptr_start);
}
#endif

void *zmalloc(size_t size, size_t align) {
#ifdef BENCHDNN_MEMORY_CHECK
    if (bench_mode & CORR) { return zmalloc_protect(size); }
#endif

    void *ptr;
#ifdef _WIN32
    ptr = _aligned_malloc(size, align);
    int rc = ((ptr) ? 0 : errno);
#else
    // TODO. Heuristics: Increasing the size to alignment increases
    // the stability of performance results.
    if ((bench_mode & PERF) && (size < align)) size = align;
    int rc = ::posix_memalign(&ptr, align, size);
#endif /* _WIN32 */
    return rc == 0 ? ptr : 0;
}

void zfree(void *ptr) {
#ifdef BENCHDNN_MEMORY_CHECK
    if (bench_mode & CORR) {
        zfree_protect(ptr);
        return;
    }
#endif

#ifdef _WIN32
    _aligned_free(ptr);
#else
    return ::free(ptr);
#endif /* _WIN32 */
}

bool str2bool(const char *str) {
    return !strcasecmp("true", str) || !strcasecmp("1", str);
}

const char *bool2str(bool value) {
    return value ? "true" : "false";
}

#ifdef _WIN32
/* NOTE: this should be supported on linux as well, but currently
 * having issues for ICC170 and Clang*/
#include <regex>

bool match_regex(const char *str, const char *pattern) {
    std::regex re(pattern);
    return std::regex_search(str, re);
}
#else
#include <regex.h>
#include <sys/types.h>

bool match_regex(const char *str, const char *pattern) {
    static regex_t regex;
    static const char *prev_pattern = NULL;
    if (pattern != prev_pattern) {
        if (prev_pattern) regfree(&regex);

        if (regcomp(&regex, pattern, 0)) {
            fprintf(stderr, "could not create regex\n");
            return true;
        }

        prev_pattern = pattern;
    }

    return !regexec(&regex, str, 0, NULL, 0);
}
#endif /* _WIN32 */

bool maybe_skip(const char *skip_impl, const char *impl_str) {
    if (skip_impl == NULL || *skip_impl == '\0') return false;

    const std::string impl(impl_str);

    const char *s_start = skip_impl;
    while (1) {
        if (*s_start == '"' || *s_start == '\'') ++s_start;

        const char *s_end = strchr(s_start, ':');
        size_t len = s_end ? s_end - s_start : strlen(s_start);

        if (s_start[len - 1] == '"' || s_start[len - 1] == '\'') --len;

        const std::string what(s_start, s_start + len);
        if (impl.find(what) != std::string::npos) return true;

        if (s_end == NULL) break;

        s_start = s_end + 1;
        if (*s_start == '\0') break;
    }

    return false;
}

#if defined(_WIN32) && !defined(__GNUC__)
#include <windows.h>
#define PATH_MAX MAX_PATH
static char *dirname(char *path) {
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    SAFE_V(_splitpath_s(path, drive, sizeof(drive), dir, sizeof(dir), NULL, 0,
                   NULL, 0) == 0
                    ? OK
                    : FAIL);
    path[0] = '\0';
    if (drive != NULL)
        SAFE_V(strncat_s(path, PATH_MAX, drive, _MAX_DRIVE) == 0 ? OK : FAIL);
    if (dir != NULL)
        SAFE_V(strncat_s(path, PATH_MAX, dir, _MAX_DIR) == 0 ? OK : FAIL);
    if (path[0] == '\0') {
        path[0] = '.';
        path[1] = '\0';
    }
    return path;
}

int readlink(const char *path, char *buf, size_t buf_max) {
    (void)path;
    // NULL means take the path of myself
    return GetModuleFileName(NULL, buf, (DWORD)buf_max);
}
#else
#include <libgen.h>
#include <unistd.h>
#endif /* _WIN32 */

std::string locate_batch_file(const std::string &fname) {
    SAFE_V(fname.length() < PATH_MAX ? OK : FAIL);

    const int max_paths = 4;

    static int n_paths = 0;
    static std::string search_paths[max_paths];

    std::string fdir;
    {
        std::string fname_copy = fname;
        fname_copy.resize(PATH_MAX);
        char *c_fdir = dirname(&fname_copy[0]);
        fdir = std::string(c_fdir);
    }

    bool dir_found = false;
    for (int n = 0; n_paths < max_paths && n < n_paths; ++n)
        if (search_paths[n].find(fdir) == 0) {
            dir_found = true;
            break;
        }
    if (!dir_found) {
        SAFE_V(n_paths < max_paths ? OK : FAIL);
        search_paths[n_paths++] = std::move(fdir);
    }

    std::ifstream ifs(fname);
    if (ifs.is_open()) return fname;

    for (int n = 0; n < n_paths; ++n) {
        const std::string fullname = search_paths[n] + "/" + fname;
        ifs.open(fullname);
        if (ifs.is_open()) {
            print(50, "batch file used: %s\n", fullname.c_str());
            return fullname;
        }
    }

    // Search in default inputs directory
    // Takes dirname(executable)/inputs/file_name on Linux
    // Takes dirname(executable)/../inputs/file_name on Windows
    fdir.resize(PATH_MAX);
    int length = readlink("/proc/self/exe", &fdir[0], PATH_MAX);
    if (length) {
        std::string s_fdir = std::string(dirname(&fdir[0]));
        for (int i_try = 0; i_try < 2; ++i_try) {
            fdir = s_fdir + std::string("/inputs/") + std::string(driver_name);
            if (i_try == 1) { // Windows has different folder structure
                fdir = s_fdir + std::string("/../inputs/")
                        + std::string(driver_name);
            }
            const std::string fullname = fdir + "/" + fname;
            ifs.open(fullname);
            if (ifs.is_open()) {
                search_paths[n_paths++] = std::move(fdir);
                print(50, "batch file used: %s\n", fullname.c_str());
                return fullname;
            }
        }
    }

    fprintf(stderr, "cannot open file %s\n", fname.c_str());
    return fname;
}

int batch(const char *fname, bench_f bench) {
    std::ifstream ifs(locate_batch_file(std::string(fname)));
    SAFE_V(ifs.is_open() ? OK : FAIL);

    std::vector<std::string> opts;
    std::string str;
    while (ifs >> str) {
        if (str.length() > 0 && str[0] == '#') {
            std::getline(ifs, str); /* stop reading till eol */
            continue;
        }
        opts.push_back(std::move(str));
    }

    std::vector<char *> c_opts;
    for (const auto &opt : opts)
        c_opts.push_back(const_cast<char *>(opt.c_str()));

    return bench(static_cast<int>(c_opts.size()), c_opts.data());
}

int flip_coin(ptrdiff_t seed, float probability) {
    const ptrdiff_t big_prime = 1000003;
    const ptrdiff_t prime = 753737;
    seed *= prime;
    return (seed % big_prime) < (probability * big_prime);
}

int64_t div_up(const int64_t a, const int64_t b) {
    SAFE_V(b != 0 ? OK : FAIL);
    return (a + b - 1) / b;
}

int64_t next_pow2(int64_t a) {
    assert(a > 0 && a <= ((int64_t)1 << 62));
    if (a > 1) a--;
    while (a & (a - 1))
        a &= (a - 1);
    return a << 1;
}

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#include <xmmintrin.h>

int mxcsr_round(float f) {
    return _mm_cvtss_si32(_mm_load_ss(&f));
}
void init_fp_mode() {
    // We set ftz to avoid denormals in perf measurements
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
}
#else
int mxcsr_round(float f) {
    return (int)nearbyintf(f);
}
void init_fp_mode() {}
#endif

void array_set(char *arr, size_t size) {
    for (size_t i = 0; i < size; ++i)
        arr[i] = 0;
}

void gemm(const char *layout, const char *transa, const char *transb, int64_t m,
        int64_t n, int64_t k, const float alpha, const float *a,
        const int64_t lda, const float *b, const int64_t ldb, const float beta,
        float *c, const int64_t ldc) {
    if (*layout == 'C') {
        dnnl_sgemm(
                *transa, *transb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);
    } else {
        dnnl_sgemm(
                *transb, *transa, n, m, k, alpha, b, ldb, a, lda, beta, c, ldc);
    }
}

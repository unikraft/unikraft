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

#ifndef COMMON_HPP
#define COMMON_HPP

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cinttypes>

#include "src/common/z_magic.hpp"

#define ABS(a) ((a) > 0 ? (a) : (-(a)))

#define MIN2(a, b) ((a) < (b) ? (a) : (b))
#define MAX2(a, b) ((a) > (b) ? (a) : (b))

#define MIN3(a, b, c) MIN2(a, MIN2(b, c))
#define MAX3(a, b, c) MAX2(a, MAX2(b, c))

#if defined(_WIN32) && !defined(__GNUC__)
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#define collapse(x)
#endif

#define IFMT "%" PRId64

#define OK 0
#define FAIL 1

enum { CRIT = 1, WARN = 2 };

#define SAFE(f, s) \
    do { \
        int status = (f); \
        if (status != OK) { \
            if (s == CRIT || s == WARN) { \
                fprintf(stderr, "@@@ error [%s:%d]: '%s' -> %d\n", \
                        __PRETTY_FUNCTION__, __LINE__, STRINGIFY(f), status); \
                fflush(0); \
                if (s == CRIT) exit(1); \
            } \
            return status; \
        } \
    } while (0)

#define SAFE_V(f) \
    do { \
        int status = (f); \
        if (status != OK) { \
            fprintf(stderr, "@@@ error [%s:%d]: '%s' -> %d\n", \
                    __PRETTY_FUNCTION__, __LINE__, STRINGIFY(f), status); \
            fflush(0); \
            exit(1); \
        } \
    } while (0)

#define SAFE_CLEAN(f, s, clean) \
    do { \
        int status = (f); \
        if (status != OK) { \
            if (s == CRIT || s == WARN) { \
                fprintf(stderr, "@@@ error [%s:%d]: '%s' -> %d\n", \
                        __PRETTY_FUNCTION__, __LINE__, STRINGIFY(f), status); \
                fflush(0); \
                if (s == CRIT) exit(1); \
            } \
            clean(); \
            return status; \
        } \
    } while (0)

extern int verbose;

#define print(v, fmt, ...) \
    do { \
        if (verbose >= v) { \
            printf(fmt, __VA_ARGS__); \
            /* printf("[%d][%s:%d]" fmt, v, __func__, __LINE__, __VA_ARGS__); */ \
            fflush(0); \
        } \
    } while (0)

#define BENCHDNN_DISALLOW_COPY_AND_ASSIGN(T) \
    T(const T &) = delete; \
    T &operator=(const T &) = delete;

enum prim_t {
    SELF,
    CONV,
    DECONV,
    IP,
    SHUFFLE,
    REORDER,
    LNORM,
    BNORM,
    RNN,
    SOFTMAX,
    POOL,
    SUM,
    ELTWISE,
    CONCAT,
    LRN,
    BINARY,
    DEF = CONV,
};

enum bench_mode_t {
    MODE_UNDEF = 0x0,
    CORR = 0x1,
    PERF = 0x2,
    LIST = 0x4,
};
const char *bench_mode2str(bench_mode_t mode);
bench_mode_t str2bench_mode(const char *str);
extern bench_mode_t bench_mode;
extern const char *driver_name;

/* perf */
extern double max_ms_per_prb; /** maximum time spends per prb in ms */
extern int min_times_per_prb; /** minimal amount of runs per prb */
extern int fix_times_per_prb; /** if non-zero run prb that many times */

struct benchdnn_timer_t {
    enum mode_t { min = 0, avg = 1, max = 2, n_modes };

    benchdnn_timer_t() { reset(); }

    void reset(); /** fully reset the measurements */

    void start(); /** restart timer */
    void stop(int add_times = 1); /** stop timer & update statistics */

    void stamp(int add_times = 1) { stop(add_times); }

    int times() const { return times_; }

    double total_ms() const { return ms_[avg]; }

    double ms(mode_t mode = min) const {
        if (!times()) return 0; // nothing to report
        return ms_[mode] / (mode == avg ? times() : 1);
    }

    double sec(mode_t mode = min) const { return ms(mode) / 1e3; }

    long long ticks(mode_t mode = min) const {
        if (!times()) return 0; // nothing to report
        return ticks_[mode] / (mode == avg ? times() : 1);
    }

    benchdnn_timer_t &operator=(const benchdnn_timer_t &rhs);

    int times_;
    long long ticks_[n_modes], ticks_start_;
    double ms_[n_modes], ms_start_;
};

/* global stats */
struct stat_t {
    int tests;
    int passed;
    int failed;
    int skipped;
    int mistrusted;
    int unimplemented;
    int listed;
    double ms[benchdnn_timer_t::mode_t::n_modes];
};
extern stat_t benchdnn_stat;

/* result structure */
enum res_state_t {
    UNTESTED = 0,
    PASSED,
    SKIPPED,
    MISTRUSTED,
    UNIMPLEMENTED,
    FAILED,
    LISTED
};
const char *state2str(res_state_t state, bool allow_unimpl);

struct res_t {
    res_state_t state;
    size_t errors, total;
    benchdnn_timer_t timer;
};

void parse_result(res_t &res, bool &want_perf_report, bool allow_unimpl,
        int status, const char *pstr);

/* misc */
void init_fp_mode();

void *zmalloc(size_t size, size_t align);
void zfree(void *ptr);

bool str2bool(const char *str);
const char *bool2str(bool value);

/* TODO: why two functions??? */
bool match_regex(const char *str, const char *pattern);
bool maybe_skip(const char *skip_impl, const char *impl_str);

typedef int (*bench_f)(int argc, char **argv);
int batch(const char *fname, bench_f bench);

/* returns 1 with given probability */
int flip_coin(ptrdiff_t seed, float probability);

int64_t div_up(const int64_t a, const int64_t b);
int64_t next_pow2(int64_t a);
int mxcsr_round(float f);

/* set '0' across *arr:+size */
void array_set(char *arr, size_t size);

/* wrapper to dnnl_sgemm
 * layout = 'F' - column major
 * layout = 'C' - row major*/
void gemm(const char *layout, const char *transa, const char *transb, int64_t m,
        int64_t n, int64_t k, const float alpha, const float *a,
        const int64_t lda, const float *b, const int64_t ldb, const float beta,
        float *c, const int64_t ldc);
#endif

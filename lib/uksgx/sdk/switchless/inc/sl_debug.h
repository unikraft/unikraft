/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _SL_DEBUG_H_
#define _SL_DEBUG_H_

/*
 * Utilities for debugging purpose
 */

#include <stdlib.h>
#include <sl_printk.h>

#define COLOR_RED               "\x1b[31m"
#define COLOR_YELLOW            "\x1b[33m"
#define COLOR_GREEN             "\x1b[32m"
#define COLOR_RESET             "\x1b[0m"

#define SHOULD_NEVER_HAPPEN     1
#define NOT_IMPLEMENTED         1

#ifdef SL_DEBUG /* For debug */

    #ifdef SL_INSIDE_ENCLAVE /* Trusted version */

    #define TRACE_ON(info)           \
        printk("[INFO] TRACE_ON %s: func %s at line %d of file %s\n",\
                (info), __func__, __LINE__, __FILENAME__)

    #define WARN_ON(cond)           \
    if ((cond) != 0) {              \
        printk(COLOR_YELLOW "[WARN] " COLOR_RESET \
                "WARN_ON(" STR(cond) "): func %s at line %d of file %s\n", \
                __func__, __LINE__, __FILENAME__); \
    }

    #define BUG_ON(cond)            \
    if ((cond) != 0) {              \
        printk(COLOR_RED "[ERROR] " COLOR_RESET \
                "BUG_ON(" STR(cond) "): func %s at line %d of file %s\n", \
                __func__, __LINE__, __FILENAME__); \
        abort();                    \
    }

    #define PANIC_ON(cond)          \
    if ((cond) != 0) {              \
        printk(COLOR_RED "[ERROR] " COLOR_RESET \
                "PANIC_ON(" STR(cond) "): func %s at line %d of file %s\n", \
                __func__, __LINE__, __FILENAME__); \
        abort();                    \
    }

    #else /* Untrusted version */

    #define TRACE_ON(info)           \
        printk("[INFO] TRACE_ON %s: func %s at line %d of file %s\n",\
                (info), __func__, __LINE__, __FILENAME__)

    #define WARN_ON(cond)           \
    if ((cond) != 0) {              \
        printk(COLOR_YELLOW "[WARN] " COLOR_RESET \
                "WARN_ON(" STR(cond) "): func %s at line %d of file %s\n", \
                __func__, __LINE__, __FILENAME__); \
    }

    #define BUG_ON(cond)            \
    if ((cond) != 0) {              \
        printk(COLOR_RED "[ERROR] " COLOR_RESET \
                "BUG_ON(" STR(cond) "): func %s at line %d of file %s\n", \
                __func__, __LINE__, __FILENAME__); \
        abort();                    \
    }

    #define PANIC_ON(cond)          \
    if ((cond) != 0) {              \
        printk(COLOR_RED "[ERROR] " COLOR_RESET \
                "PANIC_ON(" STR(cond) "): func %s at line %d of file %s\n", \
                __func__, __LINE__, __FILENAME__); \
        abort();                    \
    }

    #endif /* SL_INSIDE_ENCLAVE */

#else /* For production */

    #define TRACE_ON(x)             ((void)(x))
    #define WARN_ON(x)              ((void)(x))
    #define BUG_ON(x)               ((void)(x))
    #define PANIC_ON(x)             if (x) abort();

#endif /* SL_DEBUG */

#endif /* _SL_DEBUG_H_ */

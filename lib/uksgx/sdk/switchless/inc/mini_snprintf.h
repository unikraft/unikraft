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

/*
 * mini_snprintf() is a minimal implementation of snprintf() that supports only
 * minimal functionality in exchange of minimal dependency and minimal memory
 * footprint. This minimalism makes this printf works in the most restricted
 * circumstances, e.g., before and upon the initialization of an enclave or a
 * thread, when handling exception or AEX, etc.
 *
 * mini_snprintf() only supports printing string and integers. The supported
 * conversion specifications are:
 *      %s      - string
 *      %d      - int
 *      %u      - unsigned int
 *      %ld     - long
 *      %lu     - unsigned long
 *
 * The arguments and return values of mini_snprintf() are the same as its libc
 * counterpart.
 */

#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "sl_util.h"

/*===========================================================================
 * Interface
 *===========================================================================*/

#if 0
static int mini_snprintf(char* buf, size_t size, const char* fmt, ...);
#endif

/*===========================================================================
 * Implementation
 *===========================================================================*/

typedef struct {
    char        digits[32]; // Long enough for any decimal numbers (64-bit)
    size_t      size;
} decimal_t;

static inline void decimal_init(decimal_t* dec) {
    dec->size = 0;
    memset(dec->digits, 0, 32);
}

static inline char* decimal_str(decimal_t* dec) {
    return & dec->digits[32 - dec->size];
}

static inline void decimal_prepand(decimal_t* dec, char c) {
    dec->digits[31 - (dec->size++)] = c;
}

static inline char __least_significant_digit(unsigned long u) {
    return (char)('0' + (u % 10));
}

static void decimal_from_ulong(decimal_t* dec, unsigned long u) {
    decimal_init(dec);
    if (u == 0) {
        decimal_prepand(dec, '0');
        return;
    }
    while (u) {
        char digit = __least_significant_digit(u);
        decimal_prepand(dec, digit);
        u /= 10UL;
    };
}

static inline unsigned long __abs(long i) {
    unsigned long u;
    if (i >= 0) {
        u = (unsigned long)i;
    }
    else {
        u = (unsigned long)(-(i + 1L)) + 1UL; // Be careful with LONG_MIN
    }
    return u;
}

static void decimal_from_long(decimal_t* dec, long i) {
    unsigned long u = __abs(i);
    decimal_from_ulong(dec, u);
    if (i < 0) decimal_prepand(dec, '-');
}

// A formated string consists of one or more directives, each directive is
// either an ordinary string or a conversion specifiation.
typedef enum {
    DT_ORDINARY,
    DT_STRING,
    DT_INT_32,
    DT_INT_64,
    DT_UINT_32,
    DT_UINT_64,
    DT_ESCAPED,
    DT_UNEXPECTED
} directive_type_t;

typedef struct {
    directive_type_t  type;
    size_t            size;
} directive_t;

static int next_directive(const char* fmt, directive_t* directive) {
    int is_long = 0;
    int is_signed = 0;

    // No more directives if EOF
    if (fmt[0] == '\0') return 0;

    // Ordinary
    if (fmt[0] != '%') {
        directive->type = DT_ORDINARY;
        directive->size = 1;
        while (fmt[directive->size] != '\0' && fmt[directive->size] != '%')
            directive->size++;
        return 1;
    }

    // Format string ends prematurely
    if (fmt[1] == '\0') goto on_error;

    // String
    if (fmt[1] == 's') {
        directive->type = DT_STRING;
        directive->size = 2;
        return 1;
    }
    // Escaped
    else if (fmt[1] == '%') {
        directive->type = DT_ESCAPED;
        directive->size = 2;
        return 1;
    }

    // Handle integers
    if (fmt[1] == 'l') {
        is_long = 1;
    }
    switch (fmt[1 + is_long]) {
    case 'd':
        is_signed = 1;
        break;
    case 'u': //is_signed = 0;
        break;
    default:
        // Format string ends prematurely or unknown/unsupported format
        // specifier
        goto on_error;
    }
    directive->size = is_long ? 3 : 2;
    directive->type = is_long ?
                        (is_signed ?
                            DT_INT_64 :  /* %ld */
                            DT_UINT_64   /* %lu */
                        ) :
                        (is_signed ?
                            DT_INT_32 :  /* %d */
                            DT_UINT_32   /* %u */
                        );
    return 1;
on_error:
    directive->type = DT_UNEXPECTED;
    return 1;
}

#define NULL_STRING                 "(null)"

static inline int __vsnprintf(char* buf, size_t size, const char* fmt, va_list ag) {
    size_t total_len = 0;

    char* str;
    size_t len;

    decimal_t dec;
    long i64;
    unsigned long u64;

    char* buf_pos = buf;
    size_t buf_remain = size - 1; // save one last byte for null
    char* fmt_pos = (char*) fmt;
    directive_t directive;
    while (next_directive(fmt_pos, &directive)) {
        switch(directive.type) {
        case DT_ORDINARY: // a sequence of ordinary chars (not %)
            str = fmt_pos;
            len = directive.size;
            break;
        case DT_STRING: // %s
            str = (char*) va_arg(ag, const char*);
            if (str == NULL) str = (char*) NULL_STRING;
            len = strlen(str);
            break;
        case DT_ESCAPED: // %%
            str = (char*) "%";
            len = 1;
            break;
        case DT_INT_32: // %d
            i64 = va_arg(ag, int);
            goto signed_int;
        case DT_INT_64: // %ld
            i64 = va_arg(ag, long);
signed_int:
            decimal_from_long(&dec, i64);
            str = decimal_str(&dec);
            len = dec.size;
            break;
        case DT_UINT_32: // %u
            u64 = va_arg(ag, unsigned int);
            goto unsigned_int;
        case DT_UINT_64: // %lu
            u64 = va_arg(ag, unsigned long);
unsigned_int:
            decimal_from_ulong(&dec, u64);
            str = decimal_str(&dec);
            len = dec.size;
            break;
        default: // unexpected
            goto on_error;
        }

        total_len += len;

        len = MIN(len, buf_remain);
        memcpy(buf_pos, str, len);

        buf_pos += len;
        buf_remain -= len;
        fmt_pos += directive.size;
    }

    // Always null-terminated
    *buf_pos = '\0';
    return (int) total_len;
on_error:
    return -1;
}


#if 0
static int mini_snprintf(char* buf, size_t size, const char* fmt, ...) {
    if (size == 0) return 0;

    va_list ag;
    va_start(ag, fmt);
    int ret = mini_vsnprintf(buf, size, fmt, ag);
    va_end(ag);
    return ret;
}
#endif

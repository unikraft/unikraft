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

#ifndef _SL_UTIL_H_
#define _SL_UTIL_H_

#include <sl_types.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)           (sizeof(array) / sizeof(array[0]))
#endif

#ifndef MIN
#define MIN(a, b)                   ((a) <= (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)                   ((a) >= (b) ? (a) : (b))
#endif

#ifndef UNUSED
#define UNUSED(x)                   ((void)(x))
#endif

/*
 * Align integers to a power of two
 *
 * Examples:
 *      ALIGN_DOWN(17, 4) == 16
 *      ALIGN_UP(17, 4)   == 20
 * */
#define ALIGN_DOWN(x, align)        ((x) & ~((align)-1))
#define ALIGN_UP(x, align)          ALIGN_DOWN((x) + ((align)-1), (align))

#define OFFSET_OF(type, member)     ((size_t) &((type*)0)->member)
#define CONTAINER_OF(ptr, type, member) (type*)((char*)ptr - OFFSET_OF(type, member))

__BEGIN_DECLS
__END_DECLS

#endif /* _SL_UTIL_H_ */

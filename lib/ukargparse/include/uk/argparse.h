/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UK_ARGPARSE__
#define __UK_ARGPARSE__

#include <uk/arch/types.h>
#include <uk/arch/limits.h>

#ifdef __cplusplus
extern "C" {
#endif

int uk_argnparse(char *argb, __sz maxlen, char *argv[], int maxcount);

#define uk_argparse(argb, argv, maxcount) \
	uk_argnparse(argb, __SZ_MAX, argv, maxcount)

#ifdef __cplusplus
}
#endif

/**
 * Argument extraction function that can be used to iterate over a C-string
 * buffer ('\0'-terminated) that contains multiple C-string sub-arguments
 * separated by a special character. One example is a colon-separated argument
 * list. On each invocation, the next argument is returned.
 * NOTE: This function modifies the input buffer by replacing a found separator
 *       in the buffer with a terminating '\0'-character.
 *
 * @param argptr Reference to a pointer that points to the current parsing
 *               offset within the C-string buffer (in- & output parameter,
 *               required).
 *               The parameter needs to be handed over on subsequent
 *               calls. Before the first call, the pointer has to be initialized
 *               with the C-string buffer address that shall be parsed.
 *               The function modifies the pointer value (`*argptr`).
 *               If `*argptr` points to `__NULL` after a function call, the end
 *               of the C-string buffer was reached.
 *               If `*argptr` points to `__NULL` for subsequent calls, the
 *               function will return `__NULL` and `*argptr` keeps pointing to
 *               `__NULL`.
 * @param separator Separating character (examples: ':', ',')
 * @return Reference to sub-C-string containing the argument (zero-copy,
 *         sub-C-string buffer is part of the original buffer). On the last
 *         argument of a buffer, `*argptr` will point to `__NULL`. Any
 *         subsequent call, the function returns also `__NULL`.
 *         Empty arguments consist of a '\0'-character only.
 */
char *uk_nextarg(char **argptr, int separator);

/**
 * Variant of `uk_nextarg()` that works for const buffers and is thread-safe.
 * Instead of a reference to the C-string argument, the sizes of the currently
 * found argument is returned. Such an argument starts with the reference value
 * of `*argptr`, before the function is called.
 *
 * @param argptr Reference to a pointer pointing to the current parsing offset
 *               within C-string buffer (input and output parameter, required).
 *               The parameter needs to be handed over on subsequent
 *               calls. Before the first call, the pointer has to be initialized
 *               with the C-string buffer address that shall be parsed.
 *               The function modifies the pointer value (`*argptr`).
 *               If `*argptr` points to `__NULL` after a function call, the end
 *               of the C-string buffer was reached.
 *               If `*argptr` points to `__NULL` for subsequent calls, the
 *               function will return `__NULL` and `*argptr` keeps pointing to
 *               `__NULL`.
 * @param separator Separating character (examples: ':', ',')
 * @return Length of a sub-C-string containing a argument. If it is `0`, an
 *         argument was empty or the end of the buffer to parse was already
 *         reached by the previous function call. This case can be distinguished
 *         with checking `*argptr` after a call. If `*argptr` does not point to
 *         `__NULL` or points to `__NULL` while it was != `__NULL` when calling
 *         the function, an argument was just empty. Otherwise, there are no
 *         more arguments.
 */
__sz uk_nextarg_r(const char **argptr, int separator);

#endif /* __UK_ARGPARSE__ */

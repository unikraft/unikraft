/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stddef.h>
#include <uk/essentials.h>

/* TODO: Implement a circular buffer. Currently, if the buffer is
 * full, tracing disables itself.
 */
#define UK_TRACE_BUFFER_SIZE 16384
char uk_trace_buffer[UK_TRACE_BUFFER_SIZE];

size_t uk_trace_buffer_free = UK_TRACE_BUFFER_SIZE;
char *uk_trace_buffer_writep = uk_trace_buffer;


/* Store a string in format "key = value" in the section
 * .uk_trace_keyvals. This can be anything what you want trace.py
 * script to know about, and what you do not want to consume any space
 * in runtime memory.
 *
 * This section will be stripped at the final states of building, but
 * the key-value data can be easily extracted by:
 *
 *     $ readelf -p .uk_trace_keyvals <your_image.gdb>
 */
#define TRACE_DEFINE_KEY(key, val)			\
	__attribute((__section__(".uk_trace_keyvals")))	\
	static const char key[] __used =		\
		#key " = " #val

TRACE_DEFINE_KEY(format_version, 1);

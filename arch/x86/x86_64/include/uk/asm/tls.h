/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKARCH_TLS_H__
#error Do not include this header directly
#endif

#include <uk/arch/types.h>
#include <string.h>

extern char _tls_start[], _etdata[], _tls_end[];

static inline __sz ukarch_tls_area_size(void)
{
	/* x86_64 ABI requires that fs:%0 contains the address of itself, to
	 * allow certain optimizations. Hence, the overall size of the size of
	 * the TLS area, plus 8 bytes.
	 */
	return _tls_end - _tls_start + 8;
}

static inline __sz ukarch_tls_area_align(void)
{
	/* The minimal required TLS area alignment should ideally come from the
	 * ELF header, but 32 should be fine for all use X86_64 instructions.
	 */
	return 32;
}

static inline void ukarch_tls_area_copy(void *tls_area)
{
	__sz tls_len = _tls_end - _tls_start;
	__sz tls_data_len = _etdata - _tls_start;
	__sz tls_bss_len = _tls_end - _etdata;

	memcpy(tls_area, _tls_start, tls_data_len);
	memset(tls_area + tls_data_len, 0, tls_bss_len);
	/* x86_64 ABI requires that fs:%0 contains the address of itself. */
	*((__uptr *)(tls_area + tls_len)) = (__uptr)(tls_area + tls_len);
}

static inline __uptr ukarch_tls_pointer(void *tls_area)
{
	return (__uptr) tls_area + ((__uptr) _tls_end - (__uptr) _tls_start);
}

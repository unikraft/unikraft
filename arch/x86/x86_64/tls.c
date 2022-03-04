/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Cyril Soldani <cyril.soldani@uliege.be>
 *          Simon Kuenzer <simon@unikraft.io>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, University of Liège. All rights reserved.
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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
#ifndef CONFIG_LIBCONTEXT_CLEAR_TBSS
#define CONFIG_LIBCONTEXT_CLEAR_TBSS 1
#endif /* CONFIG_LIBCONTEXT_CLEAR_TBSS */

#include <string.h>
#include <uk/print.h>
#include <uk/hexdump.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>

extern char _tls_start[], _etdata[], _tls_end[];

/*
 * This file implements a static exec TLS layout
 * (variant 2 with one static TLS block).
 *
 * aligned --> +-------------------------+ \
 * allocation  |                         | |
 *             | .tdata                  | |
 *             |                         | |
 *             +-------------------------+ |
 *             |                         | |
 *             | .tbss                   |  > tls_area
 *             |                         | |
 *             +-------------------------+ |
 *             | / PADDING / / / / / / / | |
 *             +-------------------------+ |
 * tlsp -------+-> self pointer (void *) | |
 *             +-------------------------+ /
 *
 * Partly derived from
 *  https://wiki.osdev.org/Thread_Local_Storage
 *  (Feb 2022)
 */

__sz ukarch_tls_area_align(void)
{
	/* NOTE: The minimal required TLS area alignment should be ideally read
	 *       from the ELF program header. Unfortunately, we can't read it
	 *       because we do not have them and the auxiliary vector populated
	 *       when booting as guest. 32 should be safe enough to take, since
	 *       it should cover all X86_64 instruction requirements.
	 * NOTE: This alignment must be bigger or equal to 8 bytes
	 *       (sizeof(void *)).
	 */
	return 0x20;
}

__sz ukarch_tls_area_size(void)
{
	/* NOTE: X86_64 ABI requires that fs:%0 contains the address of itself,
	 *       to allow certain optimizations. Hence, the overall size of an
	 *       TLS allocation is the aligned up TLS area plus 8 bytes for this
	 *       self-pointer.
	 */
	__sz tls_len =  ALIGN_UP((__uptr) _tls_end - (__uptr) _tls_start,
				   sizeof(void *));
	return tls_len + sizeof(void *);
}

__uptr ukarch_tls_tlsp(void *tls_area)
{
	UK_ASSERT(IS_ALIGNED((__uptr) tls_area, ukarch_tls_area_align()));

	return (__uptr) tls_area + ukarch_tls_area_size() - sizeof(void *);
}

void *ukarch_tls_area_get(__uptr tlsp)
{
	__uptr tls_area;

	tls_area = tlsp - (ukarch_tls_area_size() - sizeof(void *));

	UK_ASSERT(IS_ALIGNED((__uptr) tls_area, ukarch_tls_area_align()));

	return (void *) tls_area;
}

void ukarch_tls_area_init(void *tls_area)
{
	const __sz tdata_len = (__uptr) _etdata  - (__uptr) _tls_start;
	const __sz tbss_len  = (__uptr) _tls_end - (__uptr) _etdata;
	const __sz padding   = ukarch_tls_area_size() - (tbss_len + tdata_len
							 + sizeof(void *));

	UK_ASSERT(IS_ALIGNED((__uptr) tls_area, ukarch_tls_area_align()));

	uk_pr_debug("tls_area_init: target: %p (%"__PRIsz" bytes)\n",
		    tls_area, ukarch_tls_area_size());
	uk_pr_debug("tls_area_init: copy (.tdata): %"__PRIsz" bytes\n",
		    tdata_len);
	uk_pr_debug("tls_area_init: uninitialized (.tbss): %"__PRIsz" bytes\n",
		    (__sz) tbss_len);
	uk_pr_debug("tls_area_init: pad: %"__PRIsz" bytes\n",
		    padding);
	uk_pr_debug("tls_area_init: self ptr: %p\n",
		    (void *) ukarch_tls_pointer(tls_area));

	/* .tdata */
	memcpy((void *)((__uptr) tls_area),
	       _tls_start, tdata_len);

#if CONFIG_LIBCONTEXT_CLEAR_TBSS
	/* clear .tbss and padding */
	memset((void *)((__uptr) tls_area + tdata_len),
	       0x0, tbss_len + padding);
#endif /* CONFIG_LIBCONTEXT_CLEAR_TBSS */

	/* x86_64 ABI requires that fs:%0 contains the address of itself. */
	*((__uptr *) ukarch_tls_tlsp(tls_area))
		= (__uptr) ukarch_tls_tlsp(tls_area);

	uk_hexdumpCd(tls_area, ukarch_tls_area_size());
}

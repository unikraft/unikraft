/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKARCH_TLS_H__
#define __UKARCH_TLS_H__

#include <uk/arch/types.h>

/*
 * As default, no or only a minimum TCB is reserved with these TLS allocation. A
 * library that is making use of TCBs (typically libCs) can enable reserving
 * space for a TCB via its `Makefile.uk` by calling `ukarch_tls_tcb_reserve`:
 *
 *   $(eval $(call ukarch_tls_tcb_reserve,<tcb_size_in_bytes>))
 *
 * In such a case, `libcontext` expects that the function
 * `ukarch_tls_tcb_init()` is provided. This function is invoked by
 * `uk_thread_tls_init()` for initializing the TCB.
 */

/**
 * Returns the alignment requirement for an allocation
 * to be used as TLS
 *
 * @return
 *  Alignment in bytes
 */
__sz ukarch_tls_area_align(void);

/**
 * Returns the required size for an allocation to be used
 * as TLS
 *
 * @return
 *  TLS area size in bytes
 */
__sz ukarch_tls_area_size(void);

/**
 * Returns the configured size of the TCB within a TLS area
 */
__sz ukarch_tls_tcb_size(void);

/**
 * Returns the TLS pointer (tlsp) that is used to activate
 * the TLS memory `tls_area` with `ukplat_tlsp_set()`
 *
 * @param tls_area
 *  TLS area to activate
 * @return
 *  TLS pointer that can be used with `ukplat_tlsp_set()`
 */
__uptr ukarch_tls_tlsp(void *tls_area);

/**
 * Derive the TLS area from a given TLS architecture pointer
 */
void *ukarch_tls_area_get(__uptr tlsp);

/**
 * Returns a pointer to the TCB from a given TLS architecture pointer
 */
void *ukarch_tls_tcb_get(__uptr tlsp);

/**
 * Returns the reserved size for a TCB as part of the TLS
 * allocation
 *
 * NOTE: A minimal TCB may contain a mandatory self-pointer on
 *       some architectures (e.g., x86). If no custom TCB is
 *       configured, `ukarch_tls_tcb_size()` returns the size
 *       of a pointer in such a case.
 *
 * @return
 *  TLS area size in bytes
 */
__sz ukarch_tls_tcb_size(void);

/**
 * Initializes/resets a memory area for TLS use based
 * on the TLS template.
 *
 * @param tls_area
 *  TLS area to initialize
 */
void ukarch_tls_area_init(void *tls_area);

#if CONFIG_UKARCH_TLS_HAVE_TCB
/**
 * Prototype for TCB initialization
 *
 * When `CONFIG_UKARCH_TLS_HAVE_TCB` is defined, the symbol  `ukarch_tcb_init()`
 * must be provided by a library (e.g., libC). The function is called by
 * `ukarch_tls_area_init()` for initializing the corresponding TCB area.
 *
 * NOTE: Please note, if your target architecture uses a self pointer in
 *       the TCB, this is the only field that is initialized. The rest of the
 *       the TCB area is not zero'd out. It is intended that
 *       `ukarch_tls_tcb_init()` is handling the resetting of the rest of TCB
 *       fields.
 *
 * @param tcbp
 *  Pointer to TCB to initialize
 *
 */
void ukarch_tls_tcb_init(void *tcbp);
#endif /* CONFIG_UKARCH_TLS_HAVE_TCB */
#endif /* __UKARCH_TLS_H__ */

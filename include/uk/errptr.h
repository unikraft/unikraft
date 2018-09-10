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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_ERRPTR_H__
#define __UK_ERRPTR_H__

#include <uk/arch/types.h>

#ifndef MAXERRNO
#define MAXERRNO (512)
#endif

/**
 * Checks if ptr is invalid and contains an error number
 * @param ptr Pointer
 * @return 0 if pointer is valid, non-0 if pointer is invalid
 */
#ifndef PTRISERR
#define PTRISERR(ptr)					\
	((__sptr)(ptr) <= 0				\
	 && (__sptr)(ptr) >= -(__sptr)(MAXERRNO))
#endif

/**
 * Converts an invalid pointer (see PTRISERR) to an error number
 * @param ptr Pointer
 * @return error number
 */
#ifndef PTR2ERR
#define PTR2ERR(ptr)					\
	((int) ((__sptr)(ptr)))
#endif

/**
 * Creates an invalid pointer containing an error number (see PTRISERR)
 * @param err Error number
 * @return Pointer
 */
#ifndef ERR2PTR
#define ERR2PTR(err)					\
	((void *) ((__sptr)(err)))
#endif

#endif /* __UK_ERRPTR_H__ */

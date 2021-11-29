/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@upb.ro>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
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
 */

#include <bindings_helper.h>
#include <uk/essentials.h>
#include <uk/arch/types.h>

#if CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#elif CONFIG_HAVE_LIBC
#include <stdlib.h>
#else /* !CONFIG_LIBUKALLOC && !CONFIG_HAVE_LIBC */
#include <uk/arch/types.h>
#endif

void *__ukrust_sys_malloc(__sz size)
{
#if CONFIG_LIBUKALLOC
	return uk_malloc(uk_alloc_get_default(), size);
#elif CONFIG_HAVE_LIBC
	return malloc(size);
#else /* !CONFIG_LIBUKALLOC && !CONFIG_HAVE_LIBC */
	return __NULL;
#endif
}

void __ukrust_sys_free(void *ptr)
{
#if CONFIG_LIBUKALLOC
	uk_free(uk_alloc_get_default(), ptr);
#elif CONFIG_HAVE_LIBC
	free(ptr);
#else /* !CONFIG_LIBUKALLOC && !CONFIG_HAVE_LIBC */
	/* __rust_dealloc does not return */
#endif
}

void *__ukrust_sys_realloc(void *ptr, __sz size)
{
#if CONFIG_LIBUKALLOC
	return uk_realloc(uk_alloc_get_default(), ptr, size);
#elif CONFIG_HAVE_LIBC
	return realloc(ptr, size);
#else /* !CONFIG_LIBUKALLOC && !CONFIG_HAVE_LIBC */
	return __NULL;
#endif
}

void *__ukrust_sys_calloc(int num, __sz size)
{
#if CONFIG_LIBUKALLOC
	return uk_calloc(uk_alloc_get_default(), num, size);
#elif CONFIG_HAVE_LIBC
	return calloc(num, size);
#else /* !CONFIG_LIBUKALLOC && !CONFIG_HAVE_LIBC */
	return __NULL;
#endif
}

void __noreturn __ukrust_sys_crash(void)
{
	UK_CRASH("Rust panic handler");
}

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
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

#include <uk/store.h>
#include <uk/alloc_impl.h>

#if CONFIG_LIBUKALLOC_IFSTATS_GLOBAL
struct uk_alloc_stats _uk_alloc_stats_global = { 0 };
#endif

void uk_alloc_stats_get(struct uk_alloc *a,
			struct uk_alloc_stats *dst)
{
	UK_ASSERT(a);
	UK_ASSERT(dst);

	uk_preempt_disable();
	memcpy(dst, &a->_stats, sizeof(*dst));
	uk_preempt_enable();
}

static int get_cur_mem_free(void *cookie __unused, __u64 *out)
{
	*out = (__u64) uk_alloc_availmem_total();
	return 0;
}
UK_STORE_STATIC_ENTRY(cur_mem_free, u64, get_cur_mem_free, NULL, NULL);

#if CONFIG_LIBUKALLOC_IFSTATS_GLOBAL
void uk_alloc_stats_get_global(struct uk_alloc_stats *dst)
{
	UK_ASSERT(dst);

	uk_preempt_disable();
	memcpy(dst, &_uk_alloc_stats_global, sizeof(*dst));
	uk_preempt_enable();
}

static int get_last_alloc_size(void *cookie __unused, __u64 *out)
{
	*out = (__u64) _uk_alloc_stats_global.last_alloc_size;
	return 0;
}
UK_STORE_STATIC_ENTRY(last_alloc_size, u64, get_last_alloc_size, NULL, NULL);

static int get_max_alloc_size(void *cookie __unused, __u64 *out)
{
	*out = (__u64) _uk_alloc_stats_global.max_alloc_size;
	return 0;
}
UK_STORE_STATIC_ENTRY(max_alloc_size, u64, get_max_alloc_size, NULL, NULL);

static int get_min_alloc_size(void *cookie __unused, __u64 *out)
{
	*out = (__u64) _uk_alloc_stats_global.min_alloc_size;
	return 0;
}
UK_STORE_STATIC_ENTRY(min_alloc_size, u64, get_min_alloc_size, NULL, NULL);

static int get_tot_nb_allocs(void *cookie __unused, __u64 *out)
{
	*out = (__u64) _uk_alloc_stats_global.tot_nb_allocs;
	return 0;
}
UK_STORE_STATIC_ENTRY(tot_nb_allocs, u64, get_tot_nb_allocs, NULL, NULL);

static int get_tot_nb_frees(void *cookie __unused, __u64 *out)
{
	*out = (__u64) _uk_alloc_stats_global.tot_nb_frees;
	return 0;
}
UK_STORE_STATIC_ENTRY(tot_nb_frees, u64, get_tot_nb_frees, NULL, NULL);

static int get_cur_nb_allocs(void *cookie __unused, __s64 *out)
{
	*out = (__s64) _uk_alloc_stats_global.cur_nb_allocs;
	return 0;
}
UK_STORE_STATIC_ENTRY(cur_nb_allocs, s64, get_cur_nb_allocs, NULL, NULL);

static int get_max_nb_allocs(void *cookie __unused, __s64 *out)
{
	*out = (__s64) _uk_alloc_stats_global.max_nb_allocs;
	return 0;
}
UK_STORE_STATIC_ENTRY(max_nb_allocs, s64, get_max_nb_allocs, NULL, NULL);

static int get_cur_mem_use(void *cookie __unused, __s64 *out)
{
	*out = (__s64) _uk_alloc_stats_global.cur_mem_use;
	return 0;
}
UK_STORE_STATIC_ENTRY(cur_mem_use, s64, get_cur_mem_use, NULL, NULL);

static int get_max_mem_use(void *cookie __unused, __s64 *out)
{
	*out = (__s64) _uk_alloc_stats_global.max_mem_use;
	return 0;
}
UK_STORE_STATIC_ENTRY(max_mem_use, s64, get_max_mem_use, NULL, NULL);

static int get_nb_enomem(void *cookie __unused, __u64 *out)
{
	*out = (__u64) _uk_alloc_stats_global.nb_enomem;
	return 0;
}
UK_STORE_STATIC_ENTRY(nb_enomem, u64, get_nb_enomem, NULL, NULL);

#endif

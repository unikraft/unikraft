/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR SOCKETINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/list.h>
#include <uk/socket_driver.h>
#include <sys/types.h>

/**
 * List of socket family drivers
 */
UK_TAILQ_HEAD(posix_socket_driver_list, struct posix_socket_driver);
struct posix_socket_driver_list posix_socket_families =
		UK_TAILQ_HEAD_INITIALIZER(posix_socket_families);
static uint16_t uk_posix_socket_family_count;

unsigned int
posix_socket_family_count(void)
{
	return uk_posix_socket_family_count;
}

void _posix_socket_family_register(struct posix_socket_driver *d,
		int fam,
		struct posix_socket_ops *ops,
		struct uk_alloc *alloc)
{
	UK_ASSERT(d);
	UK_ASSERT(ops);

	if (unlikely(fam == 0)) {
		uk_pr_crit("Cannot register unspecified socket family: %p\n", ops);
		return;
	}

	uk_pr_debug("Registering socket handler: library: %s, family: %d: %p...\n", d->libname, fam, ops);

	struct posix_socket_driver *e;
	UK_TAILQ_FOREACH(e, &posix_socket_families, _list) {
		if (e->af_family == fam) {
			uk_pr_warn("Socket family (%d) already registerd to library %s!", fam, e->libname);
			return;
		}
	}

	new_posix_socket_family(d, fam, alloc, ops);
	UK_TAILQ_INSERT_TAIL(&posix_socket_families, d, _list);
	uk_posix_socket_family_count++;
}

void
_posix_socket_family_unregister(struct posix_socket_driver *d)
{
	UK_ASSERT(d);
	UK_ASSERT(uk_posix_socket_family_count > 0);

	uk_pr_debug("Unregistering socket handler: %d\n", d->af_family);

	UK_TAILQ_REMOVE(&posix_socket_families, d, _list);
	uk_posix_socket_family_count--;
}

static int
posix_socket_family_init(struct posix_socket_driver *d)
{
	UK_ASSERT(d);
	UK_ASSERT(d->ops);

	uk_pr_debug("Initializing socket handler: %d\n", d->af_family);

	/* Assign the default allocator if not specified */
	if (d->allocator == NULL) {
		d->allocator = uk_alloc_get_default();
		uk_pr_debug("Using default memory allocator for socket family: %d: %p\n", d->af_family, d->allocator);
	}

	if (!d->ops->init)
		return 0;

	return d->ops->init(d);
}

/* Returns the number of successfully initialized device sockets */
static int
posix_socket_family_lib_init(void)
{
	uk_pr_info("Initializing socket handlers...\n");

	struct posix_socket_driver *d;
	unsigned int ret = 0;

	if (posix_socket_family_count() == 0)
		return 0;

	UK_TAILQ_FOREACH(d, &posix_socket_families, _list) {
		if (posix_socket_family_init(d) >= 0)
			++ret;
	}

	return ret;
}

struct posix_socket_driver *
posix_socket_driver_get(int af_family)
{
	struct posix_socket_driver *d;

	UK_TAILQ_FOREACH(d, &posix_socket_families, _list) {
		if (d->af_family == af_family)
			return d;
	}

	return NULL;
}

struct posix_socket_driver *
posix_socket_get_family(int sock)
{
	return NULL;
}

uk_initcall_class_prio(posix_socket_family_lib_init, POSIX_SOCKET_FAMILY_INIT_CLASS, POSIX_SOCKET_FAMILY_INIT_PRIO);

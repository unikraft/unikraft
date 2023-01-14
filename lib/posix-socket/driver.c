/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT).
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
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR SOCKETINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/init.h>
#include <uk/socket_driver.h>

/** List of socket family drivers */
extern struct posix_socket_driver posix_socket_driver_list_start[];
extern struct posix_socket_driver posix_socket_driver_list_end[];

#define POSIX_SOCKET_DRIVER_COUNT \
	(posix_socket_driver_list_end - posix_socket_driver_list_start)

unsigned int
posix_socket_family_count(void)
{
	return POSIX_SOCKET_DRIVER_COUNT;
}

struct posix_socket_driver *
posix_socket_driver_get(int family)
{
	struct posix_socket_driver *d = posix_socket_driver_list_start;

	while (d != posix_socket_driver_list_end) {
		if (d->family == family)
			return d;

		d++;
	}

	return NULL;
}

static int
posix_socket_family_init(struct posix_socket_driver *d)
{
	int rc = 0;

	UK_ASSERT(d);
	UK_ASSERT(d->ops);

	uk_pr_debug("Installing socket family: %d library: %s (%p)\n",
		    d->family, d->libname, d->ops);

	if (d->ops->init)
		rc = d->ops->init(d);

	/* Assign the default allocator if the driver did not already set one */
	if (d->allocator == NULL)
		d->allocator = uk_alloc_get_default();

	return rc;
}

static int
posix_socket_family_lib_init(void)
{
	struct posix_socket_driver *d = posix_socket_driver_list_start;
	unsigned int ret = 0;

	while (d != posix_socket_driver_list_end) {
		/* Ensure that there are not two drivers that want to register
		 * the same address family. Since this would violate the API,
		 * we do this check in debug builds only. As the getter returns
		 * the first driver handling the family, it fails for a driver
		 * that wants to register the family again.
		 */
		UK_ASSERT(posix_socket_driver_get(d->family) == d);

		if (posix_socket_family_init(d) >= 0)
			++ret;

		d++;
	}

	return ret;
}

#define POSIX_SOCKET_FAMILY_INIT_CLASS UK_INIT_CLASS_EARLY
#define POSIX_SOCKET_FAMILY_INIT_PRIO 0
#define POSIX_SOCKET_FAMILY_REGISTER_PRIO 2

uk_initcall_class_prio(posix_socket_family_lib_init,
		       POSIX_SOCKET_FAMILY_INIT_CLASS,
		       POSIX_SOCKET_FAMILY_INIT_PRIO);

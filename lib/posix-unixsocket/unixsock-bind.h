/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_UNIXSOCK_BIND_H__
#define __UK_POSIX_UNIXSOCK_BIND_H__

#include <uk/socket_driver.h>

struct unix_addr_entry {
	struct unix_addr_entry *next;
	const char *name;
	size_t namelen;
	posix_sock *sock;
};

#define UNIX_ADDR_ENTRY(n, l, s) ((struct unix_addr_entry) { \
	.next = NULL, \
	.name = (n), \
	.namelen = (l), \
	.sock = (s) \
})

int unix_addr_bind(struct unix_addr_entry *ent);

posix_sock *unix_addr_lookup(const char *name, size_t namelen);

int unix_addr_release(const char *name, size_t namelen);

#endif /* __UK_POSIX_UNIXSOCK_BIND_H__ */

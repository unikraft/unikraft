/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Andrei Tatar <andrei@unikraft.io>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
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

#ifndef __UK_SOCKET__
#define __UK_SOCKET__

#include <sys/socket.h>

struct posix_socket_driver;

struct posix_socket_node {
	/** The fd or data used internally by the socket implementation */
	void *sock_data;
	/** The driver to use for this socket */
	struct posix_socket_driver *driver;
};

#ifdef CONFIG_LIBPOSIX_SOCKET_PRINT_ERRORS
#include <uk/print.h>
#define PSOCKET_ERR(msg, ...) uk_pr_err(msg, ##__VA_ARGS__)
#else /* CONFIG_LIBPOSIX_SOCKET_PRINT_ERRORS */
#define PSOCKET_ERR(msg, ...) do { } while (0)
#endif /* !CONFIG_LIBPOSIX_SOCKET_PRINT_ERRORS */

#include <uk/socket_driver.h>

struct uk_file *uk_socket_create(int family, int type, int protocol);

int uk_socketpair_create(int family, int type, int protocol,
			 const struct uk_file *sv[2]);

const struct uk_file *uk_socket_accept(const struct uk_file *sock, int blocking,
				       struct sockaddr *addr,
				       socklen_t *addr_len, int flags);

int uk_sys_socket(int family, int type, int protocol);

int uk_sys_socketpair(int family, int type, int protocol, int sv[2]);

int uk_sys_accept(const struct uk_file *sock, int blocking,
		  struct sockaddr *addr, socklen_t *addr_len, int flags);

#endif /* __UK_SOCKET__ */

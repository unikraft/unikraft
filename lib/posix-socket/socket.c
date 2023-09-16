/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <uk/socket.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/trace.h>
#include <uk/syscall.h>
#include <errno.h>

UK_TRACEPOINT(trace_posix_socket_create, "%d %d %d", int, int, int);
UK_TRACEPOINT(trace_posix_socket_create_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_create_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, socket, int, family, int, type, int, protocol)
{
	struct posix_socket_driver *d;
	struct posix_socket_file dummy;
	void *sock;
	int vfs_fd;
	int ret;

	trace_posix_socket_create(family, type, protocol);

	d = posix_socket_driver_get(family);
	if (unlikely(d == NULL)) {
		ret = -EAFNOSUPPORT;
		goto EXIT_ERR;
	}

	/* Create the socket using the driver */
	sock = posix_socket_create(d, family, type, protocol);
	if (unlikely(PTRISERR(sock))) {
		ret = PTR2ERR(sock);
		goto EXIT_ERR;
	}

	/* Allocate the file descriptor */
	vfs_fd = posix_socket_alloc_fd(d, type, sock);
	if (unlikely(vfs_fd < 0)) {
		ret = vfs_fd;
		goto EXIT_ERR_CLOSE;
	}

	ret = vfs_fd;

	trace_posix_socket_create_ret(ret);
	return ret;
EXIT_ERR_CLOSE:
	dummy = (struct posix_socket_file){
		.sock_data = sock,
		.vfs_file = NULL,
		.driver = d,
		.type = VSOCK,
	};
	posix_socket_close(&dummy);
EXIT_ERR:
	PSOCKET_ERR("socket failed for family %d, type: %d, protocol: %d: %d\n",
		    family, type, protocol, ret);
	trace_posix_socket_create_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_accept, "%d %p %p", int,
		struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_accept_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_accept_err, "%d", int);

int do_accept4(int sock, struct sockaddr *addr, socklen_t *addr_len,
	       int flags)
{
	struct posix_socket_file *file;
	struct posix_socket_file dummy;
	void *new_sock;
	int vfs_fd;
	int ret;

	trace_posix_socket_accept(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Accept an incoming connection */
	new_sock = posix_socket_accept4(file, addr, addr_len, flags);
	if (unlikely(PTRISERR(new_sock))) {
		ret = PTR2ERR(new_sock);
		goto EXIT_ERR_PUT;
	}

	/* Allocate a file descriptor for the accepted connection */
	vfs_fd = posix_socket_alloc_fd(file->driver, file->type, new_sock);
	if (unlikely(vfs_fd < 0)) {
		ret = vfs_fd;
		goto EXIT_ERR_CLOSE;
	}

	ret = vfs_fd;

	vfscore_put_file(file->vfs_file);
	trace_posix_socket_accept_ret(ret);
	return ret;
EXIT_ERR_CLOSE:
	dummy = (struct posix_socket_file){
		.sock_data = new_sock,
		.vfs_file = NULL,
		.driver = file->driver,
		.type = VSOCK,
	};
	posix_socket_close(&dummy);
EXIT_ERR_PUT:
	vfscore_put_file(file->vfs_file);
EXIT_ERR:
	PSOCKET_ERR("accept on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_accept_err(ret);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, accept, int, sock, struct sockaddr *, addr,
		    socklen_t *, addr_len)
{
	return do_accept4(sock, addr, addr_len, 0);
}

UK_SYSCALL_R_DEFINE(int, accept4, int, sock, struct sockaddr *, addr,
		    socklen_t *, addr_len, int, flags)
{
	return do_accept4(sock, addr, addr_len, flags);
}

UK_TRACEPOINT(trace_posix_socket_bind, "%d %p %d",
	      int, const struct sockaddr *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_bind_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_bind_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, bind, int, sock, const struct sockaddr *, addr,
		    socklen_t, addr_len)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_bind(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Bind an incoming connection */
	ret = posix_socket_bind(file, addr, addr_len);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret == -1))
		goto EXIT_ERR;

	trace_posix_socket_bind_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("bind on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_bind_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_shutdown, "%d %d", int, int);
UK_TRACEPOINT(trace_posix_socket_shutdown_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_shutdown_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, shutdown, int, sock, int, how)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_shutdown(sock, how);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Shutdown socket */
	ret = posix_socket_shutdown(file, how);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret < 0))
		goto EXIT_ERR;

	trace_posix_socket_shutdown_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("shutdown on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_shutdown_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getpeername, "%d %p %d", int,
		struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_getpeername_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getpeername_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, getpeername, int, sock,
		    struct sockaddr *restrict, addr,
		    socklen_t *restrict, addr_len)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_getpeername(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Get peer name of socket */
	ret = posix_socket_getpeername(file, addr, addr_len);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret < 0))
		goto EXIT_ERR;

	trace_posix_socket_getpeername_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("getpeername on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_getpeername_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getsockname, "%d %p %p",
	      int, struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_getsockname_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getsockname_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, getsockname, int, sock,
		    struct sockaddr *restrict, addr,
		    socklen_t *restrict, addr_len)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_getsockname(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Get name of socket */
	ret = posix_socket_getsockname(file, addr, addr_len);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret < 0))
		goto EXIT_ERR;

	trace_posix_socket_getsockname_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("getsockname on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_getsockname_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getsockopt, "%d %d %d %p %d",
	      int, int, int, void *, socklen_t *);
UK_TRACEPOINT(trace_posix_socket_getsockopt_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getsockopt_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, getsockopt, int, sock, int, level, int, optname,
		    void *restrict, optval, socklen_t *restrict, optlen)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_getsockopt(sock, level, optname, optval, optlen);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Get socket options */
	ret = posix_socket_getsockopt(file, level, optname, optval, optlen);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret < 0))
		goto EXIT_ERR;

	trace_posix_socket_getsockopt_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("getsockopt on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_getsockopt_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_setsockopt, "%d %d %d %p %d",
	      int, int, int, const void *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_setsockopt_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_setsockopt_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, setsockopt, int, sock, int, level, int, optname,
		    const void *, optval, socklen_t, optlen)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_setsockopt(sock, level, optname, optval, optlen);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Set socket options */
	ret = posix_socket_setsockopt(file, level, optname, optval, optlen);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret < 0))
		goto EXIT_ERR;

	trace_posix_socket_setsockopt_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("setsockopt on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_setsockopt_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_connect, "%d %p %d",
	      int, const struct sockaddr *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_connect_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_connect_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, connect, int, sock, const struct sockaddr *, addr,
		    socklen_t, addr_len)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_connect(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Connect to the socket */
	ret = posix_socket_connect(file, addr, addr_len);

	vfscore_put_file(file->vfs_file);

	if (unlikely((ret < 0) && (ret != -EINPROGRESS)))
		goto EXIT_ERR;

	trace_posix_socket_connect_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("connect on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_connect_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_listen, "%d %d", int, int);
UK_TRACEPOINT(trace_posix_socket_listen_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_listen_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, listen, int, sock, int, backlog)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_listen(sock, backlog);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Listen on the socket */
	ret = posix_socket_listen(file, backlog);

	vfscore_put_file(file->vfs_file);

	if (unlikely(ret < 0))
		goto EXIT_ERR;

	trace_posix_socket_listen_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("listen on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_listen_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_recvfrom, "%d %p %d %d %p %p",
	      int, struct sockaddr *, size_t, int, void *, socklen_t *);
UK_TRACEPOINT(trace_posix_socket_recvfrom_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_recvfrom_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, recvfrom, int, sock, void *, buf, size_t, len,
		    int, flags, struct sockaddr *, from, socklen_t *, fromlen)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_recvfrom(sock, buf, len, flags, from, fromlen);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Receive a buffer from a socket */
	ret = posix_socket_recvfrom(file, buf, len, flags, from, fromlen);

	vfscore_put_file(file->vfs_file);

	if (unlikely((ret < 0) && (ret != -EAGAIN)))
		goto EXIT_ERR;

	trace_posix_socket_recvfrom_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("recvfrom on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_recvfrom_err(ret);
	return ret;
}

#if UK_LIBC_SYSCALLS
/* Provide wrapper (if not provided by Musl) or some other libc. */
ssize_t recv(int sock, void *buf, size_t len, int flags)
{
	return uk_syscall_e_recvfrom(sock, (long) buf, len, flags, (long) NULL, (long) NULL);
}
#endif /* UK_LIBC_SYSCALLS */

UK_TRACEPOINT(trace_posix_socket_recvmsg, "%d %p %d", int, struct msghdr*, int);
UK_TRACEPOINT(trace_posix_socket_recvmsg_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_recvmsg_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, recvmsg, int, sock, struct msghdr *, msg,
		    int, flags)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_recvmsg(sock, msg, flags);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Receive a structured message from a socket */
	ret = posix_socket_recvmsg(file, msg, flags);

	vfscore_put_file(file->vfs_file);

	if (unlikely((ret < 0) && (ret != -EAGAIN)))
		goto EXIT_ERR;

	trace_posix_socket_recvmsg_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("recvmsg on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_recvmsg_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_sendmsg, "%d %p %d", int,
	      const struct msghdr *, int);
UK_TRACEPOINT(trace_posix_socket_sendmsg_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_sendmsg_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, sendmsg, int, sock, const struct msghdr *, msg,
		    int, flags)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_sendmsg(sock, msg, flags);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Send a structured message to a socket */
	ret = posix_socket_sendmsg(file, msg, flags);

	vfscore_put_file(file->vfs_file);

	if (unlikely((ret < 0) && (ret != -EAGAIN)))
		goto EXIT_ERR;

	trace_posix_socket_sendmsg_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("sendmsg on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_sendmsg_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_sendto, "%d %p %d %d %p %d",
	      int, const void *, size_t, int,
	      const struct sockaddr *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_sendto_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_sendto_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, sendto, int, sock, const void *, buf, size_t, len,
		    int, flags, const struct sockaddr *, dest_addr,
		    socklen_t, addrlen)
{
	struct posix_socket_file *file;
	int ret;

	trace_posix_socket_sendto(sock, buf, len, flags, dest_addr, addrlen);

	file = posix_socket_file_get(sock);
	if (unlikely(PTRISERR(file))) {
		ret = PTR2ERR(file);
		goto EXIT_ERR;
	}

	/* Send to an address over a socket */
	ret = posix_socket_sendto(file, buf, len, flags,
		dest_addr, addrlen);

	vfscore_put_file(file->vfs_file);

	if (unlikely((ret < 0) && (ret != -EAGAIN)))
		goto EXIT_ERR;

	trace_posix_socket_sendto_ret(ret);
	return ret;
EXIT_ERR:
	PSOCKET_ERR("sendto on socket %d failed: %d\n", sock, ret);
	trace_posix_socket_sendto_err(ret);
	return ret;
}

#if UK_LIBC_SYSCALLS
/* Provide wrapper (if not provided by Musl) or some other libc. */
ssize_t send(int sock, const void *buf, size_t len, int flags)
{
	return uk_syscall_e_sendto(sock, (long) buf, len, flags, (long) NULL, 0);
}
#endif /* UK_LIBC_SYSCALLS */

UK_TRACEPOINT(trace_posix_socket_socketpair, "%d %d %d %p", int, int, int,
	      int *);
UK_TRACEPOINT(trace_posix_socket_socketpair_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_socketpair_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, socketpair, int, family, int, type, int, protocol,
		    int *, usockfd)
{
	struct posix_socket_driver *d;
	struct posix_socket_file dummy;
	void *usockdata[2];
	int vfs_fd1, vfs_fd2;
	int ret;

	trace_posix_socket_socketpair(family, type, protocol, usockfd);

	d = posix_socket_driver_get(family);
	if (unlikely(d == NULL)) {
		ret = -EAFNOSUPPORT;
		goto EXIT_ERR;
	}

	/* Create the socketpair using the driver */
	ret = posix_socket_socketpair(d, family, type, protocol, usockdata);
	if (unlikely(ret < 0))
		goto EXIT_ERR;

	/* Allocate the file descriptors */
	vfs_fd1 = posix_socket_alloc_fd(d, type, usockdata[0]);
	if (unlikely(vfs_fd1 < 0)) {
		ret = PTR2ERR(vfs_fd1);
		goto EXIT_ERR_CLOSE;
	}

	vfs_fd2 = posix_socket_alloc_fd(d, type, usockdata[1]);
	if (unlikely(vfs_fd2 < 0)) {
		ret = PTR2ERR(vfs_fd2);
		goto EXIT_ERR_FREE_FD1;
	}

	usockfd[0] = vfs_fd1;
	usockfd[1] = vfs_fd2;

	trace_posix_socket_socketpair_ret(ret);
	return ret;
EXIT_ERR_FREE_FD1:
	/* TODO: Broken error handling. Leaking vfs_fd1! */
EXIT_ERR_CLOSE:
	dummy = (struct posix_socket_file){
		.sock_data = usockdata[0],
		.vfs_file = NULL,
		.driver = d,
		.type = VSOCK,
	};
	posix_socket_close(&dummy);
	dummy = (struct posix_socket_file){
		.sock_data = usockdata[1],
		.vfs_file = NULL,
		.driver = d,
		.type = VSOCK,
	};
	posix_socket_close(&dummy);
EXIT_ERR:
	PSOCKET_ERR("socketpair failed: %d\n", ret);
	trace_posix_socket_socketpair_err(ret);
	return ret;
}

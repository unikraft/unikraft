/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
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

#include <sys/types.h>
#include <uk/socket.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/trace.h>
#include <errno.h>

UK_TRACEPOINT(trace_posix_socket_create, "%d %d %d", int, int, int);
UK_TRACEPOINT(trace_posix_socket_create_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_create_err, "%d", int);

int
socket(int family, int type, int protocol)
{
	int ret = 0;
	int vfs_fd = 0xff;
	void *sock = NULL;
	struct posix_socket_driver *d;

	trace_posix_socket_create(family, type, protocol);

	d = posix_socket_driver_get(family);
	if (d == NULL) {
		ret = -1;
		SOCKET_ERR(EAFNOSUPPORT, "no socket implementation for family");
		goto EXIT_ERR;
	}

	/* Create the socket using the driver */
	sock = posix_socket_create(d, family, type, protocol);
	if (sock == NULL) {
		ret = -1;
		SOCKET_ERR(ENOMEM, "failed to create socket");
		goto EXIT_ERR;
	}

	/* Allocate the file descriptor */
	vfs_fd = socket_alloc_fd(d, type, sock);
	if (vfs_fd < 0) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(vfs_fd), "failed to allocate descriptor");
		goto SOCKET_CLEANUP;
	}

	/* Returning the file descriptor to the user */
	ret = vfs_fd;

EXIT:
	trace_posix_socket_create_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_create_err(ret);
	return ret;
SOCKET_CLEANUP:
	posix_socket_close(sock);
	goto EXIT;
}

UK_TRACEPOINT(trace_posix_socket_accept, "%d %p %p", int,
		struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_accept_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_accept_err, "%d", int);

int
accept(int sock, struct sockaddr *restrict addr,
		socklen_t *restrict addr_len)
{
	int vfs_fd;
	int ret = 0;
	void *new_sock;
	struct posix_socket_file *file;

	trace_posix_socket_accept(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT;
	}

	/* Accept an incoming connection */
	new_sock = posix_socket_accept(file, addr, addr_len);
	if (new_sock == NULL) {
		ret = -1;
		uk_pr_err("failed to accept on socket\n");
		goto EXIT_FDROP;
	}

	/* Allocate a file descriptor for the accepted connection of the same type */
	vfs_fd = socket_alloc_fd(file->driver, file->type, new_sock);
	if (vfs_fd < 0) {
		ret = -1;
		SOCKET_ERR(ENOMEM, "failed to allocate descriptor for accepted connection");
		goto SOCKET_CLEANUP;
	}

	ret = vfs_fd;

EXIT_FDROP:
	vfscore_put_file(file->vfs_file); /* release refcount */
	trace_posix_socket_accept_err(ret);
	return ret;
EXIT:
	trace_posix_socket_accept_ret(ret);
	return ret;
SOCKET_CLEANUP:
	posix_socket_close(file);
	goto EXIT_FDROP;
}

UK_TRACEPOINT(trace_posix_socket_bind, "%d %p %d", int, const struct sockaddr *,
		socklen_t);
UK_TRACEPOINT(trace_posix_socket_bind_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_bind_err, "%d", int);

int
bind(int sock, const struct sockaddr *addr, socklen_t addr_len)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_bind(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Bind an incoming connection */
	ret = posix_socket_bind(file, addr, addr_len);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret == -1) {
		uk_pr_err("failed to bind with socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_bind_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_bind_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_shutdown, "%d %d", int, int);
UK_TRACEPOINT(trace_posix_socket_shutdown_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_shutdown_err, "%d", int);

int
shutdown(int sock, int how)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_shutdown(sock, how);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Shutdown socket */
	ret = posix_socket_shutdown(file, how);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to shutdown socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_shutdown_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_shutdown_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getpeername, "%d %p %d", int,
		struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_getpeername_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getpeername_err, "%d", int);

int
getpeername(int sock, struct sockaddr *restrict addr,
		socklen_t *restrict addr_len)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_getpeername(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Get peern name of socket */
	ret = posix_socket_getpeername(file, addr, addr_len);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to getpeername of socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_getpeername_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_getpeername_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getsockname, "%d %p %p", int,
		struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_getsockname_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getsockname_err, "%d", int);

int
getsockname(int sock, struct sockaddr *restrict addr,
		socklen_t *restrict addr_len)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_getsockname(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Get socket name of socket */
	ret = posix_socket_getsockname(file, addr, addr_len);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to getsockname of socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_getsockname_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_getsockname_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getsockopt, "%d %d %d %p %d", int, int, int,
		void *, socklen_t *);
UK_TRACEPOINT(trace_posix_socket_getsockopt_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getsockopt_err, "%d", int);

int getsockopt(int sock, int level, int optname, void *restrict optval,
		socklen_t *restrict optlen)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_getsockopt(sock, level, optname, optval, optlen);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Get socket options */
	ret = posix_socket_getsockopt(file, level, optname,
		optval, optlen);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to getsockopt of socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_getsockopt_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_getsockopt_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_setsockopt, "%d %d %d %p %d", int, int, int,
		const void *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_setsockopt_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_setsockopt_err, "%d", int);

int
setsockopt(int sock, int level, int optname, const void *optval,
		socklen_t optlen)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_setsockopt(sock, level, optname, optval, optlen);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Set socket options */
	ret = posix_socket_setsockopt(file, level, optname,
		optval, optlen);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to getsockopt of socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_setsockopt_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_setsockopt_err(ret);
	return ret;
}

int
getnameinfo(const struct sockaddr *restrict sa, socklen_t sl,
		char *restrict node, socklen_t nodelen, char *restrict serv,
		socklen_t servlen, int flags)
{
	uk_pr_crit("%s: not implemented\n", __func__);
	errno = ENOTSUP;
	return -1;
}

UK_TRACEPOINT(trace_posix_socket_connect, "%d %p %d", int,
		const struct sockaddr *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_connect_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_connect_err, "%d", int);

int
connect(int sock, const struct sockaddr *addr, socklen_t addr_len)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_connect(sock, addr, addr_len);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Connect to the socket */
	ret = posix_socket_connect(file, addr, addr_len);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to connect to socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_connect_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_connect_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_listen, "%d %d", int, int);
UK_TRACEPOINT(trace_posix_socket_listen_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_listen_err, "%d", int);

int
listen(int sock, int backlog)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_listen(sock, backlog);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Listen to the socket */
	ret = posix_socket_listen(file, backlog);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to listen to socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_listen_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_listen_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_recv, "%d %p %d %d", int, void *, size_t, int);
UK_TRACEPOINT(trace_posix_socket_recv_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_recv_err, "%d", int);

ssize_t
recv(int sock, void *buf, size_t len, int flags)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_recv(sock, buf, len, flags);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Recieve a buffer from a socket */
	ret = posix_socket_recv(file, buf, len, flags);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to recv on socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_recv_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_recv_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_recvfrom, "%d %p %d %d %p %p", int,
		struct sockaddr *, size_t, int, void *, socklen_t *);
UK_TRACEPOINT(trace_posix_socket_recvfrom_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_recvfrom_err, "%d", int);

ssize_t
recvfrom(int sock, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromlen)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_recvfrom(sock, buf, len, flags, from, fromlen);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Recieve a buffer from a socket */
	ret = posix_socket_recvfrom(file, buf, len, flags,
		from, fromlen);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to recvfrom to socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_recvfrom_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_recvfrom_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_recvmsg, "%d %p %d", int, struct msghdr*, int);
UK_TRACEPOINT(trace_posix_socket_recvmsg_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_recvmsg_err, "%d", int);

ssize_t
recvmsg(int sock, struct msghdr *msg, int flags)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_recvmsg(sock, msg, flags);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Receive a structured message from a socket */
	ret = posix_socket_recvmsg(file, msg, flags);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to recvmsg on socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_recvmsg_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_recvmsg_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_send, "%d %p %d %d", int, const void *, size_t,
		int);
UK_TRACEPOINT(trace_posix_socket_send_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_send_err, "%d", int);

ssize_t
send(int sock, const void *buf, size_t len, int flags)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_send(sock, buf, len, flags);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Send a structured message to a sockets */
	ret = posix_socket_send(file, buf, len, flags);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to send on socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_send_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_send_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_sendmsg, "%d %p %d", int,
		const struct msghdr *, int);
UK_TRACEPOINT(trace_posix_socket_sendmsg_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_sendmsg_err, "%d", int);

ssize_t
sendmsg(int sock, const struct msghdr *msg, int flags)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_sendmsg(sock, msg, flags);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Send a structured message to a sockets */
	ret = posix_socket_sendmsg(file, msg, flags);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to sendmsg on socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_sendmsg_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_sendmsg_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_sendto, "%d %p %d %d %p %d", int, const void *,
		size_t, int, const struct sockaddr *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_sendto_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_sendto_err, "%d", int);

ssize_t
sendto(int sock, const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int ret = 0;
	struct posix_socket_file *file = NULL;

	trace_posix_socket_sendto(sock, buf, len, flags, dest_addr, addrlen);

	file = posix_socket_file_get(sock);
	if (PTRISERR(file)) {
		ret = -1;
		SOCKET_ERR(PTR2ERR(file), "failed to identify socket descriptor");
		goto EXIT_ERR;
	}

	/* Send to an address over a socket */
	ret = posix_socket_sendto(file, buf, len, flags,
		dest_addr, addrlen);

	/* release refcount */
	vfscore_put_file(file->vfs_file);

	if (ret < 0) {
		uk_pr_err("failed to sendto on socket\n");
		goto EXIT_ERR;
	}

	trace_posix_socket_sendto_ret(ret);
	return ret;
EXIT_ERR:
	trace_posix_socket_sendto_err(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_socketpair, "%d %d %d %p", int, int, int,
			int *);
UK_TRACEPOINT(trace_posix_socket_socketpair_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_socketpair_err, "%d", int);

int
socketpair(int family, int type, int protocol, int usockfd[2])
{
	int ret;
	void *usockdata[2];
	int vfs_fd1, vfs_fd2;
	struct posix_socket_driver *d;
	struct posix_socket_file *u1, *u2;

	trace_posix_socket_socketpair(family, type, protocol, usockfd);

	d = posix_socket_driver_get(family);
	if (d == NULL) {
		ret = -1;
		SOCKET_ERR(EAFNOSUPPORT, "no socket implementation for family");
		goto EXIT_ERR;
	}

	/* Create the socketpair using the driver */
	ret = posix_socket_socketpair(d, family, type, protocol, usockdata);
	if (ret < 0) {
		uk_pr_err("failed to create socket\n");
		goto EXIT_ERR;
	}

	/* Allocate the file descriptor */
	vfs_fd1 = socket_alloc_fd(d, type, usockdata[0]);
	if (vfs_fd1 < 0) {
		ret = vfs_fd1;
		SOCKET_ERR(PTR2ERR(vfs_fd1), "failed to allocate descriptor");
		goto EXIT_ERR;
	}

	vfs_fd2 = socket_alloc_fd(d, type, usockdata[1]);
	if (vfs_fd2 < 0) {
		ret = vfs_fd2;
		SOCKET_ERR(PTR2ERR(vfs_fd2), "failed to allocate descriptor");
		goto EXIT_ERR;
	}

	/* Return the file descriptors to the user */
	usockfd[0] = vfs_fd1;
	usockfd[1] = vfs_fd2;

EXIT:
	trace_posix_socket_socketpair_ret(ret);
	return ret;

EXIT_ERR:
	trace_posix_socket_socketpair_err(ret);
	return ret;
}

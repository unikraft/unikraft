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
#include <sys/stat.h>

#include <uk/socket.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-fd.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/trace.h>
#include <uk/syscall.h>
#include <errno.h>


static const char POSIX_SOCKET_VOLID[] = "posix_socket_vol";

#define SOCKET_MODE (O_RDWR|UKFD_O_NOSEEK|UKFD_O_NOIOLOCK)

#define _ERR_BLOCK(r) ((r) == -EAGAIN || (r) == -EWOULDBLOCK)
#define _SHOULD_BLOCK(m) !((m) & O_NONBLOCK)

struct socket_alloc {
	struct uk_file f;
	uk_file_refcnt fref;
	struct uk_file_state fstate;
	struct posix_socket_node node;
};


static struct uk_ofile *socketfd_get(int fd)
{
	struct uk_ofile *of = uk_fdtab_get(fd);

	if (unlikely(!of))
		return ERR2PTR(-EBADF);
	if (unlikely(of->file->vol != POSIX_SOCKET_VOLID)) {
		uk_fdtab_ret(of);
		return ERR2PTR(-ENOTSOCK);
	}
	return of;
}


static ssize_t
socket_read(const struct uk_file *sock,
	    const struct iovec *iov, int iovcnt,
	    off_t off, long flags __unused)
{
	ssize_t ret;
	struct posix_socket_driver *d;

	if (unlikely(sock->vol != POSIX_SOCKET_VOLID))
		return -EINVAL;
	if (unlikely(off))
		return -ESPIPE;

	d = posix_sock_get_driver(sock);
	uk_file_rlock(sock);
	if (d->ops->read) {
		ret = posix_socket_read(sock, iov, iovcnt);
	} else {
		struct msghdr msg = {
			.msg_name = NULL,
			.msg_namelen = 0,
			.msg_iov = (struct iovec *)iov,
			.msg_iovlen = iovcnt,
			.msg_control = NULL,
			.msg_controllen = 0
		};
		ret = posix_socket_recvmsg(sock, &msg, 0);
	}
	uk_file_runlock(sock);
	return ret;
}

static ssize_t
socket_write(const struct uk_file *sock,
	     const struct iovec *iov, int iovcnt,
	     off_t off, long flags __unused)
{
	ssize_t ret;
	struct posix_socket_driver *d;

	if (unlikely(sock->vol != POSIX_SOCKET_VOLID))
		return -EINVAL;
	if (unlikely(off))
		return -ESPIPE;

	d = posix_sock_get_driver(sock);
	uk_file_rlock(sock);
	if (d->ops->write) {
		ret = posix_socket_write(sock, iov, iovcnt);
	} else {
		struct msghdr msg = {
			.msg_name = NULL,
			.msg_namelen = 0,
			.msg_iov = (struct iovec *)iov,
			.msg_iovlen = iovcnt,
			.msg_control = NULL,
			.msg_controllen = 0
		};
		ret = posix_socket_sendmsg(sock, &msg, 0);
	}
	uk_file_runlock(sock);
	return ret;
}

static int
socket_ctl(const struct uk_file *sock, int fam, int req,
	   uintptr_t arg1, uintptr_t arg2 __unused, uintptr_t arg3 __unused)
{
	switch (fam) {
	case UKFILE_CTL_IOCTL:
	{
		int ret;

		uk_file_rlock(sock);
		ret = posix_socket_ioctl(sock, req, (void *)arg1);
		uk_file_runlock(sock);
		return ret;
	}
	default:
		return -ENOSYS;
	}
}

#define SOCKET_STATX_MASK \
	(UK_STATX_TYPE|UK_STATX_MODE|UK_STATX_NLINK|UK_STATX_INO|UK_STATX_SIZE)

static int
socket_getstat(const struct uk_file *sock,
	       unsigned int mask __unused, struct uk_statx *arg)
{
	/* Since all information is immediately available, ignore mask arg */
	arg->stx_mask = SOCKET_STATX_MASK;
	arg->stx_mode = S_IFSOCK|0777;
	arg->stx_nlink = 1;
	arg->stx_ino = (uintptr_t)sock; /* Must be unique per-socket */
	arg->stx_size = 0;

	/* Following fields are always filled in, not in stx_mask */
	arg->stx_dev_major = 0;
	arg->stx_dev_minor = 8; /* Same value Linux returns for sockets */
	arg->stx_rdev_major = 0;
	arg->stx_rdev_minor = 0;
	arg->stx_blksize = 0x1000;
	return 0;
}

static const struct uk_file_ops socket_file_ops = {
	.read = socket_read,
	.write = socket_write,
	.getstat = socket_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = socket_ctl
};

static void socket_release(const struct uk_file *sock, int what)
{
	UK_ASSERT(sock->vol == POSIX_SOCKET_VOLID);
	if (what & UK_FILE_RELEASE_RES) {
		posix_socket_close(sock);
	}
	if (what & UK_FILE_RELEASE_OBJ) {
		struct socket_alloc *al = __containerof(sock,
							struct socket_alloc, f);

		uk_free(al->node.driver->allocator, al);
	}
}


static void _socket_init(struct socket_alloc *al,
			 struct posix_socket_driver *d, void *sock_data)
{
	al->node = (struct posix_socket_node){
		.sock_data = sock_data,
		.driver = d
	};
	al->fstate = UK_FILE_STATE_INITIALIZER(al->fstate);
	al->fref = UK_FILE_REFCNT_INITIALIZER;
	al->f = (struct uk_file){
		.vol = POSIX_SOCKET_VOLID,
		.node = &al->node,
		.ops = &socket_file_ops,
		.refcnt = &al->fref,
		.state = &al->fstate,
		._release = socket_release
	};
	posix_socket_poll(&al->f);
}


struct uk_file *uk_socket_create(int family, int type, int protocol)
{
	struct posix_socket_driver *d = posix_socket_driver_get(family);
	struct socket_alloc *al;
	void *sock_data;

	if (unlikely(!d))
		return ERR2PTR(-EAFNOSUPPORT);

	al = uk_malloc(d->allocator, sizeof(*al));
	if (unlikely(!al))
		return ERR2PTR(-ENOMEM);

	sock_data = posix_socket_create(d, family, type, protocol);
	/* NULL is a valid return value on success */
	if (unlikely(sock_data && PTRISERR(sock_data))) {
		uk_free(d->allocator, al);
		return sock_data;
	}

	_socket_init(al, d, sock_data);
	return &al->f;
}

/* Internal API & Syscalls */

int uk_sys_socket(int family, int type, int protocol)
{
	int fd;
	unsigned int mode = SOCKET_MODE;
	struct uk_file *sock = uk_socket_create(family, type, protocol);

	if (unlikely(PTRISERR(sock)))
		return PTR2ERR(sock);

	if (type & SOCK_NONBLOCK)
		mode |= O_NONBLOCK;
	if (type & SOCK_CLOEXEC)
		mode |= O_CLOEXEC;
	fd = uk_fdtab_open(sock, mode);
	uk_file_release(sock);
	return fd;
}


UK_TRACEPOINT(trace_posix_socket_create, "%d %d %d", int, int, int);
UK_TRACEPOINT(trace_posix_socket_create_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_create_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, socket, int, family, int, type, int, protocol)
{
	int ret;

	trace_posix_socket_create(family, type, protocol);

	ret = uk_sys_socket(family, type, protocol);

	if (ret >= 0)
		trace_posix_socket_create_ret(ret);
	else
		trace_posix_socket_create_err(ret);

	return ret;
}


int uk_sys_accept(const struct uk_file *sock, int blocking,
		  struct sockaddr *addr, socklen_t *addr_len, int flags)
{
	int fd;
	void *new_data;
	struct socket_alloc *al;
	unsigned int mode = SOCKET_MODE;
	struct posix_socket_node *n = (struct posix_socket_node *)sock->node;

	al = uk_malloc(n->driver->allocator, sizeof(*al));
	if (unlikely(!al))
		return -ENOMEM;

	for (;;) {
		uk_file_rlock(sock);
		new_data = posix_socket_accept4(sock, addr, addr_len, flags);
		uk_file_runlock(sock);
		if (!blocking ||
		    !PTRISERR(new_data) || !_ERR_BLOCK(PTR2ERR(new_data)))
			break;
		(void)uk_file_poll(sock, UKFD_POLLIN);
	}
	if (unlikely(PTRISERR(new_data))) {
		uk_free(n->driver->allocator, al);
		return PTR2ERR(new_data);
	}

	_socket_init(al, n->driver, new_data);


	if (flags & SOCK_NONBLOCK)
		mode |= O_NONBLOCK;
	if (flags & SOCK_CLOEXEC)
		mode |= O_CLOEXEC;
	fd = uk_fdtab_open(&al->f, mode);
	uk_file_release(&al->f);
	return fd;
}


UK_TRACEPOINT(trace_posix_socket_accept, "%d %p %p", int,
		struct sockaddr *restrict, socklen_t *restrict);
UK_TRACEPOINT(trace_posix_socket_accept_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_accept_err, "%d", int);

int do_accept4(int sock, struct sockaddr *addr, socklen_t *addr_len,
	       int flags)
{
	int ret;
	unsigned int mode;
	struct uk_ofile *of;

	trace_posix_socket_accept(sock, addr, addr_len);

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	mode = of->mode;
	ret = uk_sys_accept(of->file, _SHOULD_BLOCK(mode),
			    addr, addr_len, flags);
	uk_fdtab_ret(of);

out:
	if (ret >= 0)
		trace_posix_socket_accept_ret(ret);
	else
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
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_bind(sock, addr, addr_len);

	if (unlikely(!addr))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_wlock(of->file);
	ret = posix_socket_bind(of->file, addr, addr_len);
	uk_file_wunlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_bind_err(ret);
	else
		trace_posix_socket_bind_ret(ret);

	return ret;
}


UK_TRACEPOINT(trace_posix_socket_shutdown, "%d %d", int, int);
UK_TRACEPOINT(trace_posix_socket_shutdown_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_shutdown_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, shutdown, int, sock, int, how)
{
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_shutdown(sock, how);

	if (unlikely(!(how == SHUT_RD || how == SHUT_WR || how == SHUT_RDWR))) {
		ret = -EINVAL;
		goto out;
	}

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_wlock(of->file);
	ret = posix_socket_shutdown(of->file, how);
	uk_file_wunlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_shutdown_err(ret);
	else
		trace_posix_socket_shutdown_ret(ret);
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
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_getpeername(sock, addr, addr_len);

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_rlock(of->file);
	ret = posix_socket_getpeername(of->file, addr, addr_len);
	uk_file_runlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_getpeername_err(ret);
	else
		trace_posix_socket_getpeername_ret(ret);
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
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_getsockname(sock, addr, addr_len);

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_rlock(of->file);
	ret = posix_socket_getsockname(of->file, addr, addr_len);
	uk_file_runlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_getsockname_err(ret);
	else
		trace_posix_socket_getsockname_ret(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_getsockopt, "%d %d %d %p %d",
	      int, int, int, void *, socklen_t *);
UK_TRACEPOINT(trace_posix_socket_getsockopt_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_getsockopt_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, getsockopt, int, sock, int, level, int, optname,
		    void *restrict, optval, socklen_t *restrict, optlen)
{
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_getsockopt(sock, level, optname, optval, optlen);

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_rlock(of->file);
	ret = posix_socket_getsockopt(of->file, level, optname, optval, optlen);
	uk_file_runlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_getsockopt_err(ret);
	else
		trace_posix_socket_getsockopt_ret(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_setsockopt, "%d %d %d %p %d",
	      int, int, int, const void *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_setsockopt_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_setsockopt_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, setsockopt, int, sock, int, level, int, optname,
		    const void *, optval, socklen_t, optlen)
{
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_setsockopt(sock, level, optname, optval, optlen);

	if (unlikely(!optval))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_rlock(of->file);
	ret = posix_socket_setsockopt(of->file, level, optname, optval, optlen);
	uk_file_runlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_setsockopt_err(ret);
	else
		trace_posix_socket_setsockopt_ret(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_connect, "%d %p %d",
	      int, const struct sockaddr *, socklen_t);
UK_TRACEPOINT(trace_posix_socket_connect_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_connect_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, connect, int, sock, const struct sockaddr *, addr,
		    socklen_t, addr_len)
{
	int ret;
	unsigned int mode;
	struct uk_ofile *of;

	trace_posix_socket_connect(sock, addr, addr_len);

	if (unlikely(!addr))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	mode = of->mode;
	uk_file_wlock(of->file);
	ret = posix_socket_connect(of->file, addr, addr_len);
	uk_file_wunlock(of->file);
	if (ret == -EINPROGRESS && _SHOULD_BLOCK(mode)) {
		socklen_t _opsz = sizeof(ret);

		(void)uk_file_poll(of->file, UKFD_POLLOUT);
		/* Get err status from getsockopt */
		uk_file_rlock(of->file);
		posix_socket_getsockopt(of->file, SOL_SOCKET, SO_ERROR,
					&ret, &_opsz);
		uk_file_runlock(of->file);
	}
	uk_fdtab_ret(of);

out:
	if (ret && ret != -EINPROGRESS)
		trace_posix_socket_connect_err(ret);
	else
		trace_posix_socket_connect_ret(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_listen, "%d %d", int, int);
UK_TRACEPOINT(trace_posix_socket_listen_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_listen_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, listen, int, sock, int, backlog)
{
	int ret;
	struct uk_ofile *of;

	trace_posix_socket_listen(sock, backlog);

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	uk_file_wlock(of->file);
	ret = posix_socket_listen(of->file, backlog);
	uk_file_wunlock(of->file);
	uk_fdtab_ret(of);

out:
	if (ret)
		trace_posix_socket_listen_err(ret);
	else
		trace_posix_socket_listen_ret(ret);
	return ret;
}


UK_TRACEPOINT(trace_posix_socket_recvfrom, "%d %p %d %d %p %p",
	      int, struct sockaddr *, size_t, int, void *, socklen_t *);
UK_TRACEPOINT(trace_posix_socket_recvfrom_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_recvfrom_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, recvfrom, int, sock, void *, buf, size_t, len,
		    int, flags, struct sockaddr *, from, socklen_t *, fromlen)
{
	ssize_t ret;
	unsigned int mode;
	struct uk_ofile *of;

	trace_posix_socket_recvfrom(sock, buf, len, flags, from, fromlen);

	if (unlikely(!buf))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	mode = of->mode;
	for (;;) {
		uk_file_rlock(of->file);
		ret = posix_socket_recvfrom(of->file, buf, len, flags,
					    from, fromlen);
		uk_file_runlock(of->file);
		if (!_SHOULD_BLOCK(mode) || !_ERR_BLOCK(ret))
			break;
		(void)uk_file_poll(of->file, UKFD_POLLIN);
	}
	uk_fdtab_ret(of);

out:
	if (ret < 0 && ret != -EAGAIN)
		trace_posix_socket_recvfrom_err(ret);
	else
		trace_posix_socket_recvfrom_ret(ret);
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
	ssize_t ret;
	unsigned int mode;
	struct uk_ofile *of;

	trace_posix_socket_recvmsg(sock, msg, flags);

	if (unlikely(!msg || !msg->msg_iov))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	mode = of->mode;
	for (;;) {
		uk_file_rlock(of->file);
		ret = posix_socket_recvmsg(of->file, msg, flags);
		uk_file_runlock(of->file);
		if (!_SHOULD_BLOCK(mode) || !_ERR_BLOCK(ret))
			break;
		(void)uk_file_poll(of->file, UKFD_POLLIN);
	}
	uk_fdtab_ret(of);

out:
	if (ret < 0 && ret != -EAGAIN)
		trace_posix_socket_recvmsg_err(ret);
	else
		trace_posix_socket_recvmsg_ret(ret);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_sendmsg, "%d %p %d", int,
	      const struct msghdr *, int);
UK_TRACEPOINT(trace_posix_socket_sendmsg_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_sendmsg_err, "%d", int);

UK_SYSCALL_R_DEFINE(ssize_t, sendmsg, int, sock, const struct msghdr *, msg,
		    int, flags)
{
	ssize_t ret;
	unsigned int mode;
	struct uk_ofile *of;

	trace_posix_socket_sendmsg(sock, msg, flags);

	if (unlikely(!msg || !msg->msg_iov))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	mode = of->mode;
	for (;;) {
		uk_file_rlock(of->file);
		ret = posix_socket_sendmsg(of->file, msg, flags);
		uk_file_runlock(of->file);
		if (!_SHOULD_BLOCK(mode) || !_ERR_BLOCK(ret))
			break;
		(void)uk_file_poll(of->file, UKFD_POLLOUT);
	}
	uk_fdtab_ret(of);

out:
	if (ret < 0 && ret != -EAGAIN)
		trace_posix_socket_sendmsg_err(ret);
	else
		trace_posix_socket_sendmsg_ret(ret);
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
	ssize_t ret;
	unsigned int mode;
	struct uk_ofile *of;

	trace_posix_socket_sendto(sock, buf, len, flags, dest_addr, addrlen);

	if (unlikely(!buf))
		return -EFAULT;

	of = socketfd_get(sock);
	if (unlikely(PTRISERR(of))) {
		ret = PTR2ERR(of);
		goto out;
	}

	mode = of->mode;
	for (;;) {
		uk_file_rlock(of->file);
		ret = posix_socket_sendto(of->file, buf, len, flags,
					  dest_addr, addrlen);
		uk_file_runlock(of->file);
		if (!_SHOULD_BLOCK(mode) || !_ERR_BLOCK(ret))
			break;
		(void)uk_file_poll(of->file, UKFD_POLLOUT);
	}
	uk_fdtab_ret(of);

out:
	if (ret < 0 && ret != -EAGAIN)
		trace_posix_socket_sendto_err(ret);
	else
		trace_posix_socket_sendto_ret(ret);
	return ret;
}

#if UK_LIBC_SYSCALLS
/* Provide wrapper (if not provided by Musl) or some other libc. */
ssize_t send(int sock, const void *buf, size_t len, int flags)
{
	return uk_syscall_e_sendto(sock, (long) buf, len, flags, (long) NULL, 0);
}
#endif /* UK_LIBC_SYSCALLS */


int uk_socketpair_create(int family, int type, int protocol,
			 const struct uk_file *sv[2])
{
	int ret;
	struct socket_alloc *al[2];
	void *sock_data[2];
	struct posix_socket_driver *d = posix_socket_driver_get(family);

	if (unlikely(!d))
		return -EAFNOSUPPORT;

	al[0] = uk_malloc(d->allocator, sizeof(*al));
	al[1] = uk_malloc(d->allocator, sizeof(*al));
	if (unlikely(!al[0] || !al[1])) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = posix_socket_socketpair(d, family, type, protocol, sock_data);
	if (unlikely(ret))
		goto err_free;

	_socket_init(al[0], d, sock_data[0]);
	_socket_init(al[1], d, sock_data[1]);
	sv[0] = &al[0]->f;
	sv[1] = &al[1]->f;
	posix_socket_socketpair_post(d, sv);
	return 0;

err_free:
	uk_free(d->allocator, al[0]);
	uk_free(d->allocator, al[1]);
	return ret;
}

int uk_sys_socketpair(int family, int type, int protocol, int sv[2])
{
	int ret;
	const struct uk_file *socks[2];
	unsigned int mode = SOCKET_MODE;

	ret = uk_socketpair_create(family, type, protocol, socks);
	if (unlikely(ret))
		return ret;

	if (type & SOCK_NONBLOCK)
		mode |= O_NONBLOCK;
	if (type & SOCK_CLOEXEC)
		mode |= O_CLOEXEC;

	ret = uk_fdtab_open(socks[0], mode);
	if (unlikely(ret < 0))
		goto out;
	sv[0] = ret;

	ret = uk_fdtab_open(socks[1], mode);
	if (unlikely(ret < 0)) {
		uk_sys_close(sv[0]);
		goto out;
	}
	sv[1] = ret;
	ret = 0;

out:
	uk_file_release(socks[0]);
	uk_file_release(socks[1]);
	return ret;
}

UK_TRACEPOINT(trace_posix_socket_socketpair, "%d %d %d %p", int, int, int,
	      int *);
UK_TRACEPOINT(trace_posix_socket_socketpair_ret, "%d", int);
UK_TRACEPOINT(trace_posix_socket_socketpair_err, "%d", int);

UK_SYSCALL_R_DEFINE(int, socketpair, int, family, int, type, int, protocol,
		    int *, usockfd)
{
	int ret;

	trace_posix_socket_socketpair(family, type, protocol, usockfd);

	if (unlikely(!usockfd))
		return -EFAULT;

	ret = uk_sys_socketpair(family, type, protocol, usockfd);

	if (ret)
		trace_posix_socket_socketpair_err(ret);
	else
		trace_posix_socket_socketpair_ret(ret);
	return ret;
}

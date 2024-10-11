/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <sys/un.h>

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/socket_driver.h>
#include <uk/posix-pipe.h>
#include <uk/file/pollqueue.h>

#include "unixsock-bind.h"


struct unix_listenmsg {
	const struct uk_file *rpipe;
	const struct uk_file *wpipe;
	posix_sock *remote;
};

struct unix_listenq {
	int size;
	volatile int count;
	volatile int rhead;
	int whead;
	struct unix_listenmsg *q;
};

#define UNIX_LISTENQ(sz, qbuf) ((struct unix_listenq){ \
	.size = (sz), \
	.count = 0, \
	.rhead = 0, \
	.whead = 0, \
	.q = (qbuf) \
})

struct unix_sock_data {
	int type;
	int flags;
	const struct uk_file *rpipe;
	const struct uk_file *wpipe;
	struct uk_poll_chain rio;
	struct uk_poll_chain rerr;
	union {
		struct {
			struct uk_poll_chain wio;
			struct uk_poll_chain werr;
		};
		struct unix_listenq listen;
		const struct uk_file *bpipe;
	};
	struct unix_addr_entry bind;
	posix_sock *remote;
};

#define _SOCK_CONNECTION(t) ((t) == SOCK_STREAM || (t) == SOCK_SEQPACKET)

#define UNIXSOCK_CONN   1
#define UNIXSOCK_BOUND  2
#define UNIXSOCK_LISTEN 4

#define UNIXSOCK_RDEV   0x100
#define UNIXSOCK_WREV   0x200


static inline
struct unix_sock_data *unix_sock_alloc(struct posix_socket_driver *d, int type)
{
	struct unix_sock_data *data = uk_malloc(d->allocator, sizeof(*data));

	if (unlikely(!data))
		return ERR2PTR(-ENOMEM);
	*data = (struct unix_sock_data){
		.type = type,
		.flags = 0,
		.rpipe = NULL,
		.wpipe = NULL,
	};
	return data;
}

static inline
void unix_sock_unnamed(struct sockaddr *restrict addr,
		       socklen_t *restrict addr_len)
{
	struct sockaddr_un *uaddr = (struct sockaddr_un *)addr;

	if (*addr_len >= sizeof(sa_family_t))
		uaddr->sun_family = AF_UNIX;
	*addr_len = sizeof(sa_family_t);
}

static inline
void unix_sock_bindname(struct unix_addr_entry *bind,
			struct sockaddr *restrict addr,
			socklen_t *restrict addr_len)
{
	struct sockaddr_un *uaddr = (struct sockaddr_un *)addr;
	size_t avail = *addr_len;

	if (*addr_len >= sizeof(sa_family_t))
		uaddr->sun_family = AF_UNIX;
	avail -= offsetof(struct sockaddr_un, sun_path);
	memcpy(uaddr->sun_path, bind->name, MIN(bind->namelen, avail));
	*addr_len = bind->namelen + offsetof(struct sockaddr_un, sun_path);
}

static inline
void unix_sock_remotename(posix_sock *remote,
			  struct sockaddr *restrict addr,
			  socklen_t *restrict addr_len)
{
	if (remote) {
		struct unix_sock_data *rd;

		uk_file_rlock(remote);
		rd = posix_sock_get_data(remote);
		if (rd && rd->flags & UNIXSOCK_BOUND) {
			unix_sock_bindname(&rd->bind, addr, addr_len);
			uk_file_runlock(remote);
			return;
		}
		uk_file_runlock(remote);
	}
	unix_sock_unnamed(addr, addr_len);
}

static
const struct uk_file *unix_sock_remotebpipe(posix_sock *file,
					    posix_sock *target)
{
	const struct uk_file *bpipe;
	struct unix_sock_data *td;
	struct unix_sock_data *data = posix_sock_get_data(file);

	uk_file_rlock(target);
	td = posix_sock_get_data(target);
	if (unlikely(!td)) {
		bpipe = ERR2PTR(-ENOENT);
		goto out;
	}
	if (unlikely(data->type != td->type)) {
		bpipe = ERR2PTR(-EPROTOTYPE);
		goto out;
	}

	bpipe = td->bpipe;
	UK_ASSERT(bpipe); /* All DGRAM socks get one on creation */
	uk_file_acquire(bpipe);
out:
	uk_file_runlock(target);
	return bpipe;
}


static
void unix_sock_rdown(uk_pollevent ev __maybe_unused,
		     enum uk_poll_chain_op op __maybe_unused,
		     struct uk_poll_chain *tick)
{
	struct unix_sock_data *d;
	struct uk_pollq *sockq;
	uk_pollevent set = EPOLLRDHUP;

	UK_ASSERT(ev == EPOLLHUP);
	UK_ASSERT(op == UK_POLL_CHAINOP_SET);

	d = __containerof(tick, struct unix_sock_data, rerr);
	sockq = (struct uk_pollq *)tick->arg;
	if (!d->wpipe || uk_file_poll_immediate(d->wpipe, EPOLLERR))
		set |= EPOLLHUP;

	uk_pollq_set(sockq, set);
}

static
void unix_sock_wdown(uk_pollevent ev __maybe_unused,
		     enum uk_poll_chain_op op __maybe_unused,
		     struct uk_poll_chain *tick)
{
	struct unix_sock_data *d;
	struct uk_pollq *sockq;

	UK_ASSERT(ev == EPOLLERR);
	UK_ASSERT(op == UK_POLL_CHAINOP_SET);

	d = __containerof(tick, struct unix_sock_data, werr);
	sockq = (struct uk_pollq *)tick->arg;
	if (!d->rpipe || uk_file_poll_immediate(d->rpipe, EPOLLHUP))
		uk_pollq_set(sockq, EPOLLHUP);
}


static
void *unix_socket_create(struct posix_socket_driver *d,
			 int family, int type, int proto)
{
	struct unix_sock_data *data;

	if (unlikely(family != AF_UNIX))
		return ERR2PTR(-EAFNOSUPPORT);
	if (unlikely(proto != PF_UNSPEC && proto != PF_UNIX))
		return ERR2PTR(-EPROTONOSUPPORT);

	type &= ~SOCK_FLAGS; /* Flags are handled by other levels */
	if (unlikely(type != SOCK_STREAM &&
		     type != SOCK_DGRAM &&
		     type != SOCK_SEQPACKET))
		return ERR2PTR(-ESOCKTNOSUPPORT);

	data = unix_sock_alloc(d, type);
	if (likely(data))
		if (type == SOCK_DGRAM) {
			/* Create rpipe */
			struct uk_file *pipes[2];
			int r = uk_pipefile_create(pipes);
			if (unlikely(r)) {
				uk_free(d->allocator, data);
				data = ERR2PTR(r);
			} else {
				data->rpipe = pipes[0];
				data->bpipe = pipes[1];
			}
		}
	return data;
}

static
void unix_socket_poll_setup(posix_sock *file)
{
	struct unix_sock_data *data = posix_sock_get_data(file);
	struct uk_pollq *sockq = &file->state->pollq;

	if (data->rpipe) {
		UK_ASSERT(!(data->flags & UNIXSOCK_RDEV));
		data->rio = UK_POLL_CHAIN_UPDATE(UKFD_POLLIN, sockq, 0);
		data->rerr = UK_POLL_CHAIN_CALLBACK(EPOLLHUP,
			unix_sock_rdown, sockq);
		posix_sock_event_set(
			file,
			uk_file_poll_immediate(data->rpipe, UKFD_POLLIN)
		);
		uk_pollq_register(&data->rpipe->state->pollq, &data->rio);
		uk_pollq_register(&data->rpipe->state->pollq, &data->rerr);
		data->flags |= UNIXSOCK_RDEV;
	}
	if (_SOCK_CONNECTION(data->type)) {
		if (data->wpipe) {
			UK_ASSERT(!(data->flags & UNIXSOCK_WREV));
			data->wio = UK_POLL_CHAIN_UPDATE(UKFD_POLLOUT,
				sockq, 0);
			data->werr = UK_POLL_CHAIN_CALLBACK(EPOLLERR,
				unix_sock_wdown, sockq);
			posix_sock_event_set(
				file,
				uk_file_poll_immediate(data->wpipe,
						       UKFD_POLLOUT)
			);
			uk_pollq_register(&data->wpipe->state->pollq,
					  &data->wio);
			uk_pollq_register(&data->wpipe->state->pollq,
					  &data->werr);
			data->flags |= UNIXSOCK_WREV;
		}
	} else {
		/* DGRAM sockets could in theory always write somewhere */
		posix_sock_event_set(file, UKFD_POLLOUT);
	}
}

static
int unix_socket_socketpair(struct posix_socket_driver *d,
			   int family, int type, int proto,
			   void *sockvec[2])
{
	int ret;
	struct unix_sock_data *dat[2];
	struct uk_file *pipes[2][2];

	if (unlikely(family != AF_UNIX))
		return -EAFNOSUPPORT;
	if (unlikely(proto != PF_UNSPEC && proto != PF_UNIX))
		return -EPROTONOSUPPORT;

	type &= ~SOCK_FLAGS; /* Flags are handled by other levels */
	if (unlikely(type != SOCK_STREAM &&
		     type != SOCK_DGRAM &&
		     type != SOCK_SEQPACKET))
		return -ESOCKTNOSUPPORT;

	dat[0] = unix_sock_alloc(d, type);
	if (unlikely(PTRISERR(dat[0])))
		return PTR2ERR(dat[0]);
	dat[1] = unix_sock_alloc(d, type);
	if (unlikely(PTRISERR(dat[1]))) {
		ret = PTR2ERR(dat[1]);
		goto err_free0;
	}

	ret = uk_pipefile_create(pipes[0]);
	if (unlikely(ret))
		goto err_free;
	ret = uk_pipefile_create(pipes[1]);
	if (unlikely(ret))
		goto err_release;

	dat[0]->rpipe = pipes[0][0];
	dat[1]->wpipe = pipes[0][1];
	dat[1]->rpipe = pipes[1][0];
	dat[0]->wpipe = pipes[1][1];
	dat[0]->flags |= UNIXSOCK_CONN;
	dat[1]->flags |= UNIXSOCK_CONN;
	if (type == SOCK_DGRAM) {
		uk_file_acquire(pipes[0][1]);
		uk_file_acquire(pipes[1][1]);
		dat[0]->bpipe = pipes[0][1];
		dat[1]->bpipe = pipes[1][1];
	}

	sockvec[0] = dat[0];
	sockvec[1] = dat[1];
	return 0;

err_release:
	uk_file_release(pipes[0][0]);
	uk_file_release(pipes[0][1]);
err_free:
	uk_free(d->allocator, dat[1]);
err_free0:
	uk_free(d->allocator, dat[0]);
	return ret;
}

static
void unix_socket_socketpair_post(struct posix_socket_driver *d __unused,
				 posix_sock *sv[2])
{
	struct unix_sock_data *d0 = posix_sock_get_data(sv[0]);
	struct unix_sock_data *d1 = posix_sock_get_data(sv[1]);

	uk_file_acquire_weak(sv[0]);
	uk_file_acquire_weak(sv[1]);
	d0->remote = sv[1];
	d1->remote = sv[0];
}

static
void *unix_socket_accept4(posix_sock *file,
			  struct sockaddr *restrict addr,
			  socklen_t *restrict addr_len, int flags)
{
	struct posix_socket_driver *d = posix_sock_get_driver(file);
	struct unix_sock_data *data = posix_sock_get_data(file);
	int prev;
	struct unix_sock_data *acc;
	int i;

	/* Validate sock is listening */
	if (unlikely(!(data->flags & UNIXSOCK_LISTEN)))
		return ERR2PTR(-EINVAL);
	if (flags)
		uk_pr_err("STUB: accept4 flags 0x%x\n", flags);
	/* Check listen queue */
	prev = data->listen.count;
	UK_ASSERT(prev >= 0);
	if (!prev) {
		posix_sock_event_clear(file, UKFD_POLLIN);
		return ERR2PTR(-EAGAIN);
	}
	/* Reserve 1 accept entry */
	while (!uk_compare_exchange_n(&data->listen.count,
					  &prev, prev - 1));

	/* New socket obj */
	acc = unix_sock_alloc(d, data->type);
	if (unlikely(!acc)) {
		/* If ENOMEM, put number back in queue */
		if (!uk_inc(&data->listen.count))
			posix_sock_event_set(file, UKFD_POLLIN);
		return ERR2PTR(-ENOMEM);
	}

	/* Grab entry from listen q (no errors past this point) */
	i = data->listen.rhead;
	while (!uk_compare_exchange_n(&data->listen.rhead,
					  &i, (i + 1) % data->listen.size));
	/* Fill in data in new socket */
	acc->rpipe = data->listen.q[i].rpipe;
	acc->wpipe = data->listen.q[i].wpipe;
	acc->remote = data->listen.q[i].remote;
	acc->flags |= UNIXSOCK_CONN;

	unix_sock_remotename(acc->remote, addr, addr_len);
	return acc;
}

static
int unix_socket_bind(posix_sock *file,
		     const struct sockaddr *addr, socklen_t addr_len)
{
	const struct sockaddr_un *uaddr = (const struct sockaddr_un *)addr;
	ssize_t len = addr_len - offsetof(struct sockaddr_un, sun_path);
	const char *name = uaddr->sun_path;
	struct posix_socket_driver *d;
	struct unix_sock_data *data;
	ssize_t bindlen;
	char *bindname;
	int err;

	/* Validate addr */
	if (unlikely(addr_len < sizeof(sa_family_t)))
		return -EINVAL;
	if (unlikely(uaddr->sun_family != AF_UNIX))
		return -EINVAL;

	/* Get name str + namelen */
	UK_ASSERT(len >= 0);
	if (!len) {
		/* STUB; no autobind yet */
		uk_pr_warn("STUB: AF_UNIX autobind\n");
		return -EINVAL;
	}
	if (len > CONFIG_LIBPOSIX_UNIXSOCKET_MAX_NAMELEN)
		len = CONFIG_LIBPOSIX_UNIXSOCKET_MAX_NAMELEN;

	/* Validate sock obj */
	d = posix_sock_get_driver(file);
	data = posix_sock_get_data(file);
	if (unlikely(data->flags & UNIXSOCK_BOUND))
		return -EINVAL;

	/* Prep name string */
	/* Linux ensures names are zero-terminated; replicate this behavior */
	bindlen = name[len-1] == '\0' ? len : len + 1;
	bindname = uk_malloc(d->allocator, bindlen);
	if (unlikely(!bindname))
		return -ENOMEM;
	memcpy(bindname, name, len);
	if (bindlen > len)
		bindname[len] = '\0';
	/* Prep sock obj */
	data->bind = UNIX_ADDR_ENTRY(bindname, bindlen, file);

	/* Bind in namespace */
	err = unix_addr_bind(&data->bind);
	if (unlikely(err))
		goto err_out;
	/* Mark UNIXSOCK_BOUND */
	data->flags |= UNIXSOCK_BOUND;

	return 0;

err_out:
	uk_free(d->allocator, bindname);
	return err;
}

static
int unix_socket_getpeername(posix_sock *file,
			    struct sockaddr *restrict addr,
			    socklen_t *restrict addr_len)
{
	struct unix_sock_data *data = posix_sock_get_data(file);

	if (unlikely(!(data->flags & UNIXSOCK_CONN)))
		return -ENOTCONN;
	unix_sock_remotename(data->remote, addr, addr_len);
	return 0;
}

static
int unix_socket_getsockname(posix_sock *file,
			    struct sockaddr *restrict addr,
			    socklen_t *restrict addr_len)
{
	struct unix_sock_data *data = posix_sock_get_data(file);

	if (data->flags & UNIXSOCK_BOUND)
		unix_sock_bindname(&data->bind, addr, addr_len);
	else
		unix_sock_unnamed(addr, addr_len);
	return 0;
}

static
int unix_socket_getsockopt(posix_sock *file, int level, int optname,
			   void *restrict optval, socklen_t *restrict optlen)
{
	struct unix_sock_data *data = posix_sock_get_data(file);

	switch (level) {
	case SOL_SOCKET:
	{
		int val;

		switch (optname) {
		case SO_ACCEPTCONN:
			val = !!(data->flags & UNIXSOCK_LISTEN);
			break;
		case SO_DOMAIN:
			val = AF_UNIX;
			break;
		case SO_PEEK_OFF:
			val = -1;
			break;
		case SO_TYPE:
			val = data->type;
			break;
		/* no-op options; return 0 */
		case SO_BROADCAST:
		case SO_ERROR:
		case SO_DONTROUTE:
		case SO_KEEPALIVE:
		case SO_LINGER:
		case SO_PRIORITY:
		case SO_PROTOCOL:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_SELECT_ERR_QUEUE:
		case SO_BUSY_POLL:
			val = 0;
			break;
		default:
			return -ENOPROTOOPT;
		}

		if (unlikely(*optlen < sizeof(val)))
			return -EINVAL;
		*optlen = sizeof(val);
		*((int *)optval) = val;
		return 0;
	}
	default:
		return -ENOSYS;
	}
}

static
int unix_socket_setsockopt(posix_sock *file __unused, int level, int optname,
			   const void *optval __unused,
			   socklen_t optlen __unused)
{
	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		/* silently ignore */
		case SO_BROADCAST:
		case SO_DONTROUTE:
		case SO_KEEPALIVE:
		case SO_LINGER:
		case SO_PRIORITY:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_SELECT_ERR_QUEUE:
		case SO_BUSY_POLL:
			return 0;
		default:
			return -ENOPROTOOPT;
		}
	default:
		return -ENOSYS;
	}
}


static
int unix_sock_connect_stream(posix_sock *file, posix_sock *target)
{
	int err;
	struct unix_sock_data *data = posix_sock_get_data(file);
	struct unix_sock_data *td;
	struct uk_file *pipes[2][2];
	struct unix_listenmsg *lmsg;

	/* Wlock target sock */
	uk_file_wlock(target);
	/* Check target sock still valid */
	td = posix_sock_get_data(target);
	if (unlikely(!td)) {
		err = -ENOENT;
		goto err_out;
	}
	/* Type-match target */
	if (unlikely(data->type != td->type)) {
		err = -EPROTOTYPE;
		goto err_out;
	}
	/* Not listening -> -ECONNREFUSED */
	if (unlikely(!(td->flags & UNIXSOCK_LISTEN))) {
		err = -ECONNREFUSED;
		goto err_out;
	}
	/* No space left in listenq -> -ECONNREFUSED */
	if (unlikely(td->listen.count >= td->listen.size)) {
		UK_ASSERT(td->listen.count == td->listen.size);
		err = -ECONNREFUSED;
		goto err_out;
	}
	/* Create pipes */
	err = uk_pipefile_create(pipes[0]);
	if (unlikely(err))
		goto err_out;
	err = uk_pipefile_create(pipes[1]);
	if (unlikely(err))
		goto err_release;

	lmsg = &td->listen.q[td->listen.whead];
	td->listen.count++;
	td->listen.whead = (td->listen.whead + 1) % td->listen.size;
	/* Put pipes in listenq */
	lmsg->rpipe = pipes[1][0];
	lmsg->wpipe = pipes[0][1];
	/* Take weakref on self & put in listenq */
	uk_file_acquire_weak(file);
	lmsg->remote = file;
	/* Update POLLIN on target */
	posix_sock_event_set(target, EPOLLIN);
	/* Wunlock target sock */
	uk_file_wunlock(target);

	/* Put info in self */
	data->rpipe = pipes[0][0];
	data->wpipe = pipes[1][1];
	data->remote = target;
	data->flags |= UNIXSOCK_CONN;
	/* Poll self (to register events & mark connected) */
	unix_socket_poll_setup(file);

	return 0;

err_release:
	uk_file_release(pipes[0][0]);
	uk_file_release(pipes[0][1]);
err_out:
	uk_file_wunlock(target);
	return err;
}

static
int unix_sock_connect_dgram(posix_sock *file, posix_sock *target)
{
	const struct uk_file *bpipe = unix_sock_remotebpipe(file, target);
	struct unix_sock_data *data = posix_sock_get_data(file);

	if (unlikely(PTRISERR(bpipe)))
		return PTR2ERR(bpipe);

	if (data->wpipe) {
		UK_ASSERT(data->flags & UNIXSOCK_CONN);
		uk_file_release(data->wpipe);
	}
	data->wpipe = bpipe;
	data->remote = target;
	data->flags |= UNIXSOCK_CONN;
	return 0;
}


static
int unix_socket_connect(posix_sock *file,
			const struct sockaddr *addr, socklen_t addr_len)
{
	struct unix_sock_data *data = posix_sock_get_data(file);
	const struct sockaddr_un *uaddr = (const struct sockaddr_un *)addr;
	const char *rname = uaddr->sun_path;
	size_t rlen = addr_len - offsetof(struct sockaddr_un, sun_path);
	posix_sock *target;
	int err;

	/* Validate sock can connect */
	if (unlikely((data->flags & UNIXSOCK_CONN) &&
		     _SOCK_CONNECTION(data->type)))
		return -EISCONN;
	/* Validate addr is AF_UNIX & has len */
	if (unlikely(addr_len < sizeof(sa_family_t)))
		return -EINVAL;
	if (unlikely(uaddr->sun_family != AF_UNIX))
		return -EINVAL;
	if (unlikely(!rlen))
		return -EINVAL;

	/* Get target */
	target = unix_addr_lookup(rname, rlen);
	if (unlikely(!target))
		return -ENOENT;

	if (_SOCK_CONNECTION(data->type))
		err = unix_sock_connect_stream(file, target);
	else
		err = unix_sock_connect_dgram(file, target);
	if (err)
		uk_file_release_weak(target);
	return err;
}

static
int unix_socket_listen(posix_sock *file, int backlog)
{
	struct unix_sock_data *data = posix_sock_get_data(file);

	if (unlikely(backlog < 0))
		return -EINVAL;

	/* Validate sock bound & can listen */
	if (unlikely(!(data->type == SOCK_STREAM ||
		       data->type == SOCK_SEQPACKET)))
		return -EOPNOTSUPP;
	if (unlikely(!(data->flags & UNIXSOCK_BOUND)))
		return -EINVAL;
	UK_ASSERT(!data->wpipe);

	if (!(data->flags & UNIXSOCK_LISTEN)) {
		struct posix_socket_driver *d = posix_sock_get_driver(file);
		void *q = uk_malloc(d->allocator,
				    backlog * sizeof(struct unix_listenmsg));

		if (unlikely(!q))
			return -ENOMEM;
		data->listen = UNIX_LISTENQ(backlog, q);
		data->flags |= UNIXSOCK_LISTEN;
	} else {
		/* Not conformant; will implement backlog resizing if needed */
		uk_pr_warn("listen(%d) called on already listening socket (%d)\n",
			   backlog, data->listen.size);
	}
	return 0;
}

static
ssize_t unix_socket_recvmsg(posix_sock *file, struct msghdr *msg, int flags)
{
	struct unix_sock_data *data = posix_sock_get_data(file);
	ssize_t ret;

	if (unlikely(flags)) {
		uk_pr_warn("Unsupported recv flags: %x\n", flags);
		return -ENOSYS;
	}
	if (!data->rpipe) {
		if (likely(data->flags & UNIXSOCK_CONN))
			return 0;
		else
			return -EINVAL;
	}

	uk_file_rlock(data->rpipe);
	ret = uk_file_read(data->rpipe, msg->msg_iov, msg->msg_iovlen, 0, 0);
	uk_file_runlock(data->rpipe);
	/* Get remote addr */
	if (msg->msg_name) {
		if (_SOCK_CONNECTION(data->type))
			unix_sock_remotename(data->remote,
					     msg->msg_name, &msg->msg_namelen);
		else
			/* TODO: impl DGRAM remote addr */
			unix_sock_unnamed(msg->msg_name, &msg->msg_namelen);
	}
	/* We ignore ancillary data & return flags for now */
	return ret;
}

static
ssize_t unix_socket_recvfrom(posix_sock *file, void *restrict buf,
			     size_t len, int flags, struct sockaddr *from,
			     socklen_t *restrict fromlen)
{
	struct iovec iov = {
		.iov_base = buf,
		.iov_len = len
	};
	struct msghdr msg = {
		.msg_name = from,
		.msg_namelen = fromlen ? *fromlen : 0,
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0
	};
	ssize_t ret = unix_socket_recvmsg(file, &msg, flags);

	if (fromlen && ret >= 0)
		*fromlen = msg.msg_namelen;
	return ret;
}

static
ssize_t unix_socket_sendmsg(posix_sock *file,
			    const struct msghdr *msg, int flags)
{
	struct unix_sock_data *data = posix_sock_get_data(file);
	posix_sock *remote = NULL;
	const struct uk_file *wpipe;
	ssize_t ret;

	if (unlikely(flags & ~MSG_NOSIGNAL)) {
		uk_pr_warn("Unsupported send flags: %x\n", flags);
		return -ENOSYS;
	}

	if (_SOCK_CONNECTION(data->type)) {
		/* Dest not supported by connection-oriented sockets */
		if (unlikely(msg->msg_name || msg->msg_namelen))
			return (data->flags & UNIXSOCK_CONN) ?
				-EISCONN : -EOPNOTSUPP;

		wpipe = data->wpipe;
	} else {
		if (data->flags & UNIXSOCK_CONN) {
			wpipe = data->wpipe;
		} else {
			const struct sockaddr_un *uaddr;
			const char *rname;
			size_t rlen;

			if (unlikely(!msg->msg_name))
				return -ENOTCONN;

			/* Establish remote */
			uaddr = (struct sockaddr_un *)msg->msg_name;
			rname = uaddr->sun_path;
			rlen = msg->msg_namelen -
			       offsetof(struct sockaddr_un, sun_path);
			remote = unix_addr_lookup(rname, rlen);

			if (remote) {
				wpipe = unix_sock_remotebpipe(file, remote);
				uk_file_release_weak(remote);
				if (unlikely(PTRISERR(wpipe)))
					wpipe = NULL;
			} else {
				wpipe = NULL;
			}
		}
	}
	if (unlikely(!wpipe))
		return (data->flags & UNIXSOCK_CONN)
			? (_SOCK_CONNECTION(data->type)
				? -EPIPE : -ECONNREFUSED)
			: (msg->msg_name
				? -ECONNREFUSED : -ENOTCONN);

	uk_file_wlock(wpipe);
	ret = uk_file_write(wpipe, msg->msg_iov, msg->msg_iovlen, 0,
			    data->type != SOCK_STREAM ? O_DIRECT : 0);
	uk_file_wunlock(wpipe);
	/* We ignore ancillary data for now */

	/* 0-length datagrams will be silently lost; warn */
	if (!ret && data->type != SOCK_STREAM)
		uk_pr_warn("0-length datagram write; message will be lost\n");

	if (!_SOCK_CONNECTION(data->type) && remote) {
		uk_file_release(wpipe);
		if (ret == -EPIPE)
			/* Convert a broken endpoint to connection refused */
			ret = -ECONNREFUSED;
	}
	return ret;
}

static
ssize_t unix_socket_sendto(posix_sock *file, const void *buf,
			   size_t len, int flags,
			   const struct sockaddr *dest_addr, socklen_t addrlen)
{
	struct iovec iov = {
		.iov_base = (void *)buf,
		.iov_len = len
	};
	struct msghdr msg = {
		.msg_name = (struct sockaddr *)dest_addr,
		.msg_namelen = addrlen,
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0
	};

	return unix_socket_sendmsg(file, &msg, flags);
}

static
ssize_t unix_socket_read(posix_sock *file,
			 const struct iovec *iov, int iovcnt)
{
	struct msghdr msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = (struct iovec *)iov,
		.msg_iovlen = iovcnt,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0
	};
	return unix_socket_recvmsg(file, &msg, 0);
}

static
ssize_t unix_socket_write(posix_sock *file,
			  const struct iovec *iov, int iovcnt)
{
	struct msghdr msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = (struct iovec *)iov,
		.msg_iovlen = iovcnt,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0
	};
	return unix_socket_sendmsg(file, &msg, 0);
}

static
int unix_sock_shutdown(posix_sock *file, int how, int notify)
{
	struct unix_sock_data *data = posix_sock_get_data(file);

	if (unlikely(_SOCK_CONNECTION(data->type) &&
		     !(data->flags & UNIXSOCK_CONN)))
		return -ENOTCONN;

	if ((how == SHUT_WR || how == SHUT_RDWR) &&
	    (data->flags & UNIXSOCK_CONN) && data->wpipe) {
		if (_SOCK_CONNECTION(data->type)) {
			uk_pollq_unregister(&data->wpipe->state->pollq,
					    &data->wio);
			uk_pollq_unregister(&data->wpipe->state->pollq,
					    &data->werr);
		}
		uk_file_release(data->wpipe);
		data->wpipe = NULL;
		if (notify)
			/* Signal events; reuse pipe callback */
			unix_sock_wdown(EPOLLERR, UK_POLL_CHAINOP_SET,
					&data->werr);
	}
	if ((how == SHUT_RD || how == SHUT_RDWR) && data->rpipe) {
		uk_pollq_unregister(&data->rpipe->state->pollq, &data->rio);
		uk_pollq_unregister(&data->rpipe->state->pollq, &data->rerr);
		uk_file_release(data->rpipe);
		data->rpipe = NULL;
		if (notify)
			/* Signal events; reuse pipe callback */
			unix_sock_rdown(EPOLLHUP, UK_POLL_CHAINOP_SET,
					&data->rerr);
	}
	return 0;
}

static
int unix_socket_shutdown(posix_sock *file, int how)
{
	return unix_sock_shutdown(file, how, 1);
}

static
int unix_socket_close(posix_sock *file)
{
	int r;
	struct posix_socket_driver *d = posix_sock_get_driver(file);
	struct unix_sock_data *data = posix_sock_get_data(file);

	uk_file_wlock(file);
	r = unix_sock_shutdown(file, SHUT_RDWR, 0);
	posix_sock_set_data(file, NULL);
	uk_file_wunlock(file);

	if (data->flags & UNIXSOCK_BOUND) {
		r = unix_addr_release(data->bind.name, data->bind.namelen);
		UK_ASSERT(!r);
		uk_free(d->allocator, (void *)data->bind.name);
	}
	if (data->flags & UNIXSOCK_LISTEN) {
		/* Release pending connections */
		int count = data->listen.count;

		for (int i = data->listen.rhead;
		     count--;
		     i = (i + 1) % data->listen.size) {
			struct unix_listenmsg *m = &data->listen.q[i];

			if (m->remote)
				uk_file_release_weak(m->remote);
			uk_file_release(m->rpipe);
			uk_file_release(m->wpipe);
		}
		uk_free(d->allocator, data->listen.q);
	}
	if (data->flags & UNIXSOCK_CONN && data->remote)
		uk_file_release_weak(data->remote);
	if (data->type == SOCK_DGRAM)
		uk_file_release(data->bpipe);
	uk_free(d->allocator, data);
	return r;
}

static
int unix_socket_ioctl(posix_sock *file __unused, int request __unused,
		      void *argp __unused)
{
	return -ENOSYS;
}


static struct posix_socket_ops unix_posix_socket_ops = {
	/* POSIX interfaces */
	.create      = unix_socket_create,
	.accept4     = unix_socket_accept4,
	.bind        = unix_socket_bind,
	.shutdown    = unix_socket_shutdown,
	.getpeername = unix_socket_getpeername,
	.getsockname = unix_socket_getsockname,
	.getsockopt  = unix_socket_getsockopt,
	.setsockopt  = unix_socket_setsockopt,
	.connect     = unix_socket_connect,
	.listen      = unix_socket_listen,
	.recvfrom    = unix_socket_recvfrom,
	.recvmsg     = unix_socket_recvmsg,
	.sendmsg     = unix_socket_sendmsg,
	.sendto      = unix_socket_sendto,
	.socketpair  = unix_socket_socketpair,
	.socketpair_post = unix_socket_socketpair_post,
	/* vfscore ops */
	.read		= unix_socket_read,
	.write		= unix_socket_write,
	.close		= unix_socket_close,
	.ioctl		= unix_socket_ioctl,
	.poll_setup	= unix_socket_poll_setup,
};

POSIX_SOCKET_FAMILY_REGISTER(AF_UNIX, &unix_posix_socket_ops);

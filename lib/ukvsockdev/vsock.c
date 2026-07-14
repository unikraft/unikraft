/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>

#include <uk/bitops.h>
#include <uk/init.h>
#include <uk/socket_driver.h>
#include <uk/tree.h>
#include <uk/vsock.h>
#include <uk/vsockdev.h>

#include <linux/vm_sockets.h>

int uk_vsock_buffer_init(struct uk_vsock_buffer *buf, struct uk_alloc *a,
			 __sz bufsize)
{
	uk_pr_debug("buf=%p bufsize=%zu\n", buf, bufsize);

	UK_ASSERT(buf);

	buf->buf = uk_malloc(a, bufsize);
	if (unlikely(!buf->buf)) {
		uk_pr_debug("allocation of %zu bytes failed\n", bufsize);
		return -ENOMEM;
	}

	buf->a = a;
	buf->head = 0;
	buf->count = 0;
	buf->capacity = bufsize;
	buf->total_processed = 0;

	uk_spin_init(&buf->lock);

	uk_pr_debug("buffer initialized at %p with capacity %zu\n",
		    buf->buf, bufsize);

	return 0;
}

int uk_vsock_buffer_destroy(struct uk_vsock_buffer *buf)
{
	UK_ASSERT(buf);
	UK_ASSERT(buf->buf);

	uk_spin_lock(&buf->lock);

	uk_pr_debug("buf=%p buf->buf=%p\n", buf, buf->buf);

	uk_free(buf->a, buf->buf);

	uk_spin_unlock(&buf->lock);

	uk_pr_debug("buffer freed\n");

	return 0;
}

int uk_vsock_buffer_append(struct uk_vsock_buffer *buf, struct uk_netbuf **nb)
{
	__sz to_copy, write_pos, appended = 0;
	struct uk_netbuf *nbi;

	uk_spin_lock(&buf->lock);

	UK_NETBUF_CHAIN_FOREACH(nbi, *nb) {
		appended += nbi->len;
		if (unlikely(appended > buf->capacity - buf->count)) {
			uk_spin_unlock(&buf->lock);
			return -ENOSPC;
		}
	}

	appended = 0;
	UK_NETBUF_CHAIN_FOREACH(nbi, *nb) {
		write_pos = (buf->head + buf->count + appended) % buf->capacity;
		to_copy = MIN(nbi->len, buf->capacity - write_pos);
		memcpy(&buf->buf[write_pos], nbi->data, to_copy);
		if (to_copy < nbi->len)
			memcpy(&buf->buf[0], (char *)nbi->data + to_copy,
			       nbi->len - to_copy);
		appended += nbi->len;
	}

	buf->count += appended;

	uk_spin_unlock(&buf->lock);

	return 0;
}

__ssz uk_vsock_buffer_read(struct uk_vsock_buffer *buf, char *out, __sz size)
{
	__sz start, end;
	__sz to_copy;

	uk_spin_lock(&buf->lock);

	uk_pr_debug("buf=%p requested=%zu available=%zu\n",
		    buf, size, buf->count);

	size = MIN(size, buf->count);

	start = buf->head;
	end = start + size;

	to_copy = MIN(end, buf->capacity) - MIN(start, buf->capacity);
	uk_pr_debug("copying %zu bytes from buf[%zu]\n", to_copy, start);
	memcpy(out, &buf->buf[start], to_copy);

	if (end > buf->capacity) {
		uk_pr_debug("wrap-around read: %zu bytes from buf[0]\n",
			    end - start - to_copy);
		memcpy(out + to_copy, &buf->buf[0], end - start - to_copy);
	}

	uk_spin_unlock(&buf->lock);

	uk_pr_debug("returning %zu bytes\n", size);
	return size;
}

/*
 * Lookup structure for socket addresses
 * X = fixed value
 * A = ANY value
 *
 * on pkt input:
 *
 *      RP [peer  cid][peer  port]
 *    LP   [local cid][local port]
 *  - XXXX: Exact connection match?
 *  - _XAA: connection request to listening port [connection req]
 *
 * port bindable check:
 *  - _XAA
 * port allocate check:
 *  - _X__
 *
 * [listen ports]:
 *  - XXAA (or)
 *    AXAA
 * [connected streams]"
 *  - XXXX
 *
 */

#define UK_VSOCK_CONN_ID_MASK_LOCAL_CID		UK_BIT(0)
#define UK_VSOCK_CONN_ID_MASK_LOCAL_PORT	UK_BIT(1)
#define UK_VSOCK_CONN_ID_MASK_PEER_CID		UK_BIT(2)
#define UK_VSOCK_CONN_ID_MASK_PEER_PORT		UK_BIT(3)

struct uk_vsock_conn_id {
	int type;
	struct uk_vsockaddr local;
	struct uk_vsockaddr peer;
	__u8 mask;
};

static int uk_vsock_sockaddr_cmp(struct uk_vsock_conn_id a,
				 struct uk_vsock_conn_id b)
{
	if (a.type != b.type)
		return a.type < b.type ? -1 : 1;
#define CMP_ID(PATH, MASK)						\
	do {								\
		if (a.PATH != b.PATH &&					\
		    !(a.mask & MASK || b.mask & MASK))			\
			return a.PATH < b.PATH ? -1 : 1;		\
	} while (0)

	CMP_ID(local.cid, UK_VSOCK_CONN_ID_MASK_LOCAL_CID);
	CMP_ID(local.port, UK_VSOCK_CONN_ID_MASK_LOCAL_PORT);
	CMP_ID(peer.cid, UK_VSOCK_CONN_ID_MASK_PEER_CID);
	CMP_ID(peer.port, UK_VSOCK_CONN_ID_MASK_PEER_PORT);
#undef CMP_ID
	return 0;
}

static struct uk_vsock_conn_id uk_vsock_sockaddr_key(struct uk_vsock *sock)
{
	return (struct uk_vsock_conn_id){
		.type = sock->sock_type,
		.local = sock->local_addr,
		.peer = sock->peer_addr,
		.mask = 0,
	};
}

/* rb tree that stores the nodes according to their local port number and type
 */
UK_RB_HEAD(uk_vsockets, uk_vsock);
UK_RB_KEY_GENERATE_STATIC(uk_vsockets, uk_vsock, rb_entry,
			  uk_vsock_sockaddr_cmp, uk_vsock_sockaddr_key);

/* All active sockets */
static struct uk_vsockets sockets;
static struct uk_spinlock sockets_lock = UK_SPINLOCK_INITIALIZER();

/* State for port searching (shared across socket types for simplicity) */
static unsigned int port_alloc_next;

int uk_vsockets_init(void)
{
	uk_pr_debug("initializing vsockets RB tree\n");
	UK_RB_INIT(&sockets);
	uk_pr_debug("vsockets RB tree initialized\n");
	return 0;
}

static __s32 vsock_alloc_port(int type)
{
	struct uk_vsock_conn_id search = {
		.type = type,
		.local = {.cid = 0, .port = 0},
		.peer = {.cid = 0, .port = 0},
		.mask = UK_VSOCK_CONN_ID_MASK_LOCAL_CID |
			UK_VSOCK_CONN_ID_MASK_PEER_CID |
			UK_VSOCK_CONN_ID_MASK_PEER_PORT,
	};
	struct uk_vsock *sock;
	unsigned int port;
	long i = 0;

	uk_spin_lock(&sockets_lock);

	uk_pr_debug("allocating port for type=%d starting at %u\n",
		    type, port_alloc_next);

	/* naive search over the ports */
	while (i <= __U32_MAX) {
		search.local.port = port_alloc_next;
		sock = UK_RB_FIND(uk_vsockets, &sockets, search);
		if (!sock) {
			uk_pr_debug("allocated port %u after %ld probes\n",
				    port_alloc_next, i);
			port = port_alloc_next++;
			uk_spin_unlock(&sockets_lock);
			return port;
		}
		uk_pr_debug("port %u already in use, trying next\n",
			    port_alloc_next);
		port_alloc_next++;
		i++;
	}

	uk_pr_debug("no free port found (exhausted after %ld probes)\n", i);
	uk_spin_unlock(&sockets_lock);
	return -EADDRNOTAVAIL;
}

int uk_vsock_init(struct uk_vsock *sock, struct uk_alloc *a, int sock_type,
		  const struct uk_vsock *parent)
{
	int rc;

	uk_pr_debug("sock=%p sock_type=%d parent=%p\n",
		    sock, sock_type, parent);

	memset(sock, 0, sizeof(*sock));

	/* Determine the per-socket buffer sizing. Accepted connections inherit
	 * it from their listening (parent) socket all other sockets use the
	 * configured defaults.
	 * buf_size is always clamped to max_buf_size.
	 */
	if (parent) {
		sock->buf_size = parent->buf_size;
		sock->max_buf_size = parent->max_buf_size;
	} else {
		sock->buf_size = UK_VSOCK_BUF_SIZE;
		sock->max_buf_size = UK_VSOCK_MAX_BUF_SIZE;
	}
	UK_ASSERT(sock->buf_size <= sock->max_buf_size);

	rc = uk_vsock_buffer_init(&sock->rx, a, sock->buf_size);
	if (unlikely(rc)) {
		uk_pr_err("Failed to initialize vsock buffer: %d\n", rc);
		return rc;
	}

	sock->state = UK_VSOCK_STATE_READY;
	sock->sock_type = sock_type;
	sock->posix_sock = NULL;
	sock->so_error = 0;
	UK_STAILQ_INIT(&sock->accept_queue);
	uk_spin_init(&sock->accept_queue_lock);

	uk_pr_debug("sock=%p initialized, state=READY type=%d\n",
		    sock, sock_type);

	return 0;
}

static void uk_vsock_drain_accept_queue(struct uk_vsockdev *dev,
					struct uk_vsock *sock)
{
	struct uk_vsock_accept_entry *entry;

	uk_spin_lock(&sock->accept_queue_lock);
	while (!UK_STAILQ_EMPTY(&sock->accept_queue)) {
		entry = UK_STAILQ_FIRST(&sock->accept_queue);
		UK_STAILQ_REMOVE_HEAD(&sock->accept_queue, entry);
		dev->ops->free_accept_entry(dev, sock, entry);
		sock->accept_queue_length--;
	}
	uk_spin_unlock(&sock->accept_queue_lock);
}

static int uk_vsock_close(struct uk_vsock *sock, int force_reset, __bool locked)
{
	struct uk_vsockdev *dev;
	int rc;

	uk_pr_debug("sock=%p state=%d force_reset=%d\n",
		    sock, sock->state, force_reset);

	dev = uk_vsockdev_get();

	if (force_reset) {
		uk_pr_debug("issuing forced reset on sock=%p\n", sock);
		rc = dev->ops->reset(dev, sock);
		if (unlikely(rc)) {
			uk_pr_warn("Unable to reset connection: %d\n", rc);
			return rc;
		}
		uk_pr_debug("forced reset succeeded\n");
	}

	switch (sock->state) {
	case UK_VSOCK_STATE_OPEN:
		uk_pr_debug("sock=%p is OPEN\n", sock);
		if (!force_reset) {
			/* TODO Technically we have to wait for the RST response
			 *      to really know that the address is reusable
			 *      again.
			 */
			uk_pr_debug("sending graceful shutdown on sock=%p\n",
				    sock);
			rc = dev->ops->shutdown(dev, sock, 1, 1);
			if (unlikely(rc)) {
				uk_pr_warn("Unable to close connection: %d\n",
					   rc);
				return rc;
			}
			uk_pr_debug("shutdown sent successfully\n");
		}
		__fallthrough;
	case UK_VSOCK_STATE_BOUND:
	case UK_VSOCK_STATE_LISTENING:
		/* The address is only known locally, therefore we only need to
		 * remove it from the sockets map
		 */
		uk_pr_debug("removing sock=%p from sockets tree (state=%d)\n",
			    sock, sock->state);
		uk_vsock_drain_accept_queue(dev, sock);

		if (!locked) {
			uk_spin_lock(&sockets_lock);
			UK_RB_REMOVE(uk_vsockets, &sockets, sock);
			uk_spin_unlock(&sockets_lock);
		} else {
			UK_RB_REMOVE(uk_vsockets, &sockets, sock);
		}

		break;
	case UK_VSOCK_STATE_WAIT_CONNECT:
		uk_pr_debug("sock=%p in WAIT_CONNECT\n", sock);
		if (!force_reset) {
			uk_pr_debug("resetting WAIT_CONNECT sock=%p\n", sock);
			rc = dev->ops->reset(dev, sock);
			if (unlikely(rc)) {
				uk_pr_err("Error on connection reset\n");
				return rc;
			}
		}
		uk_pr_debug("removing WAIT_CONNECT sock=%p from tree\n", sock);

		if (!locked) {
			uk_spin_lock(&sockets_lock);
			UK_RB_REMOVE(uk_vsockets, &sockets, sock);
			uk_spin_unlock(&sockets_lock);
		} else {
			UK_RB_REMOVE(uk_vsockets, &sockets, sock);
		}

		break;
	case UK_VSOCK_STATE_CLOSED:
		/* nothing to do */
		uk_pr_debug("sock=%p already in state=%d, nothing to do, returning\n",
			    sock, sock->state);
		return 0;
	case UK_VSOCK_STATE_READY:
		uk_pr_debug("sock=%p in state=%d, nothing to do\n",
			    sock, sock->state);
		break;
	default:
		UK_CRASH("Invalid sock=%p state=%d\n", sock, sock->state);
	}

	sock->state = UK_VSOCK_STATE_CLOSED;
	sock->rx_shutdown = 1;
	sock->tx_shutdown = 1;

	uk_pr_debug("sock=%p transitioned to CLOSED, clearing EPOLLOUT\n",
		    sock);

	posix_sock_event_clear(sock->posix_sock, EPOLLOUT);
	posix_sock_event_set(sock->posix_sock, EPOLLHUP);

	uk_pr_debug("sock=%p close complete\n", sock);

	return 0;
}

static int uk_vsock_destroy(struct uk_vsock *sock)
{
	struct uk_vsockdev *dev;
	int rc;

	uk_pr_debug("sock=%p state=%d\n", sock, sock->state);

	/* Close the socket in case it wasn't closed already */
	rc = uk_vsock_close(sock, 0, __false);
	if (unlikely(rc)) {
		uk_pr_err("Failed to destroy vsock %p: %d\n", sock, rc);
		return rc;
	}

	uk_pr_debug("destroying rx buffer for sock=%p\n", sock);

	rc = uk_vsock_buffer_destroy(&sock->rx);
	if (unlikely(rc)) {
		uk_pr_err("Failed to destroy vsock buffer %p: %d\n",
			  &sock->rx, rc);
		return rc;
	}

	uk_pr_debug("delegating final destroy to driver for sock=%p\n", sock);

	dev = uk_vsockdev_get();
	return dev->ops->destroy(dev, sock);
}

struct uk_vsock *uk_vsock_lookup(int sock_type,
				 struct uk_vsockaddr local,
				 struct uk_vsockaddr peer)
{
	struct uk_vsock *sock;

	uk_pr_debug("type=%d local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    sock_type, local.cid, local.port, peer.cid, peer.port);

	/* Try to find an exact match */
	uk_spin_lock(&sockets_lock);
	sock = UK_RB_FIND(uk_vsockets, &sockets,
			  ((struct uk_vsock_conn_id){
				.type = sock_type,
				.local = local,
				.peer = peer,
				.mask = 0,
			  }));
	if (sock) {
		uk_pr_debug("exact match found sock=%p\n", sock);
		uk_spin_unlock(&sockets_lock);
		return sock;
	}

	uk_pr_debug("no exact match, searching for listening socket\n");

	/* Try to find an listening socket */
	sock = UK_RB_FIND(uk_vsockets, &sockets,
			  ((struct uk_vsock_conn_id){
				.type = sock_type,
				.local = local,
				.peer = {
					.cid = VMADDR_CID_ANY,
					.port = VMADDR_PORT_ANY
				},
				.mask = UK_VSOCK_CONN_ID_MASK_LOCAL_CID,
			  }));

	uk_spin_unlock(&sockets_lock);

	uk_pr_debug("listening socket lookup result: %p\n", sock);

	return sock;
}

void uk_vsock_notify_writable(struct uk_vsock *sock, int writable)
{
	uk_pr_debug("sock=%p writable=%d\n", sock, writable);

	UK_ASSERT(sock);
	if (writable) {
		uk_pr_debug("setting EPOLLOUT on sock=%p\n", sock);
		posix_sock_event_set(sock->posix_sock, EPOLLOUT);
	} else {
		uk_pr_debug("clearing EPOLLOUT on sock=%p\n", sock);
		posix_sock_event_clear(sock->posix_sock, EPOLLOUT);
	}
}

int uk_vsock_rx_payload(struct uk_vsock *sock, struct uk_netbuf **packet)
{
	int rc;

	uk_pr_debug("sock=%p state=%d\n", sock, sock->state);

	if (sock->state != UK_VSOCK_STATE_OPEN) {
		uk_pr_info("Received data on an non-established connection\n");
		return -EINVAL;
	}

	uk_pr_debug("appending packet to rx buffer of sock=%p\n", sock);

	rc = uk_vsock_buffer_append(&sock->rx, packet);
	if (unlikely(rc)) {
		uk_pr_err("Unable to append the packet to the socket buffer: %d\n",
			  rc);
		return rc;
	}

	uk_pr_debug("packet appended, setting EPOLLIN on sock=%p\n", sock);

	UK_ASSERT(sock->posix_sock);
	posix_sock_event_set(sock->posix_sock, EPOLLIN);

	return 0;
}

int uk_vsock_conn_response(struct uk_vsock *sock)
{
	int rc;

	uk_pr_debug("sock=%p state=%d\n", sock, sock->state);

	if (sock->state != UK_VSOCK_STATE_WAIT_CONNECT) {
		uk_pr_debug("sock=%p not in WAIT_CONNECT (state=%d), forcing close\n",
			    sock, sock->state);
		rc = uk_vsock_close(sock, 1, __false);
		if (unlikely(rc))
			uk_pr_err("Failed to close socket %p: %d\n", sock, rc);

		return -EINVAL;
	}

	/* Emit the event without checking the actual buffer size. If we have
	 * no buffer then something is very wrong.
	 */
	UK_ASSERT(sock->posix_sock);

	uk_pr_debug("connection accepted, transitioning sock=%p to OPEN\n",
		    sock);

	sock->state = UK_VSOCK_STATE_OPEN;
	posix_sock_event_set(sock->posix_sock, EPOLLOUT);

	uk_pr_debug("sock=%p is now OPEN\n", sock);

	return 0;
}

int uk_vsock_conn_reset(struct uk_vsock *sock)
{
	struct uk_vsockdev *dev;
	int rc;

	uk_pr_debug("sock=%p state=%d\n", sock, sock->state);

	UK_ASSERT(sock->posix_sock);
	posix_sock_event_clear(sock->posix_sock, EPOLLOUT);
	posix_sock_event_set(sock->posix_sock,
			     EPOLLERR | EPOLLHUP | EPOLLRDHUP);

	switch (sock->state) {
	case UK_VSOCK_STATE_CLOSED:
		uk_pr_debug("sock=%p already CLOSED, nothing to do\n", sock);
		break;
	case UK_VSOCK_STATE_WAIT_CONNECT:
		/* The peer rejected the connection attempt */
		uk_pr_debug("peer rejected connection on sock=%p, setting ECONNREFUSED\n",
			    sock);
		sock->so_error = -ECONNREFUSED;
		dev = uk_vsockdev_get();
		rc = dev->ops->reset(dev, sock);
		if (unlikely(rc)) {
			uk_pr_err("Error on connection reset\n");
			return rc;
		}
		uk_pr_debug("reset issued for WAIT_CONNECT sock=%p\n", sock);
		__fallthrough;
	case UK_VSOCK_STATE_OPEN:
		uk_pr_debug("removing sock=%p from sockets tree, state -> READY\n",
			    sock);
		uk_spin_lock(&sockets_lock);
		UK_RB_REMOVE(uk_vsockets, &sockets, sock);
		uk_spin_unlock(&sockets_lock);
		sock->state = UK_VSOCK_STATE_READY;
		break;
	default:
		uk_pr_debug("Unexpected connection reset in state %d\n",
			    sock->state);
		break;
	}

	uk_pr_debug("reset handling complete for sock=%p\n", sock);

	return 0;
}

int uk_vsock_conn_shutdown(struct uk_vsock *sock, int peer_rx, int peer_tx)
{
	int rc;

	uk_pr_debug("sock=%p state=%d peer_rx=%d peer_tx=%d\n",
		    sock, sock->state, peer_rx, peer_tx);

	if (sock->state != UK_VSOCK_STATE_OPEN) {
		uk_pr_info("Unexpected connection shutdown in state %d\n",
			   sock->state);
		return -EINVAL;
	}

	UK_ASSERT(sock->posix_sock);
	if (peer_rx) {
		/* peer will not receive any more data */
		uk_pr_debug("tx shutdown on sock=%p, clearing EPOLLOUT\n",
			    sock);
		posix_sock_event_clear(sock->posix_sock, EPOLLOUT);
		sock->tx_shutdown = 1;
	}
	if (peer_tx) {
		/* peer will not send any more data */
		uk_pr_debug("rx shutdown on sock=%p, setting EPOLLRDHUP\n",
			    sock);
		posix_sock_event_set(sock->posix_sock, EPOLLRDHUP);
		sock->rx_shutdown = 1;
	}
	if (sock->rx_shutdown && sock->tx_shutdown) {
		uk_pr_debug("both directions shut down on sock=%p, closing\n",
			    sock);
		rc = uk_vsock_close(sock, 0, __false);
		if (unlikely(rc)) {
			uk_pr_err("Failed to close socket %p: %d\n", sock, rc);
			return rc;
		}
	}

	uk_pr_debug("shutdown complete for sock=%p\n", sock);

	return 0;
}

int uk_vsock_conn_request(struct uk_vsock *sock,
			  struct uk_vsock_accept_entry *entry)
{
	uk_spin_lock(&sock->accept_queue_lock);

	uk_pr_debug("sock=%p state=%d queue_len=%d queue_limit=%d\n",
		    sock, sock->state,
		    sock->accept_queue_length, sock->accept_queue_limit);

	/* Ignore connection requests to sockets not in listen state */
	if (sock->state != UK_VSOCK_STATE_LISTENING) {
		uk_pr_debug("sock=%p is not listening (state=%d), refusing\n",
			    sock, sock->state);
		uk_spin_unlock(&sock->accept_queue_lock);
		return -ECONNREFUSED;
	}

	if (sock->accept_queue_length >= sock->accept_queue_limit) {
		uk_pr_debug("accept queue full on sock=%p (%d/%d), refusing\n",
			    sock,
			    sock->accept_queue_length,
			    sock->accept_queue_limit);
		uk_spin_unlock(&sock->accept_queue_lock);
		return -ECONNREFUSED;
	}

	uk_pr_debug("enqueuing accept entry=%p on sock=%p\n", entry, sock);

	UK_STAILQ_INSERT_TAIL(&sock->accept_queue, entry, entry);
	sock->accept_queue_length++;

	uk_pr_debug("queue length now %d, setting EPOLLIN on sock=%p\n",
		    sock->accept_queue_length, sock);

	uk_spin_unlock(&sock->accept_queue_lock);

	UK_ASSERT(sock->posix_sock);
	posix_sock_event_set(sock->posix_sock, EPOLLIN);
	return 0;
}

int uk_vsockets_term(void)
{
	struct uk_vsock *sock, *tmp;
	int rc, ret = 0;

	uk_pr_debug("terminating all vsockets\n");

	uk_spin_lock(&sockets_lock);
	UK_RB_FOREACH_SAFE(sock, uk_vsockets, &sockets, tmp) {
		uk_pr_debug("visiting sock=%p state=%d local={cid=%u port=%u}\n",
			    sock, sock->state,
			    sock->local_addr.cid, sock->local_addr.port);

		switch (sock->state) {
		case UK_VSOCK_STATE_READY:
		case UK_VSOCK_STATE_CLOSED:
			/* Not affected by a transport reset */
			uk_pr_debug("sock=%p in READY/CLOSED, skipping\n",
				    sock);
			continue;

		case UK_VSOCK_STATE_WAIT_CONNECT:
		case UK_VSOCK_STATE_OPEN:
			/* Transport reset invalidates them */
			uk_pr_debug("sock=%p in WAIT_CONNECT/OPEN, will close\n",
				    sock);
			break;

		case UK_VSOCK_STATE_BOUND:
		case UK_VSOCK_STATE_LISTENING:
			/* Bound/listening sockets are fine as long as they
			 * don't listen on a specific CID.
			 */
			if (sock->local_addr.cid == VMADDR_CID_ANY) {
				uk_pr_debug("sock=%p bound to CID_ANY, skipping\n",
					    sock);
				continue;
			}
			uk_pr_debug("sock=%p bound to specific CID %u, will close\n",
				    sock, sock->local_addr.cid);
			break;
		}

		uk_pr_debug("closing sock=%p\n", sock);
		rc = uk_vsock_close(sock, 1, __true);
		if (unlikely(rc)) {
			uk_pr_err("Failed to close sock %p: %d\n", sock, rc);
			ret = rc;
		}
	}
	uk_spin_unlock(&sockets_lock);

	uk_pr_debug("termination complete\n");

	return ret;
}

/*
 * posix-socket implementation
 */

static void *vsock_create(struct posix_socket_driver *d __unused,
			  int family __maybe_unused, int type, int protocol)
{
	struct uk_vsockdev *dev;
	struct uk_vsock *sock;
	int rc;

	uk_pr_debug("family=%d type=%d protocol=%d\n", family, type, protocol);

	UK_ASSERT(family == AF_VSOCK);

	dev = uk_vsockdev_get();
	if (unlikely(!dev)) {
		uk_pr_debug("no vsockdev available\n");
		return ERR2PTR(-EAFNOSUPPORT);
	}

	UK_ASSERT(dev->ops->create);

	uk_pr_debug("calling driver create, dev=%p\n", dev);

	rc = dev->ops->create(dev, type, protocol, &sock);
	if (unlikely(rc)) {
		uk_pr_debug("driver create failed: %d\n", rc);
		return ERR2PTR(rc);
	}

	uk_pr_debug("created sock=%p\n", sock);

	return sock;
}

static void *vsock_accept4(posix_sock *sock, struct sockaddr *restrict addr,
			   socklen_t *restrict addr_len, int flags __unused)
{
	struct uk_vsock_accept_entry *accept_entry;
	struct uk_vsockdev *dev;
	struct sockaddr_vm svm;
	struct uk_vsock *vsock;
	struct uk_vsock *out;
	__sz size;
	int rc;

	uk_pr_debug("sock=%p addr=%p\n", sock, addr);

	if (unlikely(addr && !addr_len)) {
		uk_pr_debug("addr provided but addr_len is NULL\n");
		return ERR2PTR(-EINVAL);
	}

	vsock = posix_sock_get_data(sock);

	uk_spin_lock(&vsock->accept_queue_lock);

	uk_pr_debug("vsock=%p state=%d queue_len=%d\n",
		    vsock, vsock->state, vsock->accept_queue_length);

	if (unlikely(vsock->state != UK_VSOCK_STATE_LISTENING)) {
		uk_pr_debug("vsock=%p not listening (state=%d)\n",
			    vsock, vsock->state);
		uk_spin_unlock(&vsock->accept_queue_lock);
		return ERR2PTR(-EINVAL);
	}

	dev = uk_vsockdev_get();
	UK_ASSERT(dev);

	accept_entry = UK_STAILQ_FIRST(&vsock->accept_queue);
	if (!accept_entry) {
		uk_pr_debug("accept queue empty on vsock=%p\n", vsock);
		uk_spin_unlock(&vsock->accept_queue_lock);
		return ERR2PTR(-EAGAIN);
	}
	UK_ASSERT(vsock->accept_queue_length > 0);

	/* Remove the accept entry from the queue */
	UK_STAILQ_REMOVE(&vsock->accept_queue, accept_entry,
			 struct uk_vsock_accept_entry, entry);
	vsock->accept_queue_length--;

	uk_pr_debug("dequeued entry=%p, queue_len now %d\n",
		    accept_entry, vsock->accept_queue_length);

	uk_spin_unlock(&vsock->accept_queue_lock);

	rc = dev->ops->accept(dev, vsock, accept_entry, &out);
	dev->ops->free_accept_entry(dev, vsock, accept_entry);
	if (unlikely(rc)) {
		uk_pr_debug("driver accept failed: %d\n", rc);
		return ERR2PTR(rc);
	}

	/* Mark the socket as open and register it in the sockets list */
	out->state = UK_VSOCK_STATE_OPEN;

	uk_pr_debug("accepted new sock=%p peer={cid=%u port=%u}, inserting into tree\n",
		    out, out->peer_addr.cid, out->peer_addr.port);

	uk_spin_lock(&sockets_lock);
	UK_RB_INSERT(uk_vsockets, &sockets, out);
	uk_spin_unlock(&sockets_lock);

	if (addr) {
		memset(&svm, 0, sizeof(svm));
		svm.svm_family = AF_VSOCK;
		svm.svm_cid = out->peer_addr.cid;
		svm.svm_port = out->peer_addr.port;
		size = MIN(sizeof(svm), *addr_len);
		memcpy(addr, &svm, size);
		*addr_len = size;
		uk_pr_debug("returning peer addr cid=%u port=%u\n",
			    svm.svm_cid, svm.svm_port);
	}

	uk_spin_lock(&vsock->accept_queue_lock);
	if (UK_STAILQ_EMPTY(&vsock->accept_queue)) {
		uk_pr_debug("accept queue now empty, clearing EPOLLIN on vsock=%p\n",
			    vsock);
		posix_sock_event_clear(sock, EPOLLIN);
	}
	uk_spin_unlock(&vsock->accept_queue_lock);

	uk_pr_debug("accept complete, returning sock=%p\n", out);

	return out;
}

static int vsock_socketpair(struct posix_socket_driver *d __unused,
			    int family __unused, int type __unused,
			    int protocol __unused, void *sockvec[2] __unused)
{
	uk_pr_debug("not supported\n");
	return -EOPNOTSUPP;
}

static int vsock_bind(posix_sock *sock, const struct sockaddr *addr,
		      socklen_t addr_len)
{
	const struct sockaddr_vm *svm;
	unsigned int port, svm_cid;
	struct uk_vsockdev *dev;
	struct uk_vsock *other;
	struct uk_vsock *vsock;
	int rc;

	uk_pr_debug("sock=%p addr_len=%u\n", sock, addr_len);

	dev = uk_vsockdev_get();
	UK_ASSERT(dev);

	if (unlikely(addr_len != sizeof(*svm))) {
		uk_pr_debug("invalid addr_len=%u expected=%zu\n",
			    addr_len, sizeof(*svm));
		return -EINVAL;
	}
	svm = (const struct sockaddr_vm *)addr;
	if (unlikely(svm->svm_family != AF_VSOCK)) {
		uk_pr_debug("wrong family=%u\n", svm->svm_family);
		return -EINVAL;
	}

	uk_pr_debug("requested bind to cid=%u port=%u\n",
		    svm->svm_cid, svm->svm_port);

	svm_cid = dev->ops->get_cid(dev);

	uk_pr_debug("local device CID=%u\n", svm_cid);

	if (unlikely(svm->svm_cid != VMADDR_CID_ANY &&
		     svm->svm_cid != svm_cid)) {
		uk_pr_debug("CID mismatch: requested=%u local=%u\n",
			    svm->svm_cid, svm_cid);
		return -EADDRNOTAVAIL;
	}

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("vsock=%p state=%d\n", vsock, vsock->state);

	if (unlikely(vsock->state != UK_VSOCK_STATE_READY)) {
		uk_pr_debug("vsock=%p not in READY state (state=%d)\n",
			    vsock, vsock->state);
		return -EINVAL;
	}

	vsock->local_addr.cid = svm_cid;
	vsock->peer_addr.cid = VMADDR_CID_ANY;
	vsock->peer_addr.port = VMADDR_PORT_ANY;

	port = svm->svm_port;
	if (port == VMADDR_PORT_ANY) {
		uk_pr_debug("port is ANY, allocating\n");
		rc = vsock_alloc_port(vsock->sock_type);
		if (unlikely(rc < 0)) {
			uk_pr_debug("port allocation failed: %d\n", rc);
			return rc;
		}
		port = rc;

		vsock->local_addr.port = port;

		uk_spin_lock(&sockets_lock);
		UK_RB_INSERT(uk_vsockets, &sockets, vsock);
		uk_spin_unlock(&sockets_lock);

		uk_pr_debug("allocated port=%u\n", port);
	} else {
		uk_pr_debug("checking if port=%u is in use\n", port);

		uk_spin_lock(&sockets_lock);
		other = UK_RB_FIND(uk_vsockets, &sockets,
				   ((struct uk_vsock_conn_id){
					.type = vsock->sock_type,
					.local = {
						.cid = 0,
						.port = port
					},
					.peer = {
						.cid = VMADDR_CID_ANY,
						.port = VMADDR_PORT_ANY
					},
					.mask = UK_VSOCK_CONN_ID_MASK_LOCAL_CID,
				   }));
		if (unlikely(other)) {
			uk_pr_debug("port=%u already in use by sock=%p\n",
				    port, other);
			uk_spin_unlock(&sockets_lock);
			return -EADDRINUSE;
		}

		vsock->local_addr.port = port;

		UK_RB_INSERT(uk_vsockets, &sockets, vsock);

		uk_spin_unlock(&sockets_lock);
	}

	vsock->state = UK_VSOCK_STATE_BOUND;

	uk_pr_debug("vsock=%p bound to cid=%u port=%u, state=BOUND\n",
		    vsock, svm_cid, port);

	return 0;
}

static int vsock_listen(posix_sock *sock, int backlog)
{
	struct sockaddr_vm bind;
	struct uk_vsock *vsock;
	int rc;

	uk_pr_debug("sock=%p backlog=%d\n", sock, backlog);

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("vsock=%p state=%d\n", vsock, vsock->state);

	if (backlog <= 0)
		backlog = 1;
	else
		backlog = MIN(backlog, UK_VSOCK_BACKLOG_MAX);

	if (vsock->state == UK_VSOCK_STATE_LISTENING) {
		/* Allow re-calling listen() to update backlog */
		uk_spin_lock(&vsock->accept_queue_lock);
		vsock->accept_queue_limit = backlog;
		uk_spin_unlock(&vsock->accept_queue_lock);
		return 0;
	}

	if (unlikely(vsock->state != UK_VSOCK_STATE_BOUND &&
		     vsock->state != UK_VSOCK_STATE_READY)) {
		uk_pr_debug("vsock=%p invalid state for listen: %d\n",
			    vsock, vsock->state);
		return -EINVAL;
	}

	if (vsock->state == UK_VSOCK_STATE_READY) {
		uk_pr_debug("vsock=%p not yet bound, performing implicit bind\n",
			    vsock);
		memset(&bind, 0, sizeof(bind));
		bind.svm_family = AF_VSOCK;
		bind.svm_cid = VMADDR_CID_ANY;
		bind.svm_port = VMADDR_PORT_ANY;

		rc = vsock_bind(sock, (const struct sockaddr *)&bind,
				sizeof(bind));
		if (unlikely(rc)) {
			uk_pr_debug("implicit bind failed: %d\n", rc);
			return rc;
		}
		uk_pr_debug("implicit bind succeeded\n");
	}
	UK_ASSERT(vsock->state == UK_VSOCK_STATE_BOUND);

	vsock->state = UK_VSOCK_STATE_LISTENING;

	uk_spin_lock(&vsock->accept_queue_lock);

	vsock->accept_queue_limit = backlog;

	uk_pr_debug("vsock=%p now LISTENING with queue limit=%d\n",
		    vsock, vsock->accept_queue_limit);

	uk_spin_unlock(&vsock->accept_queue_lock);

	return 0;
}

static int vsock_shutdown(posix_sock *sock, int how)
{
	struct uk_vsockdev *dev;
	struct uk_vsock *vsock;
	int rx = 0, tx = 0;
	int rc;

	uk_pr_debug("sock=%p how=%d\n", sock, how);

	switch (how) {
	case SHUT_RD:
		rx = 1;
		break;
	case SHUT_WR:
		tx = 1;
		break;
	case SHUT_RDWR:
		rx = 1;
		tx = 1;
		break;
	default:
		uk_pr_debug("invalid how=%d\n", how);
		return -EINVAL;
	}

	uk_pr_debug("rx=%d tx=%d\n", rx, tx);

	dev = uk_vsockdev_get();
	UK_ASSERT(dev);

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("vsock=%p state=%d\n", vsock, vsock->state);

	if (vsock->state != UK_VSOCK_STATE_OPEN) {
		uk_pr_debug("vsock=%p not in OPEN state (state=%d)\n",
			    vsock, vsock->state);
		return -ENOTCONN;
	}

	uk_pr_debug("calling driver shutdown rx=%d tx=%d\n", rx, tx);

	rc = dev->ops->shutdown(dev, vsock, rx, tx);
	if (unlikely(rc)) {
		uk_pr_debug("driver shutdown failed: %d\n", rc);
		return rc;
	}

	uk_pr_debug("driver shutdown ok, updating connection state\n");

	return uk_vsock_conn_shutdown(vsock, rx, tx);
}

static int vsock_close(posix_sock *sock)
{
	struct uk_vsock *vsock;

	uk_pr_debug("sock=%p\n", sock);

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("destroying vsock=%p state=%d\n", vsock, vsock->state);

	return uk_vsock_destroy(vsock);
}

static int vsock_connect(posix_sock *sock, const struct sockaddr *addr,
			 socklen_t addr_len)
{
	enum uk_vsock_state old_state;
	const struct sockaddr_vm *svm;
	struct uk_vsockaddr old_peer;
	struct uk_vsockdev *dev;
	struct uk_vsock *vsock;
	__s32 port;
	int rc;

	uk_pr_debug("sock=%p addr_len=%u\n", sock, addr_len);

	dev = uk_vsockdev_get();
	UK_ASSERT(dev);

	vsock = posix_sock_get_data(sock);
	old_peer = vsock->peer_addr;

	uk_pr_debug("vsock=%p state=%d\n", vsock, vsock->state);

	if (unlikely(addr_len != sizeof(*svm))) {
		uk_pr_debug("invalid addr_len=%u expected=%zu\n",
			    addr_len, sizeof(*svm));
		return -EINVAL;
	}

	svm = (const struct sockaddr_vm *)addr;
	if (unlikely(svm->svm_family != AF_VSOCK)) {
		uk_pr_debug("wrong family=%u\n", svm->svm_family);
		return -EAFNOSUPPORT;
	}

	uk_pr_debug("connecting to peer cid=%u port=%u\n",
		    svm->svm_cid, svm->svm_port);

	if (vsock->state == UK_VSOCK_STATE_WAIT_CONNECT)
		return -EALREADY;

	if (vsock->state != UK_VSOCK_STATE_BOUND &&
	    vsock->state != UK_VSOCK_STATE_READY) {
		uk_pr_debug("vsock=%p already connected or invalid state=%d\n",
			    vsock, vsock->state);
		return -EISCONN;
	}

	if (unlikely(vsock->state == UK_VSOCK_STATE_BOUND)) {
		/* The peer address changed => reinsert into sockets tree */
		uk_pr_debug("vsock=%p was BOUND, reinserting into tree\n",
			    vsock);

		uk_spin_lock(&sockets_lock);

		vsock->peer_addr.cid = svm->svm_cid;
		vsock->peer_addr.port = svm->svm_port;
		UK_RB_REINSERT(uk_vsockets, &sockets, vsock);

		uk_spin_unlock(&sockets_lock);
	} else {
		/* The address field was not initialized yet */
		vsock->local_addr.cid = dev->ops->get_cid(dev);

		uk_pr_debug("vsock=%p was READY, allocating local port (cid=%u)\n",
			    vsock, vsock->local_addr.cid);

		port = vsock_alloc_port(vsock->sock_type);
		if (unlikely(port < 0)) {
			uk_pr_debug("port allocation failed: %d, undoing peer_addr\n",
				    (int)port);
			/* Undo peer_addr modification before bailing */
			vsock->peer_addr.cid  = VMADDR_CID_ANY;
			vsock->peer_addr.port = VMADDR_PORT_ANY;
			return (int)port;
		}

		vsock->local_addr.port = (unsigned int)port;
		vsock->peer_addr.cid = svm->svm_cid;
		vsock->peer_addr.port = svm->svm_port;

		uk_pr_debug("local address set to cid=%u port=%u, inserting into tree\n",
			    vsock->local_addr.cid, vsock->local_addr.port);

		uk_spin_lock(&sockets_lock);
		UK_RB_INSERT(uk_vsockets, &sockets, vsock);
		uk_spin_unlock(&sockets_lock);
	}

	old_state = vsock->state;
	vsock->state = UK_VSOCK_STATE_WAIT_CONNECT;

	uk_pr_debug("vsock=%p state -> WAIT_CONNECT, calling driver connect\n",
		    vsock);

	rc = dev->ops->connect(dev, vsock);

	uk_pr_debug("driver connect returned %d for vsock=%p\n", rc, vsock);

	if (unlikely(rc && rc != -EINPROGRESS)) {
		uk_pr_debug("connect failed (rc=%d), restoring state=%d\n",
			    rc, old_state);

		if (old_state == UK_VSOCK_STATE_BOUND) {
			/* Restore peer and fix up tree key */
			vsock->peer_addr = old_peer;

			uk_spin_lock(&sockets_lock);
			UK_RB_REINSERT(uk_vsockets, &sockets, vsock);
			uk_spin_unlock(&sockets_lock);
		} else {
			/* READY path: free the allocated port,
			 * remove from tree
			 */
			uk_spin_lock(&sockets_lock);
			UK_RB_REMOVE(uk_vsockets, &sockets, vsock);
			uk_spin_unlock(&sockets_lock);

			vsock->local_addr.cid = VMADDR_CID_ANY;
			vsock->local_addr.port = VMADDR_PORT_ANY;
			vsock->peer_addr = old_peer;
		}

		vsock->state = old_state;
	}

	return rc;
}

static int getsockopt_copy(void *restrict value, size_t value_size,
			   void *restrict optval, socklen_t *restrict optlen)
{
	uk_pr_debug("value=%p value_size=%zu optlen=%u\n",
		    value, value_size, optlen ? *optlen : 0);

	if (unlikely(!optlen))
		return -EINVAL;
	memcpy(optval, value, MIN(value_size, *optlen));
	*optlen = MIN(value_size, *optlen);
	return 0;
}

static int vsock_getsockopt(posix_sock *sock, int level, int optname,
			    void *restrict optval, socklen_t *restrict optlen)
{
	struct uk_vsock *vsock;
	int rc;

	uk_pr_debug("sock=%p level=%d optname=%d\n", sock, level, optname);

	vsock = posix_sock_get_data(sock);

	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		case SO_ERROR:
			uk_pr_debug("SO_ERROR requested, current so_error=%d\n",
				    vsock->so_error);
			rc = getsockopt_copy(&vsock->so_error,
					     sizeof(vsock->so_error),
					     optval, optlen);
			vsock->so_error = 0;
			uk_pr_debug("SO_ERROR cleared\n");
			break;
		default:
			uk_pr_debug("Unsupported socket option name %d\n",
				    optname);
			rc = -ENOPROTOOPT;
			break;
		}
		break;
	default:
		uk_pr_debug("Unsupported socket level %d\n", level);
		rc = -EINVAL;
		break;
	}

	uk_pr_debug("returning rc=%d\n", rc);

	return rc;
}

static int vsock_setsockopt(posix_sock *sock __unused, int level __unused,
			    int optname __unused, const void *optval __unused,
			    socklen_t optlen __unused)
{
	/* We do not support any socket options */
	uk_pr_debug("level=%d optname=%d — not supported\n", level, optname);
	return -ENOPROTOOPT;
}

static int vsock_getsockname(posix_sock *sock, struct sockaddr *restrict addr,
			     socklen_t *restrict addr_len)
{
	struct sockaddr_vm svm;
	struct uk_vsock *vsock;
	__sz size;

	uk_pr_debug("sock=%p\n", sock);

	if (unlikely(!addr_len)) {
		uk_pr_debug("addr_len is NULL\n");
		return -EINVAL;
	}

	vsock = posix_sock_get_data(sock);

	/* Because getsockname does not specify the behaviour when the socket is
	 * not bound locally, we just always return whatever is currently in
	 * cid/port.
	 */

	uk_pr_debug("vsock=%p local_addr={cid=%u port=%u}\n",
		    vsock, vsock->local_addr.cid, vsock->local_addr.port);

	memset(&svm, 0, sizeof(svm));
	svm.svm_family = AF_VSOCK;
	svm.svm_cid = vsock->local_addr.cid;
	svm.svm_port = vsock->local_addr.port;

	size = MIN(*addr_len, sizeof(svm));
	memcpy(addr, &svm, size);
	*addr_len = size;

	uk_pr_debug("returning local addr size=%zu\n", size);

	return 0;
}

static int vsock_getpeername(posix_sock *sock, struct sockaddr *restrict addr,
			     socklen_t *restrict addr_len)
{
	struct sockaddr_vm svm;
	struct uk_vsock *vsock;
	__sz size;

	uk_pr_debug("sock=%p\n", sock);

	if (unlikely(!addr_len)) {
		uk_pr_debug("addr_len is NULL\n");
		return -EINVAL;
	}

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("vsock=%p state=%d peer_addr={cid=%u port=%u}\n",
		    vsock, vsock->state,
		    vsock->peer_addr.cid, vsock->peer_addr.port);

	if (vsock->state != UK_VSOCK_STATE_OPEN) {
		uk_pr_debug("vsock=%p not in OPEN state (state=%d)\n",
			    vsock, vsock->state);
		return -ENOTCONN;
	}

	memset(&svm, 0, sizeof(svm));
	svm.svm_family = AF_VSOCK;
	svm.svm_cid = vsock->peer_addr.cid;
	svm.svm_port = vsock->peer_addr.port;

	size = MIN(*addr_len, sizeof(svm));
	memcpy(addr, &svm, size);
	*addr_len = size;

	uk_pr_debug("returning peer addr size=%zu\n", size);

	return 0;
}

static ssize_t vsock_write(posix_sock *sock,
			   const struct iovec *iov, size_t iovcnt)
{
	struct uk_vsockdev *dev;
	struct uk_vsock *vsock;
	ssize_t written = 0;
	ssize_t res;
	size_t ioi;

	uk_pr_debug("sock=%p iovcnt=%zu\n", sock, iovcnt);

	dev = uk_vsockdev_get();
	UK_ASSERT(dev);

	vsock = posix_sock_get_data(sock);
	UK_ASSERT(vsock);

	/* In order to write to a vsock socket, it must have been opened */
	if (unlikely(vsock->state != UK_VSOCK_STATE_OPEN)) {
		if (vsock->state == UK_VSOCK_STATE_CLOSED ||
		    vsock->tx_shutdown) {
			uk_pr_debug("vsock=%p is closed or tx shut down\n",
				    vsock);
			return -EPIPE;
		}

		return -ENOTCONN;
	}

	for (ioi = 0; ioi < iovcnt; ioi++) {
		uk_pr_debug("sending iov[%zu] base=%p len=%zu\n",
			    ioi, iov[ioi].iov_base, iov[ioi].iov_len);
		res = dev->ops->send(dev, vsock,
				     iov[ioi].iov_base, iov[ioi].iov_len);
		if (unlikely(res < 0)) {
			/* Only return error conditions if we have not written
			 * any data yet.
			 */
			uk_pr_debug("send error on iov[%zu]: %zd\n", ioi, res);
			goto err_out;
		}
		if (res == 0) {
			uk_pr_debug("send returned 0 on iov[%zu], clearing EPOLLOUT\n",
				    ioi);
			res = -EAGAIN;
			posix_sock_event_clear(sock, EPOLLOUT);
			goto err_out;
		}
		uk_pr_debug("sent %zd bytes from iov[%zu]\n", res, ioi);
		written += res;
	}
out:
	uk_pr_debug("total written=%zd\n", written);
	return written;
err_out:
	if (written == 0)
		return res;
	goto out;
}

/* FIXME: Add support for flags */
static ssize_t vsock_sendto(posix_sock *sock, const void *buf, size_t len,
			    int flags __unused,
			    const struct sockaddr *dest_addr, socklen_t addrlen)
{
	struct uk_vsockdev *dev;
	struct uk_vsock *vsock;
	ssize_t res;

	uk_pr_debug("sock=%p len=%zu dest_addr=%p addrlen=%u\n",
		    sock, len, dest_addr, addrlen);

	/* We currently only support connection oriented sockets */
	if (unlikely(dest_addr || addrlen)) {
		uk_pr_debug("dest_addr/addrlen provided on connected socket\n");
		return -EISCONN;
	}

	dev = uk_vsockdev_get();
	UK_ASSERT(dev);

	vsock = posix_sock_get_data(sock);
	UK_ASSERT(vsock);

	if (unlikely(vsock->state == UK_VSOCK_STATE_CLOSED ||
		     vsock->tx_shutdown)) {
		uk_pr_debug("vsock=%p is closed or tx shut down\n", vsock);
		return -EPIPE;
	}

	uk_pr_debug("sending %zu bytes on vsock=%p\n", len, vsock);

	res = dev->ops->send(dev, vsock, buf, len);
	if (!res) {
		uk_pr_debug("send returned 0, clearing EPOLLOUT on vsock=%p\n",
			    vsock);
		posix_sock_event_clear(sock, EPOLLOUT);
		res = -EAGAIN;
	}

	uk_pr_debug("sendto result=%zd\n", res);

	return res;
}

static ssize_t vsock_read(posix_sock *sock,
			  const struct iovec *iov, size_t iovcnt)
{
	struct uk_vsock *vsock;
	ssize_t ret = 0;
	ssize_t res;
	size_t ioi;

	uk_pr_debug("sock=%p iovcnt=%zu\n", sock, iovcnt);

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("vsock=%p state=%d rx_shutdown=%d\n",
		    vsock, vsock->state, vsock->rx_shutdown);

	if (unlikely(vsock->state != UK_VSOCK_STATE_OPEN &&
		     vsock->state != UK_VSOCK_STATE_CLOSED)) {
		uk_pr_debug("vsock=%p not in OPEN/CLOSED state (state=%d)\n",
			    vsock, vsock->state);
		return -ENOTCONN;
	}

	for (ioi = 0; ioi < iovcnt; ioi++) {
		uk_pr_debug("reading into iov[%zu] base=%p len=%zu\n",
			    ioi, iov[ioi].iov_base, iov[ioi].iov_len);
		res = uk_vsock_buffer_read(&vsock->rx,
					   iov[ioi].iov_base, iov[ioi].iov_len);
		if (unlikely(res < 0)) {
			/* Only return error conditions if we have not read any
			 * data yet.
			 */
			uk_pr_debug("buffer read error on iov[%zu]: %zd\n",
				    ioi, res);
			goto err_out;
		}
		if (res == 0) {
			/* Check that we are not in a EOF condition */
			if (!vsock->rx_shutdown) {
				uk_pr_debug("no data and rx not shut down, returning EAGAIN\n");
				res = -EAGAIN;
			} else {
				uk_pr_debug("rx shutdown and buffer empty, EOF\n");
			}
			goto err_out;
		}
		uk_pr_debug("read %zd bytes from iov[%zu], consuming\n",
			    res, ioi);
		uk_vsock_buffer_consume(&vsock->rx, res);
		ret += res;
	}

out:
	if (uk_vsock_buffer_is_empty(&vsock->rx)) {
		uk_pr_debug("rx buffer empty, clearing EPOLLIN on vsock=%p\n",
			    vsock);
		posix_sock_event_clear(sock, EPOLLIN);
	}

	uk_pr_debug("total read=%zd\n", ret);
	return ret;
err_out:
	if (ret == 0)
		ret = res;
	goto out;
}

/* FIXME: Add support for flags */
static ssize_t vsock_recvfrom(posix_sock *sock, void *restrict buf, size_t len,
			      int flags __unused, struct sockaddr *from,
			      socklen_t *restrict fromlen)
{
	struct sockaddr_vm svm;
	struct uk_vsock *vsock;
	ssize_t res;
	size_t size;

	uk_pr_debug("sock=%p len=%zu from=%p\n", sock, len, from);

	vsock = posix_sock_get_data(sock);

	uk_pr_debug("vsock=%p state=%d rx_shutdown=%d\n",
		    vsock, vsock->state, vsock->rx_shutdown);

	if (unlikely(vsock->state != UK_VSOCK_STATE_OPEN &&
		     vsock->state != UK_VSOCK_STATE_CLOSED)) {
		uk_pr_debug("vsock=%p not in OPEN/CLOSED state (state=%d)\n",
			    vsock, vsock->state);
		return -ENOTCONN;
	}

	/* Fill in peer address if the caller asked for it */
	if (from && fromlen) {
		memset(&svm, 0, sizeof(svm));
		svm.svm_family = AF_VSOCK;
		svm.svm_cid = vsock->peer_addr.cid;
		svm.svm_port = vsock->peer_addr.port;

		size = MIN(*fromlen, sizeof(svm));
		memcpy(from, &svm, size);
		*fromlen = size;
	}

	res = uk_vsock_buffer_read(&vsock->rx, buf, len);

	uk_pr_debug("buffer read returned %zd\n", res);

	if (!res && !vsock->rx_shutdown) {
		uk_pr_debug("no data and rx not shut down, returning EAGAIN\n");
		return -EAGAIN;
	} else if (res > 0) {
		uk_pr_debug("consuming %zd bytes from rx buffer\n", res);
		uk_vsock_buffer_consume(&vsock->rx, res);
	}

	if (uk_vsock_buffer_is_empty(&vsock->rx)) {
		uk_pr_debug("rx buffer empty, clearing EPOLLIN on vsock=%p\n",
			    vsock);
		posix_sock_event_clear(sock, EPOLLIN);
	}

	uk_pr_debug("recvfrom returning %zd\n", res);

	return res;
}

static int vsock_ioctl(posix_sock *sock __unused, int request __unused,
		       void *argp __unused)
{
	uk_pr_debug("ioctl not supported\n");
	return -ENOTTY;
}

static void vsock_poll_setup(posix_sock *sock)
{
	struct uk_vsock *vsock;

	uk_pr_debug("sock=%p\n", sock);

	vsock = posix_sock_get_data(sock);
	vsock->posix_sock = sock;

	uk_pr_debug("vsock=%p linked to posix_sock=%p\n", vsock, sock);
}

static struct posix_socket_ops socket_vsock_ops = {
	.create = vsock_create,
	.accept4 = vsock_accept4,
	.socketpair = vsock_socketpair,

	.bind = vsock_bind,
	.listen = vsock_listen,
	.connect = vsock_connect,

	.shutdown = vsock_shutdown,
	.getsockname = vsock_getsockname,
	.getpeername = vsock_getpeername,

	.getsockopt = vsock_getsockopt,
	.setsockopt = vsock_setsockopt,

	.recvfrom = vsock_recvfrom,
	.recvmsg = NULL,
	.sendto = vsock_sendto,
	.sendmsg = NULL,

	.write = vsock_write,
	.read = vsock_read,
	.close = vsock_close,
	.ioctl = vsock_ioctl,
	.poll_setup = vsock_poll_setup,
};

POSIX_SOCKET_FAMILY_REGISTER(AF_VSOCK, &socket_vsock_ops);

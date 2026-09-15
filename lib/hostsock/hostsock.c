/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors. */

/*
 * hostsock — Unikraft socket driver backed by Hyperlight host functions.
 *
 * Each socket operation makes a synchronous host call via hl_hcall_*.
 * The host manages the real sockets; the guest holds the host-side id
 * plus what it takes to re-create the socket after a snapshot restore
 * (struct hostsock, hostsock_resume).
 *
 * Registers for AF_INET (2) and AF_INET6 (10).
 */

#include <uk/essentials.h>
#include <uk/alloc.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/list.h>

#include <uk/socket_driver.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <hyperlight-x86/hcall.h>

/*
 * Static I/O buffers — too large for the stack (64 KB each would overflow
 * musl's 80 KB default thread stack).  Single vCPU + spinlock in the hcall
 * path guarantees exclusive access.
 */
static __u8 g_recv_buf[4 + 7 + 16 + 65536]; /* header + addr + data */
static __u8 g_send_buf[65536];

/*
 * Maximum payload for a single hcall send — the FlatBuffer encoder
 * (g_generic_fc_buf) is 64 KiB, of which ~256 bytes are headers/metadata.
 * Cap user data at 60 KiB to leave headroom.  sendall() loops call
 * send() repeatedly, so the kernel returning a short count is correct.
 */
#define HCALL_SEND_MAX  (60 * 1024)

/* ── Tracked sockets for poll rescan ─────────────────────────────── */

#define MAX_TRACKED 64
static posix_sock *tracked_socks[MAX_TRACKED];
static int tracked_count;

static void hostsock_track(posix_sock *sock)
{
	for (int i = 0; i < tracked_count; i++)
		if (tracked_socks[i] == sock)
			return;
	if (tracked_count < MAX_TRACKED) {
		tracked_socks[tracked_count++] = sock;
	} else {
		uk_pr_warn("hostsock: too many tracked sockets (%d), "
			   "socket won't participate in idle-loop wakeups\n",
			   MAX_TRACKED);
	}
}

static void hostsock_untrack(posix_sock *sock)
{
	for (int i = 0; i < tracked_count; i++) {
		if (tracked_socks[i] == sock) {
			tracked_socks[i] = tracked_socks[--tracked_count];
			return;
		}
	}
}

/* ── Per-socket state ─────────────────────────────────────────────── */

/*
 * What the guest knows about one of its host sockets.  The host holds the
 * real socket; this is what it takes to put an equivalent one back after
 * the guest is restored from a snapshot in a host that no longer has it
 * (see hostsock_resume()).
 */
enum hostsock_state {
	HS_FRESH,	/* created, not bound */
	HS_BOUND,	/* bind() done */
	HS_LISTENING,	/* listen() done */
	HS_CONNECTED,	/* connect() done, or returned by accept() */
	HS_DEAD,	/* a connection whose host end is gone: EOF / EPIPE */
};

struct hostsock {
	int host_fd;			/* host-side id; -1 once HS_DEAD */
	int family, type, protocol;	/* socket() arguments */
	enum hostsock_state state;
	struct sockaddr_storage local;	/* bind() address */
	socklen_t local_len;
	struct sockaddr_storage peer;	/* connect() / accept() peer */
	socklen_t peer_len;
	int backlog;			/* listen() argument */
	int reuseaddr, reuseport;	/* SOL_SOCKET options to replay */
	struct uk_list_head list;	/* on hostsock_all */
};

/* Every live socket, in creation order, for hostsock_resume(). */
static UK_LIST_HEAD(hostsock_all);

static inline struct hostsock *hs_of(posix_sock *sock)
{
	return (struct hostsock *)posix_sock_get_data(sock);
}

static inline int sock_fd(posix_sock *sock)
{
	return hs_of(sock)->host_fd;
}

static inline int sock_dead(posix_sock *sock)
{
	return hs_of(sock)->state == HS_DEAD;
}

static struct hostsock *hs_new(int host_fd, int family, int type,
			       int protocol)
{
	struct hostsock *hs;

	hs = uk_calloc(uk_alloc_get_default(), 1, sizeof(*hs));
	if (unlikely(!hs))
		return NULL;
	hs->host_fd = host_fd;
	hs->family = family;
	hs->type = type;
	hs->protocol = protocol;
	hs->state = HS_FRESH;
	uk_list_add_tail(&hs->list, &hostsock_all);
	return hs;
}

static void hs_free(struct hostsock *hs)
{
	uk_list_del(&hs->list);
	uk_free(uk_alloc_get_default(), hs);
}

static void hs_remember(struct sockaddr_storage *dst, socklen_t *dst_len,
			const struct sockaddr *addr, socklen_t addrlen)
{
	if (!addr || addrlen == 0) {
		*dst_len = 0;
		return;
	}
	if (addrlen > sizeof(*dst))
		addrlen = sizeof(*dst);
	memcpy(dst, addr, addrlen);
	*dst_len = addrlen;
}

static int hs_recall(const struct sockaddr_storage *src, socklen_t src_len,
		     struct sockaddr *addr, socklen_t *addrlen)
{
	if (!src_len)
		return -ENOTCONN;
	if (addr && addrlen) {
		socklen_t n = *addrlen < src_len ? *addrlen : src_len;

		memcpy(addr, src, n);
		*addrlen = src_len;
	}
	return 0;
}

/* ── Helpers ──────────────────────────────────────────────────────── */

/* Format an IPv4 address as "d.d.d.d". */
static void fmt_ipv4(char *buf, size_t sz, const struct in_addr *addr)
{
	const unsigned char *b = (const unsigned char *)addr;

	snprintf(buf, sz, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
}

/* Format an IPv6 address in abbreviated hex notation. */
static void fmt_ipv6(char *buf, size_t sz, const struct in6_addr *addr)
{
	const unsigned char *b = (const unsigned char *)addr;

	snprintf(buf, sz,
		 "%x:%x:%x:%x:%x:%x:%x:%x",
		 (b[0] << 8) | b[1], (b[2] << 8) | b[3],
		 (b[4] << 8) | b[5], (b[6] << 8) | b[7],
		 (b[8] << 8) | b[9], (b[10] << 8) | b[11],
		 (b[12] << 8) | b[13], (b[14] << 8) | b[15]);
}

/*
 * Format a sockaddr into (family, addr_string, port) for the host.
 * Returns 0 on success, -errno on failure.
 */
static int format_addr(const struct sockaddr *addr, socklen_t addrlen,
		       int *family, char *addrbuf, size_t addrbuf_sz,
		       int *port)
{
	if (addr->sa_family == AF_INET) {
		const struct sockaddr_in *sin =
			(const struct sockaddr_in *)addr;

		if (addrlen < sizeof(*sin))
			return -EINVAL;

		*family = AF_INET;
		*port = ntohs(sin->sin_port);
		fmt_ipv4(addrbuf, addrbuf_sz, &sin->sin_addr);
		return 0;
	} else if (addr->sa_family == AF_INET6) {
		const struct sockaddr_in6 *sin6 =
			(const struct sockaddr_in6 *)addr;

		if (addrlen < sizeof(*sin6))
			return -EINVAL;

		*family = AF_INET6;
		*port = ntohs(sin6->sin6_port);
		fmt_ipv6(addrbuf, addrbuf_sz, &sin6->sin6_addr);
		return 0;
	}

	return -EAFNOSUPPORT;
}

/*
 * Decode a packed address from a host result buffer into a sockaddr.
 *
 * Packed format at buf[off..]:
 *   i32 family, u16 port, u8 addr_len, [addr_len] addr bytes
 *
 * Returns number of bytes consumed, or 0 on error.
 */
static size_t unpack_addr(const __u8 *buf, size_t len, size_t off,
			  struct sockaddr *addr, socklen_t *addrlen)
{
	if (off + 7 > len)
		return 0;

	int32_t family = buf[off] | (buf[off+1] << 8) |
			 (buf[off+2] << 16) | (buf[off+3] << 24);
	uint16_t port = buf[off+4] | (buf[off+5] << 8);
	uint8_t alen = buf[off+6];

	if (off + 7 + alen > len)
		return 0;

	if (family == AF_INET && alen == 4) {
		struct sockaddr_in *sin = (struct sockaddr_in *)addr;

		if (addrlen && *addrlen < sizeof(*sin))
			return 0;

		memset(sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
		sin->sin_port = htons(port);
		memcpy(&sin->sin_addr, buf + off + 7, 4);
		if (addrlen)
			*addrlen = sizeof(*sin);
		return 7 + alen;
	} else if (family == AF_INET6 && alen == 16) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr;

		if (addrlen && *addrlen < sizeof(*sin6))
			return 0;

		memset(sin6, 0, sizeof(*sin6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = htons(port);
		memcpy(&sin6->sin6_addr, buf + off + 7, 16);
		if (addrlen)
			*addrlen = sizeof(*sin6);
		return 7 + alen;
	}

	return 0;
}

static inline __s32 rd_i32(const __u8 *b, size_t off)
{
	return (__s32)(b[off] | (b[off+1] << 8) |
		      (b[off+2] << 16) | (b[off+3] << 24));
}

/*
 * Check if a host socket has pending events using net_poll(timeout=0).
 *
 * This is critical for intra-guest networking: a blocking host call
 * (accept, recv) freezes the single vCPU, preventing other guest
 * threads from running.  By polling with timeout=0 first and returning
 * -EAGAIN when not ready, we let Unikraft's scheduler yield to other
 * threads that can make progress (e.g. a client connecting).
 */
static int hostsock_check_ready(int host_fd, int events)
{
	/* Pack one pollfd: i32 fd + i16 events + i16 pad = 8 bytes */
	__u8 req[8];
	__u8 resp[6]; /* i32 retval + i16 revents */
	__sz resp_len;

	req[0] = host_fd & 0xFF;
	req[1] = (host_fd >> 8) & 0xFF;
	req[2] = (host_fd >> 16) & 0xFF;
	req[3] = (host_fd >> 24) & 0xFF;
	req[4] = events & 0xFF;
	req[5] = (events >> 8) & 0xFF;
	req[6] = 0;
	req[7] = 0;

	struct hl_param p[2];

	p[0].type = HL_PV_HLVECBYTES;
	p[0].vec.ptr = req;
	p[0].vec.len = sizeof(req);
	p[1].type = HL_PV_HLINT;
	p[1].i32_val = 0; /* timeout_ms = 0 (non-blocking) */

	if (hl_hcall_vecbytes("net_poll", p, 2,
			      resp, sizeof(resp), &resp_len) < 0)
		return 0;

	if (resp_len < 6)
		return 0;

	/* retval at [0..4], revents at [4..6] */
	__s32 retval = rd_i32(resp, 0);

	if (retval <= 0)
		return 0;

	return (int)(__s16)(resp[4] | (resp[5] << 8));
}

static unsigned hostsock_publish_events(posix_sock *sock, int revents,
				       int mask);

/* An error condition on a socket makes it "ready" in every direction:
 * poll(2) semantics, and what lets a blocked recv/send return the real
 * error (or EOF) instead of waiting for data that can never arrive.  The
 * host reports POLLNVAL for a socket it no longer has -- every connection
 * that was open when a guest was snapshotted and restored elsewhere -- so
 * without this a server parked in recv() on such a connection hangs
 * forever and never gets back to accept().
 */
#define HOSTSOCK_ERR	(POLLERR | POLLHUP | POLLNVAL)
#define HOSTSOCK_READY_IN	(POLLIN | HOSTSOCK_ERR)
#define HOSTSOCK_READY_OUT	(POLLOUT | HOSTSOCK_ERR)

/*
 * Ask the host whether @sock is ready for @events (POLLIN and/or POLLOUT)
 * and publish the answer as the socket's guest-side readiness state.
 *
 * Every readiness check must go through here, not just the idle-loop
 * rescan: the poll layer waits on the published bits, and they are
 * level-triggered state that only the host can refresh.  A bit left set
 * after the host said "not ready" is a livelock -- a listener whose only
 * pending connection was just accepted keeps POLLIN, so select() returns
 * at once, accept() asks the host, gets EAGAIN, and round it goes, one
 * host call per iteration, without the scheduler ever going idle.
 */
static int hostsock_ready(posix_sock *sock, int events)
{
	int revents;

	/* A dead connection is ready for everything: the read returns EOF
	 * and the write EPIPE, exactly what a parked thread must see.
	 */
	if (sock_dead(sock))
		revents = POLLIN | POLLOUT | POLLHUP;
	else
		revents = hostsock_check_ready(sock_fd(sock), events);
	hostsock_publish_events(sock, revents, events);
	return revents;
}

/* ── Host calls shared by the socket ops and hostsock_resume() ───── */

/* net_socket: a new host socket, or -errno. */
static int hc_socket(int family, int type, int protocol)
{
	struct hl_param p[3];
	__s32 ret;

	p[0].type = HL_PV_HLINT; p[0].i32_val = family;
	p[1].type = HL_PV_HLINT; p[1].i32_val = type;
	p[2].type = HL_PV_HLINT; p[2].i32_val = protocol;
	if (hl_hcall_int("net_socket", p, 3, &ret) < 0)
		return -EIO;
	return ret;
}

static int hc_close(int host_fd)
{
	struct hl_param p[1];
	__s32 ret;

	p[0].type = HL_PV_HLINT; p[0].i32_val = host_fd;
	if (hl_hcall_int("net_close", p, 1, &ret) < 0)
		return -EIO;
	return 0;
}

static int hc_setsockopt(int host_fd, int level, int optname, int value)
{
	struct hl_param p[4];
	__s32 ret;

	p[0].type = HL_PV_HLINT; p[0].i32_val = host_fd;
	p[1].type = HL_PV_HLINT; p[1].i32_val = level;
	p[2].type = HL_PV_HLINT; p[2].i32_val = optname;
	p[3].type = HL_PV_HLINT; p[3].i32_val = value;
	if (hl_hcall_int("net_setsockopt", p, 4, &ret) < 0)
		return -EIO;
	return ret < 0 ? ret : 0;
}

/* net_bind / net_connect: both take (fd, family, addr, port). */
static int hc_addr_op(const char *op, int host_fd,
		      const struct sockaddr *addr, socklen_t addrlen)
{
	int family, port;
	char addrbuf[64];
	struct hl_param p[4];
	__s32 ret;
	int err;

	err = format_addr(addr, addrlen, &family, addrbuf, sizeof(addrbuf),
			  &port);
	if (err)
		return err;
	p[0].type = HL_PV_HLINT;    p[0].i32_val = host_fd;
	p[1].type = HL_PV_HLINT;    p[1].i32_val = family;
	p[2].type = HL_PV_HLSTRING; p[2].str.ptr = addrbuf;
				     p[2].str.len = strlen(addrbuf);
	p[3].type = HL_PV_HLINT;    p[3].i32_val = port;
	if (hl_hcall_int(op, p, 4, &ret) < 0)
		return -EIO;
	return ret < 0 ? ret : 0;
}

static int hc_listen(int host_fd, int backlog)
{
	struct hl_param p[2];
	__s32 ret;

	p[0].type = HL_PV_HLINT; p[0].i32_val = host_fd;
	p[1].type = HL_PV_HLINT; p[1].i32_val = backlog;
	if (hl_hcall_int("net_listen", p, 2, &ret) < 0)
		return -EIO;
	return ret < 0 ? ret : 0;
}

/* ── Socket operations ───────────────────────────────────────────── */

static void *
hostsock_create(struct posix_socket_driver *d __unused,
		int family, int type, int protocol)
{
	struct hostsock *hs;
	int host_fd = hc_socket(family, type, protocol);

	if (host_fd < 0)
		return ERR2PTR(host_fd);
	hs = hs_new(host_fd, family, type, protocol);
	if (unlikely(!hs)) {
		hc_close(host_fd);
		return ERR2PTR(-ENOMEM);
	}
	return hs;
}

static int
hostsock_bind(posix_sock *sock,
	      const struct sockaddr *addr, socklen_t addrlen)
{
	struct hostsock *hs = hs_of(sock);
	int err;

	if (hs->state == HS_DEAD)
		return -EPIPE;
	err = hc_addr_op("net_bind", hs->host_fd, addr, addrlen);
	if (err)
		return err;
	hs_remember(&hs->local, &hs->local_len, addr, addrlen);
	if (hs->state == HS_FRESH)
		hs->state = HS_BOUND;
	return 0;
}

static int
hostsock_listen(posix_sock *sock, int backlog)
{
	struct hostsock *hs = hs_of(sock);
	int err;

	if (hs->state == HS_DEAD)
		return -EPIPE;
	err = hc_listen(hs->host_fd, backlog);
	if (err)
		return err;
	hs->backlog = backlog;
	hs->state = HS_LISTENING;
	return 0;
}

static void *
hostsock_accept4(posix_sock *sock,
		 struct sockaddr *restrict addr,
		 socklen_t *restrict addrlen,
		 int flags __unused)
{
	struct hostsock *parent = hs_of(sock), *hs;
	struct sockaddr_storage peer;
	socklen_t peer_len = sizeof(peer);

	/* A listener that could not be re-created after a restore. */
	if (parent->state == HS_DEAD)
		return ERR2PTR(-EIO);

	/*
	 * Check readiness before calling the host's accept.  A blocking
	 * host call freezes the entire VM (single vCPU), so we must never
	 * let accept block on the host side.  Return EAGAIN and let
	 * Unikraft's poll/scheduler layer handle the wait.
	 */
	{
		int ready = hostsock_ready(sock, POLLIN);

		if (!(ready & HOSTSOCK_READY_IN))
			return ERR2PTR(-EAGAIN);
	}

	struct hl_param p[1];
	__u8 buf[64];
	__sz len;

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = parent->host_fd;

	if (hl_hcall_vecbytes("net_accept", p, 1, buf, sizeof(buf), &len) < 0)
		return ERR2PTR(-EIO);

	if (len < 4)
		return ERR2PTR(-EIO);

	__s32 new_fd = rd_i32(buf, 0);

	if (new_fd < 0)
		return ERR2PTR(new_fd);

	/* Keep the peer: it is all getpeername() can answer once the host
	 * end is gone after a restore.
	 */
	if (len > 4)
		unpack_addr(buf, len, 4, (struct sockaddr *)&peer, &peer_len);
	else
		peer_len = 0;
	if (addr && addrlen && peer_len) {
		socklen_t n = *addrlen < peer_len ? *addrlen : peer_len;

		memcpy(addr, &peer, n);
		*addrlen = peer_len;
	}

	hs = hs_new(new_fd, parent->family, parent->type, parent->protocol);
	if (unlikely(!hs)) {
		hc_close(new_fd);
		return ERR2PTR(-ENOMEM);
	}
	hs->state = HS_CONNECTED;
	hs_remember(&hs->peer, &hs->peer_len, (struct sockaddr *)&peer,
		    peer_len);
	return hs;
}

static int
hostsock_connect(posix_sock *sock,
		 const struct sockaddr *addr, socklen_t addrlen)
{
	struct hostsock *hs = hs_of(sock);
	int err;

	if (hs->state == HS_DEAD)
		return -EPIPE;
	err = hc_addr_op("net_connect", hs->host_fd, addr, addrlen);
	if (err)
		return err;
	hs_remember(&hs->peer, &hs->peer_len, addr, addrlen);
	hs->state = HS_CONNECTED;
	return 0;
}

static int
hostsock_shutdown(posix_sock *sock, int how)
{
	struct hl_param p[2];
	__s32 ret;

	if (sock_dead(sock))
		return 0;

	p[0].type = HL_PV_HLINT; p[0].i32_val = sock_fd(sock);
	p[1].type = HL_PV_HLINT; p[1].i32_val = how;

	if (hl_hcall_int("net_shutdown", p, 2, &ret) < 0)
		return -EIO;

	return ret < 0 ? ret : 0;
}

static ssize_t
hostsock_sendto(posix_sock *sock, const void *buf, size_t len,
		int flags __unused,
		const struct sockaddr *dest_addr, socklen_t addrlen)
{
	if (sock_dead(sock))
		return -EPIPE;

	/*
	 * Check writability before calling the host's send/sendto.
	 * A blocking send on a full socket buffer freezes the VM's
	 * single vCPU, preventing the receiver thread from draining
	 * the buffer.  Return EAGAIN and let the scheduler yield.
	 */
	{
		int ready = hostsock_ready(sock, POLLOUT);

		if (!(ready & HOSTSOCK_READY_OUT))
			return -EAGAIN;
	}

	/* Cap to hcall buffer capacity — sendall() loops will retry. */
	if (len > HCALL_SEND_MAX)
		len = HCALL_SEND_MAX;

	if (dest_addr) {
		/* sendto with destination */
		int family, port;
		char addrbuf[64];
		int err = format_addr(dest_addr, addrlen, &family, addrbuf,
				      sizeof(addrbuf), &port);
		if (err)
			return err;

		struct hl_param p[5];
		__s32 ret;

		p[0].type = HL_PV_HLINT;     p[0].i32_val = sock_fd(sock);
		p[1].type = HL_PV_HLVECBYTES; p[1].vec.ptr = (const __u8 *)buf;
					       p[1].vec.len = len;
		p[2].type = HL_PV_HLINT;      p[2].i32_val = family;
		p[3].type = HL_PV_HLSTRING;   p[3].str.ptr = addrbuf;
					       p[3].str.len = strlen(addrbuf);
		p[4].type = HL_PV_HLINT;      p[4].i32_val = port;

		if (hl_hcall_int("net_sendto", p, 5, &ret) < 0)
			return -EIO;

		return ret;
	} else {
		/* send (no destination) */
		struct hl_param p[2];
		__s32 ret;

		p[0].type = HL_PV_HLINT;     p[0].i32_val = sock_fd(sock);
		p[1].type = HL_PV_HLVECBYTES; p[1].vec.ptr = (const __u8 *)buf;
					       p[1].vec.len = len;

		if (hl_hcall_int("net_send", p, 2, &ret) < 0)
			return -EIO;

		return ret;
	}
}

static ssize_t
hostsock_recvfrom(posix_sock *sock, void *restrict buf, size_t len,
		  int flags __unused,
		  struct sockaddr *from, socklen_t *restrict fromlen)
{
	/* The host end went away with the process that was checkpointed:
	 * end of file, the same as a peer that closed.
	 */
	if (sock_dead(sock))
		return 0;

	/*
	 * Check readiness — a blocking recv host call would freeze the
	 * entire VM.  See the comment in hostsock_accept4.
	 */
	{
		int ready = hostsock_ready(sock, POLLIN);

		if (!(ready & HOSTSOCK_READY_IN))
			return -EAGAIN;
	}

	struct hl_param p[2];
	__sz rlen;

	p[0].type = HL_PV_HLINT; p[0].i32_val = sock_fd(sock);
	p[1].type = HL_PV_HLINT; p[1].i32_val = len > 65536 ? 65536 : len;

	if (hl_hcall_vecbytes("net_recvfrom", p, 2,
			      g_recv_buf, sizeof(g_recv_buf), &rlen) < 0)
		return -EIO;

	if (rlen < 4)
		return -EIO;

	__s32 nbytes = rd_i32(g_recv_buf, 0);

	if (nbytes < 0)
		return nbytes;

	/* Decode the source address. */
	size_t addr_consumed = 0;

	if (rlen > 4 && from && fromlen)
		addr_consumed = unpack_addr(g_recv_buf, rlen, 4, from, fromlen);
	else if (rlen > 4)
		/* Skip the addr even if caller doesn't want it. */
		addr_consumed = 7 + (rlen > 10 ? g_recv_buf[10] : 0);

	/* Copy received data. */
	size_t data_off = 4 + addr_consumed;
	size_t data_avail = rlen > data_off ? rlen - data_off : 0;
	size_t copy = data_avail < len ? data_avail : len;

	if (copy > 0)
		memcpy(buf, g_recv_buf + data_off, copy);

	return (ssize_t)copy;
}

static ssize_t
hostsock_write(posix_sock *sock, const struct iovec *iov, size_t iovcnt)
{
	if (sock_dead(sock))
		return -EPIPE;

	/* Check writability — same guard as sendto. */
	{
		int ready = hostsock_ready(sock, POLLOUT);

		if (!(ready & HOSTSOCK_READY_OUT))
			return -EAGAIN;
	}

	/* Gather all iovecs into a single buffer and call net_send. */
	size_t total = 0;

	for (size_t i = 0; i < iovcnt; i++)
		total += iov[i].iov_len;

	if (total > sizeof(g_send_buf)) {
		uk_pr_warn("hostsock: write truncated from %zu to %zu bytes\n",
			   total, sizeof(g_send_buf));
		total = sizeof(g_send_buf);
	}

	size_t off = 0;

	for (size_t i = 0; i < iovcnt && off < total; i++) {
		size_t chunk = iov[i].iov_len;

		if (off + chunk > total)
			chunk = total - off;
		memcpy(g_send_buf + off, iov[i].iov_base, chunk);
		off += chunk;
	}

	struct hl_param p[2];
	__s32 ret;

	p[0].type = HL_PV_HLINT;      p[0].i32_val = sock_fd(sock);
	p[1].type = HL_PV_HLVECBYTES; p[1].vec.ptr = g_send_buf;
				       p[1].vec.len = off;

	if (hl_hcall_int("net_send", p, 2, &ret) < 0)
		return -EIO;

	return ret;
}

static ssize_t
hostsock_read(posix_sock *sock, const struct iovec *iov, size_t iovcnt)
{
	if (iovcnt == 0)
		return 0;

	/* Read into the first iovec only, clamped to its actual size.
	 * POSIX allows short reads, so the caller retries as needed. */
	return hostsock_recvfrom(sock, iov[0].iov_base, iov[0].iov_len,
				 0, NULL, NULL);
}

static int
hostsock_close(posix_sock *sock)
{
	struct hostsock *hs = hs_of(sock);
	int err = 0;

	hostsock_untrack(sock);
	if (hs->state != HS_DEAD)
		err = hc_close(hs->host_fd);
	hs_free(hs);
	return err;
}

static int
hostsock_getpeername(posix_sock *sock,
		    struct sockaddr *restrict addr,
		    socklen_t *restrict addrlen)
{
	struct hl_param p[1];
	__u8 buf[32];
	__sz len;

	if (sock_dead(sock)) {
		struct hostsock *hs = hs_of(sock);

		return hs_recall(&hs->peer, hs->peer_len, addr, addrlen);
	}

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = sock_fd(sock);

	if (hl_hcall_vecbytes("net_getpeername", p, 1,
			      buf, sizeof(buf), &len) < 0)
		return -EIO;

	if (len < 4)
		return -EIO;

	__s32 status = rd_i32(buf, 0);

	if (status < 0)
		return status;

	if (addr && addrlen)
		unpack_addr(buf, len, 4, addr, addrlen);

	return 0;
}

static int
hostsock_getsockname(posix_sock *sock,
		     struct sockaddr *restrict addr,
		     socklen_t *restrict addrlen)
{
	struct hl_param p[1];
	__u8 buf[32];
	__sz len;

	if (sock_dead(sock)) {
		struct hostsock *hs = hs_of(sock);

		return hs_recall(&hs->local, hs->local_len, addr, addrlen);
	}

	p[0].type = HL_PV_HLINT;
	p[0].i32_val = sock_fd(sock);

	if (hl_hcall_vecbytes("net_getsockname", p, 1,
			      buf, sizeof(buf), &len) < 0)
		return -EIO;

	if (len < 4)
		return -EIO;

	__s32 status = rd_i32(buf, 0);

	if (status < 0)
		return status;

	if (addr && addrlen)
		unpack_addr(buf, len, 4, addr, addrlen);

	return 0;
}

static int
hostsock_getsockopt(posix_sock *sock, int level, int optname,
		    void *restrict optval, socklen_t *restrict optlen)
{
	struct hl_param p[3];
	__s32 ret;

	/* Nothing left to ask about; report no pending error. */
	if (sock_dead(sock)) {
		if (optval && optlen && *optlen >= sizeof(int)) {
			*(int *)optval = 0;
			*optlen = sizeof(int);
		}
		return 0;
	}

	p[0].type = HL_PV_HLINT; p[0].i32_val = sock_fd(sock);
	p[1].type = HL_PV_HLINT; p[1].i32_val = level;
	p[2].type = HL_PV_HLINT; p[2].i32_val = optname;

	if (hl_hcall_int("net_getsockopt", p, 3, &ret) < 0)
		return -EIO;

	if (ret < 0)
		return ret;

	/* Write the option value back. */
	if (optval && optlen && *optlen >= sizeof(int)) {
		*(int *)optval = ret;
		*optlen = sizeof(int);
	}

	return 0;
}

static int
hostsock_setsockopt(posix_sock *sock, int level, int optname,
		    const void *optval, socklen_t optlen)
{
	struct hostsock *hs = hs_of(sock);
	int value = 0;

	if (optval && optlen >= sizeof(int))
		value = *(const int *)optval;
	if (hs->state == HS_DEAD)
		return 0;
	/* Remembered so a re-created listener gets them again. */
	if (level == SOL_SOCKET && optname == SO_REUSEADDR)
		hs->reuseaddr = value;
	else if (level == SOL_SOCKET && optname == SO_REUSEPORT)
		hs->reuseport = value;

	struct hl_param p[4];
	__s32 ret;

	p[0].type = HL_PV_HLINT; p[0].i32_val = sock_fd(sock);
	p[1].type = HL_PV_HLINT; p[1].i32_val = level;
	p[2].type = HL_PV_HLINT; p[2].i32_val = optname;
	p[3].type = HL_PV_HLINT; p[3].i32_val = value;

	if (hl_hcall_int("net_setsockopt", p, 4, &ret) < 0)
		return -EIO;

	return ret < 0 ? ret : 0;
}

static ssize_t
hostsock_sendmsg(posix_sock *sock, const struct msghdr *msg, int flags)
{
	/* Flatten msg_iov and delegate to sendto. */
	if (msg->msg_iovlen == 0)
		return 0;

	size_t total = 0;

	for (size_t i = 0; i < (size_t)msg->msg_iovlen; i++)
		total += msg->msg_iov[i].iov_len;

	if (total > sizeof(g_send_buf)) {
		uk_pr_warn("hostsock: sendmsg truncated from %zu to %zu bytes\n",
			   total, sizeof(g_send_buf));
		total = sizeof(g_send_buf);
	}

	size_t off = 0;

	for (size_t i = 0; i < (size_t)msg->msg_iovlen && off < total; i++) {
		size_t chunk = msg->msg_iov[i].iov_len;

		if (off + chunk > total)
			chunk = total - off;
		memcpy(g_send_buf + off, msg->msg_iov[i].iov_base, chunk);
		off += chunk;
	}

	return hostsock_sendto(sock, g_send_buf, off, flags,
			       msg->msg_name, msg->msg_namelen);
}

static ssize_t
hostsock_recvmsg(posix_sock *sock, struct msghdr *msg, int flags)
{
	/* Receive into first iovec. */
	if (msg->msg_iovlen == 0)
		return 0;

	socklen_t addrlen = msg->msg_namelen;

	return hostsock_recvfrom(sock,
				msg->msg_iov[0].iov_base,
				msg->msg_iov[0].iov_len,
				flags,
				msg->msg_name, &addrlen);
}

static int
hostsock_ioctl(posix_sock *sock __unused, int request __unused,
	       void *argp __unused)
{
	return -ENOTSUP;
}

/*
 * Publish the host's answer as the socket's readiness state.
 *
 * The host poll result is authoritative and level-triggered, so a bit
 * that came back clear must be *cleared*, not merely left alone.  Only
 * ever setting bits latches a socket readable forever: POLLIN goes up
 * on the first connection or byte and never comes down, so poll() and
 * select() keep reporting the fd ready, the caller retries accept/recv,
 * hostsock re-checks with the host, gets "not ready" and returns EAGAIN
 * -- a livelock that never reaches the idle loop.  A listener that has
 * just accepted its only pending connection hits this every time.
 *
 * Clear first: a set is what wakes waiters, so raising the new edge
 * last avoids a spurious wake on a bit about to be dropped.
 *
 * Only the bits in @mask -- the ones the host was actually asked about --
 * are touched, so a POLLIN-only check leaves POLLOUT as it was.
 *
 * Returns the events now set, so callers can tell whether this socket
 * has anything to wake on.
 */
static unsigned hostsock_publish_events(posix_sock *sock, int revents,
				       int mask)
{
	unsigned want = ((mask & POLLIN) ? UKFD_POLLIN : 0)
		      | ((mask & POLLOUT) ? UKFD_POLLOUT : 0);
	unsigned set = (((revents & POLLIN) ? UKFD_POLLIN : 0)
		      | ((revents & POLLOUT) ? UKFD_POLLOUT : 0)) & want;
	unsigned clr;

	/* Error, hang-up or a vanished socket: wake every waiter so the
	 * operation it retries reports the condition (see HOSTSOCK_ERR).
	 */
	if (revents & HOSTSOCK_ERR)
		set = want;
	clr = want & ~set;

	if (clr)
		posix_sock_event_clear(sock, clr);
	if (set)
		posix_sock_event_set(sock, set);

	return set;
}

static void
hostsock_poll_setup(posix_sock *sock)
{
	/* Check real readiness via host poll(timeout=0). */
	hostsock_ready(sock, POLLIN | POLLOUT);
	hostsock_track(sock);
}

/*
 * Rescan all tracked sockets for readiness.  Called from the
 * platform's idle loop (and by the cooperative step pump on every
 * re-entry) so Unikraft's scheduler can wake threads that are blocked
 * on socket I/O.
 *
 * Returns 1 if any socket has pending events, 0 otherwise.
 */
int hostsock_rescan_events(void)
{
	int woke = 0;

	for (int i = 0; i < tracked_count; i++) {
		posix_sock *sock = tracked_socks[i];
		int revents;

		/* Published once when it died; nothing more can happen. */
		if (sock_dead(sock))
			continue;
		revents = hostsock_check_ready(sock_fd(sock), POLLIN | POLLOUT);
		if (hostsock_publish_events(sock, revents, POLLIN | POLLOUT))
			woke = 1;
	}

	return woke;
}

/* ── Snapshot restore ────────────────────────────────────────────── */

/* Open, bind and listen a socket equal to @hs on the fresh host, with
 * the reuse options the guest had set plus SO_REUSEADDR, since the old
 * incarnation's connections may linger in TIME_WAIT on the same port.
 */
static void hs_recreate(struct hostsock *hs)
{
	int id = hc_socket(hs->family, hs->type, hs->protocol);

	if (id < 0)
		goto dead;
	if (hs->reuseaddr || hs->state != HS_FRESH)
		hc_setsockopt(id, SOL_SOCKET, SO_REUSEADDR, 1);
	if (hs->reuseport)
		hc_setsockopt(id, SOL_SOCKET, SO_REUSEPORT, 1);
	if (hs->state != HS_FRESH &&
	    hc_addr_op("net_bind", id, (struct sockaddr *)&hs->local,
		       hs->local_len) < 0)
		goto close_dead;
	if (hs->state == HS_LISTENING && hc_listen(id, hs->backlog) < 0)
		goto close_dead;
	hs->host_fd = id;
	return;

close_dead:
	hc_close(id);
dead:
	uk_pr_err("hostsock: cannot re-create a %s socket after restore\n",
		  hs->state == HS_LISTENING ? "listening" :
		  hs->state == HS_BOUND ? "bound" : "fresh");
	hs->host_fd = -1;
	hs->state = HS_DEAD;
}

/*
 * The host has restored this guest from a snapshot.  Every host socket
 * the guest held belonged to the process that took the snapshot, so:
 *
 *   listeners and bound sockets are opened, bound and listened again
 *   under the same address -- a server keeps accepting without knowing
 *   anything happened;
 *
 *   unbound sockets are opened again;
 *
 *   connections cannot come back, their peers are gone with the old host,
 *   so they become HS_DEAD: reads return EOF, writes EPIPE, getpeername()
 *   still answers, and any thread parked on one is woken so a server gets
 *   back to accept().
 */
void hostsock_resume(void)
{
	struct hostsock *hs;

	uk_list_for_each_entry(hs, &hostsock_all, list) {
		switch (hs->state) {
		case HS_CONNECTED:
			hs->host_fd = -1;
			hs->state = HS_DEAD;
			break;
		case HS_FRESH:
		case HS_BOUND:
		case HS_LISTENING:
			hs_recreate(hs);
			break;
		case HS_DEAD:
			break;
		}
	}
	/* Wake whoever is parked on a connection that just died. */
	for (int i = 0; i < tracked_count; i++)
		if (sock_dead(tracked_socks[i]))
			hostsock_publish_events(tracked_socks[i],
						POLLIN | POLLOUT | POLLHUP,
						POLLIN | POLLOUT);
}

/* ── Driver registration ─────────────────────────────────────────── */

static const struct posix_socket_ops hostsock_ops = {
	.init          = NULL,
	.create        = hostsock_create,
	.accept4       = hostsock_accept4,
	.bind          = hostsock_bind,
	.shutdown      = hostsock_shutdown,
	.getpeername   = hostsock_getpeername,
	.getsockname   = hostsock_getsockname,
	.getsockopt    = hostsock_getsockopt,
	.setsockopt    = hostsock_setsockopt,
	.connect       = hostsock_connect,
	.listen        = hostsock_listen,
	.recvfrom      = hostsock_recvfrom,
	.recvmsg       = hostsock_recvmsg,
	.sendmsg       = hostsock_sendmsg,
	.sendto        = hostsock_sendto,
	.socketpair    = NULL,
	.socketpair_post = NULL,
	.write         = hostsock_write,
	.read          = hostsock_read,
	.close         = hostsock_close,
	.ioctl         = hostsock_ioctl,
#if CONFIG_LIBPOSIX_SOCKET_POLLED
	.poll          = NULL,
#endif
	.poll_setup    = hostsock_poll_setup,
};

/* Register for AF_INET */
POSIX_SOCKET_FAMILY_REGISTER(AF_INET, &hostsock_ops);

/* Register for AF_INET6 */
POSIX_SOCKET_FAMILY_REGISTER(AF_INET6, &hostsock_ops);

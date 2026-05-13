/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * hostsock: host-proxied AF_INET socket driver for Hyperlight
 *
 * Every POSIX socket operation is forwarded to the Hyperlight host via
 * the __dispatch RPC interface. The host performs real networking on
 * behalf of the guest. Binary payloads are base64-encoded in JSON.
 *
 * Copyright (c) 2025, Microsoft Corporation. All rights reserved.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/socket_driver.h>

#include <hyperlight-x86/hcall.h>

/* Max single RPC payload (matches HCALL_MAX_PAYLOAD). */
#define HOSTSOCK_RPC_MAX 65536

/* Per-socket driver data. */
struct hostsock_data {
	uint32_t host_fd;
};

/* Static per-call buffers. Not thread-safe; callers serialise via
 * posix-socket and vfscore locking. */
static char rpc_req[HOSTSOCK_RPC_MAX];
static char rpc_resp[HOSTSOCK_RPC_MAX];

/* -------- base64 codec (same as hostfs) -------- */

static const char b64_enc[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t b64_encoded_len(size_t n)
{
	return ((n + 2) / 3) * 4;
}

static size_t b64_encode(const void *src_, size_t n, char *dst)
{
	const unsigned char *src = src_;
	size_t i, o = 0;

	for (i = 0; i + 3 <= n; i += 3) {
		unsigned v = ((unsigned)src[i] << 16) |
			     ((unsigned)src[i + 1] << 8) |
			     (unsigned)src[i + 2];
		dst[o++] = b64_enc[(v >> 18) & 0x3F];
		dst[o++] = b64_enc[(v >> 12) & 0x3F];
		dst[o++] = b64_enc[(v >> 6) & 0x3F];
		dst[o++] = b64_enc[v & 0x3F];
	}
	if (i < n) {
		unsigned v = (unsigned)src[i] << 16;
		if (i + 1 < n)
			v |= (unsigned)src[i + 1] << 8;
		dst[o++] = b64_enc[(v >> 18) & 0x3F];
		dst[o++] = b64_enc[(v >> 12) & 0x3F];
		dst[o++] = (i + 1 < n) ? b64_enc[(v >> 6) & 0x3F] : '=';
		dst[o++] = '=';
	}
	return o;
}

static int b64_val(char c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

static long b64_decode(const char *src, size_t n, void *dst_, size_t cap)
{
	unsigned char *dst = dst_;
	size_t i = 0, o = 0;
	int v[4], k;

	while (n > 0 && (src[n-1] == '\n' || src[n-1] == '\r'
			 || src[n-1] == ' ' || src[n-1] == '\t'))
		n--;

	while (i + 4 <= n) {
		for (k = 0; k < 4; k++) {
			char c = src[i + k];
			if (c == '=')
				v[k] = -2;
			else {
				v[k] = b64_val(c);
				if (v[k] < 0)
					return -1;
			}
		}
		i += 4;
		unsigned bits = ((unsigned)(v[0] < 0 ? 0 : v[0]) << 18) |
				((unsigned)(v[1] < 0 ? 0 : v[1]) << 12) |
				((unsigned)(v[2] < 0 ? 0 : v[2]) << 6) |
				(unsigned)(v[3] < 0 ? 0 : v[3]);
		if (o >= cap) return -1;
		dst[o++] = (bits >> 16) & 0xFF;
		if (v[2] != -2) {
			if (o >= cap) return -1;
			dst[o++] = (bits >> 8) & 0xFF;
		}
		if (v[3] != -2) {
			if (o >= cap) return -1;
			dst[o++] = bits & 0xFF;
		}
	}
	if (i != n) return -1;
	return (long)o;
}

/* -------- IPv4 address helpers (no arpa/inet.h in nolibc) -------- */

static void ipv4_ntop(const struct in_addr *src, char *dst, size_t sz)
{
	const unsigned char *b = (const unsigned char *)&src->s_addr;
	snprintf(dst, sz, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
}

static int ipv4_pton(const char *src, struct in_addr *dst)
{
	unsigned a, b, c, d;
	if (sscanf(src, "%u.%u.%u.%u", &a, &b, &c, &d) != 4)
		return 0;
	unsigned char *p = (unsigned char *)&dst->s_addr;
	p[0] = a; p[1] = b; p[2] = c; p[3] = d;
	return 1;
}

/* -------- tiny JSON helpers (same pattern as hostfs) -------- */

static const char *json_scan_key(const char *json, const char *key)
{
	char needle[64];
	int n = snprintf(needle, sizeof(needle), "\"%s\"", key);
	if (n < 0 || (size_t)n >= sizeof(needle))
		return NULL;
	const char *p = strstr(json, needle);
	if (!p) return NULL;
	p += n;
	while (*p == ' ' || *p == '\t' || *p == ':')
		p++;
	return p;
}

static int json_has_error(const char *json)
{
	return strstr(json, "\"error\"") != NULL;
}

static int json_errno(const char *json)
{
	const char *p = json_scan_key(json, "error");
	if (!p || *p != '"')
		return -EIO;
	p++;
	if (strstr(p, "AddrInUse") || strstr(p, "address already in use"))
		return -EADDRINUSE;
	if (strstr(p, "AddrNotAvail"))
		return -EADDRNOTAVAIL;
	if (strstr(p, "ConnectionRefused") || strstr(p, "refused"))
		return -ECONNREFUSED;
	if (strstr(p, "ConnectionReset") || strstr(p, "reset"))
		return -ECONNRESET;
	if (strstr(p, "ConnectionAborted"))
		return -ECONNABORTED;
	if (strstr(p, "NotConnected"))
		return -ENOTCONN;
	if (strstr(p, "TimedOut") || strstr(p, "timed out"))
		return -ETIMEDOUT;
	if (strstr(p, "WouldBlock"))
		return -EWOULDBLOCK;
	if (strstr(p, "permission") || strstr(p, "Permission"))
		return -EACCES;
	if (strstr(p, "InvalidInput") || strstr(p, "Invalid argument"))
		return -EINVAL;
	if (strstr(p, "bad_fd"))
		return -EBADF;
	return -EIO;
}

static int json_get_int(const char *json, const char *key, long long *out)
{
	const char *p = json_scan_key(json, key);
	if (!p) return -1;
	char *end;
	long long v = strtoll(p, &end, 10);
	if (end == p) return -1;
	*out = v;
	return 0;
}

static int json_get_string(const char *json, const char *key,
			   char *out, size_t cap)
{
	const char *p = json_scan_key(json, key);
	if (!p || *p != '"') return -1;
	p++;
	size_t o = 0;
	while (*p && *p != '"') {
		if (o + 1 >= cap) return -1;
		if (*p == '\\' && p[1]) {
			out[o++] = p[1];
			p += 2;
		} else {
			out[o++] = *p++;
		}
	}
	out[o] = '\0';
	return (int)o;
}

/* -------- RPC round-trip -------- */

static int rpc_exchange(size_t req_len, size_t *resp_len)
{
	__sz rlen = 0;
	int rc = hyperlight_hcall((const __u8 *)rpc_req, req_len,
				  (__u8 *)rpc_resp,
				  sizeof(rpc_resp) - 1, &rlen);
	if (rc < 0) {
		uk_pr_err("hostsock: hcall failed: %d\n", rc);
		return -EIO;
	}
	rpc_resp[rlen] = '\0';
	if (resp_len) *resp_len = rlen;
	if (json_has_error(rpc_resp))
		return json_errno(rpc_resp);
	return 0;
}

static int build_req(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(rpc_req, sizeof(rpc_req), fmt, ap);
	va_end(ap);
	if (n < 0 || (size_t)n >= sizeof(rpc_req))
		return -ENOMEM;
	return n;
}

/* -------- sockaddr helpers -------- */

static void sockaddr_to_json(const struct sockaddr *addr, socklen_t len,
			     char *buf, size_t cap)
{
	if (addr->sa_family == AF_INET && len >= sizeof(struct sockaddr_in)) {
		const struct sockaddr_in *in = (const struct sockaddr_in *)addr;
		char ip[INET_ADDRSTRLEN];
		ipv4_ntop(&in->sin_addr, ip, sizeof(ip));
		snprintf(buf, cap, "\"addr\":\"%s\",\"port\":%u",
			 ip, ntohs(in->sin_port));
	} else {
		snprintf(buf, cap, "\"addr\":\"0.0.0.0\",\"port\":0");
	}
}

static void json_to_sockaddr(const char *json, struct sockaddr *addr,
			     socklen_t *addr_len)
{
	char ip[64];
	long long port = 0;
	if (json_get_string(json, "addr", ip, sizeof(ip)) < 0)
		return;
	json_get_int(json, "port", &port);

	struct sockaddr_in *in = (struct sockaddr_in *)addr;
	memset(in, 0, sizeof(*in));
	in->sin_family = AF_INET;
	in->sin_port = htons((uint16_t)port);
	ipv4_pton(ip, &in->sin_addr);
	if (addr_len)
		*addr_len = sizeof(struct sockaddr_in);
}

/* -------- socket operations -------- */

static uint32_t get_host_fd(posix_sock *sock)
{
	struct hostsock_data *d = posix_sock_get_data(sock);
	return d->host_fd;
}

static void *hostsock_create(struct posix_socket_driver *d,
			     int family, int type, int protocol)
{
	int sock_type = type & ~SOCK_FLAGS;
	int n = build_req(
		"{\"name\":\"net_socket\",\"args\":"
		"{\"family\":%d,\"type\":%d,\"protocol\":%d}}",
		family, sock_type, protocol);
	if (n < 0)
		return ERR2PTR(-ENOMEM);

	int rc = rpc_exchange(n, NULL);
	if (rc < 0)
		return ERR2PTR(rc);

	long long fd = -1;
	if (json_get_int(rpc_resp, "fd", &fd) < 0)
		return ERR2PTR(-EIO);

	struct hostsock_data *sd = uk_malloc(d->allocator, sizeof(*sd));
	if (!sd)
		return ERR2PTR(-ENOMEM);
	sd->host_fd = (uint32_t)fd;

	uk_pr_debug("hostsock: create fd=%u (family=%d type=%d)\n",
		    sd->host_fd, family, sock_type);
	return sd;
}

static int hostsock_bind(posix_sock *sock,
			 const struct sockaddr *addr, socklen_t addr_len)
{
	char abuf[128];
	sockaddr_to_json(addr, addr_len, abuf, sizeof(abuf));
	int n = build_req(
		"{\"name\":\"net_bind\",\"args\":{\"fd\":%u,%s}}",
		get_host_fd(sock), abuf);
	if (n < 0) return -ENOMEM;
	return rpc_exchange(n, NULL);
}

static int hostsock_listen(posix_sock *sock, int backlog)
{
	int n = build_req(
		"{\"name\":\"net_listen\",\"args\":{\"fd\":%u,\"backlog\":%d}}",
		get_host_fd(sock), backlog);
	if (n < 0) return -ENOMEM;
	return rpc_exchange(n, NULL);
}

static void *hostsock_accept4(posix_sock *sock,
			      struct sockaddr *restrict addr,
			      socklen_t *restrict addr_len,
			      int flags __attribute__((unused)))
{
	int n = build_req(
		"{\"name\":\"net_accept\",\"args\":{\"fd\":%u}}",
		get_host_fd(sock));
	if (n < 0) return ERR2PTR(-ENOMEM);

	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return ERR2PTR(rc);

	long long new_fd = -1;
	if (json_get_int(rpc_resp, "fd", &new_fd) < 0)
		return ERR2PTR(-EIO);

	struct posix_socket_driver *drv = posix_sock_get_driver(sock);
	struct hostsock_data *sd = uk_malloc(drv->allocator, sizeof(*sd));
	if (!sd)
		return ERR2PTR(-ENOMEM);
	sd->host_fd = (uint32_t)new_fd;

	if (addr && addr_len)
		json_to_sockaddr(rpc_resp, addr, addr_len);

	uk_pr_debug("hostsock: accept -> fd=%u\n", sd->host_fd);
	return sd;
}

static int hostsock_connect(posix_sock *sock,
			    const struct sockaddr *addr, socklen_t addr_len)
{
	char abuf[128];
	sockaddr_to_json(addr, addr_len, abuf, sizeof(abuf));
	int n = build_req(
		"{\"name\":\"net_connect\",\"args\":{\"fd\":%u,%s}}",
		get_host_fd(sock), abuf);
	if (n < 0) return -ENOMEM;
	return rpc_exchange(n, NULL);
}

static ssize_t hostsock_sendto(posix_sock *sock,
			       const void *buf, size_t len, int flags,
			       const struct sockaddr *dest_addr,
			       socklen_t addrlen)
{
	size_t enc_len = b64_encoded_len(len);
	char abuf[128] = {0};
	if (dest_addr && addrlen > 0)
		sockaddr_to_json(dest_addr, addrlen, abuf, sizeof(abuf));

	/* Build header */
	int n;
	if (abuf[0]) {
		n = snprintf(rpc_req, sizeof(rpc_req),
			"{\"name\":\"net_sendto\",\"args\":"
			"{\"fd\":%u,\"flags\":%d,%s,\"data\":\"",
			get_host_fd(sock), flags, abuf);
	} else {
		n = snprintf(rpc_req, sizeof(rpc_req),
			"{\"name\":\"net_send\",\"args\":"
			"{\"fd\":%u,\"flags\":%d,\"data\":\"",
			get_host_fd(sock), flags);
	}
	if (n < 0 || (size_t)n + enc_len + 4 >= sizeof(rpc_req))
		return -EMSGSIZE;

	enc_len = b64_encode(buf, len, rpc_req + n);
	n += (int)enc_len;
	memcpy(rpc_req + n, "\"}}", 3);
	n += 3;

	int rc = rpc_exchange((size_t)n, NULL);
	if (rc < 0) return rc;

	long long sent = 0;
	if (json_get_int(rpc_resp, "sent", &sent) < 0)
		return -EIO;
	return (ssize_t)sent;
}

static ssize_t hostsock_recvfrom(posix_sock *sock,
				 void *restrict buf, size_t len, int flags,
				 struct sockaddr *from,
				 socklen_t *restrict fromlen)
{
	int n = build_req(
		"{\"name\":\"net_recvfrom\",\"args\":"
		"{\"fd\":%u,\"len\":%zu,\"flags\":%d}}",
		get_host_fd(sock), len, flags);
	if (n < 0) return -ENOMEM;

	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;

	/* Extract base64 data */
	const char *p = strstr(rpc_resp, "\"data\":\"");
	if (!p) return -EIO;
	p += 8;
	const char *end = strchr(p, '"');
	if (!end) return -EIO;

	long decoded = b64_decode(p, (size_t)(end - p), buf, len);
	if (decoded < 0) return -EIO;

	if (from && fromlen)
		json_to_sockaddr(rpc_resp, from, fromlen);

	return (ssize_t)decoded;
}

static ssize_t hostsock_sendmsg(posix_sock *sock,
				const struct msghdr *msg, int flags)
{
	size_t total = 0;
	for (size_t i = 0; i < (size_t)msg->msg_iovlen; i++)
		total += msg->msg_iov[i].iov_len;

	/* Flatten iovecs into a contiguous buffer if needed */
	if (msg->msg_iovlen == 1) {
		return hostsock_sendto(sock, msg->msg_iov[0].iov_base,
				      msg->msg_iov[0].iov_len, flags,
				      msg->msg_name, msg->msg_namelen);
	}

	/* Multi-iovec: copy into temp buffer */
	char tmp[HOSTSOCK_RPC_MAX / 2];
	if (total > sizeof(tmp))
		return -EMSGSIZE;
	size_t off = 0;
	for (size_t i = 0; i < (size_t)msg->msg_iovlen; i++) {
		memcpy(tmp + off, msg->msg_iov[i].iov_base,
		       msg->msg_iov[i].iov_len);
		off += msg->msg_iov[i].iov_len;
	}
	return hostsock_sendto(sock, tmp, total, flags,
			       msg->msg_name, msg->msg_namelen);
}

static ssize_t hostsock_recvmsg(posix_sock *sock,
				struct msghdr *msg, int flags)
{
	size_t total = 0;
	for (size_t i = 0; i < (size_t)msg->msg_iovlen; i++)
		total += msg->msg_iov[i].iov_len;

	if (msg->msg_iovlen == 1) {
		return hostsock_recvfrom(sock, msg->msg_iov[0].iov_base,
					msg->msg_iov[0].iov_len, flags,
					msg->msg_name, &msg->msg_namelen);
	}

	char tmp[HOSTSOCK_RPC_MAX / 2];
	if (total > sizeof(tmp))
		return -EMSGSIZE;

	ssize_t got = hostsock_recvfrom(sock, tmp, total, flags,
					msg->msg_name, &msg->msg_namelen);
	if (got <= 0)
		return got;

	/* Scatter into iovecs */
	size_t off = 0;
	for (size_t i = 0; i < (size_t)msg->msg_iovlen && off < (size_t)got;
	     i++) {
		size_t chunk = msg->msg_iov[i].iov_len;
		if (chunk > (size_t)got - off)
			chunk = (size_t)got - off;
		memcpy(msg->msg_iov[i].iov_base, tmp + off, chunk);
		off += chunk;
	}
	return got;
}

static int hostsock_shutdown(posix_sock *sock, int how)
{
	int n = build_req(
		"{\"name\":\"net_shutdown\",\"args\":{\"fd\":%u,\"how\":%d}}",
		get_host_fd(sock), how);
	if (n < 0) return -ENOMEM;
	return rpc_exchange(n, NULL);
}

static int hostsock_getpeername(posix_sock *sock,
				struct sockaddr *restrict addr,
				socklen_t *restrict addr_len)
{
	int n = build_req(
		"{\"name\":\"net_getpeername\",\"args\":{\"fd\":%u}}",
		get_host_fd(sock));
	if (n < 0) return -ENOMEM;
	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;
	json_to_sockaddr(rpc_resp, addr, addr_len);
	return 0;
}

static int hostsock_getsockname(posix_sock *sock,
				struct sockaddr *restrict addr,
				socklen_t *restrict addr_len)
{
	int n = build_req(
		"{\"name\":\"net_getsockname\",\"args\":{\"fd\":%u}}",
		get_host_fd(sock));
	if (n < 0) return -ENOMEM;
	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;
	json_to_sockaddr(rpc_resp, addr, addr_len);
	return 0;
}

static int hostsock_getsockopt(posix_sock *sock,
			       int level, int optname,
			       void *restrict optval,
			       socklen_t *restrict optlen)
{
	int n = build_req(
		"{\"name\":\"net_getsockopt\",\"args\":"
		"{\"fd\":%u,\"level\":%d,\"optname\":%d}}",
		get_host_fd(sock), level, optname);
	if (n < 0) return -ENOMEM;
	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;

	long long val = 0;
	if (json_get_int(rpc_resp, "value", &val) == 0) {
		if (optlen && *optlen >= sizeof(int)) {
			*(int *)optval = (int)val;
			*optlen = sizeof(int);
		}
	}
	return 0;
}

static int hostsock_setsockopt(posix_sock *sock,
			       int level, int optname,
			       const void *optval, socklen_t optlen)
{
	int val = 0;
	if (optval && optlen >= sizeof(int))
		val = *(const int *)optval;

	int n = build_req(
		"{\"name\":\"net_setsockopt\",\"args\":"
		"{\"fd\":%u,\"level\":%d,\"optname\":%d,\"value\":%d}}",
		get_host_fd(sock), level, optname, val);
	if (n < 0) return -ENOMEM;
	return rpc_exchange(n, NULL);
}

static ssize_t hostsock_read(posix_sock *sock,
			     const struct iovec *iov, size_t iovcnt)
{
	if (iovcnt == 0)
		return 0;
	/* Read into first iovec only for simplicity */
	return hostsock_recvfrom(sock, iov[0].iov_base, iov[0].iov_len,
				 0, NULL, NULL);
}

static ssize_t hostsock_write(posix_sock *sock,
			      const struct iovec *iov, size_t iovcnt)
{
	if (iovcnt == 0)
		return 0;
	return hostsock_sendto(sock, iov[0].iov_base, iov[0].iov_len,
			       0, NULL, 0);
}

static int hostsock_close(posix_sock *sock)
{
	struct hostsock_data *sd = posix_sock_get_data(sock);
	uint32_t fd = sd->host_fd;

	uk_pr_debug("hostsock: close fd=%u\n", fd);

	int n = build_req(
		"{\"name\":\"net_close\",\"args\":{\"fd\":%u}}", fd);
	if (n >= 0)
		rpc_exchange(n, NULL);

	struct posix_socket_driver *drv = posix_sock_get_driver(sock);
	uk_free(drv->allocator, sd);
	return 0;
}

static int hostsock_ioctl(posix_sock *sock __attribute__((unused)),
			  int request __attribute__((unused)),
			  void *argp __attribute__((unused)))
{
	return -ENOSYS;
}

static int hostsock_socketpair(struct posix_socket_driver *d __attribute__((unused)),
			       int family __attribute__((unused)),
			       int type __attribute__((unused)),
			       int protocol __attribute__((unused)),
			       void *sockvec[2] __attribute__((unused)))
{
	return -EOPNOTSUPP;
}

static void hostsock_poll_setup(posix_sock *sock __attribute__((unused)))
{
}

static struct posix_socket_ops hostsock_ops = {
	.create      = hostsock_create,
	.accept4     = hostsock_accept4,
	.bind        = hostsock_bind,
	.shutdown    = hostsock_shutdown,
	.getpeername = hostsock_getpeername,
	.getsockname = hostsock_getsockname,
	.getsockopt  = hostsock_getsockopt,
	.setsockopt  = hostsock_setsockopt,
	.connect     = hostsock_connect,
	.listen      = hostsock_listen,
	.recvfrom    = hostsock_recvfrom,
	.recvmsg     = hostsock_recvmsg,
	.sendmsg     = hostsock_sendmsg,
	.sendto      = hostsock_sendto,
	.socketpair  = hostsock_socketpair,
	.read        = hostsock_read,
	.write       = hostsock_write,
	.close       = hostsock_close,
	.ioctl       = hostsock_ioctl,
	.poll_setup  = hostsock_poll_setup,
};

POSIX_SOCKET_FAMILY_REGISTER(AF_INET, &hostsock_ops);

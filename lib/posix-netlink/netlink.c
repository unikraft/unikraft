/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <unistd.h>

#include <uk/netlink/driver.h>
#include <uk/socket_driver.h>
#include <uk/print.h>

#define sock2nlctx(s) ((struct nl_ctx *)posix_sock_get_data(s))

extern struct posix_netlink_protocol posix_netlink_protocol_list_start[];
extern struct posix_netlink_protocol posix_netlink_protocol_list_end[];

static const struct posix_netlink_protocol *nl_driver_find(int protocol)
{
	const struct posix_netlink_protocol *drv;

	drv = posix_netlink_protocol_list_start;
	while (drv != posix_netlink_protocol_list_end) {
		if (drv->protocol == protocol)
			return drv;
		drv++;
	}
	return NULL;
}

static void *nl_create(struct posix_socket_driver *drv,
		       int family __maybe_unused, int type, int protocol)
{
	const struct posix_netlink_protocol *nl_drv;
	struct nl_ctx *nl_ctx;
	int err;

	UK_ASSERT(drv);
	UK_ASSERT(family == AF_NETLINK);

	/* AF_NETLINK sockets must be created with SOCK_RAW or SOCK_DGRAM */
	if (unlikely(((type & ~(SOCK_NONBLOCK | SOCK_CLOEXEC)) != SOCK_RAW) &&
		     ((type & ~(SOCK_NONBLOCK | SOCK_CLOEXEC)) != SOCK_DGRAM))) {
		err = EINVAL;
		goto err_out;
	}

	/* Lookup a driver for the requested protocol */
	nl_drv = nl_driver_find(protocol);
	if (unlikely(!nl_drv)) {
		uk_pr_debug("did not find protocol handler for %d\n", protocol);
		err = EAFNOSUPPORT;
		goto err_out;
	}

	nl_ctx = uk_zalloc(drv->allocator, sizeof(*nl_ctx));
	if (unlikely(!nl_ctx)) {
		err = ENOMEM;
		goto err_out;
	}
	nl_ctx->a = drv->allocator;
	nl_ctx->drv = nl_drv;
#if CONFIG_LIBPOSIX_PROCESS
	/* FIXME: PID is here like port number and must be unique. It just
	 *        starts with PID if free.
	 */
	nl_ctx->nl_pid = getpid();
#endif /* CONFIG_LIBPOSIX_PROCESS */

	nl_ctx->nl_recvqueue = uk_mbox_create(nl_ctx->a, 512);
	if (unlikely(!nl_ctx->nl_recvqueue)) {
		err = ENOMEM;
		goto err_free_ctx;
	}

	/* Optional driver initialization */
	if (nl_ctx->drv->ops->create) {
		err = nl_ctx->drv->ops->create(nl_ctx);
		if (unlikely(err))
			goto err_free_recvqueue;
	}

	uk_pr_debug("Created netlink socket: %p, protocol: %d, drv: %s\n",
		    nl_ctx, nl_ctx->drv->protocol, nl_ctx->drv->libname);
	return nl_ctx;

err_free_recvqueue:
	uk_mbox_free(nl_ctx->a, nl_ctx->nl_recvqueue);
err_free_ctx:
	uk_free(nl_ctx->a, nl_ctx);
err_out:
	return ERR2PTR(-err);
}

static int nl_close(posix_sock *s)
{
	struct nl_ctx *nl_ctx = sock2nlctx(s);
	struct uk_streambuf *nlbuf;

	if (nl_ctx->drv->ops->close)
		nl_ctx->drv->ops->close(nl_ctx);

	/* Release queued and unsent messages */
	while (uk_mbox_recv_try(nl_ctx->nl_recvqueue, (void **)&nlbuf) == 0) {
		uk_pr_debug("Releasing unconsumed netlink message %p\n", nlbuf);
		nlbuf_free(nlbuf);
	}

	uk_mbox_free(nl_ctx->a, nl_ctx->nl_recvqueue);
	uk_free(nl_ctx->a, nl_ctx);
	return 0;
}

static int nl_bind(posix_sock *s,
		   const struct sockaddr *addr,
		   socklen_t addr_len)
{
	const struct sockaddr_nl *nl_addr;
	struct nl_ctx *nl_ctx = sock2nlctx(s);

	if (unlikely(nl_ctx->bound))
		return -EADDRINUSE;
	if (unlikely(!addr || addr_len < sizeof(*nl_addr)))
		return -EINVAL;
	nl_addr = (const struct sockaddr_nl *)addr;

	if (unlikely((nl_addr->nl_family != AF_NETLINK) ||
		     (nl_addr->nl_pad != 0))
		return -EINVAL;

	/* We only support the client to connect to us: the kernel */
	if (nl_addr->nl_pid != 0)
		return -EHOSTUNREACH;

	nl_ctx->nl_groups = nl_addr->nl_groups;
	nl_ctx->bound = 1;
	return 0;
}

static int nl_sockname(posix_sock *s,
		       struct sockaddr *restrict addr,
		       socklen_t *restrict addr_len)
{
	struct sockaddr_nl *nl_addr;
	struct nl_ctx *nl_ctx = sock2nlctx(s);

	if (*addr_len < sizeof(*nl_addr)) {
		/* addr_len is too small, only inform about needed size */
		goto out;
	}

	nl_addr = (struct sockaddr_nl *)addr;
	nl_addr->nl_family = AF_NETLINK;
	nl_addr->nl_pid = nl_ctx_pid(nl_ctx);
	nl_addr->nl_pad = 0;
	nl_addr->nl_groups = nl_ctx->nl_groups;

out:
	*addr_len = sizeof(*nl_addr);
	return 0;
}

static inline int nl_handle(struct nl_ctx *nl_ctx, const void *buf, size_t len)
{
	const struct nlmsghdr *nlh;
	size_t tmp_len;
	int ret = 0;

	UK_ASSERT(nl_ctx);
	UK_ASSERT(nl_ctx->drv);
	UK_ASSERT(nl_ctx->drv->ops);
	UK_ASSERT(nl_ctx->drv->ops->handle);

	uk_pr_debug("Handle incoming netlink msg\n");
	if (!buf || len < sizeof(struct nlmsghdr))
		return -EINVAL;

	/* sanity check multi-messages */
	tmp_len = len;
	for (nlh = (const struct nlmsghdr *)buf;
	     NLMSG_OK(nlh, tmp_len);
	     nlh = NLMSG_NEXT(nlh, tmp_len)) {
		if (tmp_len < nlh->nlmsg_len) {
			uk_pr_debug("Sanity check failed\n");
			return -EINVAL;
		}
	}

	/* forward message by message */
	for (nlh = (const struct nlmsghdr *)buf;
	     NLMSG_OK(nlh, len);
	     nlh = NLMSG_NEXT(nlh, len)) {
		uk_pr_debug("Call handler for msg %p (driver %s)\n",
			    nlh, nl_ctx->drv->libname);
		ret = nl_ctx->drv->ops->handle(nl_ctx, nlh);
	}

	return ret;
}

static ssize_t nl_sendto(posix_sock *s,
			 const void *buf, size_t len, int flags __unused,
			 const struct sockaddr *dest_addr, socklen_t addrlen)
{
	struct nl_ctx *nl_ctx = sock2nlctx(s);
	struct sockaddr_nl *nl_addr;
	int ret;

	if (addrlen < sizeof(*nl_addr))
		return -EINVAL;

	nl_addr = (struct sockaddr_nl *)dest_addr;
	if (unlikely(nl_addr->nl_family != AF_NETLINK || nl_addr->nl_pid != 0))
		return -EHOSTUNREACH;

	ret = nl_handle(nl_ctx, buf, len);
	if (unlikely(ret < 0))
		return -EINVAL;
	return len;
}

static size_t copy2iovec(struct iovec iovec[], size_t iovec_len,
			 const void *src, size_t len)
{
	size_t left = len;
	size_t cpylen;
	size_t i = 0;

	for (i = 0; i < iovec_len && left; ++i) {
		cpylen = MIN(iovec[i].iov_len, left);
		memcpy(iovec[i].iov_base, src, cpylen);

		left -= cpylen;
		src  += cpylen;
	}

	return (len - left);
}

static ssize_t nl_recvmsg(posix_sock *s, struct msghdr *msg,
			  int flags __unused)
{
	struct uk_streambuf *nlbuf;
	struct nl_ctx *nl_ctx = sock2nlctx(s);
	size_t cpylen;

	uk_pr_debug("Picking up packet from netlink mbox\n");
	uk_mbox_recv(nl_ctx->nl_recvqueue, (void **)&nlbuf);
	if (unlikely(!nlbuf))
		return -EIO;

	cpylen = copy2iovec(msg->msg_iov, msg->msg_iovlen,
			    nlbuf_data(nlbuf), nlbuf_len(nlbuf));
	nlbuf_free(nlbuf);
	uk_pr_debug("Message received\n");
	return (ssize_t)cpylen;
}

static ssize_t nl_recvfrom(posix_sock *s,
			   void *buf, size_t len, int flags __unused,
			   struct sockaddr *from, socklen_t *fromlen)
{
	struct sockaddr_nl *nl_addr;
	struct uk_streambuf *nlbuf;
	struct nl_ctx *nl_ctx = sock2nlctx(s);
	size_t cpylen;

	uk_pr_debug("Picking up packet from netlink mbox\n");
	uk_mbox_recv(nl_ctx->nl_recvqueue, (void **)&nlbuf);
	if (unlikely(!nlbuf))
		return -EIO;

	cpylen = MIN(nlbuf_len(nlbuf), len);
	memcpy(buf, nlbuf_data(nlbuf), cpylen);
	nlbuf_free(nlbuf);
	uk_pr_debug("Message received\n");

	nl_addr = (struct sockaddr_nl *)from;
	nl_addr->nl_family = AF_NETLINK;
	nl_addr->nl_pid = 0;
	nl_addr->nl_pad = 0;
	nl_addr->nl_groups = nl_ctx->nl_groups;
	*fromlen = sizeof(*nl_addr);

	return (ssize_t)cpylen;
}

static int nl_sockopt(const posix_sock *s __unused,
		      int lvl __unused, int opt __unused,
		      const void *val __unused,
		      socklen_t optlen __unused)
{
	UK_WARN_STUBBED();
	return 0;
}

static const struct posix_socket_ops netlink_vops = {
	.create		= nl_create,
	.close		= nl_close,
	.bind		= nl_bind,
	.getsockname    = nl_sockname,
	.sendto		= nl_sendto,
	.recvmsg	= nl_recvmsg,
	.recvfrom	= nl_recvfrom,
	.setsockopt	= nl_sockopt,
};

POSIX_SOCKET_FAMILY_REGISTER(AF_NETLINK, &netlink_vops);

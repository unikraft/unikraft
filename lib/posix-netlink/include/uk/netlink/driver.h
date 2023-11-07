/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_NETLINK_DRIVER_H__
#define __UK_NETLINK_DRIVER_H__

#include <uk/essentials.h>
#include <uk/socket_driver.h>
#include <linux/netlink.h>
#include <uk/mbox.h>
#include <uk/streambuf.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct nl_ctx {
	struct uk_alloc *a;
	const struct posix_netlink_protocol *drv;

	int bound;
#if CONFIG_LIBPOSIX_PROCESS
	pid_t nl_pid; /* port of client */
#endif /* CONFIG_LIBPOSIX_PROCESS */
	__u32 nl_groups;

	struct uk_mbox *nl_recvqueue;
};

/*
 * Helper to get pid from ctx
 */
#if CONFIG_LIBPOSIX_PROCESS
#define nl_ctx_pid(ctx) ({ (ctx)->nl_pid; })
#else /* !CONFIG_LIBPOSIX_PROCESS */
#define nl_ctx_pid(ctx) (0)
#endif /* !CONFIG_LIBPOSIX_PROCESS */

/**
 * Accept a connection on a socket.
 *
 * @param sock Reference to the socket
 * @param addr The address of the peer socket
 * @param addr_len Specifies the size, in bytes, of the address structure
 *    pointed to by addr
 * @param flags Additional flags to be set for the accepted connection. If
 *    flags is 0, accept4 is the same as POSIX accept.
 *
 * @return NULL or a valid pointer to driver-specific data on success,
 *    ERR2PTR(-errno) otherwise
 */
typedef int (*posix_netlink_protocol_create_func_t)(struct nl_ctx *ctx);

typedef int (*posix_netlink_protocol_handle_func_t)
	     (struct nl_ctx *ctx, const struct nlmsghdr *nlh);

typedef void (*posix_netlink_protocol_close_func_t)(struct nl_ctx *ctx);

struct posix_netlink_protocol_ops {
	posix_netlink_protocol_create_func_t create;
	posix_netlink_protocol_handle_func_t handle;
	posix_netlink_protocol_close_func_t close;
};

/**
 * The POSIX netlink protocol driver defines the operations to be used for the
 * specified (classic) netlink protocol as well as the memory allocator.
 */
struct __align(8) posix_netlink_protocol {
	/** The netlink protocol ID */
	const int protocol;
	/** Name of the driver library */
	const char *libname;
	/** The interfaces for this socket */
	const struct posix_netlink_protocol_ops *ops;
	/** Private data for this socket driver */
	void *private;
};

static inline struct uk_streambuf *nlbuf_alloc(struct nl_ctx *ctx, size_t len)
{
	return uk_streambuf_alloc2(ctx->a, len, sizeof(long),
				   UK_STREAMBUF_C_WIPEZERO);
}

#define nlbuf_reserve(nlbuf, len)	uk_streambuf_reserve((nlbuf), (len))
#define nlbuf_align(nlbuf, align)					       \
	uk_streambuf_reserve((nlbuf), ALIGN_UP(uk_streambuf_seek((nlbuf)),     \
					       (align)) -		       \
					       uk_streambuf_seek((nlbuf)))
#define nlbuf_free(nlbuf)		uk_streambuf_free((nlbuf))

static inline void nlbuf_send(struct nl_ctx *ctx, struct uk_streambuf *nlbuf)
{
	return uk_mbox_post(ctx->nl_recvqueue, nlbuf);
}

#define nlbuf_len(nlbuf)		uk_streambuf_seek((nlbuf))
#define nlbuf_data(nlbuf)		uk_streambuf_buf((nlbuf))

/**
 * Registers a netlink protocol driver
 */
#define _POSIX_NETLINK_PROTOCOL_DRVRNAME(lib, prot)			\
	posix_netlink_protocol_ ## lib ## _ ## prot

#define _POSIX_NETLINK_PROTOCOL_SECNAME(lib, prot)			\
	STRINGIFY(_POSIX_NETLINK_PROTOCOL_DRVRNAME(lib, prot))

/*
 * Creates a static struct posix_netlink_protocol for the AF protocol
 */
#define _POSIX_NETLINK_PROTOCOL_REGISTER(lib, prot, vops)		\
	__used __align(8)						\
		__section("." _POSIX_NETLINK_PROTOCOL_SECNAME(lib, prot))\
	static struct posix_netlink_protocol				\
	_POSIX_NETLINK_PROTOCOL_DRVRNAME(lib, prot) = {			\
		.protocol = prot,					\
		.libname = STRINGIFY(lib),				\
		.ops = vops						\
	}

#define POSIX_NETLINK_PROTOCOL_REGISTER(prot, vops) \
	_POSIX_NETLINK_PROTOCOL_REGISTER(__LIBNAME__, prot, vops)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_NETLINK_DRIVER_H__ */

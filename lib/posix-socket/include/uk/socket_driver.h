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

#ifndef __UK_SOCKET_DRIVER_H__
#define __UK_SOCKET_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/alloc.h>
#include <uk/socket.h>
#include <uk/errptr.h>
#include <uk/file.h>
#include <uk/posix-fd.h>

#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>

struct posix_socket_ops;

#define SOCK_FLAGS (SOCK_NONBLOCK|SOCK_CLOEXEC)

/**
 * The POSIX socket driver defines the operations to be used for the
 * specified AF family as well as the memory allocator.
 */
struct __align(8) posix_socket_driver {
	/** The AF family ID */
	const int family;
	/** Name of the driver library */
	const char *libname;
	/** The interfaces for this socket */
	const struct posix_socket_ops *ops;
	/** The memory allocator to be used for this socket driver */
	struct uk_alloc *allocator;
	/** Private data for this socket driver */
	void *private;
};

/* Driver API */

/**
 * Abstract type for socket objects for the driver API.
 * Drivers receive a reference of this type and can use it to call the
 * utility functions listed below on the socket file.
 */
typedef const struct uk_file posix_sock;

/* Driver utils */

/**
 * Get the socket node from a posix socket.
 */
static inline
struct posix_socket_node *posix_sock_get_node(posix_sock *sock)
{
	struct posix_socket_node *n;

	UK_ASSERT(sock);
	n = (struct posix_socket_node *)sock->node;
	UK_ASSERT(n);
	return n;
}

/**
 * Get the driver-specific socket data from a posix socket.
 */
static inline
void *posix_sock_get_data(posix_sock *sock)
{
	return posix_sock_get_node(sock)->sock_data;
}

/**
 * Replace the driver-specific socket data pointer.
 */
static inline
void posix_sock_set_data(posix_sock *sock, void *data)
{
	posix_sock_get_node(sock)->sock_data = data;
}

/**
 * Get the driver of a posix socket object.
 */
static inline
struct posix_socket_driver *posix_sock_get_driver(posix_sock *sock)
{
	return posix_sock_get_node(sock)->driver;
}

/**
 * Clear event flags from posix socket object.
 */
static inline
void posix_sock_event_clear(posix_sock *sock, unsigned int events)
{
	uk_file_event_clear(sock, events);
}

/**
 * Set event flags on posix socket object.
 */
static inline
void posix_sock_event_set(posix_sock *sock, unsigned int events)
{
	uk_file_event_set(sock, events);
}

/**
 * Clear and set the event flags on posix socket object to a specified value.
 */
static inline
void posix_sock_event_assign(posix_sock *sock, unsigned int events)
{
	uk_file_event_assign(sock, events);
}

/* Socket operations */

/**
 * The initialization function called for this socket family. It's here that
 * additional configuration for the driver can be made after it has been
 * registered. For instance, an alternative memory allocator can be provided.
 *
 * @param d The socket driver
 *
 * @return 0 on success, -errno code otherwise
 */
typedef int (*posix_socket_driver_init_func_t)(struct posix_socket_driver *d);

/**
 * Create a connection on a socket.
 *
 * @param d The socket driver
 * @param family Specifies a communication family domain and thus driver
 * @param type Specifies communication semantics
 * @param protocol Specifies a particular protocol to be used with the socket
 *
 * @return NULL or a valid pointer to driver-specific data on success,
 *    ERR2PTR(-errno) otherwise
 */
typedef void *(*posix_socket_create_func_t)(struct posix_socket_driver *d,
		int family, int type, int protocol);

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
typedef void *(*posix_socket_accept4_func_t)(posix_sock *sock,
		struct sockaddr *restrict addr, socklen_t *restrict addr_len,
		int flags);

/**
 * Bind a name to a socket.
 *
 * @param sock Reference to the socket
 * @param addr The assigned address
 * @param addr_len Specifies the size, in bytes, of the address structure
 *    pointed to by addr
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_bind_func_t)(posix_sock *sock,
		const struct sockaddr *addr, socklen_t addr_len);

/**
 * Shut down part of a full-duplex connection.
 *
 * @param sock Reference to the socket
 * @param how The flag to specify the means of shuting down the socket
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_shutdown_func_t)(posix_sock *sock, int how);

/**
 * Get name of connected peer socket.
 *
 * @param sock Reference to the socket
 * @param addr The assigned address
 * @param addr_len Specifies the size, in bytes, of the address structure
 *    pointed to by addr
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_getpeername_func_t)(posix_sock *sock,
		struct sockaddr *restrict addr, socklen_t *restrict addr_len);

/**
 * Get socket name.
 *
 * @param sock Reference to the socket
 * @param addr The assigned address
 * @param addr_len Specifies the size, in bytes, of the address structure
 *    pointed to by addr
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_getsockname_func_t)(posix_sock *sock,
		struct sockaddr *restrict addr, socklen_t *restrict addr_len);

/**
 * Get options on the socket.
 *
 * @param sock Reference to the socket
 * @param level Manipulate the socket at either the API level or protocol level
 * @param optname Any specified options are passed uninterpreted to the
 *    appropriate protocol module for interpretation
 * @param optval The option value
 * @param optlen The option value length
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_getsockopt_func_t)(posix_sock *sock,
		int level, int optname, void *restrict optval,
		socklen_t *restrict optlen);

/**
 * Set options on the socket.
 *
 * @param sock Reference to the socket
 * @param level Manipulate the socket at either the API level or protocol level
 * @param optname Any specified options are passed uninterpreted to the
 *    appropriate protocol module for interpretation
 * @param optval The option value
 * @param optlen The option value length
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_setsockopt_func_t)(posix_sock *sock,
		int level, int optname, const void *optval, socklen_t optlen);

/**
 * Initiate a connection on a socket.
 *
 * @param sock Reference to the socket
 * @param addr The address to connect to on the socket
 * @param addr_len Specifies the size, in bytes, of the address structure
 *    pointed to by addr
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_connect_func_t)(posix_sock *sock,
		const struct sockaddr *addr, socklen_t addr_len);

/**
 * Listen for connections on a socket.
 *
 * @param sock Reference to the socket
 * @param backlog Defines the maximum length to which the queue of pending
 *    connections for the socket
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_listen_func_t)(posix_sock *sock,
		int backlog);

/**
 * Read from a socket.
 *
 * @param sock Reference to the socket
 * @param buf The buffer for received data from the socket
 * @param len The size of the buffer
 * @param flags Bitmap of options for receiving a message
 * @param from The source address
 * @param fromlen The source address length
 *
 * @return The number of bytes read on success, -errno otherwise
 */
typedef ssize_t (*posix_socket_recvfrom_func_t)(posix_sock *sock,
		void *restrict buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *restrict fromlen);

/**
 * Read from a socket.
 *
 * @param sock Reference to the socket
 * @param msg Message structure to minimize the number of directly supplied
 *    arguments
 * @param flags Bitwise OR of zero or more flags for the socket
 *
 * @return The number of bytes read on success, -errno otherwise
 */
typedef ssize_t (*posix_socket_recvmsg_func_t)(posix_sock *sock,
		struct msghdr *msg, int flags);

/**
 * Send a message on a socket.
 *
 * @param sock Reference to the socket
 * @param msg Message structure to minimize the number of directly supplied
 *    arguments
 * @param flags Bitwise OR of zero or more flags for the socket
 *
 * @return The number of bytes sent on success, -errno otherwise
 */
typedef ssize_t (*posix_socket_sendmsg_func_t)(posix_sock *sock,
		const struct msghdr *msg, int flags);

/**
 * Send a message on a socket.
 *
 * @param sock Reference to the socket
 * @param buf The buffer for sending data to the socket
 * @param len The length of the data to send on the socket
 * @param flags Bitwise OR of zero or more flags for the socket
 * @param dest_addr The destination address to send data
 * @param addrlen The length of the address to send data to
 *
 * @return The number of bytes sent on success, -errno otherwise
 */
typedef ssize_t (*posix_socket_sendto_func_t)(posix_sock *sock,
		const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen);

/**
 * Create a pair of connected sockets.
 *
 * @param d The socket driver
 * @param family The domain of the sockets
 * @param type The specified type of the sockets
 * @param protocol Optionally the protocol
 * @param sockvec The structure used in referencing the new sockets are
 *    returned in sockvec[0] and sockvec[1]
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_socketpair_func_t)(struct posix_socket_driver *d,
		int family, int type, int protocol, void *sockvec[2]);

/**
 * Write to a socket file descriptor.
 *
 * @param sock Reference to the socket
 * @param iov Pointer to an array of iovec structures describing the buffers to
 *    write to the socket
 * @param iovcnt The number of iovec structures in the iovec array
 *
 * @return The number of bytes written on success, -errno otherwise
 */
typedef ssize_t (*posix_socket_write_func_t)(posix_sock *sock,
		const struct iovec *iov, int iovcnt);

/**
 * Read from a socket file descriptor.
 *
 * @param sock Reference to the socket
 * @param iov Pointer to an array of iovec structures describing the buffers to
 *    store received data from the socket
 * @param iovcnt The number of iovec structures in the iovec array
 *
 * @return The number of bytes read on success, -errno otherwise
 */
typedef ssize_t (*posix_socket_read_func_t)(posix_sock *sock,
		const struct iovec *iov, int iovcnt);

/**
 * Close the socket.
 *
 * @param sock Reference to the socket
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_close_func_t)(posix_sock *sock);

/**
 * Manipulate the socket.
 *
 * @param sock Reference to the socket
 * @param request Code of the I/O control
 * @param argp Optional pointer to I/O-control-specific argument
 *
 * @return 0 on success, -errno otherwise
 */
typedef int (*posix_socket_ioctl_func_t)(posix_sock *sock,
		int request, void *argp);

/**
 * Poll the socket, updating `sock` with the currently set events.
 * This is guaranteed to be called exactly once on initialization,
 * allowing the driver to set up any data structures required by callbacks.
 * The driver from that point responsible for updating the events on every
 * operation using the `posix_socket_event_*` functions.
 *
 * @param sock Reference to the socket
 */
typedef void (*posix_socket_poll_func_t)(posix_sock *sock);

/**
 * A structure containing the functions exported by a Unikraft socket driver
 */
struct posix_socket_ops {
	/* The initialization function on socket registration. */
	posix_socket_driver_init_func_t	init;
	/* POSIX socket interface */
	posix_socket_create_func_t	create;
	posix_socket_accept4_func_t	accept4;
	posix_socket_bind_func_t	bind;
	posix_socket_shutdown_func_t	shutdown;
	posix_socket_getpeername_func_t	getpeername;
	posix_socket_getsockname_func_t	getsockname;
	posix_socket_getsockopt_func_t	getsockopt;
	posix_socket_setsockopt_func_t	setsockopt;
	posix_socket_connect_func_t	connect;
	posix_socket_listen_func_t	listen;
	posix_socket_recvfrom_func_t	recvfrom;
	posix_socket_recvmsg_func_t	recvmsg;
	posix_socket_sendmsg_func_t	sendmsg;
	posix_socket_sendto_func_t	sendto;
	posix_socket_socketpair_func_t	socketpair;
	/* file ops */
	posix_socket_write_func_t	write;
	posix_socket_read_func_t	read;
	posix_socket_close_func_t	close;
	posix_socket_ioctl_func_t	ioctl;
	posix_socket_poll_func_t	poll;
};

static inline void *
posix_socket_create(struct posix_socket_driver *d, int family, int type,
		    int protocol)
{
	UK_ASSERT(d);
	UK_ASSERT(d->ops->create);

	return d->ops->create(d, family, type, protocol);
}

static inline void *
posix_socket_accept4(posix_sock *sock,
		     struct sockaddr *restrict addr,
		     socklen_t *restrict addr_len, int flags)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->accept4);
	return d->ops->accept4(sock, addr, addr_len, flags);
}

static inline int
posix_socket_bind(posix_sock *sock,
		  const struct sockaddr *addr, socklen_t addr_len)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->bind);
	return d->ops->bind(sock, addr, addr_len);
}

static inline int
posix_socket_shutdown(posix_sock *sock, int how)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->shutdown);
	return d->ops->shutdown(sock, how);
}

static inline int
posix_socket_getpeername(posix_sock *sock,
			 struct sockaddr *restrict addr,
			 socklen_t *restrict addr_len)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->getpeername);
	return d->ops->getpeername(sock, addr, addr_len);
}

static inline int
posix_socket_getsockname(posix_sock *sock,
			 struct sockaddr *restrict addr,
			 socklen_t *restrict addr_len)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->getsockname);
	return d->ops->getsockname(sock, addr, addr_len);
}

static inline int
posix_socket_getsockopt(posix_sock *sock, int level, int optname,
			void *restrict optval, socklen_t *restrict optlen)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->getsockopt);
	return d->ops->getsockopt(sock, level, optname, optval, optlen);
}

static inline int
posix_socket_setsockopt(posix_sock *sock, int level, int optname,
			const void *optval, socklen_t optlen)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->setsockopt);
	return d->ops->setsockopt(sock, level, optname, optval, optlen);
}

static inline int
posix_socket_connect(posix_sock *sock,
		     const struct sockaddr *addr, socklen_t addr_len)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->connect);
	return d->ops->connect(sock, addr, addr_len);
}

static inline int
posix_socket_listen(posix_sock *sock, int backlog)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->listen);
	return d->ops->listen(sock, backlog);
}

static inline ssize_t
posix_socket_recvfrom(posix_sock *sock, void *restrict buf,
		      size_t len, int flags, struct sockaddr *from,
		      socklen_t *fromlen)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->recvfrom);
	return d->ops->recvfrom(sock, buf, len, flags, from, fromlen);
}

static inline ssize_t
posix_socket_recvmsg(posix_sock *sock, struct msghdr *msg,
		     int flags)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->recvmsg);
	return d->ops->recvmsg(sock, msg, flags);
}

static inline ssize_t
posix_socket_sendmsg(posix_sock *sock,
		     const struct msghdr *msg, int flags)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->sendmsg);
	return d->ops->sendmsg(sock, msg, flags);
}

static inline ssize_t
posix_socket_sendto(posix_sock *sock, const void *buf,
		    size_t len, int flags, const struct sockaddr *dest_addr,
		    socklen_t addrlen)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->sendto);
	return d->ops->sendto(sock, buf, len, flags, dest_addr, addrlen);
}

static inline int
posix_socket_socketpair(struct posix_socket_driver *d, int family, int type,
			int protocol, void *usockvec[2])
{
	UK_ASSERT(d);
	UK_ASSERT(d->ops->socketpair);
	return d->ops->socketpair(d, family, type, protocol, usockvec);
}

static inline ssize_t
posix_socket_write(posix_sock *sock, const struct iovec *iov,
		   int iovcnt)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->write);
	return d->ops->write(sock, iov, iovcnt);
}

static inline ssize_t
posix_socket_read(posix_sock *sock, const struct iovec *iov,
		  int iovcnt)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->read);
	return d->ops->read(sock, iov, iovcnt);
}

static inline int
posix_socket_close(posix_sock *sock)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->close);
	return d->ops->close(sock);
}

static inline int
posix_socket_ioctl(posix_sock *sock, int request, void *argp)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->ioctl);
	return d->ops->ioctl(sock, request, argp);
}

static inline void
posix_socket_poll(posix_sock *sock)
{
	struct posix_socket_driver *d = posix_sock_get_driver(sock);

	UK_ASSERT(d->ops->poll);
	d->ops->poll(sock);
}

/**
 * Return the driver to the corresponding AF family number
 *
 * @param family Socket address family
 */
struct posix_socket_driver *
posix_socket_driver_get(int family);

/**
 * Returns the number of registered socket families
 */
unsigned int
posix_socket_family_count(void);

/**
 * Registers a socket family driver to the socket system
 */
#define _POSIX_SOCKET_FAMILY_DRVRNAME(lib, fam)				\
	posix_socket_driver_ ## lib ## _ ## fam

#define _POSIX_SOCKET_FAMILY_SECNAME(lib, fam)				\
	STRINGIFY(_POSIX_SOCKET_FAMILY_DRVRNAME(lib, fam))

/*
 * Creates a static struct posix_socket_driver for the AF family
 */
#define _POSIX_SOCKET_FAMILY_REGISTER(lib, fam, vops)			\
	__used __align(8)						\
		__section("." _POSIX_SOCKET_FAMILY_SECNAME(lib, fam))	\
	static struct posix_socket_driver				\
	_POSIX_SOCKET_FAMILY_DRVRNAME(lib, fam) = {			\
		.family = fam,						\
		.libname = STRINGIFY(lib),				\
		.ops = vops						\
	}

#define POSIX_SOCKET_FAMILY_REGISTER(fam, vops) \
	_POSIX_SOCKET_FAMILY_REGISTER(__LIBNAME__, fam, vops)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_SOCKET_DRIVER_H__ */

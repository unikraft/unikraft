/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <alexander.jung@neclab.eu>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
 *         The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

/* Derived from OpenBSD commit 15b62b7 and musl commit 6dc1e40 */

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include <uk/config.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_socklen_t
#define __NEED_size_t
#define __NEED_ssize_t

#include <nolibc-internal/shareddefs.h>

#ifdef CONFIG_LIBPOSIX_SOCKET

/*
 * Types
 */
#define SOCK_STREAM		1	/* stream socket */
#define SOCK_DGRAM		2	/* datagram socket */
#define SOCK_RAW		3	/* raw-protocol interface */
#define SOCK_RDM		4	/* reliably-delivered message */
#define SOCK_SEQPACKET		5	/* sequenced packet stream */

/*
 * Address families
 */
#define AF_UNSPEC		0
#define AF_LOCAL		1
#define AF_UNIX			AF_LOCAL
#define AF_FILE			AF_LOCAL
#define AF_INET			2
#define AF_AX25			3
#define AF_IPX			4
#define AF_APPLETALK		5
#define AF_NETROM		6
#define AF_BRIDGE		7
#define AF_ATMPVC		8
#define AF_X25			9
#define AF_INET6		10
#define AF_ROSE			11
#define AF_DECnet		12
#define AF_NETBEUI		13
#define AF_SECURITY		14
#define AF_KEY			15
#define AF_NETLINK		16
#define AF_ROUTE		AF_NETLINK
#define AF_PACKET		17
#define AF_ASH			18
#define AF_ECONET		19
#define AF_ATMSVC		20
#define AF_RDS			21
#define AF_SNA			22
#define AF_IRDA			23
#define AF_PPPOX		24
#define AF_WANPIPE		25
#define AF_LLC			26
#define AF_IB			27
#define AF_MPLS			28
#define AF_CAN			29
#define AF_TIPC			30
#define AF_BLUETOOTH		31
#define AF_IUCV			32
#define AF_RXRPC		33
#define AF_ISDN			34
#define AF_PHONET		35
#define AF_IEEE802154		36
#define AF_CAIF			37
#define AF_ALG			38
#define AF_NFC			39
#define AF_VSOCK		40
#define AF_KCM			41
#define AF_QIPCRTR		42
#define AF_SMC			43
#define AF_XDP			44
#define AF_MAX			45

/*
 * Protocol families, same as address families for now.
 */
#define PF_UNSPEC		AF_UNSPEC
#define PF_LOCAL		AF_LOCAL
#define PF_UNIX			AF_UNIX
#define PF_FILE			AF_FILE
#define PF_INET			AF_INET
#define PF_AX25			AF_AX25
#define PF_IPX			AF_IPX
#define PF_APPLETALK		AF_APPLETALK
#define PF_NETROM		AF_NETROM
#define PF_BRIDGE		AF_BRIDGE
#define PF_ATMPVC		AF_ATMPVC
#define PF_X25			AF_X25
#define PF_INET6		AF_INET6
#define PF_ROSE			AF_ROSE
#define PF_DECnet		AF_DECnet
#define PF_NETBEUI		AF_NETBEUI
#define PF_SECURITY		AF_SECURITY
#define PF_KEY			AF_KEY
#define PF_NETLINK		AF_NETLINK
#define PF_ROUTE		AF_ROUTE
#define PF_PACKET		AF_PACKET
#define PF_ASH			AF_ASH
#define PF_ECONET		AF_ECONET
#define PF_ATMSVC		AF_ATMSVC
#define PF_RDS			AF_RDS
#define PF_SNA			AF_SNA
#define PF_IRDA			AF_IRDA
#define PF_PPPOX		AF_PPPOX
#define PF_WANPIPE		AF_WANPIPE
#define PF_LLC			AF_LLC
#define PF_IB			AF_IB
#define PF_MPLS			AF_MPLS
#define PF_CAN			AF_CAN
#define PF_TIPC			AF_TIPC
#define PF_BLUETOOTH		AF_BLUETOOTH
#define PF_IUCV			AF_IUCV
#define PF_RXRPC		AF_RXRPC
#define PF_ISDN			AF_ISDN
#define PF_PHONET		AF_PHONET
#define PF_IEEE802154		AF_IEEE802154
#define PF_CAIF			AF_CAIF
#define PF_ALG			AF_ALG
#define PF_NFC			AF_NFC
#define PF_VSOCK		AF_VSOCK
#define PF_KCM			AF_KCM
#define PF_QIPCRTR		AF_QIPCRTR
#define PF_SMC			AF_SMC
#define PF_XDP			AF_XDP
#define PF_MAX			AF_MAX

/*
 * These are the valid values for the "how" field used by shutdown(2).
 */
#define SHUT_RD			0
#define SHUT_WR			1
#define SHUT_RDWR		2

/*
 * Maximum queue length specifiable by listen(2).
 */
#define SOMAXCONN		128

struct msghdr;
struct cmsghdr;
struct sockaddr;

int socket(int family, int type, int protocol);
int socketpair(int family, int type, int protocol, int usockfd[2]);

int shutdown(int sock, int how);

int bind(int sock, const struct sockaddr *addr, socklen_t addr_len);
int connect(int sock, const struct sockaddr *addr, socklen_t addr_len);
int listen(int sock, int backlog);
int accept(int sock, struct sockaddr *restrict addr,
	   socklen_t *restrict addr_len);
#ifdef _GNU_SOURCE
int accept4(int sock, struct sockaddr *restrict addr,
	    socklen_t *restrict addr_len, int flags);
#endif /* _GNU_SOURCE */

int getsockname(int sock, struct sockaddr *restrict addr,
		socklen_t *restrict addr_len);
int getpeername(int sock, struct sockaddr *restrict addr,
		socklen_t *restrict addr_len);

ssize_t send(int sock, const void *buf, size_t len, int flags);
ssize_t recv(int sock, void *buf, size_t len, int flags);
ssize_t sendto(int sock, const void *buf, size_t len, int flags,
	       const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sock, void *buf, size_t len, int flags,
		 struct sockaddr *from, socklen_t *fromlen);
ssize_t sendmsg(int sock, const struct msghdr *msg, int flags);
ssize_t recvmsg(int sock, struct msghdr *msg, int flags);

int getsockopt(int sock, int level, int optname, void *restrict optval,
	       socklen_t *restrict optlen);
int setsockopt(int sock, int level, int optname, const void *optval,
	       socklen_t optlen);

#endif /* CONFIG_LIBPOSIX_SOCKET */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SOCKET_H */

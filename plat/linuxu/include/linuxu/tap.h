/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 */
#ifndef __PLAT_LINUXU_TAP_H__
#define __PLAT_LINUXU_TAP_H__

#include <uk/arch/types.h>
#include <linuxu/syscall.h>
#include <linuxu/ioctl.h>

/**
 * TAP Device Path
 */
#define TAPDEV_PATH		 "/dev/net/tun"
/**
 * Using the musl as reference for the data structure definition
 * Commit-id: 39ef612aa193
 */
#define IFNAMSIZ        16



typedef __u16 uk_in_port_t;
typedef __u32 uk_in_addr_t;
typedef __u16 uk_sa_family_t;

struct uk_in_addr {
	uk_in_addr_t s_addr;
};

struct uk_sockaddr_in {
	uk_sa_family_t sin_family;
	uk_in_port_t sin_port;
	struct uk_in_addr sin_addr;
	__u8 sin_zero[8];
};

struct uk_sockaddr {
	uk_sa_family_t sa_family;
	char sa_data[14];
};

struct uk_ifmap {
	unsigned long int mem_start;
	unsigned long int mem_end;
	unsigned short int base_addr;
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
};

struct uk_ifreq {
	union {
		char ifrn_name[IFNAMSIZ];
	} uk_ifr_ifrn;

	union {
		struct uk_sockaddr ifru_addr;
		struct uk_sockaddr ifru_dstaddr;
		struct uk_sockaddr ifru_broadaddr;
		struct uk_sockaddr ifru_netmask;
		struct uk_sockaddr ifru_hwaddr;
		short int       ifru_flags;
		int             ifru_ivalue;
		int             ifru_mtu;
		struct uk_ifmap    ifru_map;
		char            ifru_slave[IFNAMSIZ];
		char            ifru_newname[IFNAMSIZ];
		char           *ifru_data;
	} uk_ifr_ifru;
};

#define ifr_name    uk_ifr_ifrn.ifrn_name
#define ifr_hwaddr  uk_ifr_ifru.ifru_hwaddr
#define ifr_addr    uk_ifr_ifru.ifru_addr
#define ifr_dstaddr uk_ifr_ifru.ifru_dstaddr
#define ifr_broadaddr   uk_ifr_ifru.ifru_broadaddr
#define ifr_netmask uk_ifr_ifru.ifru_netmask
#define ifr_flags   uk_ifr_ifru.ifru_flags
#define ifr_metric  uk_ifr_ifru.ifru_ivalue
#define ifr_mtu     uk_ifr_ifru.ifru_mtu
#define ifr_map     uk_ifr_ifru.ifru_map
#define ifr_slave   uk_ifr_ifru.ifru_slave
#define ifr_data    uk_ifr_ifru.ifru_data
#define ifr_ifindex uk_ifr_ifru.ifru_ivalue
#define ifr_bandwidth   uk_ifr_ifru.ifru_ivalue
#define ifr_qlen    uk_ifr_ifru.ifru_ivalue
#define ifr_newname uk_ifr_ifru.ifru_newname

#define UK_TUNSETIFF     (0x400454ca)
#define UK_SIOCGIFNAME   (0x8910)
#define UK_SIOCGIFFLAGS  (0x8913)
#define UK_SIOCSIFFLAGS  (0x8914)
#define UK_SIOCGIFADDR   (0x8915)
#define UK_SIOCSIFADDR   (0x8916)
#define UK_SIOCGIFMTU    (0x8921)
#define UK_SIOCSIFMTU    (0x8922)
#define UK_SIOCSIFHWADDR (0x8924)
#define UK_SIOCGIFHWADDR (0x8927)
#define UK_SIOCGIFTXQLEN (0x8942)
#define UK_SIOCSIFTXQLEN (0x8943)
#define UK_SIOCGIFINDEX  (0x8933)
/* TUNSETIFF ifr flags */
#define UK_IFF_TUN     (0x0001)
#define UK_IFF_TAP     (0x0002)
#define UK_IFF_NO_PI   (0x1000)
/* This flag has no real effect */
#define UK_IFF_ONE_QUEUE   (0x2000)
#define UK_IFF_VNET_HDR    (0x4000)
#define UK_IFF_TUN_EXCL    (0x8000)
#define UK_IFF_MULTI_QUEUE (0x0100)
#define UK_IFF_ATTACH_QUEUE (0x0200)
#define UK_IFF_DETACH_QUEUE (0x0400)
/* read-only flag */
#define UK_IFF_PERSIST	(0x0800)
#define UK_IFF_NOFILTER	(0x1000)
#define UK_IFF_UP	(0x1)
#define UK_IFF_PROMISC	(0x100)

/* Adding the bridge interface */
#define UK_SIOCBRADDIF (0x89a2)

#endif /* __PLAT_LINUXU_TAP_H */

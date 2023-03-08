/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marco Schlumpp <marco@unikraft.io>
 *
 * Copyright (c) 2022, Unikraft GmbH. All rights reserved.
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#ifndef __UK_NETSTRUCTS__
#define __UK_NETSTRUCTS__

#include <stdint.h>
#include <uk/essentials.h>

#include "netdev_core.h"

/*
 * Some basic Ethernet constants.
 */
#define	UK_ETHER_ADDR_LEN	6	/* length of an Ethernet address */
#define	UK_ETHER_TYPE_LEN	2	/* length of the Ethernet type field */
#define	UK_ETHER_CRC_LEN	4	/* length of the Ethernet CRC */
#define	UK_ETHER_HDR_LEN	(UK_ETHER_ADDR_LEN*2+UK_ETHER_TYPE_LEN)
#define	UK_ETHER_MIN_LEN	64	/* minimum frame len, including CRC */
#define	UK_ETHER_MAX_LEN	1518	/* maximum frame len, including CRC */
#define	UK_ETHER_MAX_LEN_JUMBO	9018	/* max jumbo frame len, including CRC */

#define	UK_ETHER_VLAN_ENCAP_LEN	4	/* len of 802.1Q VLAN encapsulation */

/*
 * 802.1q Virtual LAN header.
 */
struct uk_ether_vlan_header {
	uint8_t evl_dhost[UK_ETH_ADDR_LEN];
	uint8_t evl_shost[UK_ETH_ADDR_LEN];
	uint16_t evl_encap_proto;
	uint16_t evl_tag;
	uint16_t evl_proto;
} __packed;

#define	UK_ETHERTYPE_IP			0x0800	/* IP protocol */
#define	UK_ETHERTYPE_VLAN		0x8100	/* IEEE 802.1Q VLAN tagging */
#define	UK_ETHERTYPE_IPV6		0x86DD	/* IP protocol version 6 */

/*
 * Structure of an internet header, naked of options.
 */
struct uk_iphdr {
#if __BYTE_ORDER__ ==  __ORDER_LITTLE_ENDIAN__
	uint8_t 	ip_hl:4,		/* header length */
			ip_v:4;			/* version */
#endif
#if __BYTE_ORDER__ ==  __ORDER_BIG_ENDIAN__
	uint8_t 	ip_v:4,			/* version */
			ip_hl:4;		/* header length */
#endif
	uint8_t 	ip_tos;			/* type of service */
	uint16_t 	ip_len;			/* total length */
	uint16_t 	ip_id;			/* identification */
	uint16_t 	ip_off;			/* fragment offset field */
#define	UK_IP_RF 0x8000				/* reserved fragment flag */
#define	UK_IP_DF 0x4000				/* dont fragment flag */
#define	UK_IP_MF 0x2000				/* more fragments flag */
#define	UK_IP_OFFMASK 0x1fff			/* mask for fragmenting bits */
	uint8_t 	ip_ttl;			/* time to live */
	uint8_t 	ip_p;			/* protocol */
	uint16_t 	ip_sum;			/* checksum */
	uint32_t 	ip_src, ip_dst;		/* source and dest address */
} __packed __align(2);

#define	UK_IPPROTO_IP		0		/* dummy for IP */
#define	UK_IPPROTO_ICMP		1		/* control message protocol */
#define	UK_IPPROTO_TCP		6		/* tcp */
#define	UK_IPPROTO_UDP		17		/* user datagram protocol */
#define	UK_IPPROTO_IPV6		41		/* IP6 header */

/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct uk_tcphdr {
	uint16_t	th_sport;		/* source port */
	uint16_t	th_dport;		/* destination port */
	uint32_t	th_seq;			/* sequence number */
	uint32_t	th_ack;			/* acknowledgement number */
#if __BYTE_ORDER__ ==  __ORDER_LITTLE_ENDIAN__
	uint8_t 	th_x2:4,		/* upper 4 (reserved) flags */
			th_off:4;		/* data offset */
#endif
#if __BYTE_ORDER__ ==  __ORDER_BIG_ENDIAN__
	uint8_t 	th_off:4,		/* data offset */
			th_x2:4;		/* upper 4 (reserved) flags */
#endif
	uint8_t	th_flags;
#define	UK_TH_FIN	0x01
#define	UK_TH_SYN	0x02
#define	UK_TH_RST	0x04
#define	UK_TH_PUSH	0x08
#define	UK_TH_ACK	0x10
#define	UK_TH_URG	0x20
#define	UK_TH_ECE	0x40
#define	UK_TH_CWR	0x80
#define	UK_TH_AE	0x100			/* maps into th_x2 */
#define	UK_TH_FLAGS	(UK_TH_FIN|UK_TH_SYN|UK_TH_RST|UK_TH_PUSH|UK_TH_ACK| \
			 UK_TH_URG|UK_TH_ECE|UK_TH_CWR)

	uint16_t	th_win;			/* window */
	uint16_t	th_sum;			/* checksum */
	uint16_t	th_urp;			/* urgent pointer */
};

#endif /* __UK_NETSTRUCTS__ */

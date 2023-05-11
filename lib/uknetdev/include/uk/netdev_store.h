/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_NETDEV_STORE_H__
#define __UK_NETDEV_STORE_H__

/* netdev stats entry IDs */
#define UK_NETDEV_STATS_TX_BYTES	0x01
#define UK_NETDEV_STATS_TX_PACKETS	0x02
#define UK_NETDEV_STATS_TX_ERRORS	0x03
#define UK_NETDEV_STATS_TX_FIFO		0x04

#define UK_NETDEV_STATS_RX_BYTES	0x10
#define UK_NETDEV_STATS_RX_PACKETS	0x20
#define UK_NETDEV_STATS_RX_ERRORS	0x30
#define UK_NETDEV_STATS_RX_FIFO		0x40

#endif /* __UK_NETDEV_STORE_H__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE /* asprintf */
#include <stdio.h>

#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/libid.h>
#include <uk/netdev.h>
#include <uk/netdev_store.h>
#include <uk/spinlock.h>
#include <uk/store.h>

static int get_tx_bytes(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.tx_m.bytes;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_tx_packets(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.tx_m.packets;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_tx_errors(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.tx_m.errors;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_tx_fifo(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.tx_m.fifo;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_rx_bytes(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.rx_m.bytes;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_rx_packets(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.rx_m.packets;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_rx_errors(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.rx_m.errors;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static int get_rx_fifo(void *cookie, __u64 *out)
{
	struct uk_netdev *dev = (struct uk_netdev *)cookie;

	UK_ASSERT(dev);

	uk_spin_lock(&dev->_stats_lock);
	*out = dev->_stats.rx_m.fifo;
	uk_spin_unlock(&dev->_stats_lock);

	return 0;
}

static const struct uk_store_entry *dyn_entries[] = {
	UK_STORE_ENTRY(UK_NETDEV_STATS_TX_BYTES, "tx_bytes", u64,
		       get_tx_bytes, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_TX_PACKETS, "tx_packets", u64,
		       get_tx_packets, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_TX_ERRORS, "tx_errors", u64,
		       get_tx_errors, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_TX_FIFO, "tx_fifo", u64,
		       get_tx_fifo, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_RX_BYTES, "rx_bytes", u64,
		       get_rx_bytes, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_RX_PACKETS, "rx_packets", u64,
		       get_rx_packets, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_RX_ERRORS, "rx_errors", u64,
		       get_rx_errors, NULL),
	UK_STORE_ENTRY(UK_NETDEV_STATS_RX_FIFO, "rx_fifo", u64,
		       get_rx_fifo, NULL),
	NULL
};

int uk_netdev_stats_init(struct uk_netdev *dev)
{
	int res;
	struct uk_store_object *obj = NULL;
	uint16_t dev_id = uk_netdev_id_get(dev);
	char *obj_name;

	memset(&dev->_stats, 0, sizeof(dev->_stats));
	ukarch_spin_init(&dev->_stats_lock);

	/* Create stats object */
	res = asprintf(&obj_name, "netdev%d", dev_id);
	if (unlikely(res == -1)) {
		uk_pr_err("Could not allocate object name\n");
		return -ENOMEM;
	}

	obj = uk_store_obj_alloc(uk_alloc_get_default(), dev_id, obj_name,
				 dyn_entries, (void *)dev);
	if (PTRISERR(obj))
		return PTR2ERR(obj);

	res = uk_store_obj_add(obj);
	if (unlikely(res))
		return res;

	return 0;
}

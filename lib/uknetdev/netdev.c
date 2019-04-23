/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2017-2018, NEC Europe Ltd., NEC Corporation.
 * All rights reserved.
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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#define _GNU_SOURCE /* for asprintf() */
#include <stdio.h>
#include <string.h>
#include <uk/netdev.h>
#include <uk/print.h>

struct uk_netdev_list uk_netdev_list =
	UK_TAILQ_HEAD_INITIALIZER(uk_netdev_list);
static uint16_t netdev_count;

static struct uk_netdev_data *_alloc_data(struct uk_alloc *a,
					  uint16_t netdev_id,
					  const char *drv_name)
{
	struct uk_netdev_data *data;

	data = uk_calloc(a, 1, sizeof(*data));
	if (!data)
		return NULL;

	data->drv_name = drv_name;
	data->state    = UK_NETDEV_UNCONFIGURED;

	/* This is the only place where we set the device ID;
	 * during the rest of the device's life time this ID is read-only
	 */
	*(DECONST(uint16_t *, &data->id)) = netdev_id;

	return data;
}

int uk_netdev_drv_register(struct uk_netdev *dev, struct uk_alloc *a,
			   const char *drv_name)
{
	UK_ASSERT(dev);
	UK_ASSERT(!dev->_data);

	/* Assert mandatory configuration */
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->info_get);
	UK_ASSERT(dev->ops->configure);
	UK_ASSERT(dev->ops->rxq_info_get);
	UK_ASSERT(dev->ops->rxq_configure);
	UK_ASSERT(dev->ops->txq_info_get);
	UK_ASSERT(dev->ops->txq_configure);
	UK_ASSERT(dev->ops->start);
	UK_ASSERT(dev->ops->promiscuous_get);
	UK_ASSERT(dev->ops->mtu_get);
	UK_ASSERT((dev->ops->rxq_intr_enable && dev->ops->rxq_intr_disable)
		  || (!dev->ops->rxq_intr_enable
		      && !dev->ops->rxq_intr_disable));
	UK_ASSERT(dev->rx_one);
	UK_ASSERT(dev->tx_one);

	dev->_data = _alloc_data(a, netdev_count,  drv_name);
	if (!dev->_data)
		return -ENOMEM;

	UK_TAILQ_INSERT_TAIL(&uk_netdev_list, dev, _list);
	uk_pr_info("Registered netdev%"PRIu16": %p (%s)\n",
		   netdev_count, dev, drv_name);

	return netdev_count++;
}

unsigned int uk_netdev_count(void)
{
	return (unsigned int) netdev_count;
}

struct uk_netdev *uk_netdev_get(unsigned int id)
{
	struct uk_netdev *dev;

	UK_TAILQ_FOREACH(dev, &uk_netdev_list, _list) {
		UK_ASSERT(dev->_data);

		if (dev->_data->id == id)
			return dev;
	}
	return NULL;
}

uint16_t uk_netdev_id_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->id;
}

const char *uk_netdev_drv_name_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->drv_name;
}

enum uk_netdev_state uk_netdev_state_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->state;
}

void uk_netdev_info_get(struct uk_netdev *dev,
			struct uk_netdev_info *dev_info)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->info_get);
	UK_ASSERT(dev_info);

	/* Clear values before querying driver for capabilities */
	memset(dev_info, 0, sizeof(*dev_info));
	dev->ops->info_get(dev, dev_info);

	/* Limit the maximum number of rx queues and tx queues
	 * according to the API configuration
	 */
	dev_info->max_rx_queues = MIN(CONFIG_LIBUKNETDEV_MAXNBQUEUES,
				      dev_info->max_rx_queues);
	dev_info->max_tx_queues = MIN(CONFIG_LIBUKNETDEV_MAXNBQUEUES,
				      dev_info->max_tx_queues);
}

const void *uk_netdev_einfo_get(struct uk_netdev *dev,
				enum uk_netdev_einfo_type einfo)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);

	if (!dev->ops->einfo_get) {
		/* driver does not provide any extra configuration */
		return NULL;
	}
	return dev->ops->einfo_get(dev, einfo);
}

int uk_netdev_rxq_info_get(struct uk_netdev *dev, uint16_t queue_id,
			   struct uk_netdev_queue_info *queue_info)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->rxq_info_get);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(queue_info);

	/* Clear values before querying driver for capabilities */
	memset(queue_info, 0, sizeof(*queue_info));
	return dev->ops->rxq_info_get(dev, queue_id, queue_info);
}

int uk_netdev_txq_info_get(struct uk_netdev *dev, uint16_t queue_id,
			   struct uk_netdev_queue_info *queue_info)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->txq_info_get);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(queue_info);

	/* Clear values before querying driver for capabilities */
	memset(queue_info, 0, sizeof(*queue_info));
	return dev->ops->txq_info_get(dev, queue_id, queue_info);
}

int uk_netdev_configure(struct uk_netdev *dev,
			const struct uk_netdev_conf *dev_conf)
{
	struct uk_netdev_info dev_info;
	int ret;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->configure);
	UK_ASSERT(dev_conf);

	if (dev->_data->state != UK_NETDEV_UNCONFIGURED)
		return -EINVAL;

	uk_netdev_info_get(dev, &dev_info);
	if (dev_conf->nb_rx_queues > dev_info.max_rx_queues)
		return -EINVAL;
	if (dev_conf->nb_tx_queues > dev_info.max_tx_queues)
		return -EINVAL;

	ret = dev->ops->configure(dev, dev_conf);
	if (ret >= 0) {
		uk_pr_info("netdev%"PRIu16": Configured interface\n",
			   dev->_data->id);
		dev->_data->state = UK_NETDEV_CONFIGURED;
	} else {
		uk_pr_err("netdev%"PRIu16": Failed to configure interface: %d\n",
			  dev->_data->id, ret);
	}
	return ret;
}

#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
static void _dispatcher(void *arg)
{
	struct uk_netdev_event_handler *handler =
		(struct uk_netdev_event_handler *) arg;

	UK_ASSERT(handler);
	UK_ASSERT(handler->callback);

	for (;;) {
		uk_semaphore_down(&handler->events);
		handler->callback(handler->dev,
				  handler->queue_id,
				  handler->cookie);
	}
}
#endif

static int _create_event_handler(uk_netdev_queue_event_t callback,
				 void *callback_cookie,
#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
				 struct uk_netdev *dev, uint16_t queue_id,
				 const char *queue_type_str,
				 struct uk_sched *s,
#endif
				 struct uk_netdev_event_handler *h)
{
	UK_ASSERT(h);
	UK_ASSERT(callback || (!callback && !callback_cookie));
#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
	UK_ASSERT(!h->dispatcher);
#endif

	h->callback = callback;
	h->cookie   = callback_cookie;

#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
	/* If we do not have a callback, we do not need a thread */
	if (!callback)
		return 0;

	h->dev = dev;
	h->queue_id = queue_id;
	uk_semaphore_init(&h->events, 0);
	h->dispatcher_s = s;

	/* Create a name for the dispatcher thread.
	 * In case of errors, we just continue without a name
	 */
	if (asprintf(&h->dispatcher_name,
		     "netdev%"PRIu16"-%s[%"PRIu16"]",
		     dev->_data->id, queue_type_str, queue_id) < 0) {
		h->dispatcher_name = NULL;
	}

	h->dispatcher = uk_sched_thread_create(h->dispatcher_s,
					       h->dispatcher_name, NULL,
					       _dispatcher, h);
	if (!h->dispatcher) {
		if (h->dispatcher_name)
			free(h->dispatcher_name);
		h->dispatcher_name = NULL;
		return -ENOMEM;
	}
#endif

	return 0;
}

static void _destroy_event_handler(struct uk_netdev_event_handler *h
				   __maybe_unused)
{
	UK_ASSERT(h);

#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
	UK_ASSERT(h->dispatcher_s);

	if (h->dispatcher) {
		uk_thread_kill(h->dispatcher);
		uk_thread_wait(h->dispatcher);
	}
	h->dispatcher = NULL;

	if (h->dispatcher_name)
		free(h->dispatcher_name);
	h->dispatcher_name = NULL;
#endif
}

int uk_netdev_rxq_configure(struct uk_netdev *dev, uint16_t queue_id,
			    uint16_t nb_desc,
			    struct uk_netdev_rxqueue_conf *rx_conf)
{
	int err;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->rxq_configure);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(rx_conf);
	UK_ASSERT(rx_conf->alloc_rxpkts);
#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
	UK_ASSERT((rx_conf->callback && rx_conf->s)
		  || !rx_conf->callback);
#endif

	if (dev->_data->state != UK_NETDEV_CONFIGURED)
		return -EINVAL;

	/* Make sure that we are not initializing this queue a second time */
	if (!PTRISERR(dev->_rx_queue[queue_id]))
		return -EBUSY;

	err = _create_event_handler(rx_conf->callback, rx_conf->callback_cookie,
#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
				    dev, queue_id, "rxq", rx_conf->s,
#endif
				    &dev->_data->rxq_handler[queue_id]);
	if (err)
		goto err_out;

	dev->_rx_queue[queue_id] = dev->ops->rxq_configure(dev, queue_id,
							   nb_desc, rx_conf);
	if (PTRISERR(dev->_rx_queue[queue_id])) {
		err = PTR2ERR(dev->_rx_queue[queue_id]);
		goto err_destroy_handler;
	}

	uk_pr_info("netdev%"PRIu16": Configured receive queue %"PRIu16"\n",
		   dev->_data->id, queue_id);
	return 0;

err_destroy_handler:
	_destroy_event_handler(&dev->_data->rxq_handler[queue_id]);
err_out:
	return err;
}

int uk_netdev_txq_configure(struct uk_netdev *dev, uint16_t queue_id,
			    uint16_t nb_desc,
			    struct uk_netdev_txqueue_conf *tx_conf)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->txq_configure);
	UK_ASSERT(tx_conf);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);

	if (dev->_data->state != UK_NETDEV_CONFIGURED)
		return -EINVAL;

	/* Make sure that we are not initializing this queue a second time */
	if (!PTRISERR(dev->_tx_queue[queue_id]))
		return -EBUSY;

	dev->_tx_queue[queue_id] = dev->ops->txq_configure(dev, queue_id,
							   nb_desc, tx_conf);
	if (PTRISERR(dev->_tx_queue[queue_id]))
		return PTR2ERR(dev->_tx_queue[queue_id]);

	uk_pr_info("netdev%"PRIu16": Configured transmit queue %"PRIu16"\n",
			   dev->_data->id, queue_id);
	return 0;
}

int uk_netdev_start(struct uk_netdev *dev)
{
	int ret;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->start);

	if (dev->_data->state != UK_NETDEV_CONFIGURED)
		return -EINVAL;

	ret = dev->ops->start(dev);
	if (ret >= 0) {
		uk_pr_info("netdev%"PRIu16": Started interface\n",
			   dev->_data->id);
		dev->_data->state = UK_NETDEV_RUNNING;
	}
	return ret;
}

int uk_netdev_hwaddr_set(struct uk_netdev *dev,
			 const struct uk_hwaddr *hwaddr)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(hwaddr);

	/* We do support changing of hwaddr
	 * only when device was configured
	 */
	UK_ASSERT(dev->_data->state == UK_NETDEV_CONFIGURED
		  || dev->_data->state == UK_NETDEV_RUNNING);

	if (dev->ops->hwaddr_set == NULL)
		return -ENOTSUP;

	return dev->ops->hwaddr_set(dev, hwaddr);
}

const struct uk_hwaddr *uk_netdev_hwaddr_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);

	/* We do support retrieving of hwaddr
	 * only when device was configured
	 */
	UK_ASSERT(dev->_data->state == UK_NETDEV_CONFIGURED
		  || dev->_data->state == UK_NETDEV_RUNNING);

	if (!dev->ops->hwaddr_get)
		return NULL;

	return dev->ops->hwaddr_get(dev);
}

unsigned uk_netdev_promiscuous_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->promiscuous_get);

	/* We do support retrieving of promiscuous mode
	 * only when device was configured
	 */
	UK_ASSERT(dev->_data->state == UK_NETDEV_CONFIGURED ||
		  dev->_data->state == UK_NETDEV_RUNNING);

	return dev->ops->promiscuous_get(dev);
}

int uk_netdev_promiscuous_set(struct uk_netdev *dev, unsigned mode)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);

	/* We do support setting of promiscuous mode
	 * only when device was configured
	 */
	UK_ASSERT(dev->_data->state == UK_NETDEV_CONFIGURED
		  || dev->_data->state == UK_NETDEV_RUNNING);

	if (unlikely(!dev->ops->promiscuous_set))
		return -ENOTSUP;

	return dev->ops->promiscuous_set(dev, mode ? 1 : 0);
}

uint16_t uk_netdev_mtu_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->ops->mtu_get);

	/* We do support getting of MTU
	 * only when device was configured
	 */
	UK_ASSERT(dev->_data->state == UK_NETDEV_CONFIGURED
		  || dev->_data->state == UK_NETDEV_RUNNING);

	return dev->ops->mtu_get(dev);
}

int uk_netdev_mtu_set(struct uk_netdev *dev, uint16_t mtu)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->ops);

	/* We do support setting of MTU
	 * only when device was configured
	 */
	UK_ASSERT(dev->_data->state == UK_NETDEV_CONFIGURED
		  || dev->_data->state == UK_NETDEV_RUNNING);

	if (dev->ops->mtu_set == NULL)
		return -ENOTSUP;

	return dev->ops->mtu_set(dev, mtu);
}

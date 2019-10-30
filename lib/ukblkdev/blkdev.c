/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
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
/* This is derived from uknetdev because of consistency reasons */
#define _GNU_SOURCE /* for asprintf() */
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/arch/atomic.h>
#include <uk/blkdev.h>

struct uk_blkdev_list uk_blkdev_list =
UK_TAILQ_HEAD_INITIALIZER(uk_blkdev_list);

static uint16_t blkdev_count;

static struct uk_blkdev_data *_alloc_data(struct uk_alloc *a,
		uint16_t blkdev_id,
		const char *drv_name)
{
	struct uk_blkdev_data *data;

	data = uk_calloc(a, 1, sizeof(*data));
	if (!data)
		return NULL;

	data->drv_name = drv_name;
	data->state    = UK_BLKDEV_UNCONFIGURED;
	data->a = a;
	/* This is the only place where we set the device ID;
	 * during the rest of the device's life time this ID is read-only
	 */
	*(DECONST(uint16_t *, &data->id)) = blkdev_id;

	return data;
}

int uk_blkdev_drv_register(struct uk_blkdev *dev, struct uk_alloc *a,
		const char *drv_name)
{
	UK_ASSERT(dev);

	/* Data must be unallocated. */
	UK_ASSERT(PTRISERR(dev->_data));
	/* Assert mandatory configuration. */
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_configure);
	UK_ASSERT(dev->dev_ops->dev_start);
	UK_ASSERT(dev->dev_ops->queue_setup);
	UK_ASSERT(dev->dev_ops->get_info);
	UK_ASSERT(dev->dev_ops->queue_get_info);
	UK_ASSERT(dev->submit_one);
	UK_ASSERT(dev->finish_reqs);
	UK_ASSERT((dev->dev_ops->queue_intr_enable &&
				dev->dev_ops->queue_intr_disable)
			|| (!dev->dev_ops->queue_intr_enable
				&& !dev->dev_ops->queue_intr_disable));

	dev->_data = _alloc_data(a, blkdev_count,  drv_name);
	if (!dev->_data)
		return -ENOMEM;

	UK_TAILQ_INSERT_TAIL(&uk_blkdev_list, dev, _list);
	uk_pr_info("Registered blkdev%"PRIu16": %p (%s)\n",
			blkdev_count, dev, drv_name);
	dev->_data->state = UK_BLKDEV_UNCONFIGURED;

	return blkdev_count++;
}

unsigned int uk_blkdev_count(void)
{
	return (unsigned int) blkdev_count;
}

struct uk_blkdev *uk_blkdev_get(unsigned int id)
{
	struct uk_blkdev *blkdev;

	UK_TAILQ_FOREACH(blkdev, &uk_blkdev_list, _list) {
		UK_ASSERT(blkdev->_data);
		if (blkdev->_data->id == id)
			return blkdev;
	}

	return NULL;
}

uint16_t uk_blkdev_id_get(struct uk_blkdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->id;
}

const char *uk_blkdev_drv_name_get(struct uk_blkdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->drv_name;
}

enum uk_blkdev_state uk_blkdev_state_get(struct uk_blkdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->state;
}

int uk_blkdev_get_info(struct uk_blkdev *dev,
		struct uk_blkdev_info *dev_info)
{
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->get_info);
	UK_ASSERT(dev_info);

	/* Clear values before querying driver for capabilities */
	memset(dev_info, 0, sizeof(*dev_info));
	dev->dev_ops->get_info(dev, dev_info);

	/* Limit the maximum number of queues
	 * according to the API configuration
	 */
	dev_info->max_queues = MIN(CONFIG_LIBUKBLKDEV_MAXNBQUEUES,
			dev_info->max_queues);

	return rc;
}

int uk_blkdev_configure(struct uk_blkdev *dev,
		const struct uk_blkdev_conf *conf)
{
	int rc = 0;
	struct uk_blkdev_info dev_info;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_configure);
	UK_ASSERT(conf);

	rc = uk_blkdev_get_info(dev, &dev_info);
	if (rc) {
		uk_pr_err("blkdev-%"PRIu16": Failed to get initial info: %d\n",
				dev->_data->id, rc);
		return rc;
	}

	if (conf->nb_queues > dev_info.max_queues)
		return -EINVAL;

	rc = dev->dev_ops->dev_configure(dev, conf);
	if (!rc) {
		uk_pr_info("blkdev%"PRIu16": Configured interface\n",
				dev->_data->id);
		dev->_data->state = UK_BLKDEV_CONFIGURED;
	} else
		uk_pr_err("blkdev%"PRIu16": Failed to configure interface %d\n",
				dev->_data->id, rc);

	return rc;
}

#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
static void _dispatcher(void *args)
{
	struct uk_blkdev_event_handler *handler =
		(struct uk_blkdev_event_handler *) args;

	UK_ASSERT(handler);
	UK_ASSERT(handler->callback);

	while (1) {
		uk_semaphore_down(&handler->events);
		handler->callback(handler->dev,
				handler->queue_id, handler->cookie);
	}
}
#endif

static int _create_event_handler(uk_blkdev_queue_event_t callback,
		void *cookie,
#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
		struct uk_blkdev *dev, uint16_t queue_id,
		struct uk_sched *s,
#endif
		struct uk_blkdev_event_handler *event_handler)
{
	UK_ASSERT(event_handler);
	UK_ASSERT(callback || (!callback && !cookie));

	event_handler->callback = callback;
	event_handler->cookie = cookie;

#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
	/* If we do not have a callback, we do not need a thread */
	if (!callback)
		return 0;

	event_handler->dev = dev;
	event_handler->queue_id = queue_id;
	uk_semaphore_init(&event_handler->events, 0);
	event_handler->dispatcher_s = s;

	/* Create a name for the dispatcher thread.
	 * In case of errors, we just continue without a name
	 */
	if (asprintf(&event_handler->dispatcher_name,
				"blkdev%"PRIu16"-q%"PRIu16"]",
				dev->_data->id, queue_id) < 0) {
		event_handler->dispatcher_name = NULL;
	}

	/* Create thread */
	event_handler->dispatcher = uk_sched_thread_create(
			event_handler->dispatcher_s,
			event_handler->dispatcher_name, NULL,
			_dispatcher, (void *)event_handler);
	if (event_handler->dispatcher == NULL) {
		if (event_handler->dispatcher_name) {
			free(event_handler->dispatcher);
			event_handler->dispatcher = NULL;
		}

		return -ENOMEM;
	}
#endif

	return 0;
}

static void _destroy_event_handler(struct uk_blkdev_event_handler *h
		__maybe_unused)
{
	UK_ASSERT(h);

#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
	if (h->dispatcher) {
		uk_semaphore_up(&h->events);
		UK_ASSERT(h->dispatcher_s);
		uk_thread_kill(h->dispatcher);
		uk_thread_wait(h->dispatcher);
		h->dispatcher = NULL;
	}

	if (h->dispatcher_name) {
		free(h->dispatcher_name);
		h->dispatcher_name = NULL;
	}
#endif
}

int uk_blkdev_queue_get_info(struct uk_blkdev *dev, uint16_t queue_id,
		struct uk_blkdev_queue_info *q_info)
{
	UK_ASSERT(dev);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(q_info);

	/* Clear values before querying driver for queue capabilities */
	memset(q_info, 0, sizeof(*q_info));
	return dev->dev_ops->queue_get_info(dev, queue_id, q_info);
}

int uk_blkdev_queue_configure(struct uk_blkdev *dev, uint16_t queue_id,
		uint16_t nb_desc,
		const struct uk_blkdev_queue_conf *queue_conf)
{
	int err = 0;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->queue_setup);
	UK_ASSERT(dev->finish_reqs);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(queue_conf);

#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
	UK_ASSERT((queue_conf->callback && queue_conf->s)
			|| !queue_conf->callback);
#endif

	if (dev->_data->state != UK_BLKDEV_CONFIGURED)
		return -EINVAL;

	/* Make sure that we are not initializing this queue a second time */
	if (!PTRISERR(dev->_queue[queue_id]))
		return -EBUSY;

	err = _create_event_handler(queue_conf->callback,
			queue_conf->callback_cookie,
#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
			dev, queue_id, queue_conf->s,
#endif
			&dev->_data->queue_handler[queue_id]);
	if (err)
		goto err_out;

	dev->_queue[queue_id] = dev->dev_ops->queue_setup(dev, queue_id,
			nb_desc,
			queue_conf);
	if (PTRISERR(dev->_queue[queue_id])) {
		err = PTR2ERR(dev->_queue[queue_id]);
		uk_pr_err("blkdev%"PRIu16"-q%"PRIu16": Failed to configure: %d\n",
				dev->_data->id, queue_id, err);
		goto err_destroy_handler;
	}

	uk_pr_info("blkdev%"PRIu16": Configured queue %"PRIu16"\n",
			dev->_data->id, queue_id);
	return 0;

err_destroy_handler:
	_destroy_event_handler(&dev->_data->queue_handler[queue_id]);
err_out:
	return err;
}

int uk_blkdev_start(struct uk_blkdev *dev)
{
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_start);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_CONFIGURED);

	rc = dev->dev_ops->dev_start(dev);
	if (rc)
		uk_pr_err("blkdev%"PRIu16": Failed to start interface %d\n",
				dev->_data->id, rc);
	else {
		uk_pr_info("blkdev%"PRIu16": Started interface\n",
				dev->_data->id);
		dev->_data->state = UK_BLKDEV_RUNNING;
	}

	return rc;
}

int uk_blkdev_queue_submit_one(struct uk_blkdev *dev,
		uint16_t queue_id,
		struct uk_blkreq *req)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->submit_one);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_RUNNING);
	UK_ASSERT(!PTRISERR(dev->_queue[queue_id]));
	UK_ASSERT(req != NULL);

	return dev->submit_one(dev, dev->_queue[queue_id], req);
}

int uk_blkdev_queue_finish_reqs(struct uk_blkdev *dev,
		uint16_t queue_id)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->finish_reqs);
	UK_ASSERT(dev->_data);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_RUNNING);
	UK_ASSERT(!PTRISERR(dev->_queue[queue_id]));

	return dev->finish_reqs(dev, dev->_queue[queue_id]);
}

#if CONFIG_LIBUKBLKDEV_SYNC_IO_BLOCKED_WAITING
/**
 * Used for sending a synchronous request.
 */
struct uk_blkdev_sync_io_request {
	struct uk_blkreq req;	/* Request structure. */

	/* Semaphore used for waiting after the response is done. */
	struct uk_semaphore s;
};

static void __sync_io_callback(struct uk_blkreq *req,
		void *cookie_callback)
{
	struct uk_blkdev_sync_io_request *sync_io_req;

	UK_ASSERT(req);
	UK_ASSERT(cookie_callback);

	sync_io_req = (struct uk_blkdev_sync_io_request *)cookie_callback;
	uk_semaphore_up(&sync_io_req->s);
}

int uk_blkdev_sync_io(struct uk_blkdev *dev,
		uint16_t queue_id,
		enum uk_blkreq_op operation,
		__sector start_sector,
		__sector nb_sectors,
		void *buf)
{
	struct uk_blkreq *req;
	int rc = 0;
	struct uk_blkdev_sync_io_request sync_io_req;

	UK_ASSERT(dev != NULL);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->submit_one);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_RUNNING);
	UK_ASSERT(!PTRISERR(dev->_queue[queue_id]));

	req = &sync_io_req.req;
	uk_blkreq_init(req, operation, start_sector, nb_sectors, buf,
			__sync_io_callback, (void *)&sync_io_req);
	uk_semaphore_init(&sync_io_req.s, 0);

	rc = uk_blkdev_queue_submit_one(dev, queue_id, req);
	if (unlikely(!uk_blkdev_status_successful(rc))) {
		uk_pr_err("blkdev%"PRIu16"-q%"PRIu16": Failed to submit I/O req: %d\n",
				dev->_data->id, queue_id, rc);
		return rc;
	}

	uk_semaphore_down(&sync_io_req.s);
	return req->result;
}
#endif

int uk_blkdev_stop(struct uk_blkdev *dev)
{
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_stop);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_RUNNING);

	uk_pr_info("Trying to stop blkdev%"PRIu16" device\n",
			dev->_data->id);
	rc = dev->dev_ops->dev_stop(dev);
	if (rc)
		uk_pr_err("Failed to stop blkdev%"PRIu16" device %d\n",
				dev->_data->id, rc);
	else {
		uk_pr_info("Stopped blkdev%"PRIu16" device\n",
				dev->_data->id);
		dev->_data->state = UK_BLKDEV_CONFIGURED;
	}

	return rc;
}

int uk_blkdev_queue_release(struct uk_blkdev *dev, uint16_t queue_id)
{
	int rc = 0;

	UK_ASSERT(dev != NULL);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->queue_release);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_CONFIGURED);
	UK_ASSERT(!PTRISERR(dev->_queue[queue_id]));

	rc = dev->dev_ops->queue_release(dev, dev->_queue[queue_id]);
	if (rc)
		uk_pr_err("Failed to release blkdev%"PRIu16"-q%"PRIu16": %d\n",
				dev->_data->id, queue_id, rc);
	else {
#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
		if (dev->_data->queue_handler[queue_id].callback)
			_destroy_event_handler(
					&dev->_data->queue_handler[queue_id]);
#endif
		uk_pr_info("Released blkdev%"PRIu16"-q%"PRIu16"\n",
				dev->_data->id, queue_id);
		dev->_queue[queue_id] = NULL;
	}

	return rc;
}

void uk_blkdev_drv_unregister(struct uk_blkdev *dev)
{
	uint16_t id;

	UK_ASSERT(dev != NULL);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_UNCONFIGURED);

	id = dev->_data->id;

	uk_free(dev->_data->a, dev->_data);
	UK_TAILQ_REMOVE(&uk_blkdev_list, dev, _list);
	blkdev_count--;

	uk_pr_info("Unregistered blkdev%"PRIu16": %p\n",
			id, dev);
}

int uk_blkdev_unconfigure(struct uk_blkdev *dev)
{
	uint16_t q_id;
	int rc;

	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_unconfigure);
	UK_ASSERT(dev->_data->state == UK_BLKDEV_CONFIGURED);
	for (q_id = 0; q_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES; ++q_id)
		UK_ASSERT(PTRISERR(dev->_queue[q_id]));

	rc = dev->dev_ops->dev_unconfigure(dev);
	if (rc)
		uk_pr_err("Failed to unconfigure blkdev%"PRIu16": %d\n",
				dev->_data->id, rc);
	else {
		uk_pr_info("Unconfigured blkdev%"PRIu16"\n", dev->_data->id);
		dev->_data->state = UK_BLKDEV_UNCONFIGURED;
	}

	return rc;
}

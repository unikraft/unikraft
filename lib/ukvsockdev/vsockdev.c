/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/alloc.h>
#include <uk/vsockdev.h>
#include <uk/event.h>
#include <uk/pm.h>
#include <uk/semaphore.h>
#include <uk/thread.h>

#include <string.h>
#include <stdio.h>

/* The used vsock device */
static struct uk_vsockdev *main_dev;

int uk_vsockets_init(void);
int uk_vsockets_term(void);

int uk_vsockdev_register(struct uk_vsockdev *dev)
{
	/* We currently do not support registering multiple drivers */
	UK_ASSERT(!main_dev);

	main_dev = dev;
	return uk_vsockets_init();
}

struct uk_vsockdev *uk_vsockdev_get(void)
{
	return main_dev;
}

int uk_vsockdev_transport_reset(struct uk_vsockdev *dev __unused)
{
	int rc;

	rc = uk_vsockets_term();
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to terminate all open vsockets: %d\n", rc);
		return rc;
	}

	rc = uk_pm_sysmigration();
	if (unlikely(rc < 0))
		return rc;

	return 0;
}

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
static __noreturn void _dispatcher(void *arg)
{
	struct uk_vsockdev_event_handler *handler =
		(struct uk_vsockdev_event_handler *)arg;

	UK_ASSERT(handler);
	UK_ASSERT(handler->callback);

	for (;;) {
		uk_semaphore_down_all(&handler->events);
		handler->callback(handler->dev,
				  handler->cookie);
	}
}
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */

static int _create_event_handler(uk_vsockdev_queue_event_func callback,
				 void *callback_cookie,
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				 struct uk_vsockdev *dev,
				 const char *queue_str,
				 struct uk_sched *s,
#endif  /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
				 struct uk_vsockdev_event_handler *h)
{
	UK_ASSERT(h);
	UK_ASSERT(callback || (!callback && !callback_cookie));
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT(!h->dispatcher);
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */

	h->callback = callback;
	h->cookie   = callback_cookie;

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	/* If we do not have a callback, we do not need a thread */
	if (!callback)
		return 0;

	h->dev = dev;
	uk_semaphore_init(&h->events, 0);
	h->dispatcher_s = s;
	h->dispatcher_name = queue_str;
	h->dispatcher = uk_sched_thread_create(h->dispatcher_s,
					       _dispatcher, h,
					       h->dispatcher_name);
	if (!h->dispatcher)
		return -ENOMEM;
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */

	return 0;
}

static void _destroy_event_handler(struct uk_vsockdev_event_handler *h
				   __maybe_unused)
{
	UK_ASSERT(h);

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT(h->dispatcher_s);

	if (h->dispatcher) {
		uk_sched_thread_terminate(h->dispatcher);
		h->dispatcher = NULL;
	}
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
}

void uk_vsockdev_rxqueue_unconfigure(struct uk_vsockdev *dev)
{
	_destroy_event_handler(&dev->_data.rxq_handler);
}

void uk_vsockdev_evqueue_unconfigure(struct uk_vsockdev *dev)
{
	_destroy_event_handler(&dev->_data.evq_handler);
}

int uk_vsockdev_rxqueue_configure(struct uk_vsockdev *dev,
				  struct uk_vsockdev_queue_conf *conf)
{
	int rc;

	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT((conf->callback && conf->s) || !conf->callback);
#endif  /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */

	rc = _create_event_handler(conf->callback, conf->callback_cookie,
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				   dev, "vsockdev-rxq", conf->s,
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
				   &dev->_data.rxq_handler);
	if (unlikely(rc))
		return rc;

	uk_pr_info("vsockdev: Configured receive queue\n");

	return 0;
}

int uk_vsockdev_evqueue_configure(struct uk_vsockdev *dev,
				  struct uk_vsockdev_queue_conf *conf)
{
	int rc;

	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT((conf->callback && conf->s) || !conf->callback);
#endif  /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */

	rc = _create_event_handler(conf->callback, conf->callback_cookie,
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				   dev, "vsockdev-evq", conf->s,
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
				   &dev->_data.evq_handler);
	if (unlikely(rc))
		return rc;

	uk_pr_info("vsockdev: Configured event queue\n");

	return 0;
}

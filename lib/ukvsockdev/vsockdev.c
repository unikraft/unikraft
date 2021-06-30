#define _GNU_SOURCE /* for asprintf() */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/arch/atomic.h>
#include <uk/vsockdev.h>

/*
 * Vsock device is different from the network device where we could have
 * multiple virtualized network interface cards on a machine. It does
 * not make sense to have multiple vsock devices on a machine, therefore
 * there is no list of devices.
 */
struct uk_vsockdev *uk_vskdev = NULL;

int uk_vsockdev_drv_register(struct uk_vsockdev *dev)
{
	UK_ASSERT(dev);

	/* Driver already registered */
	if (uk_vskdev) {
		uk_pr_err("There is already a vsock device registered");
		return 0;
	}

	UK_ASSERT(dev->dev_ops);

	uk_vskdev = dev;
	uk_pr_info("Registered vsockdev: %p\n", dev);

	return 1;
}

struct uk_vsockdev *uk_vsockdev_get()
{
	return uk_vskdev;
}

void uk_vsockdev_drv_unregister(struct uk_vsockdev *dev)
{
	UK_ASSERT(dev != NULL);

	uk_vskdev = NULL;

	uk_pr_info("Unregistered vsockdev: %p\n", dev);
}


#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
static void _dispatcher(void *arg)
{
	struct uk_vsockdev_event_handler *handler =
		(struct uk_vsockdev_event_handler *) arg;

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

static int _create_event_handler(uk_vsockdev_queue_event_t callback,
				 void *callback_cookie,
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				 struct uk_vsockdev *dev, uint16_t queue_id,
				 const char *queue_type_str,
				 struct uk_sched *s,
#endif
				 struct uk_vsockdev_event_handler *h)
{
	UK_ASSERT(h);
	UK_ASSERT(callback || (!callback && !callback_cookie));
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT(!h->dispatcher);
#endif

	h->callback = callback;
	h->cookie   = callback_cookie;

#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
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
		     "vsockdev%"PRIu16"-%s[%"PRIu16"]",
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

static void _destroy_event_handler(struct uk_vsockdev_event_handler *h
				   __maybe_unused)
{
	UK_ASSERT(h);

#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
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

static int uk_vsockdev_rx_one(struct uk_vsockdev *dev, 
			   uint16_t queue_id __unused, void *argp __unused)
{
	struct uk_alloc *a;
	struct virtio_vsock_packet *pkt;
	int rc = 0;

	a = uk_alloc_get_default();
	pkt = uk_calloc(a, 1, sizeof(struct virtio_vsock_packet));
	pkt->data = uk_calloc(a, VIRTIO_VSOCK_RX_DATA_SIZE, sizeof(__u8));

	UK_ASSERT(dev);
	UK_ASSERT(dev->rx_one);
	UK_ASSERT(pkt);
	
	rc = dev->rx_one(dev, dev->_rx_queue, &pkt);
	if (rc)
		return;

	return rc;
}

int uk_vsockev_rxq_configure(struct uk_vsockdev *dev, uint16_t queue_id,
			    uint16_t nb_desc,
			    struct uk_vsockdev_rxqueue_conf *rx_conf)
{
	int err;

	UK_ASSERT(dev);
	UK_ASSERT(rx_conf);

	err = _create_event_handler(rx_conf->callback, rx_conf->callback_cookie,
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				    dev, queue_id, "rxq", rx_conf->s,
#endif
				    &dev->rx_handler);
	if (err)
		goto err_out;

	dev->_rx_queue = dev->dev_ops->rxq_configure(dev, queue_id,
							   nb_desc, rx_conf);
	if (PTRISERR(dev->_rx_queue)) {
		err = PTR2ERR(dev->_rx_queue);
		goto err_destroy_handler;
	}

	return 0;

err_destroy_handler:
	_destroy_event_handler(&dev->rx_handler);
err_out:
	return err;
}

int uk_vsockdev_init() {
	int rc = 0;
	struct uk_vsockdev_rxqueue_conf rx_conf;
	struct uk_vsockdev_evqueue_conf ev_conf;
	struct uk_vsockdev_txqueue_conf tx_conf;
	struct uk_alloc *a;
	struct uk_vsockdev *dev = uk_vskdev;

	UK_ASSERT(dev);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_start);

	a = uk_alloc_get_default();

	rx_conf.a = a;
	rx_conf.callback = uk_vsockdev_rx_one;
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	rxq_conf.s = uk_sched_get_default();
	if (!rxq_conf.s)
		return ERR_IF;

#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
	rc = uk_vsockev_rxq_configure(dev, 0, 0, &rx_conf);

	ev_conf.a = a;
	dev->_ev_queue = dev->dev_ops->evq_configure(dev, 1, 0, &ev_conf);

	tx_conf.a = a;
	dev->_tx_queue = dev->dev_ops->txq_configure(dev, 2, 0, &tx_conf);

	rc = dev->dev_ops->dev_start(dev);
	
	return rc;
}

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_VSOCKDEV_CORE_H__
#define __UK_VSOCKDEV_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <uk/vsock.h>
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/isr/semaphore.h>
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */

struct uk_vsockdev;

/**
 * Queue Structure used for both requests and responses.
 * This is private to the drivers.
 * In the API, this structure is used only for type checking.
 */
struct uk_vsockdev_queue;

/**
 * Function type used for queue event callbacks.
 * Called when events occur on receive or event queues that require processing.
 * The callback is responsible for handling all available work in the queue.
 *
 * @param dev The Unikraft VSOCK device that generated the event
 * @param argp Extra argument that can be defined on callback registration
 */
typedef void (*uk_vsockdev_queue_event_func)(struct uk_vsockdev *dev,
					     void *argp);

/**
 * Configuration structure for setting up vsock device queues.
 * Contains all parameters needed to configure receive and event queues
 * including memory allocation, event callbacks, and thread scheduling.
 */
struct uk_vsockdev_queue_conf {
	/** Memory allocator used for descriptor rings and buffers */
	struct uk_alloc *a;
	/** Event callback function invoked when queue events occur */
	uk_vsockdev_queue_event_func callback;
	/** Argument pointer passed to callback function */
	void *callback_cookie;

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	/** Scheduler for dispatcher thread execution */
	struct uk_sched *s;
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
};

/*
 * ukvsockdev -> vsock driver interface
 */

/**
 * Driver callback to retrieve the Context ID (CID) of the device.
 * The CID uniquely identifies this virtual machine in the vsock transport.
 *
 * @param dev The vsock device
 * @return The CID assigned to this device
 */
typedef __u32 (*uk_vsockdev_get_cid_func)(struct uk_vsockdev *dev);

/**
 * Driver callback for creating a new socket.
 * Allocates and initializes driver-specific socket resources. The driver
 * must create a uk_vsock structure and return it via the out parameter.
 *
 * @param dev The vsock device
 * @param type Socket type (SOCK_STREAM, SOCK_SEQPACKET, etc.)
 * @param protocol Number of the procotol (typically 0 for vsock)
 * @param out Pointer to store the created socket structure
 * @return 0 on success, negative errno code on error
 */
typedef int (*uk_vsockdev_create_func)(struct uk_vsockdev *dev, int type,
				       int protocol, struct uk_vsock **out);

/**
 * Driver callback to accept an incoming connection.
 * Creates a new connected socket from a pending connection request.
 * This function will call the driver's free_accept_entry as it does not own
 * its allocation.
 *
 * @param dev The vsock device
 * @param sock The listening socket that received the connection
 * @param entry The accept entry containing peer connection information
 * @param out Pointer to store the newly created connected socket
 * @return 0 on success, negative errno code on error
 */
typedef int (*uk_vsockdev_accept_func)(struct uk_vsockdev *dev,
				       struct uk_vsock *sock,
				       struct uk_vsock_accept_entry *entry,
				       struct uk_vsock **out);

/**
 * Driver callback to free an accept entry created by the driver.
 *
 * @param dev The vsock device
 * @param entry The accept entry to free
 * @return 0 on success, negative errno code on error
 */
typedef
void (*uk_vsockdev_free_accept_entry_func)(struct uk_vsockdev *dev,
					   struct uk_vsock *sock,
					   struct uk_vsock_accept_entry *entry);

/**
 * Driver callback to send data over a socket.
 * Transmits data to the connected peer. The amount of data sent may be
 * less than requested due to flow control or buffer limitations.
 *
 * @param dev The vsock device
 * @param sock The socket to send data on
 * @param buf Buffer containing data to send
 * @param size Number of bytes to send
 * @return Number of bytes actually sent, or negative errno code on error
 */
typedef __ssz (*uk_vsockdev_send_func)(struct uk_vsockdev *dev,
				       struct uk_vsock *sock, const char *buf,
				       __sz size);

/**
 * Driver callback to initiate a connection to a peer.
 * Sends a connection request to the specified peer address.
 * The ukvsockdev library will have already initialized the address
 * structures in the uk_vsock before calling this function.
 *
 * @param dev The vsock device
 * @param sock The socket to connect (addresses already set)
 * @return 0 on success, negative errno code on error
 */
typedef int (*uk_vsockdev_connect_func)(struct uk_vsockdev *dev,
					struct uk_vsock *sock);

/**
 * Driver callback to shutdown a socket connection.
 * Initiates shutdown of the connection in one or both directions.
 * The peer will be notified of the shutdown.
 *
 * @param dev The vsock device
 * @param sock The socket to shutdown
 * @param rx Non-zero to shutdown receive direction
 * @param tx Non-zero to shutdown transmit direction
 * @return 0 on success, negative errno code on error
 */
typedef int (*uk_vsockdev_shutdown_func)(struct uk_vsockdev *dev,
					 struct uk_vsock *sock, int rx, int tx);

/**
 * Driver callback to reset a socket connection.
 * Forcibly terminates the connection and notifies the peer.
 * The uk_vsock structure will not be used after this call.
 *
 * @param dev The vsock device
 * @param sock The socket to reset
 * @return 0 on success, negative errno code on error
 */
typedef int (*uk_vsockdev_reset_func)(struct uk_vsockdev *dev,
				      struct uk_vsock *sock);

/**
 * Driver callback to destroy a socket and free resources.
 * Called when the socket is being destroyed. The driver should free
 * all resources associated with the socket, including the socket itself.
 *
 * @param dev The vsock device
 * @param sock The socket to destroy
 * @return 0 on success, negative errno code on error
 */
typedef int (*uk_vsockdev_destroy_func)(struct uk_vsockdev *dev,
					struct uk_vsock *sock);

/**
 * Driver operation callbacks structure.
 * Contains all the callback functions that drivers must implement
 * to provide vsock functionality.
 */
struct uk_vsockdev_ops {
	uk_vsockdev_get_cid_func get_cid;
	uk_vsockdev_create_func create;
	uk_vsockdev_accept_func accept;
	uk_vsockdev_free_accept_entry_func free_accept_entry;
	uk_vsockdev_send_func send;
	uk_vsockdev_connect_func connect;
	uk_vsockdev_shutdown_func shutdown;
	uk_vsockdev_reset_func reset;
	uk_vsockdev_destroy_func destroy;
};

/**
 * @internal
 * Event handler configuration (internal to libukvsockdev).
 * Manages callback execution and optional dispatcher thread for queue events.
 */
struct uk_vsockdev_event_handler {
	/** Callback function for queue events */
	uk_vsockdev_queue_event_func callback;
	/** Parameter for callback function */
	void *cookie;

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	/** Semaphore to trigger dispatcher thread events */
	struct uk_semaphore events;
	/** Reference to vsock device */
	struct uk_vsockdev *dev;
	/** Dispatcher thread for handling events */
	struct uk_thread *dispatcher;
	/** Name identifier for the dispatcher thread */
	const char *dispatcher_name;
	/** Scheduler for dispatcher thread execution */
	struct uk_sched *dispatcher_s;
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
};

/**
 * @internal
 * Internal data structure associated with each vsock device.
 * Contains event handlers for managing receive and event queues.
 */
struct uk_vsockdev_data {
	/** Event handler for receive queue processing */
	struct uk_vsockdev_event_handler rxq_handler;
	/** Event handler for event queue processing */
	struct uk_vsockdev_event_handler evq_handler;
};

/**
 * Representation of a VSOCK device.
 * The main device structure that drivers embed and extend with their
 * own device-specific data.
 */
struct uk_vsockdev {
	/** Internal library data for event handling */
	struct uk_vsockdev_data _data;
	/** Driver operation callbacks */
	const struct uk_vsockdev_ops *ops;
};

struct uk_vsockdev *uk_vsockdev_get(void);

/*
 * vsock driver -> ukvsockdev interface
 */

/**
 * Notify the ukvsockdev library of a transport reset.
 * Called when the underlying transport has been reset, such as during
 * VM migration, snapshot restore, or device reset. This will terminate
 * all established connections and preserve only listening sockets bound
 * to CID_ANY.
 *
 * @param dev The vsock device that experienced the transport reset
 * @return 0 on success, negative errno code on error
 */
int uk_vsockdev_transport_reset(struct uk_vsockdev *dev);

/**
 * Register a vsock device with the ukvsockdev subsystem.
 * Makes the device available for socket operations and initializes
 * the global socket tracking structures. Currently only supports
 * one device registration.
 *
 * @param dev The vsock device to register
 * @return 0 on success, negative errno code on error
 */
int uk_vsockdev_register(struct uk_vsockdev *dev);

/**
 * Forward a receive queue event to the registered callback.
 * Should be called from device interrupt context when data is available
 * in the receive queue. The event will be processed either immediately
 * or scheduled for dispatcher thread execution.
 *
 * @param dev The vsock device with receive queue activity
 */
__isr static inline void uk_vsockdev_notify_rxq(struct uk_vsockdev *dev)
{
	struct uk_vsockdev_event_handler *rxq_handler;

	UK_ASSERT(dev);

	rxq_handler = &dev->_data.rxq_handler;

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	uk_semaphore_up_isr(&rxq_handler->events);
#else /* !CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
	if (rxq_handler->callback)
		rxq_handler->callback(dev, rxq_handler->cookie);
#endif /* !CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
}

/**
 * Forward an event queue event to the registered callback.
 * Should be called from device interrupt context when events are available
 * in the event queue. The event will be processed either immediately
 * or scheduled for dispatcher thread execution.
 *
 * @param dev The vsock device with event queue activity
 */
__isr static inline void uk_vsockdev_notify_evq(struct uk_vsockdev *dev)
{
	struct uk_vsockdev_event_handler *evq_handler;

	UK_ASSERT(dev);

	evq_handler = &dev->_data.evq_handler;

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	uk_semaphore_up_isr(&evq_handler->events);
#else /* !CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
	if (evq_handler->callback)
		evq_handler->callback(dev, evq_handler->cookie);
#endif /* !CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
}

/**
 * Configure the event queue for a vsock device.
 * Sets up event handling for device events such as transport resets.
 * The callback will be invoked when events need processing.
 *
 * @param dev The vsock device to configure
 * @param conf Configuration parameters for the event queue
 * @return 0 on success, negative errno code on error
 */
int uk_vsockdev_evqueue_configure(struct uk_vsockdev *dev,
				  struct uk_vsockdev_queue_conf *conf);

/**
 * Configure the receive queue for a vsock device.
 * Sets up receive data handling for incoming packets and connection events.
 * The callback will be invoked when data or connection events are available.
 *
 * @param dev The vsock device to configure
 * @param conf Configuration parameters for the receive queue
 * @return 0 on success, negative errno code on error
 */
int uk_vsockdev_rxqueue_configure(struct uk_vsockdev *dev,
				  struct uk_vsockdev_queue_conf *conf);

/**
 * Unconfigure the event queue for a vsock device.
 * Stops event processing and cleans up associated resources including
 * any dispatcher threads that were created for event handling.
 *
 * @param dev The vsock device to unconfigure
 */
void uk_vsockdev_evqueue_unconfigure(struct uk_vsockdev *dev);

/**
 * Unconfigure the receive queue for a vsock device.
 * Stops receive processing and cleans up associated resources including
 * any dispatcher threads that were created for receive handling.
 *
 * @param dev The vsock device to unconfigure
 */
void uk_vsockdev_rxqueue_unconfigure(struct uk_vsockdev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __UK_VSOCKDEV_CORE_H__ */

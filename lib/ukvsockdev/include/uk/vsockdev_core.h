
#ifndef __UK_VSOCKDEV_CORE__
#define __UK_VSOCKDEV_CORE__

#define VIRTIO_VSOCK_TYPE_STREAM	1
#define VIRTIO_VSOCK_TYPE_DGRAM		2

#define VIRTIO_VSOCK_RX_DATA_SIZE	1024 * 4
#define VIRTIO_VSOCK_BUF_DATA_SIZE	1024 * 4

#include <uk/list.h>
#include <uk/config.h>
#include <fcntl.h>
#if defined(CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS)
#include <uk/sched.h>
#include <uk/semaphore.h>
#endif

/**
 * Unikraft vsock API common declarations.
 *
 * This header contains all API data types. Some of them are part of the
 * public API and some are part of the internal API.
 *
 * The device data and operations are separated. This split allows the
 * function pointer and driver data to be per-process, while the actual
 * configuration data for the device is shared.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct uk_vsockdev;

struct virtio_vsock_hdr {
	__u64	src_cid;
	__u64	dst_cid;
	__u32	src_port;
	__u32	dst_port;
	__u32	len;
	__u16	type;
	__u16	op;
	__u32	flags;
	__u32	buf_alloc;
	__u32	fwd_cnt;
} __packed;

struct virtio_vsock_packet {
    struct virtio_vsock_hdr hdr;
    __u8 *data;
};

enum virtio_vsock_op {
	VIRTIO_VSOCK_OP_INVALID = 0,

	/* Connect operations */
	VIRTIO_VSOCK_OP_REQUEST = 1,
	VIRTIO_VSOCK_OP_RESPONSE = 2,
	VIRTIO_VSOCK_OP_RST = 3,
	VIRTIO_VSOCK_OP_SHUTDOWN = 4,

	/* To send payload */
	VIRTIO_VSOCK_OP_RW = 5,

	/* Tell the peer our credit info */
	VIRTIO_VSOCK_OP_CREDIT_UPDATE = 6,
	/* Request the peer to send the credit info to us */
	VIRTIO_VSOCK_OP_CREDIT_REQUEST = 7,
};

enum virtio_vsock_event_id {
	VIRTIO_VSOCK_EVENT_TRANSPORT_RESET = 0,
};

struct virtio_vsock_event {
	__u32 id;
};

/**
 * Function type used for queue event callbacks.
 *
 * @param dev
 *   The Unikraft Vsock Device.
 * @param queue
 *   The queue on the Unikraft vsock device on which the event happened.
 * @param argp
 *   Extra argument that can be defined on callback registration.
 */
typedef void (*uk_vsockdev_queue_event_t)(struct uk_vsockdev *dev,
					uint16_t queue_id, void *argp);

/**
 * User callback used by the driver to allocate vsockbufs
 * that are used to setup receive descriptors.
 *
 * @param argp
 *   User-provided argument.
 * @param pkts
 *   Array for vsockbuf pointers that the function should allocate.
 * @param count
 *   Number of vsockbufs requested (equal to length of pkts).
 * @return
 *   Number of successful allocated vsockbufs,
 *   has to be in range [0, count].
 *   References to allocated packets are placed to pkts[0]...pkts[count -1].
 */
typedef uint16_t (*uk_vsockdev_alloc_rxpkts)(void *argp,
					   struct virtio_vsock_packet *pkts[],
					   uint16_t count);


/**
 * A structure used to configure an Unikraft vsock device RX queue.
 */
struct uk_vsockdev_rxqueue_conf {
	uk_vsockdev_queue_event_t callback; /**< Event callback function. */
	void *callback_cookie;            /**< Argument pointer for callback. */

	struct uk_alloc *a;               /**< Allocator for descriptors. */

	uk_vsockdev_alloc_rxpkts alloc_rxpkts; /**< Allocator for rx vsockbufs */
	void *alloc_rxpkts_argp;             /**< Argument for alloc_rxpkts */
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	struct uk_sched *s;               /**< Scheduler for dispatcher. */
#endif
};

/**
 * A structure used to configure an Unikraft vsock device TX queue.
 */
struct uk_vsockdev_txqueue_conf {
	struct uk_alloc *a;               /* Allocator for descriptors. */
};

/**
 * A structure used to configure an Unikraft vsock device EV queue.
 */
struct uk_vsockdev_evqueue_conf {
	struct uk_alloc *a;               /* Allocator for descriptors. */
};

/** Driver callback type to set up a RX queue of an Unikraft vsock device. */
typedef struct uk_vsockdev_tx_queue * (*uk_vsockdev_txq_configure_t)(
	struct uk_vsockdev *dev, uint16_t queue_id, uint16_t nb_desc,
	struct uk_vsockdev_txqueue_conf *tx_conf);

/** Driver callback type to set up a TX queue of an Unikraft vsock device. */
typedef struct uk_vsockdev_rx_queue * (*uk_vsockdev_rxq_configure_t)(
	struct uk_vsockdev *dev, uint16_t queue_id, uint16_t nb_desc,
	struct uk_vsockdev_rxqueue_conf *rx_conf);

/** Driver callback type to set up a EV queue of an Unikraft vsock device. */
typedef struct uk_vsockdev_ev_queue * (*uk_vsockdev_evq_configure_t)(
	struct uk_vsockdev *dev, uint16_t queue_id, uint16_t nb_desc,
	struct uk_vsockdev_evqueue_conf *ev_conf);

/** Driver callback type to start a configured Unikraft vsock device. */
typedef int (*uk_vsockdev_start_t)(struct uk_vsockdev *dev);

/** Driver callback type to stop a configured Unikraft vsock device. */
typedef int (*uk_vsockdev_stop_t)(struct uk_vsockdev *dev);

/** Driver callback type to get cid of a configured Unikraft vsock device. */
typedef __u64 (*uk_vsockdev_get_cid_t)(struct uk_vsockdev *dev);

struct uk_vsockdev_ops {
	uk_vsockdev_txq_configure_t			txq_configure;
	uk_vsockdev_rxq_configure_t			rxq_configure;
	uk_vsockdev_evq_configure_t			evq_configure;
	uk_vsockdev_start_t					dev_start;
	uk_vsockdev_stop_t					dev_stop;
	uk_vsockdev_get_cid_t				get_cid;
};

/** Driver callback type to retrieve one packet from a RX queue. */
typedef int (*uk_vsockdev_rx_one_t)(struct uk_vsockdev *dev,
				  struct uk_vsockdev_rx_queue *queue,
				  struct virtio_vsock_packet **pkt);

/** Driver callback type to submit one packet to a TX queue. */
typedef int (*uk_vsockdev_tx_one_t)(struct uk_vsockdev *dev,
				  struct uk_vsockdev_tx_queue *queue,
				  struct virtio_vsock_packet *pkt);

/** Driver callback type to retrieve one packet from a EV queue. */
typedef int (*uk_vsockdev_ev_one_t)(struct uk_vsockdev *dev,
				  struct uk_vsockdev_ev_queue *queue,
				  struct virtio_vsock_packet **pkt);

/**
 * @internal
 * Event handler configuration (internal to libvsockdev)
 */
struct uk_vsockdev_event_handler {
	uk_vsockdev_queue_event_t callback;
	void                    *cookie;

#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	struct uk_semaphore events;      /**< semaphore to trigger events */
	struct uk_vsockdev    *dev;        /**< reference to vsock device */
	uint16_t            queue_id;    /**< queue id which caused event */
	struct uk_thread    *dispatcher; /**< dispatcher thread */
	char                *dispatcher_name; /**< reference to thread name */
	struct uk_sched     *dispatcher_s;    /**< Scheduler for dispatcher. */
#endif
};

struct uk_vsockdev {
	/** Packet transmission. */
	uk_vsockdev_tx_one_t          tx_one; /* by driver */

	/** Packet reception. */
	uk_vsockdev_rx_one_t          rx_one; /* by driver */

	/** Pointers to queues (API-private) */
	struct uk_vsockdev_rx_queue   *_rx_queue;
	struct uk_vsockdev_tx_queue   *_tx_queue;
	struct uk_vsockdev_ev_queue   *_ev_queue;

	struct uk_vsockdev_event_handler rx_handler;

	/* Functions callbacks by driver. */
	const struct uk_vsockdev_ops *dev_ops;
};

/**
 * Function type used for queue event callbacks.
 *
 * @param dev
 *   The Unikraft Vsock Device.
 * @param queue
 *   The queue on the Unikraft vsock device on which the event happened.
 * @param argp
 *   Extra argument that can be defined on callback registration.
 *
 * Note: This should call dev->finish_reqs function in order to process the
 *   received responses.
 */
typedef void (*uk_vsockdev_queue_event_t)(struct uk_vsockdev *dev,
		uint16_t queue_id, void *argp);


/**
 * Structure used to configure an Unikraft vsock device queue.
 *
 */
struct uk_vsockdev_queue_conf {
	/* Allocator used for descriptor rings */
	struct uk_alloc *a;
	/* Event callback function */
	uk_vsockdev_queue_event_t callback;
	/* Argument pointer for callback*/
	void *callback_cookie;

#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	/* Scheduler for dispatcher. */
	struct uk_sched *s;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_VSOCKDEV_CORE__ */

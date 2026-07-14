/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __VIRTIO_CONSOLE_PRIV_H__
#define __VIRTIO_CONSOLE_PRIV_H__

#include <uk/alloc.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/semaphore.h>
#include <uk/sched.h>

#include <virtio/virtio_console.h>

/*
 * Size of each pre-allocated RX buffer. One buffer is posted per descriptor
 * slot; the host fills it with incoming bytes and reports the used length.
 */
#define VTCONS_RX_BUFSZ				4096UL

/*
 * Maximum length of a port name carried in a VIRTIO_CONSOLE_PORT_NAME
 * control message (excluding the NUL terminator).
 */
#define VTCONS_PORT_NAME_LEN			64UL

/*
 * Size of each control RX buffer.  Must be large enough to hold a
 * struct virtio_console_control header plus a trailing port name.
 */
#define VTCONS_CTL_RX_BUFSZ						\
	(sizeof(struct virtio_console_control) + VTCONS_PORT_NAME_LEN)

#if CONFIG_LIBUKFS_DEVFS
/*
 * List of all probed vtcons_dev instances.  Used by the devfs initcall
 * to create device nodes and symlinks that were pending while devfs was
 * not yet available.
 */
extern struct uk_list_head vtcons_dev_list;
#endif /* CONFIG_LIBUKFS_DEVFS */

/* Cookie stored in the virtqueue for each posted RX descriptor. */
struct vtcons_rxbuf {
	char data[VTCONS_RX_BUFSZ];
};

struct vtcons_rxq {
	/* The virtqueue used for receiving data from the host. */
	struct virtqueue *vq;
	/* Number of descriptor slots in use. */
	__u16 nb_desc;
	/* Maximum number of descriptor slots the device supports. */
	__u16 max_nb_desc;
};

struct vtcons_txq {
	/* The virtqueue used for sending data to the host. */
	struct virtqueue *vq;
	/* Number of descriptor slots in use. */
	__u16 nb_desc;
	/* Maximum number of descriptor slots the device supports. */
	__u16 max_nb_desc;
};

/* Forward declaration needed by vtcons_port. */
struct vtcons_dev;

struct vtcons_port {
	__u32 id;
	struct vtcons_rxq rxq;
	struct vtcons_txq txq;
	char name[VTCONS_PORT_NAME_LEN + 1];
	/* ukconsole async device representation for this port. */
	struct uk_console_async con_drv;
	/* 1 if currently registered with uk_console, 0 otherwise. */
	int is_registered;
	/* 1 if registered with UK_CONSOLE_FLAG_STDOUT via CONSOLE_PORT. */
	int is_console;
	/* Back-pointer to the owning vtcons_dev. */
	struct vtcons_dev *dev;
	/*
	 * Partially consumed RX buffer held between vtcons_in calls.
	 * When the caller's buffer is too small to drain an entire rxbuf,
	 * the rxbuf is stashed here instead of being freed, so that the
	 * remaining bytes are delivered on the next call.
	 * NULL if no partial buffer is pending.
	 */
	struct vtcons_rxbuf *cached_rxbuf;
	/* Number of valid bytes the host wrote into cached_rxbuf->data. */
	__sz cached_rxbuf_len;
	/*
	 * Byte offset into cached_rxbuf->data of the next unread byte.
	 * Valid only when cached_rxbuf != NULL.
	 */
	__sz cached_rxbuf_pos;
};

#if CONFIG_LIBUKFS_DEVFS
/* Defined in virtio_console_devfs.c */
void vtcons_devfs_mknode(struct vtcons_port *port);
void vtcons_devfs_rmnode(struct vtcons_port *port);
void vtcons_devfs_mksymlink(struct vtcons_port *port);
#endif /* CONFIG_LIBUKFS_DEVFS */

struct vtcons_dev {
	/* Underlying virtio device. */
	struct virtio_dev *vdev;

	/* Number of ports the device supports (read from virtio_console_config
	 * at probe time).
	 */
	__u32 max_nr_ports;

	/* Per-port state (dynamically allocated array of max_nr_ports
	 * entries).
	 */
	struct vtcons_port *ports;

	/* All queue descriptor sizes returned by virtio_find_vqs, kept for
	 * lazy per-port queue setup.
	 */
	__u16 *qdesc_sizes;

	/* Control virtqueues, valid only when VIRTIO_CONSOLE_F_MULTIPORT
	 * has been negotiated.
	 */
	struct vtcons_rxq ctlrxq; /* control receiveq  (device -> driver) */
	struct vtcons_txq ctltxq; /* control transmitq (driver -> device) */

	/* Semaphore woken by the control virtqueue callbacks.  The
	 * control-receiver thread sleeps on this and processes both control
	 * RX messages and control TX completions.
	 */
	struct uk_semaphore ctl_sem;

	/* Control-receiver thread (vtcons_ctlr<N>). */
	struct uk_thread *ctl_thread;

	/* Storage for the formatted thread name so the pointer passed to
	 * uk_sched_thread_create() stays valid for the thread's lifetime.
	 */
	char ctl_thr_name[32];

	/* Unique device index assigned at probe time; used to form
	 * /dev/vportXportY node names.
	 */
	__u32 id;

#if CONFIG_LIBUKFS_DEVFS
	/* Set to 1 by vtcons_devfs_init once /dev is writable. */
	int devfs_ready;
	/* Link into vtcons_dev_list. */
	struct uk_list_head list;
#endif /* CONFIG_LIBUKFS_DEVFS */
};

#endif /* __VIRTIO_CONSOLE_PRIV_H__ */

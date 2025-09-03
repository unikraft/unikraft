/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/**
 * Taken and adapted from the Linux kernel.
 * include/uapi/linux/virtio_fs.h
 *
 * Tag: v6.11
 */

#ifndef __VIRTIO_FS_H__
#define __VIRTIO_FS_H__

#include <virtio/virtio_ids.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_types.h>

struct virtio_fs_config {
	/* Filesystem name (UTF-8, not NUL-terminated, padded with NULs) */
	__u8 tag[36];

	/* Number of request queues */
	__virtio_le32 num_request_queues;
} __attribute__((packed));

/* For the id field in virtio_pci_shm_cap */
#define VIRTIO_FS_SHMCAP_ID_CACHE 0

#endif /* __VIRTIO_FS_H__ */

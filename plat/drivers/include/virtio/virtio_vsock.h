#ifndef __PLAT_DRV_VIRTIO_VSOCK_H
#define __PLAT_DRV_VIRTIO_VSOCK_H

#include <virtio/virtio_ids.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_types.h>

struct virtio_vsock_config {
	__u64 guest_cid;
} __packed;

#endif /* __PLAT_DRV_VIRTIO_VSOCK_H */

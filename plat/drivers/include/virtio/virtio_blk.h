/* SPDX-License-Identifier: BSD-3-Clause */
/* This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 **/
/**
 * Taken and modified from Linux kernel
 * include/uapi/linux/virtio_blk.h
 *
 * commit-id: 8005803a2
 */
#ifndef __PLAT_DRV_VIRTIO_BLK_H
#define __PLAT_DRV_VIRTIO_BLK_H
#include <virtio/virtio_ids.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_types.h>

/* Feature bits */
#define VIRTIO_BLK_F_RO		5	/* Disk is read-only */
#define VIRTIO_BLK_F_BLK_SIZE	6	/* Block size of disk is available*/
#define VIRTIO_BLK_F_MQ		12	/* support more than one vq */

struct virtio_blk_config {
	/* The capacity (in 512-byte sectors). */
	__u64 capacity;

	/* block size of device (if VIRTIO_BLK_F_BLK_SIZE) */
	__u32 blk_size;

	/* number of vqs, only available when VIRTIO_BLK_F_MQ is set */
	__u16 num_queues;
} __packed;

#endif /* __PLAT_DRV_VIRTIO_BLK_H */

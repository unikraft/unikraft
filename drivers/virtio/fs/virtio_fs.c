/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <errno.h>

#include <uk/arch/paging.h>
#include <uk/virtio_fs.h>
#include <uk/sched.h>
#include <uk/thread.h>

#include <virtio/virtio_bus.h>
#include <virtio/virtio_fs.h>

static struct uk_alloc *virtiofs_alloc;

static int virtio_fs_init(struct uk_alloc *al)
{
	virtiofs_alloc = al;
	return 0;
}

struct uk_virtiofs_dev {
	struct uk_virtiofs_dev *next;
	/* Filled in at device discovery */
	struct virtio_dev *vdev;
	char tag[36];
	/* Filled in once configured */
	__u16 vq_len[2];
	struct virtqueue *vq_prio;
	struct virtqueue *vq_req;
};

static struct uk_virtiofs_dev *devlist;

static struct virtio_driver virtio_fs_drv;

static
int virtiofs_dev_negotiate(struct virtio_dev *vdev)
{
	__u64 dev_features;
	__u64 drv_features = 0;
	__u8 dev_status;
	int r;

	dev_features = virtio_feature_get(vdev);
	uk_pr_debug("Dev %p supports features %lx\n", vdev, dev_features);

	if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_F_VERSION_1))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_VERSION_1);
	else if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_F_ANY_LAYOUT))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_ANY_LAYOUT);

	uk_pr_debug("Dev %p requesting features %lx\n", vdev, drv_features);
	vdev->features = drv_features;
	virtio_feature_set(vdev);
	r = virtio_dev_status_update(vdev,
				     (VIRTIO_CONFIG_STATUS_ACK |
				      VIRTIO_CONFIG_STATUS_DRIVER |
				      VIRTIO_CONFIG_STATUS_FEATURES_OK));
	/* Check device is OK with our features */
	if (unlikely(r))
		return r;

	dev_status = virtio_dev_status_get(vdev);
	if (unlikely(!(dev_status & VIRTIO_CONFIG_STATUS_FEATURES_OK)))
		return -EINVAL;
	return 0;
}

static int virtio_fs_add_dev(struct virtio_dev *dev)
{
	struct uk_virtiofs_dev *vfsdev;
	int r;

	r = virtiofs_dev_negotiate(dev);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;

	vfsdev = uk_malloc(virtiofs_alloc, sizeof(*vfsdev));
	if (unlikely(!vfsdev))
		return -ENOMEM;

	vfsdev->vdev = dev;
	/* Read in filesystem tag from device config space */
	r = virtio_config_get(dev, __offsetof(struct virtio_fs_config, tag),
			      vfsdev->tag, sizeof(vfsdev->tag), 1);
	UK_ASSERT(r <= 0);
	if (unlikely(r)) {
		uk_free(virtiofs_alloc, vfsdev);
		return r;
	}
	/* Add to device list */
	vfsdev->next = devlist;
	/* Devices are discovered sequentially, no synchronization needed */
	devlist = vfsdev;

	uk_pr_debug("Dev %p discovered with tag \"%.36s\"\n",
		    dev, vfsdev->tag);
	return 0;
}

static const struct virtio_dev_id virtio_fs_dev_ids[] = {
	{VIRTIO_ID_FS},
	{VIRTIO_ID_INVALID} /* Terminator */
};

static struct virtio_driver virtio_fs_drv = {
	.dev_ids = virtio_fs_dev_ids,
	.init = virtio_fs_init,
	.add_dev = virtio_fs_add_dev
};

VIRTIO_BUS_REGISTER_DRIVER(&virtio_fs_drv);

struct uk_virtiofs_dev *uk_virtiofs_dev_lookup(const char *tag)
{
	struct uk_virtiofs_dev *p = devlist;

	while (p && strncmp(tag, p->tag, 36))
		p = p->next;
	return p;
}

/* TODO: replace with a waitq-based approach */
struct virtiofs_thread_cookie {
	struct uk_thread *thread;
	__u32 rlen;
};

/* Called when a reply is received; dequeue and wake the target thread */
static
int virtiofs_recv(struct virtqueue *vq, void *priv __unused)
{
	struct virtiofs_thread_cookie *cookie;
	__u32 len;
	int r;

	r = virtqueue_buffer_dequeue(vq, (void **)&cookie, &len);
	if (unlikely(r < 0))
		return r;
	cookie->rlen = len;
	uk_thread_wake(cookie->thread);
	return 0;
}

static
int virtiofs_dev_vqalloc(struct uk_virtiofs_dev *dev, unsigned int num_req)
{
	int vq_avail;

	if (unlikely(num_req < 1))
		return -EINVAL;
	if (num_req > 1)
		uk_pr_info("Multiqueue device driven through a single queue\n");

	/* We only support using 1 request queue, ignore others */
	/* Virtio FS devices use 2 VQs per request queue */
	vq_avail = virtio_find_vqs(dev->vdev, 2, dev->vq_len);

	uk_pr_debug("Dev %p found %d virtqueues\n", dev, vq_avail);
	if (unlikely(vq_avail != 2))
		return -ENOMEM;

	/* Set up normal request queue (index 1) */
	dev->vq_req = virtio_vqueue_setup(dev->vdev, 1, dev->vq_len[1],
					  virtiofs_recv, virtiofs_alloc);
	if (unlikely(PTRISERR(dev->vq_req)))
		return PTR2ERR(dev->vq_req);

	/* Set up priority request queue (index 0) */
	dev->vq_prio = virtio_vqueue_setup(dev->vdev, 0, dev->vq_len[0],
					   virtiofs_recv, virtiofs_alloc);
	if (unlikely(PTRISERR(dev->vq_prio))) {
		virtio_vqueue_release(dev->vdev, dev->vq_req, virtiofs_alloc);
		return PTR2ERR(dev->vq_prio);
	}

	return 0;
}

static
int virtiofs_dev_vqsetup(struct uk_virtiofs_dev *dev)
{
	__virtio_le32 nq_req = 0;
	int r;

	r = virtio_config_get(dev->vdev,
			      __offsetof(struct virtio_fs_config,
					 num_request_queues),
			      &nq_req, sizeof(nq_req), 1);
	UK_ASSERT(r <= 0);
	if (unlikely(r))
		return r;

	return virtiofs_dev_vqalloc(dev, nq_req);
}

int uk_virtiofs_dev_configure(struct uk_virtiofs_dev *dev)
{
	int r;

	r = virtiofs_dev_vqsetup(dev);
	if (unlikely(r))
		return r;
	/* Driver up */
	virtio_dev_drv_up(dev->vdev);
	/* Enable vq interrupts */
	virtqueue_intr_enable(dev->vq_req);
	virtqueue_intr_enable(dev->vq_prio);
	return 0;
}

void uk_virtiofs_dev_shutdown(struct uk_virtiofs_dev *dev)
{
	UK_ASSERT(dev->vq_prio);
	UK_ASSERT(dev->vq_req);
	/* TODO: Cleanup pending requests ? */
	virtqueue_intr_disable(dev->vq_prio);
	virtqueue_intr_disable(dev->vq_req);
	virtio_vqueue_release(dev->vdev, dev->vq_prio, virtiofs_alloc);
	virtio_vqueue_release(dev->vdev, dev->vq_req, virtiofs_alloc);
}

/**
 * Compute the maximum amount of physical segments needed to cover iov[iovlen].
 *
 * Assumes the worst-case scenario where no two pages are physically contiguous.
 * If the result would be greater than `max`, return `max` instead.
 */
static
__u16 virtiofs_maxsegs(const struct uk_virtiofs_iovec *iov,
		       __u16 iovlen, __u16 max)
{
	__sz segs = 0;

	for (int i = 0; i < iovlen; i++)
		segs += iov[i].iov_len / PAGE_SIZE + 1;
	return (segs > max) ? max : (__u16)segs;
}

/**
 * Prepare a sglist to point to the contents of iov[iovlen].
 */
static
int virtiofs_prep_sglist(struct uk_sglist *sg,
			 const struct uk_virtiofs_iovec *iov, __u16 iovlen)
{
	for (int i = 0; i < iovlen; i++) {
		int r = uk_sglist_append(sg, iov[i].iov_base, iov[i].iov_len);

		if (unlikely(r))
			return r;
	}
	return 0;
}

/**
 * Perform a synchronous request on `vq`, as described by iovecs[num_iovec].
 */
static
__ssz virtiofs_do_request_vec(struct virtqueue *vq, __u16 vqlen,
			      const struct uk_virtiofs_iovecs *iovecs,
			      __sz num_iovec, __sz in_iovcnt, __sz out_iovcnt)
{
	struct uk_thread *const current = uk_thread_current();
	struct virtiofs_thread_cookie cookie = { .thread = current };
	struct uk_sglist sg;
	struct uk_sglist sg_out;
	__u16 nseg;
	__u16 in_segs;
	__u16 out_segs;
	__sz maxsegs = 0;
	__sz ivec = 0;
	__sz pos = 0;
	__sz rem;
	int r;

	for (__sz i = 0; i < num_iovec; i++) {
		maxsegs += virtiofs_maxsegs(iovecs[i].iov, iovecs[i].iovcnt,
					    vqlen);
		if (maxsegs > vqlen) {
			maxsegs = vqlen;
			break;
		}
	}
	nseg = (__u16)maxsegs;
	/* We must declare this here, as its size is runtime-computed */
	struct uk_sglist_seg sgsegs[nseg];

	/* Prepare input iovs/segments */
	uk_sglist_init(&sg, nseg, sgsegs);
	while (in_iovcnt) {
		UK_ASSERT(ivec < num_iovec);
		while (pos >= iovecs[ivec].iovcnt) {
			ivec++;
			pos = 0;
			UK_ASSERT(ivec < num_iovec);
		}
		/* ivec & pos valid */
		rem = MIN(in_iovcnt, iovecs[ivec].iovcnt - pos);
		UK_ASSERT(rem);

		r = virtiofs_prep_sglist(&sg, &iovecs[ivec].iov[pos], rem);
		if (unlikely(r))
			return r;

		pos += rem;
		in_iovcnt -= rem;
	}
	in_segs = sg.sg_nseg;
	/* Prepare output iovs/segments */
	uk_sglist_init(&sg_out, nseg - in_segs, &sgsegs[in_segs]);
	while (out_iovcnt) {
		UK_ASSERT(ivec < num_iovec);
		while (pos >= iovecs[ivec].iovcnt) {
			ivec++;
			pos = 0;
			UK_ASSERT(ivec < num_iovec);
		}
		/* ivec & pos valid */
		rem = MIN(out_iovcnt, iovecs[ivec].iovcnt - pos);
		UK_ASSERT(rem);

		r = virtiofs_prep_sglist(&sg_out, &iovecs[ivec].iov[pos], rem);
		if (unlikely(r))
			return r;

		pos += rem;
		out_iovcnt -= rem;
	}
	out_segs = sg_out.sg_nseg;

	sg.sg_nseg += out_segs;

	/* Enqueue request */
	r = virtqueue_buffer_enqueue(vq, &cookie, &sg, in_segs, out_segs);
	if (unlikely(r < 0))
		return r;
	/* Notify the host & block until awoken by request completion */
	uk_thread_block_until(current, 0);
	virtqueue_host_notify(vq);
	uk_sched_yield();

	/* Awoken, request complete; cookie has length of reply from device */
	return cookie.rlen;
}

/**
 * Perform a synchronous request on `vq`, as described by iov[].
 */
static
__ssz virtiofs_do_request(struct virtqueue *vq, __u16 vqlen,
			  const struct uk_virtiofs_iovec *iov,
			  __u16 in_iovlen, __u16 out_iovlen)
{
	struct uk_thread *const current = uk_thread_current();
	struct virtiofs_thread_cookie cookie = { .thread = current };
	const __u16 iovlen = in_iovlen + out_iovlen;
	const __u16 nseg = virtiofs_maxsegs(iov, iovlen, vqlen);
	struct uk_sglist sg;
	struct uk_sglist sg_out;
	struct uk_sglist_seg sgsegs[nseg];
	__u16 in_segs;
	__u16 out_segs;
	int r;

	/* Prepare input iovs/segments */
	uk_sglist_init(&sg, nseg, sgsegs);
	r = virtiofs_prep_sglist(&sg, iov, in_iovlen);
	if (unlikely(r))
		return r;
	in_segs = sg.sg_nseg;

	/* Prepare output iovs/segments */
	uk_sglist_init(&sg_out, nseg - in_segs, &sgsegs[in_segs]);
	r = virtiofs_prep_sglist(&sg_out, &iov[in_iovlen], out_iovlen);
	if (unlikely(r))
		return r;
	out_segs = sg_out.sg_nseg;

	sg.sg_nseg += out_segs;

	/* Enqueue request */
	r = virtqueue_buffer_enqueue(vq, &cookie, &sg, in_segs, out_segs);
	if (unlikely(r < 0))
		return r;
	/* Notify the host & block until awoken by request completion */
	uk_thread_block_until(current, 0);
	virtqueue_host_notify(vq);
	uk_sched_yield();

	/* Awoken, request complete; cookie has length of reply from device */
	return cookie.rlen;
}

/* Minimal stub for a fuse header, enough to check length & opcode */
struct virtiofs_fuse_hdr_stub {
	__u32 len;
	__u32 opcode;
};

/* Minimal valid FUSE header size, according to our stub */
#define VIRTIOFS_MIN_HDRSZ						\
	(__offsetof(struct virtiofs_fuse_hdr_stub, opcode) +		\
	 sizeof(((struct virtiofs_fuse_hdr_stub *)0)->opcode))

/* We declare priority opcodes here, as they need to be sent on the prio VQ */
#define FUSE_FORGET 2
#define FUSE_INTERRUPT 36
#define FUSE_BATCH_FORGET 42

/* Convenience return type; complete description of a VQ with its capacity */
struct vqret {
	struct virtqueue *vq;
	__u16 vqlen;
};

/* Pick the appropriate VQs for a FUSE request; muxes by opcode */
static inline
struct vqret virtiofs_getvq(struct uk_virtiofs_dev *dev, const void *hdrbuf)
{
	switch (((struct virtiofs_fuse_hdr_stub *)hdrbuf)->opcode) {
	case FUSE_FORGET:
	case FUSE_INTERRUPT:
	case FUSE_BATCH_FORGET:
		/* Priority request queue */
		return (struct vqret){
			.vq = dev->vq_prio,
			.vqlen = dev->vq_len[0]
		};
	default:
		/* Normal request queue */
		return (struct vqret){
			.vq = dev->vq_req,
			.vqlen = dev->vq_len[1]
		};
	}
}

__ssz uk_virtiofs_request_vec(struct uk_virtiofs_dev *dev,
			      const struct uk_virtiofs_iovecs *iovecs,
			      __sz num_iovec, __sz in_iovcnt, __sz out_iovcnt)
{
	struct vqret vq;

	if (unlikely(num_iovec < 1 || in_iovcnt < 1 ||
		     iovecs[0].iov[0].iov_len < VIRTIOFS_MIN_HDRSZ))
		return -EINVAL;

	vq = virtiofs_getvq(dev, iovecs[0].iov[0].iov_base);
	return virtiofs_do_request_vec(vq.vq, vq.vqlen, iovecs,
				       num_iovec, in_iovcnt, out_iovcnt);
}

__ssz uk_virtiofs_request(struct uk_virtiofs_dev *dev,
			  const struct uk_virtiofs_iovec *iov,
			  __sz in_iovcnt, __sz out_iovcnt)
{
	struct vqret vq;

	if (unlikely(in_iovcnt < 1 || iov[0].iov_len < VIRTIOFS_MIN_HDRSZ))
		return -EINVAL;

	vq = virtiofs_getvq(dev, iov[0].iov_base);
	return virtiofs_do_request(vq.vq, vq.vqlen, iov, in_iovcnt, out_iovcnt);
}

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Steven Smith (sos22@cam.ac.uk)
 *          Grzegorz Milos (gm281@cam.ac.uk)
 *          John D. Ramsdell
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2006, Cambridge University
 *               2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
/*
 * Communication with Xenstore
 * Ported from Mini-OS xenbus.c
 */

#include <string.h>
#include <uk/errptr.h>
#include <uk/bitmap.h>
#include <uk/wait.h>
#include <uk/arch/spinlock.h>
#include <common/events.h>
#include <common/hypervisor.h>
#include <xen-x86/mm.h>
#include <xen-x86/setup.h>
#include <xenbus/client.h>
#include "xs_comms.h"
#include "xs_watch.h"


/*
 * Xenstore handler structure
 */
struct xs_handler {
	/**< Communication: event channel */
	evtchn_port_t evtchn;
	/**< Communication: shared memory */
	struct xenstore_domain_interface *buf;
	/**< Thread processing incoming xs replies */
	struct uk_thread *thread;
	/**< Waiting queue for notifying incoming xs replies */
	struct uk_waitq waitq;
};

static struct xs_handler xsh = {
	.waitq = __WAIT_QUEUE_INITIALIZER(xsh.waitq),
};

/*
 * In-flight request structure.
 */
struct xs_request {
	/**< used when queueing requests */
	UK_TAILQ_ENTRY(struct xs_request) next;
	/**< Waiting queue for incoming reply notification */
	struct uk_waitq waitq;
	/**< Request header */
	struct xsd_sockmsg hdr;
	/**< Request payload iovecs */
	const struct xs_iovec *payload_iovecs;
	/**< Received reply */
	struct {
		/**< Reply string + size */
		struct xs_iovec iovec;
		/**< Error number */
		int errornum;
		/**< Non-zero for incoming replies */
		int recvd;
	} reply;
};
UK_TAILQ_HEAD(xs_request_list, struct xs_request);

/*
 * Pool of in-flight requests.
 * Request IDs are reused, hence the limited set of entries.
 */
struct xs_request_pool {
	/**< Number of live requests */
	__u32 num_live;
	/**< Last probed request index */
	__u32 last_probed;
	/**< Lock */
	spinlock_t lock;
	/**< Waiting queue for 'not-full' notifications */
	struct uk_waitq waitq;
	/**< Queue for requests to be sent */
	struct xs_request_list queued;

	/* Map size is power of 2 */
#define XS_REQ_POOL_SHIFT  5
#define XS_REQ_POOL_SIZE   (1 << XS_REQ_POOL_SHIFT)
#define XS_REQ_POOL_MASK   (XS_REQ_POOL_SIZE - 1)
	unsigned long entries_bm[UK_BITS_TO_LONGS(XS_REQ_POOL_SIZE)
			* sizeof(long)];
	/**< Entries */
	struct xs_request entries[XS_REQ_POOL_SIZE];
};

static struct xs_request_pool xs_req_pool;

static void xs_request_pool_init(struct xs_request_pool *pool)
{
	struct xs_request *xs_req;

	pool->num_live = 0;
	pool->last_probed = -1;
	ukarch_spin_lock_init(&pool->lock);
	uk_waitq_init(&pool->waitq);
	UK_TAILQ_INIT(&pool->queued);
	uk_bitmap_zero(pool->entries_bm, XS_REQ_POOL_SIZE);
	for (int i = 0; i < XS_REQ_POOL_SIZE; i++) {
		xs_req = &pool->entries[i];
		xs_req->hdr.req_id = i;
		uk_waitq_init(&xs_req->waitq);
	}
}

/*
 * Allocate an identifier for a Xenstore request.
 * Blocks if none are available.
 */
static struct xs_request *xs_request_get(void)
{
	unsigned long entry_idx;

	/* wait for an available entry */
	while (1) {
		ukarch_spin_lock(&xs_req_pool.lock);

		if (xs_req_pool.num_live < XS_REQ_POOL_SIZE)
			break;

		ukarch_spin_unlock(&xs_req_pool.lock);

		uk_waitq_wait_event(&xs_req_pool.waitq,
			(xs_req_pool.num_live < XS_REQ_POOL_SIZE));
	}

	/* find an available entry */
	entry_idx =
		uk_find_next_zero_bit(xs_req_pool.entries_bm, XS_REQ_POOL_SIZE,
			(xs_req_pool.last_probed + 1) & XS_REQ_POOL_MASK);

	if (entry_idx == XS_REQ_POOL_SIZE)
		entry_idx = uk_find_next_zero_bit(xs_req_pool.entries_bm,
			XS_REQ_POOL_SIZE, 0);

	uk_set_bit(entry_idx, xs_req_pool.entries_bm);
	xs_req_pool.last_probed = entry_idx;
	xs_req_pool.num_live++;

	ukarch_spin_unlock(&xs_req_pool.lock);

	return &xs_req_pool.entries[entry_idx];
}

/* Release a request identifier */
static void xs_request_put(struct xs_request *xs_req)
{
	__u32 reqid = xs_req->hdr.req_id;

	ukarch_spin_lock(&xs_req_pool.lock);

	UK_ASSERT(uk_test_bit(reqid, xs_req_pool.entries_bm) == 1);

	uk_clear_bit(reqid, xs_req_pool.entries_bm);
	xs_req_pool.num_live--;

	if (xs_req_pool.num_live == XS_REQ_POOL_SIZE - 1)
		uk_waitq_wake_up(&xs_req_pool.waitq);

	ukarch_spin_unlock(&xs_req_pool.lock);
}

static struct xs_request *xs_request_peek(void)
{
	struct xs_request *xs_req;

	ukarch_spin_lock(&xs_req_pool.lock);
	xs_req = UK_TAILQ_FIRST(&xs_req_pool.queued);
	ukarch_spin_unlock(&xs_req_pool.lock);

	return xs_req;
}

static void xs_request_enqueue(struct xs_request *xs_req)
{
	ukarch_spin_lock(&xs_req_pool.lock);
	UK_TAILQ_INSERT_TAIL(&xs_req_pool.queued, xs_req, next);
	ukarch_spin_unlock(&xs_req_pool.lock);
}

static struct xs_request *xs_request_dequeue(void)
{
	struct xs_request *xs_req;

	ukarch_spin_lock(&xs_req_pool.lock);
	xs_req = UK_TAILQ_FIRST(&xs_req_pool.queued);
	if (xs_req)
		UK_TAILQ_REMOVE(&xs_req_pool.queued, xs_req, next);
	ukarch_spin_unlock(&xs_req_pool.lock);

	return xs_req;
}

static int xs_avail_to_read(void)
{
	return (xsh.buf->rsp_prod != xsh.buf->rsp_cons);
}

static int xs_avail_space_for_read(unsigned int size)
{
	return (xsh.buf->rsp_prod - xsh.buf->rsp_cons >= size);
}

static int xs_avail_to_write(void)
{
	return (xsh.buf->req_prod - xsh.buf->req_cons != XENSTORE_RING_SIZE &&
		!UK_TAILQ_EMPTY(&xs_req_pool.queued));
}

static int xs_avail_space_for_write(unsigned int size)
{
	return (xsh.buf->req_prod - xsh.buf->req_cons +
		size <= XENSTORE_RING_SIZE);
}

static int xs_avail_work(void)
{
	return (xs_avail_to_read() || xs_avail_to_write());
}

/*
 * Send request to Xenstore. A request is made of multiple iovecs which are
 * preceded by a single iovec referencing the request header. The iovecs are
 * seen by Xenstore as if sent atomically. This can block.
 */
static int xs_msg_write(struct xsd_sockmsg *xsd_req,
	const struct xs_iovec *iovec)
{
	XENSTORE_RING_IDX prod;
	const struct xs_iovec *crnt_iovec;
	struct xs_iovec hdr_iovec;
	unsigned int req_size, req_off;
	unsigned int buf_off;
	unsigned int this_chunk_len;
	int rc;

	req_size = sizeof(*xsd_req) + xsd_req->len;
	if (req_size > XENSTORE_RING_SIZE)
		return -ENOSPC;

	if (!xs_avail_space_for_write(req_size))
		return -ENOSPC;

	/* We must write requests after reading the consumer index. */
	mb();

	/*
	 * We're now guaranteed to be able to send the message
	 * without overflowing the ring. Do so.
	 */

	hdr_iovec.data = xsd_req;
	hdr_iovec.len  = sizeof(*xsd_req);

	/* The batched iovecs are preceded by a single header. */
	crnt_iovec = &hdr_iovec;

	prod = xsh.buf->req_prod;
	req_off = 0;
	buf_off = 0;
	while (req_off < req_size) {
		this_chunk_len = MIN(crnt_iovec->len - buf_off,
			XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(prod));

		memcpy(
			(char *) xsh.buf->req + MASK_XENSTORE_IDX(prod),
			(char *) crnt_iovec->data + buf_off,
			this_chunk_len
		);

		prod += this_chunk_len;
		req_off += this_chunk_len;
		buf_off += this_chunk_len;

		if (buf_off == crnt_iovec->len) {
			buf_off = 0;
			if (crnt_iovec == &hdr_iovec)
				crnt_iovec = iovec;
			else
				crnt_iovec++;
		}
	}

	uk_pr_debug("Complete main loop of %s.\n", __func__);
	UK_ASSERT(buf_off == 0);
	UK_ASSERT(req_off == req_size);
	UK_ASSERT(prod <= xsh.buf->req_cons + XENSTORE_RING_SIZE);

	/* Remote must see entire message before updating indexes */
	wmb();

	xsh.buf->req_prod += req_size;

	/* Send evtchn to notify remote */
	rc = notify_remote_via_evtchn(xsh.evtchn);
	UK_ASSERT(rc == 0);

	return 0;
}

int xs_msg_reply(enum xsd_sockmsg_type msg_type, xenbus_transaction_t xbt,
	const struct xs_iovec *req_iovecs, int req_iovecs_num,
	struct xs_iovec *rep_iovec)
{
	struct xs_request *xs_req;
	int err;

	if (req_iovecs == NULL)
		return -EINVAL;

	xs_req = xs_request_get();
	xs_req->hdr.type = msg_type;
	/* req_id was set on pool init  */
	xs_req->hdr.tx_id = xbt;
	xs_req->hdr.len = 0;
	for (int i = 0; i < req_iovecs_num; i++)
		xs_req->hdr.len += req_iovecs[i].len;

	xs_req->payload_iovecs = req_iovecs;
	xs_req->reply.recvd = 0;

	/* enqueue the request */
	xs_request_enqueue(xs_req);
	/* wake xenstore thread to send it */
	uk_waitq_wake_up(&xsh.waitq);

	/* wait reply */
	uk_waitq_wait_event(&xs_req->waitq,
		xs_req->reply.recvd != 0);

	err = -xs_req->reply.errornum;
	if (err == 0) {
		if (rep_iovec)
			*rep_iovec = xs_req->reply.iovec;
		else
			free(xs_req->reply.iovec.data);
	}

	xs_request_put(xs_req);

	return err;
}

void xs_send(void)
{
	struct xs_request *xs_req;
	int err;

	xs_req = xs_request_peek();
	while (xs_req != NULL) {
		err = xs_msg_write(&xs_req->hdr, xs_req->payload_iovecs);
		if (err) {
			if (err != -ENOSPC)
				uk_pr_warn("Error sending message err=%d\n",
					   err);
			break;
		}

		/* remove it from queue */
		xs_request_dequeue();

		xs_req = xs_request_peek();
	}
}

/*
 * Converts a Xenstore reply error to a positive error number.
 * Returns 0 if the reply is successful.
 */
static int reply_to_errno(const char *reply)
{
	int err = 0;

	for (int i = 0; i < (int) ARRAY_SIZE(xsd_errors); i++) {
		if (!strcmp(reply, xsd_errors[i].errstring)) {
			err = xsd_errors[i].errnum;
			goto out;
		}
	}

	uk_pr_warn("Unknown Xenstore error: %s\n", reply);
	err = EINVAL;

out:
	return err;
}

/* Process an incoming xs reply */
static void process_reply(struct xsd_sockmsg *hdr, char *payload)
{
	struct xs_request *xs_req;

	if (!uk_test_bit(hdr->req_id, xs_req_pool.entries_bm)) {
		uk_pr_warn("Invalid reply id=%d\n", hdr->req_id);
		free(payload);
		return;
	}

	xs_req = &xs_req_pool.entries[hdr->req_id];

	if (hdr->type == XS_ERROR) {
		xs_req->reply.errornum = reply_to_errno(payload);
		free(payload);

	} else if (hdr->type != xs_req->hdr.type) {
		uk_pr_warn("Mismatching message type: %d\n", hdr->type);
		free(payload);
		return;

	} else {
		/* set reply */
		xs_req->reply.iovec.data = payload;
		xs_req->reply.iovec.len = hdr->len;
		xs_req->reply.errornum = 0;
	}

	xs_req->reply.recvd = 1;

	/* notify waiting requester */
	uk_waitq_wake_up(&xs_req->waitq);
}

/* Process an incoming xs watch event */
static void process_watch_event(char *watch_msg)
{
	struct xs_watch *watch;
	char *path, *token;

	path  = watch_msg;
	token = watch_msg + strlen(path) + 1;

	watch = xs_watch_find(path, token);
	free(watch_msg);

	if (watch)
		xenbus_watch_notify_event(&watch->base);
	else
		uk_pr_err("Invalid watch event.");
}

static void memcpy_from_ring(const char *ring, char *dest, int off, int len)
{
	int c1, c2;

	c1 = MIN(len, XENSTORE_RING_SIZE - off);
	c2 = len - c1;

	memcpy(dest, ring + off, c1);
	if (c2)
		memcpy(dest + c1, ring, c2);
}

static void xs_msg_read(struct xsd_sockmsg *hdr)
{
	XENSTORE_RING_IDX cons;
	char *payload;

	payload = malloc(hdr->len + 1);
	if (payload == NULL) {
		uk_pr_warn("No memory available for saving Xenstore message!\n");
		return;
	}

	cons = xsh.buf->rsp_cons;

	/* copy payload */
	memcpy_from_ring(
		xsh.buf->rsp,
		payload,
		MASK_XENSTORE_IDX(cons + sizeof(*hdr)),
		hdr->len
	);
	payload[hdr->len] = '\0';

	/* Remote must not see available space until we've copied the reply */
	mb();
	xsh.buf->rsp_cons += sizeof(*hdr) + hdr->len;

	if (xsh.buf->rsp_prod - cons >= XENSTORE_RING_SIZE)
		notify_remote_via_evtchn(xsh.evtchn);

	if (hdr->type == XS_WATCH_EVENT)
		process_watch_event(payload);
	else
		process_reply(hdr, payload);
}

static void xs_recv(void)
{
	struct xsd_sockmsg msg;

	while (1) {
		uk_pr_debug("Rsp_cons %d, rsp_prod %d.\n",
			    xsh.buf->rsp_cons, xsh.buf->rsp_prod);

		if (!xs_avail_space_for_read(sizeof(msg)))
			break;

		/* Make sure data is read after reading the indexes */
		rmb();

		/* copy the message header */
		memcpy_from_ring(
			xsh.buf->rsp,
			(char *) &msg,
			MASK_XENSTORE_IDX(xsh.buf->rsp_cons),
			sizeof(msg)
		);

		uk_pr_debug("Msg len %lu, %u avail, id %u.\n",
			    msg.len + sizeof(msg),
			    xsh.buf->rsp_prod - xsh.buf->rsp_cons,
			    msg.req_id);

		if (!xs_avail_space_for_read(sizeof(msg) + msg.len))
			break;

		/* Make sure data is read after reading the indexes */
		rmb();

		uk_pr_debug("Message is good.\n");
		xs_msg_read(&msg);
	}
}

static void xs_thread_func(void *ign __unused)
{
	for (;;) {
		uk_waitq_wait_event(&xsh.waitq, xs_avail_work());

		if (xs_avail_to_write())
			xs_send();

		if (xs_avail_to_read())
			xs_recv();
	}
}

static void xs_evtchn_handler(evtchn_port_t port,
		struct __regs *regs __unused, void *ign __unused)
{
	UK_ASSERT(xsh.evtchn == port);
	uk_waitq_wake_up(&xsh.waitq);
}

int xs_comms_init(void)
{
	struct uk_thread *thread;
	evtchn_port_t port;

	xs_request_pool_init(&xs_req_pool);

	uk_waitq_init(&xsh.waitq);

	thread = uk_thread_create("xenstore", xs_thread_func, NULL);
	if (PTRISERR(thread))
		return PTR2ERR(thread);

	xsh.thread = thread;

	xsh.evtchn = HYPERVISOR_start_info->store_evtchn;
	xsh.buf = mfn_to_virt(HYPERVISOR_start_info->store_mfn);

	port = bind_evtchn(xsh.evtchn, xs_evtchn_handler, NULL);
	UK_ASSERT(port == xsh.evtchn);
	unmask_evtchn(xsh.evtchn);

	uk_pr_info("Xenstore connection initialised on port %d, buf %p (mfn %#lx)\n",
		   port, xsh.buf, HYPERVISOR_start_info->store_mfn);

	return 0;
}

void xs_comms_fini(void)
{
	mask_evtchn(xsh.evtchn);
	unbind_evtchn(xsh.evtchn);

	xsh.buf = NULL;

	/* TODO stop thread, instead of killing it */
	uk_thread_kill(xsh.thread);
	xsh.thread = NULL;
}

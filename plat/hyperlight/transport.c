/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight virtqueue transport: the driver side of the two packed
 * rings (see transport.h).
 *
 * Layout reference: hyperlight-common layout.rs (TransportArena) and
 * virtq/ (packed ring, canonical images), Hyperlight 0.18.
 *
 * Pools.  The host leaves the G2H pool to the guest and validates only
 * H2G's.  G2H is split in two halves: a host call's request goes in the
 * first, its reply in the second, each as one descriptor; a guest
 * function result reuses the request half.  H2G is cut into receive
 * buffers of the size the host published, buffer i posted with
 * descriptor ID i, which is the canonical form a snapshot needs anyway.
 */

#include <string.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/print.h>

#include <hyperlight/mem.h>
#include <hyperlight/outb.h>
#include <hyperlight/transport.h>

/* Descriptor flags (VIRTIO 1.1 packed ring). */
#define VQ_DESC_F_NEXT		(1 << 0)
#define VQ_DESC_F_WRITE		(1 << 1)
#define VQ_DESC_F_AVAIL		(1 << 7)
#define VQ_DESC_F_USED		(1 << 15)

#define VQ_DESC_ALIGN		16
#define VQ_EVENT_SIZE		4

/* Descriptor IDs are tracked in fixed arrays: queues larger than this are
 * refused at init.  Hyperlight's defaults are 64 (G2H) and 32 (H2G).
 */
#define VQ_MAX_SIZE		256

#define HL_PAGE			4096ULL

struct vq_desc {
	__u64 addr;
	__u32 len;
	__u16 id;
	__u16 flags;
};

UK_CTASSERT(sizeof(struct vq_desc) == 16);

/* One packed ring, from the driver's side. */
struct vq {
	struct vq_desc *desc;
	__u32 *drv_evt;
	__u16 size;
	__u16 avail_head;
	__u16 used_head;
	__u16 num_free;
	__u8 avail_wrap;
	__u8 used_wrap;
	/* Descriptors in the chain with this ID, 0 if the ID is free. */
	__u16 id_num[VQ_MAX_SIZE];
};

struct msg_header {
	__u8 kind;
	__u8 reserved[3];
	__u32 cid;
	__u32 payload_len;
};

UK_CTASSERT(sizeof(struct msg_header) == HL_MSG_HEADER_SIZE);

static struct vq g2h, h2g;

static __u8 *g2h_req;		/* request half of the G2H pool */
static __u8 *g2h_reply;		/* reply half */
static __u64 g2h_half;		/* bytes in each half */

static __u8 *h2g_pool;
static __u64 h2g_buf_size;
static __u16 h2g_bufs;		/* receive buffers posted at a time */

static volatile __u64 *mbx;	/* snapshot checkpoint mailbox */

/* Checkpoint mailbox values (hyperlight-common transport.rs MailboxValue). */
#define HL_MBX_CHECKPOINT_PENDING	0
#define HL_MBX_CHECKPOINT_COMPLETE	1

static __u32 next_cid = 1;
static int transport_ready;	/* init succeeded */
static int transport_broken;	/* rings unusable: transport_lost() */
static int result_id = -1;	/* G2H ID of a queued result, -1 if none */

/* ── Packed ring, driver side ──────────────────────────────────── */

static __u64 vq_ring_len(__u16 size)
{
	/* Descriptors, then the driver and device event suppressions. */
	return (__u64)size * sizeof(struct vq_desc) + 2 * VQ_EVENT_SIZE;
}

static void vq_attach(struct vq *q, __u8 *base, __u16 size)
{
	q->desc = (struct vq_desc *)base;
	q->drv_evt = (__u32 *)(base + (__u64)size * sizeof(struct vq_desc));
	q->size = size;
	q->avail_head = 0;
	q->used_head = 0;
	q->avail_wrap = 1;
	q->used_wrap = 1;
	q->num_free = size;
	memset(q->id_num, 0, sizeof(q->id_num));
}

/*
 * Clear every descriptor and the driver event: the canonical empty ring.
 * The device event is the host's to reset.
 */
static void vq_reset(struct vq *q)
{
	memset(q->desc, 0, (__u64)q->size * sizeof(struct vq_desc));
	__atomic_store_n(q->drv_evt, 0, __ATOMIC_RELEASE);
	vq_attach(q, (__u8 *)q->desc, q->size);
}

static void vq_advance(__u16 *head, __u8 *wrap, __u16 size, __u16 n)
{
	__u32 pos = (__u32)*head + n;

	while (pos >= size) {
		pos -= size;
		*wrap ^= 1;
	}
	*head = pos;
}

static __u16 vq_avail_flags(__u16 flags, __u8 wrap)
{
	return flags | (wrap ? VQ_DESC_F_AVAIL : VQ_DESC_F_USED);
}

/*
 * Publish a chain of @n descriptors under @id.  The tail goes in first
 * and the head's flags last, with release order, so the host never sees
 * half a chain.
 */
static int vq_submit(struct vq *q, __u16 id, const __u64 *addr,
		     const __u32 *len, const __u16 *flags, __u16 n)
{
	struct vq_desc *head = &q->desc[q->avail_head];
	__u16 pos = q->avail_head, head_flags = 0, i;
	__u8 wrap = q->avail_wrap;

	if (n == 0 || q->num_free < n || id >= q->size || q->id_num[id])
		return -1;

	for (i = 0; i < n; i++) {
		struct vq_desc *d = &q->desc[pos];
		__u16 f = vq_avail_flags(flags[i] |
					 (i + 1 < n ? VQ_DESC_F_NEXT : 0), wrap);

		d->addr = addr[i];
		d->len = len[i];
		d->id = id;
		if (i)
			d->flags = f;
		else
			head_flags = f;
		vq_advance(&pos, &wrap, q->size, 1);
	}
	__atomic_store_n(&head->flags, head_flags, __ATOMIC_RELEASE);

	q->id_num[id] = n;
	q->num_free -= n;
	q->avail_head = pos;
	q->avail_wrap = wrap;
	return 0;
}

/*
 * Take the next completed chain.  Returns 1 with its ID and the bytes
 * the host wrote, 0 if there is none yet, -1 if the ring is corrupt.
 */
static int vq_poll_used(struct vq *q, __u16 *id, __u32 *len)
{
	struct vq_desc *d = &q->desc[q->used_head];
	__u16 flags = __atomic_load_n(&d->flags, __ATOMIC_ACQUIRE);
	__u8 avail = !!(flags & VQ_DESC_F_AVAIL);
	__u8 used = !!(flags & VQ_DESC_F_USED);
	__u16 n;

	if (avail != q->used_wrap || used != q->used_wrap)
		return 0;

	*id = d->id;
	*len = d->len;
	if (*id >= q->size || !(n = q->id_num[*id]))
		return -1;

	vq_advance(&q->used_head, &q->used_wrap, q->size, n);
	q->num_free += n;
	q->id_num[*id] = 0;
	return 1;
}

/* Lowest free descriptor ID. */
static int vq_free_id(struct vq *q)
{
	__u16 id;

	for (id = 0; id < q->size; id++)
		if (!q->id_num[id])
			return id;
	return -1;
}

static void vq_notify(void)
{
	/* Descriptors are published before the exit, and the host's writes
	 * are visible after it.
	 */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	hyperlight_out32(HYPERLIGHT_PORT_VIRTQ_NOTIFY, 0);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
}

/* ── Pools ─────────────────────────────────────────────────────── */

/* Retire every G2H chain the host has completed. */
static int g2h_reclaim(void)
{
	__u16 id;
	__u32 len;
	int rc;

	while ((rc = vq_poll_used(&g2h, &id, &len)) > 0)
		if (id == result_id)
			result_id = -1;
	return rc;
}

static __u8 *h2g_buf(__u16 i)
{
	return h2g_pool + (__u64)i * h2g_buf_size;
}

static int h2g_post(__u16 i)
{
	__u64 addr = (__u64)h2g_buf(i);
	__u32 len = h2g_buf_size;
	__u16 flags = VQ_DESC_F_WRITE;

	return vq_submit(&h2g, i, &addr, &len, &flags, 1);
}

/* Post every receive buffer, from an empty ring. */
static int h2g_prefill(void)
{
	__u16 i;

	for (i = 0; i < h2g_bufs; i++)
		if (h2g_post(i) < 0)
			return -1;
	return 0;
}

/* ── Init ──────────────────────────────────────────────────────── */

static __u64 meta(__u64 off)
{
	return *(volatile __u64 *)HL_SCRATCH_META_GVA(off);
}

static int queue_size_ok(__u64 size)
{
	return size && size <= VQ_MAX_SIZE && !(size & (size - 1));
}

int hl_transport_init(void)
{
	__u64 scratch_size = meta(HL_SCRATCH_SIZE_OFF);
	__u64 arena_gpa = meta(HL_SCRATCH_ARENA_GPA_OFF);
	__u64 g2h_size = meta(HL_SCRATCH_G2H_QUEUE_SIZE_OFF);
	__u64 g2h_pages = meta(HL_SCRATCH_G2H_POOL_PAGES_OFF);
	__u64 h2g_size = meta(HL_SCRATCH_H2G_QUEUE_SIZE_OFF);
	__u64 h2g_pages = meta(HL_SCRATCH_H2G_POOL_PAGES_OFF);
	__u64 base_gpa, h2g_ring, mbx_off, g2h_pool, end;
	__u8 *arena;

	h2g_buf_size = meta(HL_SCRATCH_H2G_BUFFER_SIZE_OFF);

	/* Bounded by scratch before any arithmetic, so nothing below wraps.
	 * A host call is a chain of two descriptors.  A G2H half is one
	 * descriptor, whose length is 32 bits, and holds strings whose
	 * length hcall.c returns as an int: it is kept under 2 GiB.
	 */
	if (!queue_size_ok(g2h_size) || g2h_size < 2 ||
	    !queue_size_ok(h2g_size) ||
	    !g2h_pages || !h2g_pages ||
	    g2h_pages > scratch_size / HL_PAGE ||
	    h2g_pages > scratch_size / HL_PAGE ||
	    g2h_pages * HL_PAGE / 2 > 0x7fffffffULL ||
	    h2g_buf_size <= HL_MSG_HEADER_SIZE ||
	    h2g_buf_size > 0xffffffffULL) {
		uk_pr_err("transport: unusable geometry (queues %lu/%lu, pools %lu/%lu pages, buffer %lu)\n",
			  (unsigned long)g2h_size, (unsigned long)h2g_size,
			  (unsigned long)g2h_pages, (unsigned long)h2g_pages,
			  (unsigned long)h2g_buf_size);
		return -1;
	}

	/* The arena is a GPA in scratch; the guest reaches it through the
	 * scratch alias at the top of the address space.
	 */
	base_gpa = HL_MAX_GPA - scratch_size + 1;
	if (arena_gpa < base_gpa || arena_gpa & (HL_PAGE - 1)) {
		uk_pr_err("transport: arena at 0x%lx is outside scratch\n",
			  (unsigned long)arena_gpa);
		return -1;
	}
	arena = (__u8 *)(arena_gpa + HL_SCRATCH_GVA_MINUS_GPA);

	/* TransportArena::new */
	h2g_ring = ALIGN_UP(vq_ring_len(g2h_size), VQ_DESC_ALIGN);
	mbx_off = ALIGN_UP(h2g_ring + vq_ring_len(h2g_size), sizeof(__u64));
	g2h_pool = ALIGN_UP(mbx_off + sizeof(__u64), HL_PAGE);
	end = g2h_pool + (g2h_pages + h2g_pages) * HL_PAGE;
	if (arena_gpa - base_gpa > scratch_size ||
	    end > scratch_size - (arena_gpa - base_gpa)) {
		uk_pr_err("transport: arena overruns scratch\n");
		return -1;
	}

	vq_attach(&g2h, arena, g2h_size);
	vq_attach(&h2g, arena + h2g_ring, h2g_size);
	mbx = (volatile __u64 *)(arena + mbx_off);

	g2h_half = g2h_pages * HL_PAGE / 2;
	g2h_req = arena + g2h_pool;
	g2h_reply = g2h_req + g2h_half;

	h2g_pool = arena + g2h_pool + g2h_pages * HL_PAGE;
	h2g_bufs = MIN(h2g_size, h2g_pages * HL_PAGE / h2g_buf_size);
	if (!h2g_bufs || h2g_prefill() < 0) {
		uk_pr_err("transport: cannot post H2G buffers\n");
		return -1;
	}

	transport_ready = 1;
	return 0;
}

int hl_transport_ready(void)
{
	return transport_ready && !transport_broken;
}

/* ── G2H: host calls and results ───────────────────────────────── */

/*
 * Ring state the guest cannot account for -- a G2H chain the host did
 * not complete as it should have, no free G2H descriptor, a corrupt H2G
 * completion (h2g_take()), an H2G buffer that cannot be posted back
 * (h2g_repost()) -- leaves buffers out of reach: a guest that can make
 * no host call or take no guest function call is finished.  The crash
 * report goes out through DebugPrint: with the transport marked lost,
 * the console no longer tries HostWrite, which the host refuses during
 * a snapshot checkpoint, an entry any H2G path may be in.
 */
static void __noreturn transport_lost(const char *what)
{
	transport_broken = 1;
	UK_CRASH("transport: %s\n", what);
}

/*
 * Whether a guest function result is queued on G2H.  The host takes G2H
 * in order once the guest halts, and expects the result first, so
 * nothing may follow it in this entry.  A result the host has already
 * taken (in an earlier entry) is retired here.
 */
static int result_queued(void)
{
	if (result_id >= 0 && g2h_reclaim() < 0)
		transport_lost("corrupt G2H completion");
	return result_id >= 0;
}

/*
 * Ready G2H for a new chain: retire the completed ones and pick a free
 * descriptor ID, -1 while a result is queued.
 */
static int g2h_begin(void)
{
	int id;

	if (g2h_reclaim() < 0)
		transport_lost("corrupt G2H completion");
	if (result_id >= 0)
		return -1;
	id = vq_free_id(&g2h);
	if (id < 0)
		transport_lost("no free G2H descriptor");
	return id;
}

static void put_header(__u8 *at, __u8 kind, __u32 cid, __u32 payload_len)
{
	struct msg_header h = {
		.kind = kind,
		.cid = cid,
		.payload_len = payload_len,
	};

	memcpy(at, &h, sizeof(h));
}

__u8 *hl_transport_request_buf(void)
{
	if (!hl_transport_ready() || result_queued())
		return NULL;
	return g2h_req + HL_MSG_HEADER_SIZE;
}

__u64 hl_transport_payload_cap(void)
{
	return transport_ready ? g2h_half - HL_MSG_HEADER_SIZE : 0;
}

int hl_transport_call(__u64 payload_len,
		      const __u8 **reply, __u64 *reply_len)
{
	struct msg_header h;
	__u64 addr[2] = { (__u64)g2h_req, (__u64)g2h_reply };
	__u32 len[2] = { HL_MSG_HEADER_SIZE + payload_len, g2h_half };
	__u16 flags[2] = { 0, VQ_DESC_F_WRITE };
	__u32 cid, written;
	__u16 used_id;
	int id;

	if (unlikely(!hl_transport_ready() ||
		     payload_len > g2h_half - HL_MSG_HEADER_SIZE))
		return HL_TRANSPORT_NOT_SENT;
	id = g2h_begin();
	if (id < 0)
		return HL_TRANSPORT_NOT_SENT;

	/* Correlation IDs are nonzero. */
	cid = next_cid++;
	if (!next_cid)
		next_cid = 1;

	put_header(g2h_req, HL_MSG_REQUEST, cid, payload_len);
	if (vq_submit(&g2h, id, addr, len, flags, 2) < 0)
		return HL_TRANSPORT_NOT_SENT;

	/* The host serves the call during this exit. */
	vq_notify();

	if (vq_poll_used(&g2h, &used_id, &written) <= 0 || used_id != id)
		transport_lost("host call not completed");

	/* What the host says it wrote is checked against the buffer it
	 * had: every read of the reply is bounded by what follows.
	 */
	memcpy(&h, g2h_reply, sizeof(h));
	if (written < HL_MSG_HEADER_SIZE || written > g2h_half ||
	    h.kind != HL_MSG_RESPONSE ||
	    h.cid != cid || h.payload_len > written - HL_MSG_HEADER_SIZE)
		return HL_TRANSPORT_BAD_REPLY;

	*reply = g2h_reply + HL_MSG_HEADER_SIZE;
	*reply_len = h.payload_len;
	return 0;
}

int hl_transport_log(__u64 payload_len)
{
	__u64 addr = (__u64)g2h_req;
	__u32 len = HL_MSG_HEADER_SIZE + payload_len;
	__u16 flags = 0;
	__u16 used_id;
	__u32 written;
	int id;

	if (unlikely(!hl_transport_ready() ||
		     payload_len > g2h_half - HL_MSG_HEADER_SIZE))
		return HL_TRANSPORT_NOT_SENT;
	id = g2h_begin();
	if (id < 0)
		return HL_TRANSPORT_NOT_SENT;

	/* A log carries no correlation ID: nothing answers it. */
	put_header(g2h_req, HL_MSG_LOG, 0, payload_len);
	if (vq_submit(&g2h, id, &addr, &len, &flags, 1) < 0)
		return HL_TRANSPORT_NOT_SENT;

	/* The host logs it during this exit; the request half is the next
	 * message's.
	 */
	vq_notify();
	if (vq_poll_used(&g2h, &used_id, &written) <= 0 || used_id != id)
		transport_lost("log not acknowledged");
	return 0;
}

int hl_transport_respond(__u32 cid, const __u8 *payload, __u64 len)
{
	__u64 addr = (__u64)g2h_req;
	__u32 dlen = HL_MSG_HEADER_SIZE + len;
	__u16 flags = 0;
	int id;

	if (unlikely(!hl_transport_ready() ||
		     len > g2h_half - HL_MSG_HEADER_SIZE))
		return -1;
	id = g2h_begin();
	if (id < 0)
		return -1;

	put_header(g2h_req, HL_MSG_RESPONSE, cid, len);
	memcpy(g2h_req + HL_MSG_HEADER_SIZE, payload, len);
	if (vq_submit(&g2h, id, &addr, &dlen, &flags, 1) < 0)
		return -1;
	result_id = id;
	return 0;
}

/* ── H2G: guest function calls ─────────────────────────────────── */

__u64 hl_transport_max_recv(void)
{
	/* Every buffer: the host keeps one spare only under a message that
	 * carries VecBytes values (write_h2g_message), and the guest serves
	 * no guest function that takes one.
	 */
	return transport_ready ?
		(__u64)h2g_bufs * h2g_buf_size - HL_MSG_HEADER_SIZE : 0;
}

/*
 * The next completed receive buffer, or NULL if there is none yet.  A
 * completion H2G cannot account for leaves its buffers out of reach: the
 * guest could take no more guest function calls, so it stops here.  The
 * transport is marked lost first, so the report goes out through
 * DebugPrint: the entry may be a snapshot checkpoint, during which the
 * host refuses a host call.
 */
static __u8 *h2g_take(__u16 *slot, __u32 *len)
{
	int rc = vq_poll_used(&h2g, slot, len);

	if (rc == 0)
		return NULL;
	if (rc < 0 || *slot >= h2g_bufs || *len > h2g_buf_size)
		transport_lost("corrupt H2G completion");
	return h2g_buf(*slot);
}

/*
 * Hand a buffer just taken back to the host.  Its slot was free a moment
 * ago, so this cannot fail on a sound ring; if it does, the buffer is out
 * of reach, as with a corrupt completion.
 */
static void h2g_repost(__u16 slot)
{
	if (h2g_post(slot) < 0)
		transport_lost("cannot repost an H2G buffer");
}

/*
 * Post back every buffer the host completed and the guest has not taken.
 * The host writes one H2G message per entry, so after one is read, or
 * refused, whatever is left over is that message's own: taking it all
 * leaves the next entry to start at a header.
 */
static void h2g_drain(void)
{
	__u16 slot;
	__u32 len;

	while (h2g_take(&slot, &len))
		h2g_repost(slot);
}

__s64 hl_transport_recv(__u8 *buf, __u64 cap, __u8 *kind, __u32 *cid)
{
	struct msg_header h;
	__u64 got = 0, off = HL_MSG_HEADER_SIZE;
	__u16 slot;
	__u32 len;
	__u8 *b;
	int refuse;

	*kind = 0;
	*cid = 0;
	if (unlikely(!transport_ready))
		return -1;

	/* A result left on G2H by the previous entry is done by now. */
	if (g2h_reclaim() < 0)
		transport_lost("corrupt G2H completion");

	b = h2g_take(&slot, &len);
	if (!b)
		return -1;
	if (len < HL_MSG_HEADER_SIZE) {
		/* No length to go by: the message is dropped whole. */
		h2g_repost(slot);
		refuse = 1;
		goto out;
	}
	memcpy(&h, b, sizeof(h));
	*kind = h.kind;
	*cid = h.cid;

	/* A message that does not fit, or is malformed, is refused; it is
	 * taken off the ring whole all the same (h2g_drain()).
	 */
	refuse = h.payload_len > cap ||
		 len - HL_MSG_HEADER_SIZE > h.payload_len;

	/* Copy out, then hand each buffer straight back to the host. */
	for (;;) {
		__u64 n = MIN((__u64)len - off, h.payload_len - got);

		if (!refuse && n)
			memcpy(buf + got, b + off, n);
		got += n;
		off = 0;
		h2g_repost(slot);
		if (got == h.payload_len)
			break;
		/* Every buffer but the one that ends the payload is full, and
		 * the payload does not stop short of its length.
		 */
		if (len != h2g_buf_size) {
			refuse = 1;
			break;
		}
		b = h2g_take(&slot, &len);
		if (!b) {
			refuse = 1;
			break;
		}
		/* Nor does any buffer hold more than the payload still owed. */
		if (len > h.payload_len - got)
			refuse = 1;
	}
out:
	h2g_drain();
	return refuse ? -1 : (__s64)got;
}

/* ── Snapshot checkpoint ───────────────────────────────────────── */

int hl_transport_checkpoint(void)
{
	if (!transport_ready)
		return -1;
	*mbx = HL_MBX_CHECKPOINT_PENDING;
	/* Past init, the failure that can happen is a corrupt G2H
	 * completion, found before either ring is touched; prefill cannot
	 * fail on rings just reset, and is checked all the same.  Either way
	 * the transport is marked lost, so nothing (the console included)
	 * posts to G2H again.
	 */
	if (g2h_reclaim() < 0)
		goto lost;
	vq_reset(&g2h);
	vq_reset(&h2g);
	result_id = -1;
	if (h2g_prefill() < 0)
		goto lost;
	/* Published last: the host takes the rings as they are once it
	 * reads complete.
	 */
	*mbx = HL_MBX_CHECKPOINT_COMPLETE;
	return 0;
lost:
	transport_broken = 1;
	return -1;
}

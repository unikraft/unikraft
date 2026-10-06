/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight virtqueue transport.
 *
 * Since Hyperlight 0.18 every message between guest and host travels
 * over two VIRTIO packed rings in scratch memory, both driven by the
 * guest:
 *
 *   G2H  guest -> host: host function calls (a readable request and a
 *        writable reply in one chain) and guest function results.
 *   H2G  host -> guest: guest function calls and snapshot checkpoints,
 *        written by the host into buffers the guest posted in advance.
 *
 * The host publishes the queue geometry at the top of scratch; the ring
 * and pool addresses follow from it (hyperlight-common layout.rs,
 * TransportArena).  Every message starts with a 12-byte header (kind,
 * correlation ID, payload length), followed by a size-prefixed
 * FlatBuffer and the bytes of any VecBytes value, in parameter order.
 *
 * Unlike a Rust guest, this one keeps no transport buffer past the call
 * that used it: an H2G message is copied out and its buffers reposted at
 * once, and a host call's reply is read before the next one starts.  So
 * the guest never retains a pool slot, and a snapshot checkpoint has only
 * the rings to reset.
 *
 * Not reentrant: callers serialise.  hcall.c holds its lock around
 * every host call and every result it queues; dispatch's receive and
 * checkpoint run with the guest otherwise stopped, and the boot
 * checkpoint with interrupts off (hyperlight_halt_boot_done()).
 */

#ifndef __HYPERLIGHT_TRANSPORT_H__
#define __HYPERLIGHT_TRANSPORT_H__

#include <uk/arch/types.h>

/* Message kinds (hyperlight-common transport.rs MsgKind). */
#define HL_MSG_REQUEST			0x01
#define HL_MSG_RESPONSE			0x02
#define HL_MSG_SNAPSHOT_CHECKPOINT	0x07

#define HL_MSG_HEADER_SIZE		12

/**
 * Locate the rings and pools from the geometry the host published at the
 * top of scratch, and post the H2G receive buffers.  Called once at boot,
 * before the first host call.
 *
 * @return 0 on success, -1 if the published geometry is unusable.
 */
int hl_transport_init(void);

/**
 * Whether host calls can be made: hl_transport_init() succeeded, and the
 * rings have not been marked lost, as they are when the guest finds ring
 * state it cannot account for, or a checkpoint fails (transport.c).
 */
int hl_transport_ready(void);

/**
 * Where a host call's payload goes: the FunctionCall FlatBuffer, then
 * any external bytes.  The buffer is shared with the host and reused by
 * every call; it holds hl_transport_payload_cap() bytes.  NULL when no
 * host call can be made: unless hl_transport_ready(), or while a guest
 * function result is queued (hl_transport_respond()), whose bytes it
 * holds.  Call with the hcall lock held, up to hl_transport_call().
 */
__u8 *hl_transport_request_buf(void);

/**
 * Largest payload a host call request, or its reply, can carry: set by
 * the geometry the host published, 0 before init.
 */
__u64 hl_transport_payload_cap(void);

/* hl_transport_call() failures. */
#define HL_TRANSPORT_NOT_SENT	(-1)	/* the host never saw the request */
#define HL_TRANSPORT_BAD_REPLY	(-2)	/* it did, but the reply is unusable */

/**
 * Send the request in hl_transport_request_buf() and wait for the reply.
 * Refused, without a VM exit, unless hl_transport_ready(), or while a
 * guest function result is queued (hl_transport_respond()), until the
 * guest halts.  A request the host does not complete crashes the guest:
 * the ring is unusable after.
 *
 * @param payload_len  Bytes of the request payload.
 * @param reply        Receives the reply payload (a FunctionCallResult
 *                     FlatBuffer, then any external bytes); valid until
 *                     the next transport call.
 * @param reply_len    Receives its length.
 * @return 0 on success, HL_TRANSPORT_NOT_SENT or HL_TRANSPORT_BAD_REPLY.
 */
int hl_transport_call(__u64 payload_len,
		      const __u8 **reply, __u64 *reply_len);

/**
 * Largest H2G message payload the posted buffers can hold.  A message
 * with VecBytes values gets one buffer fewer (the host keeps it spare);
 * no guest function the kernel serves takes one.
 */
__u64 hl_transport_max_recv(void);

/**
 * Take the host's message for this VM entry off H2G.  The payload is
 * copied into @a buf and the buffers it came in are posted again.  A
 * message that does not fit @a cap, or is malformed, is taken off all
 * the same, with every other buffer the host completed for this entry
 * (it writes one message per entry), so the next one starts clean.
 *
 * @param kind  Receives the message kind (HL_MSG_*), 0 if unknown.
 * @param cid   Receives the correlation ID to answer a request with, 0
 *              if unknown; set even when the payload is refused.
 * @return The payload length, or -1 if it was refused or lost.
 */
__s64 hl_transport_recv(__u8 *buf, __u64 cap, __u8 *kind, __u32 *cid);

/**
 * Queue the result of guest function call @a cid on G2H.  The host reads
 * it once the guest halts, so no notification is sent, and no host call
 * can be made until then.  One result per entry.
 *
 * @return 0 on success, -1 on a transport error.
 */
int hl_transport_respond(__u32 cid, const __u8 *payload, __u64 len);

/**
 * Put both rings into the canonical state a snapshot captures: G2H empty,
 * H2G holding one receive buffer per pool slot, both from cursor zero.
 * Then report the checkpoint complete through the mailbox.
 *
 * The host requires this for every SnapshotCheckpoint message and, as
 * well, at the halt that ends boot (hyperlight_halt_boot_done()).
 *
 * @return 0 on success; -1 before hl_transport_init(), or on a corrupt
 *         G2H completion, after which the mailbox still reads pending,
 *         the host refuses the snapshot, and the transport is out of
 *         use (hl_transport_ready() is 0).
 */
int hl_transport_checkpoint(void);

#endif /* __HYPERLIGHT_TRANSPORT_H__ */

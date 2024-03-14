/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_VSOCKDEV_VSOCK_H__
#define __UK_VSOCKDEV_VSOCK_H__

#include <uk/essentials.h>
#include <uk/compat_list.h>
#include <uk/netbuf.h>
#include <uk/tree.h>
#include <uk/socket.h>
#include <uk/spinlock.h>

/**
 * Default and maximum per-socket buffer size, derived from the library
 * configuration. UK_VSOCK_BUF_SIZE is the size new sockets start with (and
 * advertise to the peer as their credit window), while UK_VSOCK_MAX_BUF_SIZE
 * is the hard upper bound the buffer size is clamped to.
 */
#define UK_VSOCK_BUF_SIZE	CONFIG_LIBUKVSOCKDEV_VSOCK_BUF_SIZE
#define UK_VSOCK_MAX_BUF_SIZE	CONFIG_LIBUKVSOCKDEV_VSOCK_BUF_MAX_SIZE

UK_CTASSERT(UK_VSOCK_MAX_BUF_SIZE >= UK_VSOCK_BUF_SIZE);

struct uk_vsockaddr {
	unsigned int cid;
	unsigned int port;
	/* TODO: flags field? (see linux definitions) */
};

struct uk_vsock_buffer {
	char *buf;
	__sz head;
	__sz count;
	__sz capacity;
	struct uk_alloc *a;
	__sz total_processed;
	struct uk_spinlock lock;
};

enum uk_vsock_state {
	UK_VSOCK_STATE_READY,
	UK_VSOCK_STATE_CLOSED,
	UK_VSOCK_STATE_BOUND,
	UK_VSOCK_STATE_WAIT_CONNECT,
	UK_VSOCK_STATE_LISTENING,
	UK_VSOCK_STATE_OPEN,
};

struct uk_vsock_accept_entry {
	UK_STAILQ_ENTRY(struct uk_vsock_accept_entry) entry;
	struct uk_vsockaddr peer;
};

UK_STAILQ_HEAD(uk_vsock_accept_queue, struct uk_vsock_accept_entry);

#define UK_VSOCK_BACKLOG_MAX				128

/**
 * Generic representation of a VSOCK socket.
 * Drivers are encouraged to embed this structure in a driver specific socket
 * data structure.
 *
 * Unless otherwise indicated the fields should be considered read-only for the
 * driver and only modified with the appropriate ukvsockdev driver interface
 * functions.
 */
struct uk_vsock {
	/** Current state of the socket */
	enum uk_vsock_state state;
	/** Whether we will be able to receive data */
	__bool rx_shutdown;
	/** Whether the peer will be able to receive data */
	__bool tx_shutdown;
	/** errno of previously failed async operations */
	int so_error;

	/** SOCK_* type */
	int sock_type;
	/**
	 * Current per-socket buffer size. This is the value we advertise to
	 * the peer as our RX credit window (buf_alloc) and the local bound on
	 * how much TX data we are willing to have in flight. New sockets are
	 * initialized to UK_VSOCK_BUF_SIZE; accepted connections inherit the
	 * value from their listening socket. It is always clamped to
	 * max_buf_size.
	 */
	__sz buf_size;
	/** Maximum value buf_size may take (clamp limit). */
	__sz max_buf_size;
	/** The local socket address */
	struct uk_vsockaddr local_addr;
	/** The socket address of the peer */
	struct uk_vsockaddr peer_addr;
	/** Entry for the sockets tracking data structure */
	UK_RB_ENTRY(uk_vsock) rb_entry;
	/** The receive socket buffer */
	struct uk_vsock_buffer rx;
	/** List of incoming connections that can be accepted */
	struct uk_vsock_accept_queue accept_queue;
	/** The current length of the accept_queue */
	int accept_queue_length;
	/**
	 * The length limit of the accept queue.
	 * Also known as the backlog parameter of `listen`
	 */
	int accept_queue_limit;
	/** Spinlock to synchronize accept queue accesses */
	struct uk_spinlock accept_queue_lock;
	/**
	 * The posix_sock responsible for managing this VSOCK.
	 * This is primarily used for indicating events from this socket to the
	 * poll component. This will be set after posix-socket's socket creation
	 * is finished and the poll callback is called.
	 */
	posix_sock *posix_sock;
};

/**
 * Initialize a vsock buffer with the specified capacity.
 * Allocates memory for the circular buffer and initializes all tracking fields.
 * The buffer uses a circular design for efficient memory utilization and
 * automatic wraparound handling.
 *
 * @param buf The buffer structure to initialize
 * @param a The memory allocator to use for buffer allocation
 * @param bufsize The size of the buffer to allocate in bytes
 * @return 0 on success, negative errno on error
 */
int uk_vsock_buffer_init(struct uk_vsock_buffer *buf, struct uk_alloc *a,
			 __sz bufsize);

/**
 * Destroy a vsock buffer and free all associated resources.
 * This function must be called to prevent memory leaks when the buffer
 * is no longer needed.
 *
 * @param buf The buffer to destroy
 * @return 0 on success, negative errno on error
 */
int uk_vsock_buffer_destroy(struct uk_vsock_buffer *buf);

/**
 * Get the total capacity of the buffer.
 *
 * @param buf The buffer to query
 * @return The total capacity of the buffer in bytes
 */
static inline __sz uk_vsock_buffer_capacity(struct uk_vsock_buffer *buf)
{
	return buf->capacity;
}

/**
 * Append data from a network buffer chain to the socket buffer.
 * The entire netbuf chain must fit into the available free space or the
 * operation will fail without partial writes. The implementation handles
 * circular buffer wraparound automatically. The netbuf may be consumed
 * by this operation.
 *
 * @param buf The buffer to append data to
 * @param nb Pointer to the netbuf chain to append. May be set to NULL
 *           if the netbuf is consumed by this operation
 * @return 0 on success, negative errno on error
 */
int uk_vsock_buffer_append(struct uk_vsock_buffer *buf, struct uk_netbuf **nb);

/**
 * Read data from the buffer into the provided output buffer.
 * This operation does not consume the data from the buffer. Use
 * uk_vsock_buffer_consume() to mark data as processed and free up
 * buffer space. Handles circular buffer wraparound automatically.
 *
 * @param buf The socket buffer to read from
 * @param out Destination buffer for the read data
 * @param size Maximum number of bytes to read (capacity of output buffer)
 * @return The actual number of bytes read, which may be less than requested
 */
__ssz uk_vsock_buffer_read(struct uk_vsock_buffer *buf, char *out, __sz size);

/**
 * Mark data as consumed and advance the buffer head pointer.
 * This operation frees up buffer space for new data and updates the
 * total processed byte counter for flow control purposes.
 *
 * @param buf The buffer to operate on
 * @param size The number of bytes to mark as consumed
 */
static inline
void uk_vsock_buffer_consume(struct uk_vsock_buffer *buf, __sz size)
{
	uk_spin_lock(&buf->lock);

	UK_ASSERT(size <= buf->count);

	buf->head = (buf->head + size) % buf->capacity;
	buf->count -= size;
	buf->total_processed += size;

	uk_spin_unlock(&buf->lock);
}

/**
 * Check whether the buffer contains any data.
 *
 * @param buf The buffer to check
 * @return Non-zero if the buffer is empty, 0 if data is available
 */
static inline int uk_vsock_buffer_is_empty(struct uk_vsock_buffer *buf)
{
	int count;

	uk_spin_lock(&buf->lock);
	count = buf->count == 0;
	uk_spin_unlock(&buf->lock);

	return count;
}

/**
 * Get the total number of bytes that have been consumed from this buffer.
 * This value is used for flow control and connection state tracking.
 *
 * @param buf The buffer to query
 * @return The total number of bytes consumed since buffer initialization
 */
static inline __sz uk_vsock_total_processed(struct uk_vsock_buffer *buf)
{
	__sz processed;

	uk_spin_lock(&buf->lock);
	processed = buf->total_processed;
	uk_spin_unlock(&buf->lock);

	return processed;
}

/**
 * Get the amount of free space available in the buffer.
 *
 * @param buf The buffer to query
 * @return The number of bytes that can be written to the buffer
 */
static inline __sz uk_vsock_free_space(struct uk_vsock_buffer *buf)
{
	__sz free_space;

	uk_spin_lock(&buf->lock);
	free_space = buf->capacity - buf->count;
	uk_spin_unlock(&buf->lock);

	return free_space;
}

/**
 * Initialize a vsock socket structure with default values.
 * Sets up the receive buffer, initializes the accept queue for listening
 * operations, and sets the initial state to READY. The socket must be bound
 * to an address before use.
 *
 * The per-socket buffer sizing (buf_size/max_buf_size) is either inherited
 * from a parent listening socket (for accepted connections) or initialized
 * from the UK_VSOCK_BUF_SIZE / UK_VSOCK_MAX_BUF_SIZE defaults. The receive
 * buffer is allocated with the resulting buf_size.
 *
 * @param sock The vsock structure to initialize
 * @param a Memory allocator for internal data structures (receive buffer)
 * @param sock_type The socket type (SOCK_STREAM, etc.)
 * @param parent Optional listening socket to inherit the buffer sizing from.
 *               Pass NULL to use the configured defaults.
 * @return 0 on success, negative errno on error
 */
int uk_vsock_init(struct uk_vsock *sock, struct uk_alloc *a, int sock_type,
		  const struct uk_vsock *parent);

/**
 * Find an existing socket matching the specified connection parameters.
 * First attempts an exact match on socket type, local address, and peer
 * address. If no exact match is found, searches for a listening socket
 * that matches the socket type and local address (ignoring peer address).
 *
 * @param sock_type The socket type to match (SOCK_STREAM, etc.)
 * @param local The local socket address to match
 * @param peer The peer socket address to match
 * @return Pointer to matching socket, or NULL if no match found
 */
struct uk_vsock *uk_vsock_lookup(int sock_type,
				 struct uk_vsockaddr local,
				 struct uk_vsockaddr peer);

/**
 * Update the writable status of a socket for event notification.
 * Sets or clears the EPOLLOUT event flag to indicate whether the socket
 * is ready for writing. This is typically called by the transport layer
 * when buffer space becomes available or unavailable.
 *
 * @param sock The socket to update
 * @param writable Non-zero if the socket is writable, 0 otherwise
 */
void uk_vsock_notify_writable(struct uk_vsock *sock, int writable);

/**
 * Process received payload data for an established connection.
 * Appends the received data to the socket's receive buffer and sets
 * the EPOLLIN event to notify waiting applications. The packet may
 * be consumed by this operation.
 *
 * @param sock The destination socket (must be in OPEN state)
 * @param packet Pointer to the received data packet, may be set to NULL
 *               if consumed
 * @return 0 on success, negative errno on error
 */
int uk_vsock_rx_payload(struct uk_vsock *sock, struct uk_netbuf **packet);

/**
 * Handle a successful connection response from the peer.
 * Transitions the socket from WAIT_CONNECT state to OPEN state and
 * sets the EPOLLOUT event to indicate the socket is ready for writing.
 * This function should be called when the transport layer receives
 * a connection acceptance from the peer.
 *
 * @param sock The socket that received the connection response
 * @return 0 on success, negative errno on error
 */
int uk_vsock_conn_response(struct uk_vsock *sock);

/**
 * Handle a connection reset notification from the peer.
 * Sets appropriate error events (EPOLLERR, EPOLLHUP, EPOLLRDHUP) and
 * updates socket state based on the current connection state. For sockets
 * in WAIT_CONNECT state, sets so_error to ECONNREFUSED.
 *
 * @param sock The socket that received the reset
 * @return 0 on success, negative errno on error
 */
int uk_vsock_conn_reset(struct uk_vsock *sock);

/**
 * Handle a connection shutdown notification from the peer.
 * Updates the socket's shutdown state and sets appropriate events based
 * on which directions were shut down. If both directions are shut down,
 * the socket will be closed automatically.
 *
 * @param sock The socket receiving the shutdown notification
 * @param peer_rx Non-zero if the peer shut down its receive side
 *                (we can no longer send data)
 * @param peer_tx Non-zero if the peer shut down its transmit side
 *                (we will receive no more data)
 * @return 0 on success, negative errno on error
 */
int uk_vsock_conn_shutdown(struct uk_vsock *sock, int peer_rx, int peer_tx);

/**
 * Handle an incoming connection request to a listening socket.
 * Attempts to queue the connection request in the socket's accept queue.
 * The request will be rejected if the socket is not listening or if
 * the accept queue is at capacity (backlog limit reached).
 *
 * @param sock The listening socket receiving the connection request
 * @param entry The accept entry containing peer connection information
 * @return 0 on success, negative errno on error
 */
int uk_vsock_conn_request(struct uk_vsock *sock,
			  struct uk_vsock_accept_entry *entry);

/**
 * Initialize the global vsock subsystem.
 * Sets up the global socket tracking data structures and port allocation
 * state. This function must be called before any socket operations.
 *
 * @return 0 on success, negative errno on error
 */
int uk_vsockets_init(void);

/**
 * Terminate the vsock subsystem and clean up active connections.
 * Closes all sockets that would be affected by a transport reset,
 * including connected and connecting sockets, and sockets bound to
 * specific CIDs. Unbound sockets and those bound to CID_ANY are
 * preserved across transport resets.
 *
 * @return 0 on success, negative errno on error
 */
int uk_vsockets_term(void);

#endif /* __UK_VSOCKDEV_VSOCK_H__ */

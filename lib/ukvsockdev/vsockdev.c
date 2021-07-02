#define _GNU_SOURCE /* for asprintf() */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/arch/atomic.h>
#include <uk/vsockdev.h>
#include <uk/vsock.h>

/*
 * Vsock device is different from the network device where we could have
 * multiple virtualized network interface cards on a machine. It does
 * not make sense to have multiple vsock devices on a machine, therefore
 * there is no list of devices.
 */
struct uk_vsockdev *uk_vskdev = NULL;

int uk_vsockdev_drv_register(struct uk_vsockdev *dev)
{
	UK_ASSERT(dev);

	/* Driver already registered */
	if (uk_vskdev) {
		uk_pr_err("There is already a vsock device registered");
		return 0;
	}

	UK_ASSERT(dev->dev_ops);

	uk_vskdev = dev;
	uk_pr_info("Registered vsockdev: %p\n", dev);

	return 1;
}

struct uk_vsockdev *uk_vsockdev_get()
{
	return uk_vskdev;
}

void uk_vsockdev_drv_unregister(struct uk_vsockdev *dev)
{
	UK_ASSERT(dev != NULL);

	uk_vskdev = NULL;

	uk_pr_info("Unregistered vsockdev: %p\n", dev);
}


#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
static void _dispatcher(void *arg)
{
	struct uk_vsockdev_event_handler *handler =
		(struct uk_vsockdev_event_handler *) arg;

	UK_ASSERT(handler);
	UK_ASSERT(handler->callback);

	for (;;) {
		uk_semaphore_down(&handler->events);
		handler->callback(handler->dev,
				  handler->queue_id,
				  handler->cookie);
	}
}
#endif

static int _create_event_handler(uk_vsockdev_queue_event_t callback,
				 void *callback_cookie,
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				 struct uk_vsockdev *dev, uint16_t queue_id,
				 const char *queue_type_str,
				 struct uk_sched *s,
#endif
				 struct uk_vsockdev_event_handler *h)
{
	UK_ASSERT(h);
	UK_ASSERT(callback || (!callback && !callback_cookie));
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT(!h->dispatcher);
#endif

	h->callback = callback;
	h->cookie   = callback_cookie;

#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	/* If we do not have a callback, we do not need a thread */
	if (!callback)
		return 0;

	h->dev = dev;
	h->queue_id = queue_id;
	uk_semaphore_init(&h->events, 0);
	h->dispatcher_s = s;

	/* Create a name for the dispatcher thread.
	 * In case of errors, we just continue without a name
	 */
	if (asprintf(&h->dispatcher_name,
		     "vsockdev%"PRIu16"-%s[%"PRIu16"]",
		     dev->_data->id, queue_type_str, queue_id) < 0) {
		h->dispatcher_name = NULL;
	}

	h->dispatcher = uk_sched_thread_create(h->dispatcher_s,
					       h->dispatcher_name, NULL,
					       _dispatcher, h);
	if (!h->dispatcher) {
		if (h->dispatcher_name)
			free(h->dispatcher_name);
		h->dispatcher_name = NULL;
		return -ENOMEM;
	}
#endif

	return 0;
}

static void _destroy_event_handler(struct uk_vsockdev_event_handler *h
				   __maybe_unused)
{
	UK_ASSERT(h);

#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	UK_ASSERT(h->dispatcher_s);

	if (h->dispatcher) {
		uk_thread_kill(h->dispatcher);
		uk_thread_wait(h->dispatcher);
	}
	h->dispatcher = NULL;

	if (h->dispatcher_name)
		free(h->dispatcher_name);
	h->dispatcher_name = NULL;
#endif
}

static int uk_vsockdev_tx(struct vsock_sock *sock, struct uk_alloc *a,
	enum virtio_vsock_op op, const void *buf, int len, int flags)
{
	int sz;
	int rc;
	int peer_free;
	int ret = 0;
	struct virtio_vsock_packet *packet;
	struct uk_vsockdev *dev;
	
	dev = uk_vskdev;
	
	do {
		int used;
		sz = len < VIRTIO_VSOCK_BUF_DATA_SIZE ? len : VIRTIO_VSOCK_BUF_DATA_SIZE;

		peer_free = sock->buf_alloc - (sock->tx_cnt - sock->fwd_cnt);
		if (sz > peer_free) {
			uk_vsockdev_tx(sock, a, VIRTIO_VSOCK_OP_CREDIT_REQUEST, NULL, 0, 0);
			break;
		}

		packet = uk_calloc(a, 1, sizeof(struct virtio_vsock_packet));
		packet->hdr.src_cid = sock->local_addr.cid;
		packet->hdr.dst_cid = sock->remote_addr.cid;
		packet->hdr.src_port = sock->local_addr.port;
		packet->hdr.dst_port = sock->remote_addr.port;
		packet->hdr.len = sz;
		packet->hdr.type = VIRTIO_VSOCK_TYPE_STREAM;
		packet->hdr.op = op;
		packet->hdr.flags = 0;
		packet->hdr.buf_alloc = sock->rxb.cap;
		used = sock->rxb.tail - sock->rxb.head;
		if (used < 0) {
			used += sock->rxb.cap;
		}
		packet->hdr.fwd_cnt = sock->rxb.cap - used;
		packet->data = uk_calloc(a, sz, 1);
		memcpy(packet->data, buf, sz);

		len -= sz;
		sock->tx_cnt += sz;
		rc = dev->tx_one(dev, NULL, packet);
		if (unlikely(rc != 0))
			return -1;

		ret += sz;
	} while (len > 0);

	return ret;
}

void handle_vsock_connection_request(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	struct vsock_sock *sock;
	struct vsock_sock *new_sock;
	int new_fd = 0;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->state == VSOCK_STATE_ACCEPT && 
			(sock->local_addr.port == pkt->hdr.dst_port || sock->local_addr.port == VMADDR_PORT_ANY)) {
			int sz;
			
			sz = VIRTIO_VSOCK_RX_DATA_SIZE * VSOCK_RING_BUFS;
			new_sock = uk_calloc(vsh->a, 1, sizeof(struct vsock_sock));

			new_sock->rxb.buf = uk_calloc(vsh->a, sz, 1);
			new_sock->rxb.cap = sz;
			new_sock->txb.buf = uk_calloc(vsh->a, sz, 1);
			new_sock->txb.cap = sz;

		    new_sock->buf_alloc = VIRTIO_VSOCK_RX_DATA_SIZE * VSOCK_RING_BUFS;
			new_sock->fd = ++vsh->cnt;
			new_sock->state = VSOCK_STATE_CONNECTED;

			new_sock->local_addr.cid = pkt->hdr.dst_cid;
			new_sock->local_addr.port = pkt->hdr.dst_port;
			new_sock->remote_addr.cid = pkt->hdr.src_cid;
			new_sock->remote_addr.port = pkt->hdr.src_port;

			uk_mutex_init(&new_sock->rx_mutex);
			uk_mutex_init(&new_sock->tx_mutex);
			uk_semaphore_init(&new_sock->conn_sem, 0);
			uk_semaphore_init(&new_sock->rx_sem, 0);
			
			new_fd = new_sock->fd;
			UK_TAILQ_INSERT_TAIL(&vsh->sock_list, new_sock, _list);
			break;
		}
	}

	if (!new_fd) {
		uk_pr_err("Couldn't handle connection request\n");
		return;
	}

	uk_semaphore_up(&sock->conn_sem);
	uk_vsockdev_tx(new_sock, vsh->a, VIRTIO_VSOCK_OP_RESPONSE, NULL, 0, 0);
}

void handle_vsock_connection_resposne(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	struct vsock_sock *sock;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->state == VSOCK_STATE_WAIT_CONNECT &&
			(sock->local_addr.port == pkt->hdr.dst_port || sock->local_addr.port == VMADDR_PORT_ANY)) {
			break;
		}
	}

	sock->remote_addr.port = pkt->hdr.src_port;
	sock->state = VSOCK_STATE_CONNECTED;
	uk_semaphore_up(&sock->conn_sem);
}

static void handle_vsock_connection_reset(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	struct vsock_sock *sock;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->local_addr.port == pkt->hdr.dst_port) {
			break;
		}
	}

	if (!sock) {
		return;
	}

	UK_TAILQ_REMOVE(&vsh->sock_list, sock, _list);

}

static void handle_vsock_connection_shutdown(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	struct vsock_sock *sock;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->local_addr.port == pkt->hdr.dst_port) {
			break;
		}
	}

	if (!sock) {
		return;
	}

	UK_TAILQ_REMOVE(&vsh->sock_list, sock, _list);
}

static void handle_vsock_data_recv(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	int len;
	struct vsock_sock *sock = NULL;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->state == VSOCK_STATE_CONNECTED &&
			sock->local_addr.port == pkt->hdr.dst_port) {
			break;
		}
	}

	if (!sock) {
		uk_pr_err("Received vsock message, but socket not found\n");
		return;
	}

	uk_mutex_lock(&sock->rx_mutex);
	if (sock->rxb.head == sock->rxb.tail && !sock->rxb.full) {
		uk_semaphore_up(&sock->rx_sem);
	}

	if (pkt->hdr.len + sock->rxb.tail >= sock->rxb.cap) {
		/* overflow, must copy at the beginning of the buffer */
		len = sock->rxb.cap - sock->rxb.tail;
		memcpy(sock->rxb.buf + sock->rxb.tail, pkt->data, len);
		memcpy(sock->rxb.buf, pkt->data + len, pkt->hdr.len - len);
	} else {
		/* fits perfectly */
		memcpy(sock->rxb.buf + sock->rxb.tail, pkt->data, pkt->hdr.len);
	}
	sock->rxb.tail += pkt->hdr.len;

	if (sock->rxb.tail >= sock->rxb.cap) {
		sock->rxb.tail -= sock->rxb.cap;
	}

	if (sock->rxb.head == sock->rxb.tail) {
		sock->rxb.full = 1;
	}
	uk_mutex_unlock(&sock->rx_mutex);
}

static void handle_vsock_credit_update(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	struct vsock_sock *sock = NULL;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->state == VSOCK_STATE_CONNECTED &&
			sock->local_addr.port == pkt->hdr.dst_port) {
			break;
		}
	}

	if (!sock) {
		return;
	}

	sock->buf_alloc = pkt->hdr.buf_alloc;
	sock->fwd_cnt = pkt->hdr.fwd_cnt;
}

static void handle_vsock_credit_request(struct vsock_handler *vsh,
	struct virtio_vsock_packet *pkt)
{
	struct vsock_sock *sock = NULL;

	UK_TAILQ_FOREACH(sock, &vsh->sock_list, _list) {
		if (sock->state == VSOCK_STATE_CONNECTED &&
			sock->local_addr.port == pkt->hdr.dst_port) {
			break;
		}
	}

	if (!sock) {
		return;
	}

	uk_vsockdev_tx(sock, vsh->a, VIRTIO_VSOCK_OP_CREDIT_REQUEST, NULL, 0, 0);
}

static void uk_vsockdev_rx_one(struct uk_vsockdev *dev, 
			   uint16_t queue_id __unused, void *argp __unused)
{
	struct uk_alloc *a;
	struct virtio_vsock_packet *pkt;
	struct vsock_handler *vsh;
	int rc = 0;

	vsh = (struct vsock_handler *) argp;
	
	a = uk_alloc_get_default();
	pkt = uk_calloc(a, 1, sizeof(struct virtio_vsock_packet));
	pkt->data = uk_calloc(a, VIRTIO_VSOCK_RX_DATA_SIZE, sizeof(__u8));

	UK_ASSERT(dev);
	UK_ASSERT(dev->rx_one);
	UK_ASSERT(pkt);
	
	rc = dev->rx_one(dev, dev->_rx_queue, &pkt);
	if (rc)
		return;

	switch (pkt->hdr.op) {
	case VIRTIO_VSOCK_OP_REQUEST:
		handle_vsock_connection_request(vsh, pkt);
		break;
	case VIRTIO_VSOCK_OP_RESPONSE:
		handle_vsock_connection_resposne(vsh, pkt);
		break;
	case VIRTIO_VSOCK_OP_RST:
		handle_vsock_connection_reset(vsh, pkt);
		break;
	case VIRTIO_VSOCK_OP_SHUTDOWN:
		handle_vsock_connection_shutdown(vsh, pkt);
		break;
	case VIRTIO_VSOCK_OP_RW:
		handle_vsock_data_recv(vsh, pkt);
		break;
	case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
		handle_vsock_credit_update(vsh, pkt);
		break;
	case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
		handle_vsock_credit_request(vsh, pkt);
		break;
	default:
		return;
	}
}

int uk_vsockev_rxq_configure(struct uk_vsockdev *dev, uint16_t queue_id,
			    uint16_t nb_desc,
			    struct uk_vsockdev_rxqueue_conf *rx_conf)
{
	int err;

	UK_ASSERT(dev);
	UK_ASSERT(rx_conf);

	err = _create_event_handler(rx_conf->callback, rx_conf->callback_cookie,
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
				    dev, queue_id, "rxq", rx_conf->s,
#endif
				    &dev->rx_handler);
	if (err)
		goto err_out;

	dev->_rx_queue = dev->dev_ops->rxq_configure(dev, queue_id,
							   nb_desc, rx_conf);
	if (PTRISERR(dev->_rx_queue)) {
		err = PTR2ERR(dev->_rx_queue);
		goto err_destroy_handler;
	}

	return 0;

err_destroy_handler:
	_destroy_event_handler(&dev->rx_handler);
err_out:
	return err;
}

struct vsock_handler vsh;

int vsock_socket() {
    struct vsock_sock *sock;
	int sz;

	sz = VIRTIO_VSOCK_RX_DATA_SIZE * VSOCK_RING_BUFS;

    sock = uk_calloc(vsh.a, 1, sizeof(struct vsock_sock));
    sock->rxb.buf = uk_calloc(vsh.a, sz, 1);
	sock->rxb.cap = sz;
    sock->txb.buf = uk_calloc(vsh.a, sz, 1);
	sock->txb.cap = sz;

    sock->buf_alloc = sz;

	uk_mutex_init(&sock->rx_mutex);
	uk_mutex_init(&sock->tx_mutex);
	uk_semaphore_init(&sock->conn_sem, 0);
	uk_semaphore_init(&sock->rx_sem, 0);

    sock->fd = ++vsh.cnt;
	UK_TAILQ_INSERT_TAIL(&vsh.sock_list, sock, _list);

    return sock->fd;
}

struct vsock_sock *vsock_get(int fd)
{
	struct vsock_sock *sock;

    UK_TAILQ_FOREACH(sock, &vsh.sock_list, _list) {
        if (sock->fd == fd) {
            return sock;
        }
	}

    return NULL;
}

int vsock_close(int sockfd)
{
    struct vsock_sock *sock;

    sock = vsock_get(sockfd);
    if (sock == NULL) {
        return -1;
    }

	uk_vsockdev_tx(sock, vsh.a, VIRTIO_VSOCK_OP_SHUTDOWN, NULL, 0,
		VIRTIO_VSOCK_SHUTDOWN_RCV | VIRTIO_VSOCK_SHUTDOWN_SEND);
	uk_vsockdev_tx(sock, vsh.a, VIRTIO_VSOCK_OP_RST, NULL, 0, 0);

	UK_TAILQ_REMOVE(&vsh.sock_list, sock, _list);

	return 0;
}

int vsock_bind(int sockfd, struct vsock_sockaddr *addr)
{
    struct vsock_sock *sock;

    if (addr->cid != VMADDR_CID_ANY) {
        return -1;   
    }

    sock = vsock_get(sockfd);
    if (sock == NULL) {
        errno = EBADF;
        return -1;
    }

	addr->cid = uk_vskdev->dev_ops->get_cid(uk_vskdev);

    sock->local_addr.cid = addr->cid;
    sock->local_addr.port = addr->port;

    return 0;
}

int vsock_listen(int sockfd, int backlog)
{
    struct vsock_sock *sk;
    
    sk = vsock_get(sockfd);
    if (sk == NULL) {
        return -1;
    }
    sk->state = VSOCK_STATE_LISTEN;

    return 0;
}

int vsock_accept(int sockfd, struct vsock_sockaddr *addr)
{
    struct vsock_sock *sk;
	struct vsock_sock *sock;

    sk = vsock_get(sockfd);
    if (sk == NULL) {
        return -1;
    }

	sk->state = VSOCK_STATE_ACCEPT;
	
	uk_semaphore_down(&sk->conn_sem);
	sock = UK_TAILQ_LAST(&vsh.sock_list, vsock_sock_list);
	addr->cid = sock->remote_addr.cid;
	addr->port = sock->remote_addr.port;

    return sock->fd;
}

int vsock_connect(int sockfd, const struct vsock_sockaddr *addr)
{
	struct vsock_sock *sock;
	
	sock = vsock_get(sockfd);
	if (!sock) {
		return -1;
	}

	sock->remote_addr.cid = addr->cid;
	sock->remote_addr.port = addr->port;

	sock->state = VSOCK_STATE_WAIT_CONNECT;
	uk_vsockdev_tx(sock, vsh.a, VIRTIO_VSOCK_OP_REQUEST, NULL, 0, 0);
	uk_semaphore_down(&sock->conn_sem);

    return 0;
}

ssize_t vsock_send(int sockfd, const void *buf, size_t len, int flags)
{
	int l;
	int sz = 0;
	unsigned long avail;
	struct vsock_sock *sock;
	
	sock = vsock_get(sockfd);
	if (!sock) {
		return -1;
	}

	if (sock->txb.full)
		return 0;

	uk_mutex_lock(&sock->tx_mutex);
	if (sock->txb.tail < sock->txb.head) {
		avail = sock->txb.head - sock->txb.tail;
	} else {
		avail = sock->txb.cap - (sock->txb.tail - sock->txb.head);
	}
	if (len > avail) {
		/* user tries to write more than available space */
		sz = avail;
	} else {
		sz = len;
	}

	if (sock->txb.tail + sz >= sock->txb.cap) {
		/* overflow, must copy at the beginning of the buffer */
		l = sock->txb.cap - sock->txb.tail;
		memcpy(sock->txb.buf + sock->txb.tail, buf, l);
		memcpy(sock->txb.buf, buf, sz - l);
	} else {
		/* fits perfectly */
		memcpy(sock->txb.buf + sock->txb.tail, buf, sz);
	}

	sock->txb.tail += sz;
	if  (sock->txb.tail >= sock->txb.cap) {
		sock->txb.tail -= sock->txb.cap; 
	}

	if (sock->txb.tail == sock->txb.head) {
		sock->txb.full = 1;
	}

	uk_vsockdev_tx(sock, vsh.a, VIRTIO_VSOCK_OP_RW, buf, sz, 0);
	uk_mutex_unlock(&sock->tx_mutex);
    return sz;
}

ssize_t vsock_recv(int sockfd, void *buf, size_t len, int flags)
{
	int l;
	int sz;
	long avail;
	struct vsock_sock *sock;
	
	sock = vsock_get(sockfd);
	if (!sock) {
		return -1;
	}

	uk_mutex_lock(&sock->rx_mutex);
	avail = sock->rxb.tail - sock->rxb.head;
	if (avail < 0 || (avail == 0 && sock->rxb.full)) {
		avail += sock->rxb.cap;
	}

	if (avail == 0) {
		/* wait to recv some data */
		uk_mutex_unlock(&sock->rx_mutex);
		uk_semaphore_down(&sock->rx_sem);
		uk_mutex_lock(&sock->rx_mutex);
	}

	if (len > avail) {
		/* the user required more data than possible */
		sz = avail;
	} else {
		/* enough data for user */
		sz = len;
	}
	if (sock->rxb.head + sz >= sock->rxb.cap) {
		l = sock->rxb.cap - sock->rxb.head;
		memcpy(buf, sock->rxb.buf + sock->rxb.head, l);
		memcpy(buf + l, sock->rxb.buf, l - sz);
	} else {
		memcpy(buf, sock->rxb.buf + sock->rxb.head, sz);
	}

	sock->rxb.head += sz;
	if (sock->rxb.head >= sock->rxb.cap) {
		sock->rxb.head -= sock->rxb.cap;
	}
	sock->rxb.full = 0;
	uk_mutex_unlock(&sock->rx_mutex);
	
    return sz;
}

void vsock_init() {
    vsh.a = uk_alloc_get_default();
    if (!vsh.sock_list_initialized) {
        UK_TAILQ_INIT(&vsh.sock_list);
        vsh.sock_list_initialized = 1;
    }
    vsh.cnt = 1;
}


int uk_vsockdev_init() {
	int rc = 0;
	struct uk_vsockdev_rxqueue_conf rx_conf;
	struct uk_vsockdev_evqueue_conf ev_conf;
	struct uk_vsockdev_txqueue_conf tx_conf;
	struct uk_alloc *a;
	struct uk_vsockdev *dev = uk_vskdev;

	UK_ASSERT(dev);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->dev_ops->dev_start);

	vsock_init();
	a = uk_alloc_get_default();

	rx_conf.a = a;
	rx_conf.callback = uk_vsockdev_rx_one;
	rx_conf.callback_cookie = &vsh;
#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	rxq_conf.s = uk_sched_get_default();
	if (!rxq_conf.s)
		return ERR_IF;

#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
	rc = uk_vsockev_rxq_configure(dev, 0, 0, &rx_conf);

	ev_conf.a = a;
	dev->_ev_queue = dev->dev_ops->evq_configure(dev, 1, 0, &ev_conf);

	tx_conf.a = a;
	dev->_tx_queue = dev->dev_ops->txq_configure(dev, 2, 0, &tx_conf);

	rc = dev->dev_ops->dev_start(dev);
	
	return rc;
}

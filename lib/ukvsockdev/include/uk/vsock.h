#ifndef __UK_VSOCK__
#define __UK_VSOCK__

#include <uk/list.h>
#include <sys/types.h>
#include <uk/mutex.h>
#include <uk/semaphore.h>

#define VSOCK_RING_BUFS		64

#define VIRTIO_VSOCK_SHUTDOWN_RCV	1 << 0
#define VIRTIO_VSOCK_SHUTDOWN_SEND	1 << 1

#define VMADDR_CID_ANY			-1
#define VMADDR_CID_HYPERVISOR	 0
#define VMADDR_CID_LOCAL		 1
#define VMADDR_CID_HOST			 2

#define VMADDR_PORT_ANY			-1

#ifdef __cplusplus
extern "C" {
#endif

enum vsock_state {
	VSOCK_STATE_READY,
	VSOCK_STATE_BOUND,
	VSOCK_STATE_WAIT_CONNECT,
	VSOCK_STATE_LISTENING,
	VSOCK_STATE_OPEN,
	VSOCK_STATE_CLOSED
};

struct vsock_sockaddr {
	unsigned long cid;
	unsigned int port;
};

struct vsock_buf {
	/* total capacity of the buffer */
	int cap;
	/* read pointer */
	int head;
	/* write pointer */
	int tail;
	/* actual data */
	char *buf;
	/* if buffer is full */
	int full;
};

struct vsock_sock;
struct vsock_sock {
    int fd;
	enum vsock_state state;
    struct vsock_sockaddr local_addr;
    struct vsock_sockaddr remote_addr;
	
	struct uk_semaphore rx_sem;
	struct uk_mutex rx_mutex;
	struct vsock_buf rxb;
	struct uk_mutex tx_mutex;
	struct vsock_buf txb;

	struct uk_semaphore conn_sem;
	int tx_cnt;
	int buf_alloc;
	int fwd_cnt;
	UK_TAILQ_ENTRY(struct vsock_sock) _list;
};

UK_TAILQ_HEAD(vsock_sock_list, struct vsock_sock);

struct vsock_handler {
	struct uk_alloc *a;
	int cnt;
	int sock_list_initialized;
	struct vsock_sock_list sock_list;
};

/**
 * Opens a vsock socket.
 *
 * @return
 *   - > 0: representing sockfd
 *   - < 0: error
 */
int vsock_socket();

/**
 * Closes a vsock socket.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @return
 *   - = 0: succes
 *   - < 0: error
 */
int vsock_close(int sockfd);

/**
 * Binds an anddress to a socket.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @param addr
 * 	 Pointer to a vsock_sockaddr representing address and port.
 * @return
 *   - = 0: succes
 *   - < 0: error
 */
int vsock_bind(int sockfd, struct vsock_sockaddr *addr);

/**
 * Set a vsock socket in listen state.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @param backlog
 *   Not used at the moment.
 * @return
 *   - = 0: succes
 *   - < 0: error
 */
int vsock_listen(int sockfd, int backlog);

/**
 * Accepts the first incoming connection.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @param addr
 * 	 Pointer to a vsock_sockaddr representing address and port.
 * @return
 *   - > 0: representing sockfd
 *   - < 0: error
 */
int vsock_accept(int sockfd, struct vsock_sockaddr *addr);

/**
 * Connects a socket to a specific address.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @param addr
 * 	 Pointer to a vsock_sockaddr representing address and port.
 * @return
 *   - = 0: succes
 *   - < 0: error
 */
int vsock_connect(int sockfd, const struct vsock_sockaddr *addr);

/**
 * Sends a message on a socket.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @param buf
 * 	 Pointer to a buffer containing the message.
 * @param len
 *   Length to be sent.
 * @param flags
 *    Not used at the moment.
 * @return
 *   - > 0: number of bytes sent
 *   - < 0: error
 */
ssize_t vsock_send(int sockfd, const void *buf, size_t len, int flags);

/**
 * Receives a message from a socket.
 *
 * @param sockfd
 *   The identifier of the vsock socket to close.
 * @param buf
 * 	 Pointer to a buffer where to be stored the read message.
 * @param len
 *   Maximum length to be read.
 * @param flags
 *    Not used at the moment.
 * @return
 *   - > 0: number of bytes received
 *   - < 0: error
 */
ssize_t vsock_recv(int sockfd, void *buf, size_t len, int flags);

/**
 * Initializes vsock.
 *
 * @return
 *   - = 0: succes
 *   - < 0: error
 */
int uk_vsockdev_init();

#ifdef __cplusplus
}
#endif

#endif /* __UK_VSOCK__ */

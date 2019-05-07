#include <uk/mbox.h>
#include <uk/assert.h>
#include <uk/arch/limits.h>

struct uk_mbox {
	size_t len;
	struct uk_semaphore readsem;
	long readpos;
	struct uk_semaphore writesem;
	long writepos;

	void *msgs[];
};

struct uk_mbox *uk_mbox_create(struct uk_alloc *a, size_t size)
{
	struct uk_mbox *m;

	UK_ASSERT(size <= __L_MAX);

	m = uk_malloc(a, sizeof(*m) + (sizeof(void *) * (size + 1)));
	if (!m)
		return NULL;

	m->len = size + 1;

	uk_semaphore_init(&m->readsem, 0);
	m->readpos = 0;

	uk_semaphore_init(&m->writesem, (long) size);
	m->writepos = 0;

	uk_pr_debug("Created mailbox %p\n", m);
	return m;
}

/* Deallocates a mailbox. If there are messages still present in the
 * mailbox when the mailbox is deallocated, it is an indication of a
 * programming error in lwIP and the developer should be notified.
 */
void uk_mbox_free(struct uk_alloc *a, struct uk_mbox *m)
{
	uk_pr_debug("Release mailbox %p\n", m);

	UK_ASSERT(a);
	UK_ASSERT(m);
	UK_ASSERT(m->readpos == m->writepos);

	uk_free(a, m);
}

/* Posts the "msg" to the mailbox, internal version that actually does the
 * post.
 */
static inline void _do_mbox_post(struct uk_mbox *m, void *msg)
{
	/* The caller got a semaphore token, so we are now allowed to increment
	 * writer, but we still need to prevent concurrency between writers
	 * (interrupt handler vs main)
	 */
	unsigned long irqf;

	UK_ASSERT(m);

	irqf = ukplat_lcpu_save_irqf();
	m->msgs[m->writepos] = msg;
	m->writepos = (m->writepos + 1) % m->len;
	UK_ASSERT(m->readpos != m->writepos);
	ukplat_lcpu_restore_irqf(irqf);
	uk_pr_debug("Posted message %p to mailbox %p\n", msg, m);

	uk_semaphore_up(&m->readsem);
}

void uk_mbox_post(struct uk_mbox *m, void *msg)
{
	/* The caller got a semaphore token, so we are now allowed to increment
	 * writer, but we still need to prevent concurrency between writers
	 * (interrupt handler vs main)
	 */
	UK_ASSERT(m);

	uk_semaphore_down(&m->writesem);
	_do_mbox_post(m, msg);
}

int uk_mbox_post_try(struct uk_mbox *m, void *msg)
{
	UK_ASSERT(m);

	if (!uk_semaphore_down_try(&m->writesem))
		return -ENOBUFS;
	_do_mbox_post(m, msg);
	return 0;
}

__nsec uk_mbox_post_to(struct uk_mbox *m, void *msg, __nsec timeout)
{
	__nsec ret;

	UK_ASSERT(m);

	ret = uk_semaphore_down_to(&m->writesem, timeout);
	if (ret != __NSEC_MAX)
		_do_mbox_post(m, msg);
	return ret;
}

/*
 * Fetch a message from a mailbox. Internal version that actually does the
 * fetch.
 */
static inline void *_do_mbox_recv(struct uk_mbox *m)
{
	unsigned long irqf;
	void *ret;

	uk_pr_debug("Receive message from mailbox %p\n", m);
	irqf = ukplat_lcpu_save_irqf();
	UK_ASSERT(m->readpos != m->writepos);
	ret = m->msgs[m->readpos];
	m->readpos = (m->readpos + 1) % m->len;
	ukplat_lcpu_restore_irqf(irqf);

	uk_semaphore_up(&m->writesem);

	return ret;
}

/* Blocks the thread until a message arrives in the mailbox.
 * The `*msg` argument will point to the received message.
 * If the `msg` parameter was set to NULL, the received message is dropped.
 */
void uk_mbox_recv(struct uk_mbox *m, void **msg)
{
	void *rmsg;

	UK_ASSERT(m);

	uk_semaphore_down(&m->readsem);
	rmsg =  _do_mbox_recv(m);
	if (msg)
		*msg = rmsg;
}


/* This is similar to uk_mbox_fetch, however if a message is not
 * present in the mailbox, it immediately returns with the code
 * SYS_MBOX_EMPTY. On success 0 is returned.
 *
 * To allow for efficient implementations, this can be defined as a
 * function-like macro in sys_arch.h instead of a normal function. For
 * example, a naive implementation could be:
 *   #define uk_mbox_tryfetch(mbox,msg) \
 *     uk_mbox_fetch(mbox,msg,1)
 * although this would introduce unnecessary delays.
 */
int uk_mbox_recv_try(struct uk_mbox *m, void **msg)
{
	void *rmsg;

	UK_ASSERT(m);

	if (!uk_semaphore_down_try(&m->readsem))
		return -ENOMSG;

	rmsg =  _do_mbox_recv(m);
	if (msg)
		*msg = rmsg;
	return 0;
}


/* Blocks the thread until a message arrives in the mailbox, but does
 * not block the thread longer than "timeout" milliseconds (similar to
 * the sys_arch_sem_wait() function). The "msg" argument is a result
 * parameter that is set by the function (i.e., by doing "*msg =
 * ptr"). The "msg" parameter maybe NULL to indicate that the message
 * should be dropped.
 *
 * The return values are the same as for the sys_arch_sem_wait() function:
 * Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
 * timeout.
 */
__nsec uk_mbox_recv_to(struct uk_mbox *m, void **msg, __nsec timeout)
{
	void *rmsg = NULL;
	__nsec ret;

	UK_ASSERT(m);

	ret = uk_semaphore_down_to(&m->readsem, timeout);
	if (ret != __NSEC_MAX)
		rmsg = _do_mbox_recv(m);

	if (msg)
		*msg = rmsg;
	return ret;
}

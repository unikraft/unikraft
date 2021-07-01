#include <uk/mbox.h>
#include <uk/isr/mbox.h>

struct uk_mbox {
	size_t len;
	struct uk_semaphore readsem;
	long readpos;
	struct uk_semaphore writesem;
	long writepos;

	void *msgs[];
};

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

int uk_mbox_recv_try_isr(struct uk_mbox *m, void **msg)
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

int uk_mbox_post_try_isr(struct uk_mbox *m, void *msg)
{
	UK_ASSERT(m);

	if (!uk_semaphore_down_try(&m->writesem))
		return -ENOBUFS;
	_do_mbox_post(m, msg);
	return 0;
}

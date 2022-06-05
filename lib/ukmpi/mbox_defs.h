#ifndef __MBOX_DEFS_H__
#define __MBOX_DEFS_H__

#include <stddef.h>
#include <uk/semaphore.h>

/*
 * NOTE: The definitions below are included by both isr-safe and normal
 * compilation units. Moving one of the inline functions from this file
 * requires special care.
 */

struct uk_mbox {
	size_t len;
	struct uk_semaphore readsem;
	long readpos;
	struct uk_semaphore writesem;
	long writepos;

	void *msgs[];
};

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

#endif /* __MBOX_DEFS_H__ */

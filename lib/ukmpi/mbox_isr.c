#include <uk/mbox.h>
#include <uk/isr/mbox.h>
#include <uk/isr/semaphore.h>
#include "mbox_defs.h"

int uk_mbox_recv_try_isr(struct uk_mbox *m, void **msg)
{
	void *rmsg;

	UK_ASSERT(m);

	if (!uk_semaphore_down_try_isr(&m->readsem))
		return -ENOMSG;

	rmsg = _do_mbox_recv_item(m);
	uk_semaphore_up_isr(&m->writesem);
	if (msg)
		*msg = rmsg;
	return 0;
}

int uk_mbox_post_try_isr(struct uk_mbox *m, void *msg)
{
	UK_ASSERT(m);

	if (!uk_semaphore_down_try_isr(&m->writesem))
		return -ENOBUFS;
	_do_mbox_post_item(m, msg);
	uk_semaphore_up_isr(&m->readsem);
	return 0;
}

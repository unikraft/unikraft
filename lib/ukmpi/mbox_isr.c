#include <uk/mbox.h>
#include <uk/isr/mbox.h>
#include <uk/isr/semaphore.h>
#include "mbox_defs.h"

int uk_mbox_recv_try_isr(struct uk_mbox *m, void **msg)
{
	void *rmsg;

	UK_ASSERT(m);

	if (unlikely(!uk_semaphore_down_try_isr(&m->readsem)))
		return -ENOMSG;

	rmsg =  _do_mbox_recv(m);
	if (msg)
		*msg = rmsg;
	return 0;
}

int uk_mbox_post_try_isr(struct uk_mbox *m, void *msg)
{
	UK_ASSERT(m);

	if (unlikely(!uk_semaphore_down_try_isr(&m->writesem)))
		return -ENOBUFS;
	_do_mbox_post(m, msg);
	return 0;
}

/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/alloc.h>
#include <uk/atomic.h>
#include <uk/list.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/spinlock.h>

#include "fdio-impl.h"

struct uk_ofile_async {
	struct uk_ofile *of;
	struct uk_poll_chain ticket;
	struct uk_list_head pending;
	unsigned long queued_generation;
};

static struct uk_mutex worker_lock = UK_MUTEX_INITIALIZER(worker_lock);
static struct uk_thread *worker_thread;
static struct uk_semaphore pending_count;
static uk_spinlock pending_lock = UK_SPINLOCK_INITIALIZER();
static UK_LIST_HEAD(pending);

/* The poll lock is held by the caller. Defer signal enqueue and notification
 * so neither process locks nor another poll chain are taken under that lock.
 */
static void async_ready(uk_pollevent events __unused,
			enum uk_poll_chain_op op, struct uk_poll_chain *ticket)
{
	struct uk_ofile_async *async = ticket->arg;
	struct uk_ofile *of = async->of;
	unsigned long flags;
	bool wake = false;

	if (op != UK_POLL_CHAINOP_SET)
		return;
	uk_spin_lock_irqsave(&pending_lock, flags);
	if (uk_list_empty(&async->pending) &&
	    uk_refcount_acquire_if_not_zero(&of->refcnt)) {
		uk_list_add_tail(&async->pending, &pending);
		wake = true;
	}
	if (!uk_list_empty(&async->pending))
		async->queued_generation = UK_READ_ONCE(of->async_generation);
	uk_spin_unlock_irqrestore(&pending_lock, flags);
	if (wake)
		uk_semaphore_up(&pending_count);
}

static void async_worker(void *arg __unused)
{
	for (;;) {
		struct uk_ofile_async *async;
		struct uk_ofile *of;
		unsigned long flags, generation;
		pid_t owner = 0;

		uk_semaphore_down(&pending_count);
		uk_spin_lock_irqsave(&pending_lock, flags);
		UK_ASSERT(!uk_list_empty(&pending));
		async = uk_list_first_entry(&pending, struct uk_ofile_async,
					    pending);
		uk_list_del_init(&async->pending);
		generation = async->queued_generation;
		of = async->of;
		uk_spin_unlock_irqrestore(&pending_lock, flags);

		/* Ignore events from an earlier registration or owner. */
		uk_mutex_lock(&of->lock);
		if (of->async_generation == generation &&
		    (of->mode & O_ASYNC)) {
			owner = of->signal_owner;
			if (owner && uk_sigowner_acquire(owner))
				owner = 0;
		}
		uk_mutex_unlock(&of->lock);
		if (owner) {
			uk_sigowner_sigio(owner);
			uk_sigowner_release(owner);
		}
		uk_ofile_release(of);
	}
}

static int async_start_worker(void)
{
	struct uk_thread *thread;
	int ret = 0;

	uk_mutex_lock(&worker_lock);
	if (!worker_thread) {
		uk_semaphore_init(&pending_count, 0);
		thread = uk_sched_thread_create(uk_sched_current(),
						async_worker, NULL, "fd-async");
		if (!thread)
			ret = -ENOMEM;
		else
			worker_thread = thread;
	}
	uk_mutex_unlock(&worker_lock);
	return ret;
}

static int async_enable(struct uk_ofile *of)
{
	struct uk_ofile_async *async = of->async;
	uk_pollevent events = UKFD_POLLIN | UKFD_POLLOUT | UKFD_POLL_ALWAYS;
	int ret;

	if (!async) {
		async = uk_calloc(uk_alloc_get_default(), 1, sizeof(*async));
		if (!async)
			return -ENOMEM;
		ret = async_start_worker();
		if (ret) {
			uk_free(uk_alloc_get_default(), async);
			return ret;
		}
		async->of = of;
		UK_INIT_LIST_HEAD(&async->pending);
		async->ticket =
			UK_POLL_CHAIN_CALLBACK(events, async_ready, async);
		of->async = async;
	}
	uk_pollq_register(&of->file->state->pollq, &async->ticket);
	return 0;
}

int fdio_set_status_flags(struct uk_ofile *of, unsigned int flags,
			  unsigned int mask)
{
	unsigned int mode, newmode;
	bool enable;
	int ret = 0;

	uk_mutex_lock(&of->lock);
	mode = uk_load_n(&of->mode);
	if ((mode ^ flags) & mask & O_ASYNC) {
		enable = !!(flags & O_ASYNC);
		UK_WRITE_ONCE(of->async_generation, of->async_generation + 1);
		if (enable) {
			ret = async_enable(of);
			if (ret)
				goto out;
		} else {
			uk_pollq_unregister(&of->file->state->pollq,
					    &of->async->ticket);
		}
	}
	do {
		newmode = (mode & ~mask) | (flags & mask);
	} while (!uk_compare_exchange_n(&of->mode, &mode, newmode));
out:
	uk_mutex_unlock(&of->lock);
	return ret;
}

void uk_ofile_async_release(struct uk_ofile *of)
{
	struct uk_ofile_async *async = of->async;

	if (!async)
		return;
	if (of->mode & O_ASYNC)
		uk_pollq_unregister(&of->file->state->pollq, &async->ticket);
	/* Every queued ticket holds a reference to the description. */
	UK_ASSERT(uk_list_empty(&async->pending));
	uk_free(uk_alloc_get_default(), async);
}

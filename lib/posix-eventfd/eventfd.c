/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <sys/eventfd.h>

#include <uk/alloc.h>
#include <uk/errptr.h>
#include <uk/file/nops.h>
#include <uk/posix-eventfd.h>
#include <uk/posix-fd.h>

#if CONFIG_LIBPOSIX_FDTAB
#include <uk/posix-fdtab.h>
#include <uk/syscall.h>
#endif /* CONFIG_LIBPOSIX_FDTAB */


static const char EVENTFD_VOLID[] = "eventfd_vol";
static const char EVENTFD_SEM_VOLID[] = "eventfd_semaphore_vol";

#define _IS_EVFVOL(v) ((v) == EVENTFD_VOLID || (v) == EVENTFD_SEM_VOLID)

typedef volatile uint64_t *evfd_node;

struct evfd_alloc {
	struct uk_alloc *alloc;
	struct uk_file f;
	uk_file_refcnt frefcnt;
	struct uk_file_state fstate;
	uint64_t counter;
};


static ssize_t evfd_read(const struct uk_file *f,
			 const struct iovec *iov, int iovcnt,
			 off_t off, long flags __unused)
{
	int semaphore;
	evfd_node n;
	uint64_t val;
	uint64_t next;
	uint64_t ret;

	UK_ASSERT(_IS_EVFVOL(f->vol));
	if (unlikely(off))
		return -ESPIPE;
	if (unlikely(!iovcnt || iov[0].iov_len < sizeof(uint64_t)))
		return -EINVAL;

	semaphore = f->vol == EVENTFD_SEM_VOLID;
	n = (evfd_node)f->node;
	val = *n;
	do {
		if (!val)
			return -EAGAIN;
		if (semaphore) {
			next = val - 1;
			ret = 1;
		} else {
			next = 0;
			ret = val;
		}
	} while (!uk_compare_exchange_n(n, &val, next));

	if (!next)
		uk_file_event_clear(f, UKFD_POLLIN);
	uk_file_event_set(f, UKFD_POLLOUT);

	*(uint64_t *)iov[0].iov_base = ret;
	return sizeof(ret);
}


static ssize_t evfd_write(const struct uk_file *f,
			  const struct iovec *iov, int iovcnt,
			  off_t off, long flags __unused)
{
	uint64_t add;
	evfd_node n;
	uint64_t val;

	UK_ASSERT(_IS_EVFVOL(f->vol));
	if (unlikely(off))
		return -ESPIPE;
	if (unlikely(!iovcnt || iov[0].iov_len < sizeof(uint64_t)))
		return -EINVAL;

	add = *(uint64_t *)iov[0].iov_base;
	if (unlikely(add == UINT64_MAX))
		return -EINVAL;

	n = (evfd_node)f->node;
	val = *n;
	do {
		if (add > UINT64_MAX - 1 - val)
			return -EAGAIN;
	} while (!uk_compare_exchange_n(n, &val, val + add));

	if (val + add == UINT64_MAX - 1)
		uk_file_event_clear(f, UKFD_POLLOUT);
	uk_file_event_set(f, UKFD_POLLIN);

	return sizeof(val);
}

static const struct uk_file_ops evfd_ops = {
	.read = evfd_read,
	.write = evfd_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};

static void evfd_release(const struct uk_file *f, int what)
{
	UK_ASSERT(_IS_EVFVOL(f->vol));
	if (what & UK_FILE_RELEASE_OBJ) {
		/* Free */
		struct evfd_alloc *al = __containerof(f, struct evfd_alloc, f);

		uk_free(al->alloc, al);
	}
}


struct uk_file *uk_eventfile_create(unsigned int count, int flags)
{
	struct uk_alloc *a = uk_alloc_get_default();
	struct evfd_alloc *al = uk_malloc(a, sizeof(*al));
	uk_pollevent initev = UKFD_POLLOUT;
	const void *vol = (flags & EFD_SEMAPHORE) ?
		EVENTFD_SEM_VOLID : EVENTFD_VOLID;

	if (unlikely(!al))
		return ERR2PTR(-ENOMEM);

	al->alloc = a;
	al->counter = count;
	al->fstate = UK_FILE_STATE_INIT_VALUE(al->fstate);
	al->frefcnt = UK_FILE_REFCNT_INIT_VALUE(al->frefcnt);
	al->f = (struct uk_file){
		.vol = vol,
		.node = &al->counter,
		.refcnt = &al->frefcnt,
		.state = &al->fstate,
		.ops = &evfd_ops,
		._release = evfd_release
	};
	if (count)
		initev |= UKFD_POLLIN;
	uk_file_event_set(&al->f, initev);

	return &al->f;
}

#if CONFIG_LIBPOSIX_FDTAB
int uk_sys_eventfd(unsigned int count, int flags)
{
	int ret;
	unsigned int mode = O_RDWR|UKFD_O_NOSEEK|UKFD_O_NOIOLOCK;
	struct uk_file *evf = uk_eventfile_create(count, flags);

	if (unlikely(PTRISERR(evf)))
		return PTR2ERR(evf);

	/* Register fd */
	if (flags & EFD_NONBLOCK)
		mode |= O_NONBLOCK;
	if (flags & EFD_CLOEXEC)
		mode |= O_CLOEXEC;
	ret = uk_fdtab_open(evf, mode);
	uk_file_release(evf);
	return ret;
}


UK_LLSYSCALL_R_DEFINE(int, eventfd, unsigned int, count)
{
	return uk_sys_eventfd(count, 0);
}

UK_SYSCALL_R_DEFINE(int, eventfd2, unsigned int, count, int, flags)
{
	return uk_sys_eventfd(count, flags);
}
#endif /* CONFIG_LIBPOSIX_FDTAB */

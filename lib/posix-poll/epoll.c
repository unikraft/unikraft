/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/atomic.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/file/nops.h>
#include <uk/file/pollqueue.h>
#include <uk/posix-fd.h>
#include <uk/posix-fdtab.h>
#include <uk/posix-poll.h>
#include <uk/timeutil.h>
#include <uk/syscall.h>

#if CONFIG_LIBVFSCORE
#include <vfscore/file.h>
#include <vfscore/vnode.h>
#include <vfscore/eventpoll.h>
#endif /* CONFIG_LIBVFSCORE */

static const char EPOLL_VOLID[] = "epoll_vol";

#define EPOLL_EVENTS \
	(UKFD_POLLIN|UKFD_POLLOUT|EPOLLRDHUP|EPOLLPRI|UKFD_POLL_ALWAYS)
#define EPOLL_OPTS (EPOLLET|EPOLLONESHOT|EPOLLWAKEUP|EPOLLEXCLUSIVE)

#define events2mask(ev) (((ev) & EPOLL_EVENTS) | UKFD_POLL_ALWAYS)

#if CONFIG_LIBVFSCORE
struct epoll_legacy {
	struct eventpoll_cb ecb;
	const struct uk_file *epf;
	unsigned int mask;
	unsigned int revents;
	struct uk_list_head f_link;
};
#endif /* CONFIG_LIBVFSCORE */

struct epoll_entry {
	struct epoll_entry *next;
#if CONFIG_LIBVFSCORE
	int legacy;
#endif /* CONFIG_LIBVFSCORE */
	int fd;
	union {
		const struct uk_file *f;
#if CONFIG_LIBVFSCORE
		struct vfscore_file *vf;
#endif /* CONFIG_LIBVFSCORE */
	};
	struct epoll_event event;
	union {
		struct {
			struct uk_poll_chain tick;
			uk_pollevent revents;
		};
#if CONFIG_LIBVFSCORE
		struct epoll_legacy legacy_cb;
#endif /* CONFIG_LIBVFSCORE */
	};
};
#define IS_EDGEPOLL(ent) (!!((ent)->event.events & EPOLLET))
#define IS_ONESHOT(ent)  (!!((ent)->event.events & EPOLLONESHOT))


struct epoll_alloc {
	struct uk_alloc *alloc;
	struct uk_file f;
	uk_file_refcnt frefcnt;
	struct uk_file_state fstate;
	struct epoll_entry *list;
};


static void epoll_unregister_entry(struct epoll_entry *ent)
{
#if CONFIG_LIBVFSCORE
	if (ent->legacy) {
		if (ent->legacy_cb.ecb.unregister)
			ent->legacy_cb.ecb.unregister(&ent->legacy_cb.ecb);
		uk_list_del(&ent->legacy_cb.f_link);
	} else
#endif /* CONFIG_LIBVFSCORE */
	{
		uk_pollq_unregister(&ent->f->state->pollq, &ent->tick);
		uk_file_release_weak(ent->f);
	}
}


static void epoll_event_callback(uk_pollevent set,
				 enum uk_poll_chain_op op,
				 struct uk_poll_chain *tick)
{
	if (op == UK_POLL_CHAINOP_SET) {
		struct epoll_entry *ent = __containerof(
			tick, struct epoll_entry, tick);
		struct uk_pollq *upq = (struct uk_pollq *)tick->arg;

		(void)uk_or(&ent->revents, set);
		uk_pollq_set_n(upq, UKFD_POLLIN,
			       IS_EDGEPOLL(ent) ? 1 : UK_POLLQ_NOTIFY_ALL);
		if (IS_ONESHOT(ent))
			tick->mask = 0;
	}
}


#if CONFIG_LIBVFSCORE
/* vfscore shim helpers */

static int vfs_poll(struct vfscore_file *vfd, unsigned int *revents,
		    struct eventpoll_cb *ecb)
{
	struct vnode *vnode = vfd->f_dentry->d_vnode;

	UK_ASSERT(vnode->v_op->vop_poll);
	return VOP_POLL(vnode, revents, ecb);
}

static int vfs_poll_register(struct vfscore_file *vfd, struct epoll_legacy *leg)
{
	int ret = vfs_poll(vfd, &leg->revents, &leg->ecb);

	if (unlikely(ret))
		return -ret; /* vfscore uses positive errnos */

	uk_list_add_tail(&leg->f_link, &vfd->f_ep);
	(void)uk_and(&leg->revents, leg->mask);
	if (leg->revents)
		uk_file_event_set(leg->epf, UKFD_POLLIN);
	return 0;
}
#endif /* CONFIG_LIBVFSCORE */

/* File ops */

static void epoll_release(const struct uk_file *epf, int what)
{
	struct epoll_alloc *al = __containerof(epf, struct epoll_alloc, f);

	if (what & UK_FILE_RELEASE_RES) {
		struct epoll_entry **list = (struct epoll_entry **)epf->node;
		struct epoll_entry *p = *list;

		/* Free entries */
		while (p) {
			struct epoll_entry *ent = p;

			p = p->next;
			epoll_unregister_entry(ent);
			uk_free(al->alloc, ent);
		}
	}
	if (what & UK_FILE_RELEASE_OBJ) {
		/* Free alloc */
		uk_free(al->alloc, al);
	}
}

/* CTL ops */

#if CONFIG_LIBVFSCORE
static struct epoll_entry **epoll_find(const struct uk_file *epf, int fd,
				       int legacy, union uk_shim_file sf)
#else /* !CONFIG_LIBVFSCORE */
static struct epoll_entry **epoll_find(const struct uk_file *epf, int fd,
				       union uk_shim_file sf)
#endif
{
	struct epoll_entry **p = (struct epoll_entry **)epf->node;

	while (*p) {
		struct epoll_entry *ent = *p;

		if (ent->fd == fd)
#if CONFIG_LIBVFSCORE
			if ((legacy == ent->legacy) &&
			    ((legacy && ent->vf == sf.vfile) ||
			     (!legacy && ent->f == sf.ofile->file)))
#else /* !CONFIG_LIBVFSCORE */
			if (ent->f == sf.ofile->file)
#endif /* !CONFIG_LIBVFSCORE */
				break;
		p = &(*p)->next;
	}
	return p;
}

static void epoll_register(const struct uk_file *epf, struct epoll_entry *ent)
{
	const int edge = !!(ent->event.events & EPOLLET);
	uk_pollevent ev;

#if CONFIG_LIBVFSCORE
	UK_ASSERT(!ent->legacy);
#endif /* CONFIG_LIBVFSCORE */

	ev = uk_pollq_poll_register(&ent->f->state->pollq, &ent->tick, 1);
	if (ev) {
		/* Need atomic OR since we're registered for updates */
		(void)uk_or(&ent->revents, ev);
		uk_pollq_set_n(&epf->state->pollq, UKFD_POLLIN,
			       edge ? 1 : UK_POLLQ_NOTIFY_ALL);
	}
}

static int epoll_add(const struct uk_file *epf, struct epoll_entry **tail,
		     int fd, const struct uk_file *f,
		     const struct epoll_event *event)
{
	struct epoll_alloc *al = __containerof(epf, struct epoll_alloc, f);
	struct epoll_entry *ent;

	/* New entry */
	ent = uk_malloc(al->alloc, sizeof(*ent));
	if (unlikely(!ent))
		return -ENOMEM;

	uk_file_acquire_weak(f);
	*ent = (struct epoll_entry){
		.next = NULL,
#if CONFIG_LIBVFSCORE
		.legacy = 0,
#endif /* CONFIG_LIBVFSCORE */
		.fd = fd,
		.f = f,
		.event = *event,
		.tick = UK_POLL_CHAIN_CALLBACK_INITIALIZER(
			events2mask(event->events),
			epoll_event_callback,
			&epf->state->pollq
		),
		.revents = 0
	};
	*tail = ent;
	/* Poll, register & update if needed */
	epoll_register(epf, ent);
	return 0;
}

#if CONFIG_LIBVFSCORE
static int epoll_add_legacy(const struct uk_file *epf,
			    struct epoll_entry **tail,
			    int fd, struct vfscore_file *vf,
			    const struct epoll_event *event)
{
	struct epoll_alloc *al = __containerof(epf, struct epoll_alloc, f);
	struct epoll_entry *ent;
	int r;

	/* New entry */
	ent = uk_malloc(al->alloc, sizeof(*ent));
	if (unlikely(!ent))
		return -ENOMEM;

	*ent = (struct epoll_entry){
		.next = NULL,
		.legacy = 1,
		.fd = fd,
		.vf = vf,
		.event = *event,
		.legacy_cb = {
			.ecb = { .unregister = NULL, .data = NULL },
			.epf = epf,
			.mask = events2mask(event->events),
			.revents = 0
		}
	};
	UK_INIT_LIST_HEAD(&ent->legacy_cb.ecb.cb_link);
	UK_INIT_LIST_HEAD(&ent->legacy_cb.f_link);
	/* Poll, register & update if needed */
	r = vfs_poll_register(vf, &ent->legacy_cb);
	if (unlikely(r)) {
		uk_free(al->alloc, ent);
		if (r == -EINVAL)
			return -EPERM;
		else
			return r;
	}

	*tail = ent;
	return 0;
}
#endif /* CONFIG_LIBVFSCORE */

static void epoll_entry_mod(const struct uk_file *epf, struct epoll_entry *ent,
			    const struct epoll_event *event)
{
#if CONFIG_LIBVFSCORE
	UK_ASSERT(!ent->legacy);
#endif /* CONFIG_LIBVFSCORE */

	uk_pollq_unregister(&ent->f->state->pollq, &ent->tick);
	ent->event = *event;
	ent->tick.mask = events2mask(event->events);
	ent->revents = 0;
	epoll_register(epf, ent);

}

#if CONFIG_LIBVFSCORE
static void epoll_entry_mod_legacy(struct epoll_entry *ent,
				   const struct epoll_event *event)
{
	UK_ASSERT(ent->legacy);
	ent->legacy_cb.revents = 0;
	ent->legacy_cb.mask = events2mask(event->events);
	ent->event = *event;
	vfs_poll_register(ent->vf, &ent->legacy_cb);
}
#endif /* CONFIG_LIBVFSCORE */

static void epoll_entry_del(const struct uk_file *epf, struct epoll_entry **p)
{
	struct epoll_alloc *al = __containerof(epf, struct epoll_alloc, f);
	struct epoll_entry *ent = *p;

	*p = ent->next;
	epoll_unregister_entry(ent);
	uk_free(al->alloc, ent);
}

#if CONFIG_LIBVFSCORE
/* vfscore shim callbacks */

/* Called by vfscore drivers to signal events to epoll */
void eventpoll_signal(struct eventpoll_cb *ecb, unsigned int revents)
{
	struct epoll_legacy *leg = __containerof(ecb, struct epoll_legacy, ecb);

	revents &= leg->mask;
	if (revents) {
		(void)uk_or(&leg->revents, revents);
		uk_file_event_set(leg->epf, UKFD_POLLIN);
	}
}

/* Called by vfscore on monitored file close */
void eventpoll_notify_close(struct vfscore_file *fp)
{
	struct uk_list_head *itr;
	struct uk_list_head *tmp;

	uk_list_for_each_safe(itr, tmp, &fp->f_ep) {
		struct epoll_legacy *leg = uk_list_entry(
			itr, struct epoll_legacy, f_link);
		struct epoll_entry *ent = __containerof(
			leg, struct epoll_entry, legacy_cb);
		union uk_shim_file sf;
		struct epoll_entry **entp;

		UK_ASSERT(ent->legacy);
		sf.vfile = ent->vf;
		entp = epoll_find(leg->epf, ent->fd, 1, sf);
		UK_ASSERT(*entp == ent);
		epoll_entry_del(leg->epf, entp);
	}
}
#endif /* CONFIG_LIBVFSCORE */

/* File creation */

struct uk_file *uk_epollfile_create(void)
{
	/* Alloc */
	struct uk_alloc *a = uk_alloc_get_default();
	struct epoll_alloc *al = uk_malloc(a, sizeof(*al));

	if (!al)
		return NULL;
	/* Set fields */
	al->alloc = a;
	al->list = NULL;
	al->fstate = UK_FILE_STATE_INIT_VALUE(al->fstate);
	al->frefcnt = UK_FILE_REFCNT_INIT_VALUE(al->frefcnt);
	al->f = (struct uk_file){
		.vol = EPOLL_VOLID,
		.node = &al->list,
		.refcnt = &al->frefcnt,
		.state = &al->fstate,
		.ops = &uk_file_nops,
		._release = epoll_release
	};
	/* ret */
	return &al->f;
}

/* Internal Syscalls */

int uk_sys_epoll_create(int flags)
{
	struct uk_file *f;
	unsigned int mode;
	int ret;

	if (unlikely(flags & ~EPOLL_CLOEXEC))
		return -EINVAL;

	f = uk_epollfile_create();
	if (unlikely(!f))
		return -ENOMEM;

	mode = O_RDONLY|UKFD_O_NOSEEK;
	if (flags & EPOLL_CLOEXEC)
		mode |= O_CLOEXEC;

	ret = uk_fdtab_open(f, mode);
	uk_file_release(f);
	return ret;
}

int uk_sys_epoll_ctl(const struct uk_file *epf, int op, int fd,
		     const struct epoll_event *event)
{
	int ret = 0;
	union uk_shim_file sf;
	struct epoll_entry **entp;
#if CONFIG_LIBVFSCORE
	int legacy;
#endif /* CONFIG_LIBVFSCORE */

	if (unlikely(epf->vol != EPOLL_VOLID))
		return -EINVAL;

#if CONFIG_LIBVFSCORE
	legacy = uk_fdtab_shim_get(fd, &sf);
	if (unlikely(legacy < 0))
		return -EBADF;
	legacy = legacy == UK_SHIM_LEGACY;
#else /* !CONFIG_LIBVFSCORE */
	ret = uk_fdtab_shim_get(fd, &sf);
	if (unlikely(ret < 0))
		return -EBADF;
	UK_ASSERT(ret == UK_SHIM_OFILE);
#endif /* !CONFIG_LIBVFSCORE */

	uk_file_wlock(epf);

#if CONFIG_LIBVFSCORE
	entp = epoll_find(epf, fd, legacy, sf);
#else /* !CONFIG_LIBVFSCORE */
	entp = epoll_find(epf, fd, sf);
#endif /* !CONFIG_LIBVFSCORE */

	switch (op) {
	case EPOLL_CTL_ADD:
		if (unlikely(!event))
			ret = -EFAULT;
		else if (unlikely(*entp))
			ret = -EEXIST;
		else
#if CONFIG_LIBVFSCORE
			if (legacy)
				ret = epoll_add_legacy(epf, entp,
						       fd, sf.vfile, event);
			else
#endif /* CONFIG_LIBVFSCORE */
				ret = epoll_add(epf, entp,
						fd, sf.ofile->file, event);
		break;

	case EPOLL_CTL_MOD:
		if (unlikely(!event))
			ret = -EFAULT;
		else if (unlikely(!*entp))
			ret = -ENOENT;
		else
#if CONFIG_LIBVFSCORE
			if (legacy)
				epoll_entry_mod_legacy(*entp, event);
			else
#endif /* CONFIG_LIBVFSCORE */
				epoll_entry_mod(epf, *entp, event);
		break;

	case EPOLL_CTL_DEL:
		if (unlikely(!*entp))
			ret = -ENOENT;
		else
			epoll_entry_del(epf, entp);
		break;

	default:
		ret = -EINVAL;
	}
	uk_file_wunlock(epf);

#if CONFIG_LIBVFSCORE
	if (legacy)
		fdrop(sf.vfile);
	else
#endif /* CONFIG_LIBVFSCORE */
		uk_fdtab_ret(sf.ofile);

	return ret;
}

int uk_sys_epoll_pwait2(const struct uk_file *epf, struct epoll_event *events,
			int maxevents, const struct timespec *timeout,
			const sigset_t *sigmask, size_t sigsetsize __unused)
{
	struct epoll_entry **list;
	__nsec deadline;

	if (unlikely(epf->vol != EPOLL_VOLID))
		return -EINVAL;
	if (unlikely(!events))
		return -EFAULT;
	if (unlikely(maxevents <= 0))
		return -EINVAL;
	if (unlikely(sigmask)) {
		uk_pr_warn_once("STUB: epoll_pwait no sigmask support\n");
		return -ENOSYS;
	}

	list = (struct epoll_entry **)epf->node;

	if (timeout) {
		__snsec tout = uk_time_spec_to_nsec(timeout);

		if (tout < 0)
			return -EINVAL;
		deadline = ukplat_monotonic_clock() + tout;
	} else {
		deadline = 0;
	}

	while (uk_file_poll_until(epf, UKFD_POLLIN, deadline)) {
		int lvlev = 0;
		int nout = 0;

		uk_file_event_clear(epf, UKFD_POLLIN);
		uk_file_rlock(epf);

		/* gather & output event list */
		for (struct epoll_entry *p = *list;
		     p && nout < maxevents;
		     p = p->next) {
			unsigned int revents;
			unsigned int *revp;

#if CONFIG_LIBVFSCORE
			if (p->legacy)
				revp = &p->legacy_cb.revents;
			else
#endif /* CONFIG_LIBVFSCORE */
				revp = &p->revents;

			revents = uk_exchange_n(revp, 0);
			if (revents) {
				if (!IS_EDGEPOLL(p)) {
					unsigned int mask;

					mask = events2mask(p->event.events);
#if CONFIG_LIBVFSCORE
					if (p->legacy) {
						vfs_poll(p->vf, &revents,
							 &p->legacy_cb.ecb);
						revents &= mask;
					} else
#endif /* CONFIG_LIBVFSCORE */
					{
						revents = uk_file_poll_immediate(p->f, mask);
					}
					if (!revents)
						continue;

					lvlev = 1;
					(void)uk_or(revp, revents);
				}

				events[nout].events = revents;
				events[nout].data = p->event.data;
				nout++;
			}
		}
		uk_file_runlock(epf);

		/* If lvlev or limited by maxevents, update pollin back in */
		if (lvlev || nout == maxevents)
			uk_file_event_set(epf, UKFD_POLLIN);

		if (nout)
			return nout;
		/* If nout == 0 loop back around */
	}
	/* Timeout */
	return 0;
}

/* Userspace Syscalls */

UK_SYSCALL_R_DEFINE(int, epoll_create, int, size)
{
	if (unlikely(size <= 0))
		return -EINVAL;
	return uk_sys_epoll_create(0);
}

UK_SYSCALL_R_DEFINE(int, epoll_create1, int, flags)
{
	return uk_sys_epoll_create(flags);
}

UK_SYSCALL_R_DEFINE(int, epoll_ctl, int, epfd, int, op, int, fd,
		    struct epoll_event *, event)
{
	int r;
	struct uk_ofile *of = uk_fdtab_get(epfd);

	if (unlikely(!of))
		return -EBADF;
	r = uk_sys_epoll_ctl(of->file, op, fd, event);
	uk_fdtab_ret(of);
	return r;
}

UK_SYSCALL_R_DEFINE(int, epoll_pwait2, int, epfd, struct epoll_event *, events,
		    int, maxevents, struct timespec *, timeout,
		    const sigset_t *, sigmask, size_t, sigsetsize)
{
	int r;
	struct uk_ofile *of = uk_fdtab_get(epfd);

	if (unlikely(!of))
		return -EBADF;
	r = uk_sys_epoll_pwait2(of->file, events, maxevents,
				timeout, sigmask, sigsetsize);
	uk_fdtab_ret(of);
	return r;
}

#ifdef epoll_pwait
#undef epoll_pwait
#endif

UK_LLSYSCALL_R_DEFINE(int, epoll_pwait, int, epfd, struct epoll_event *, events,
		      int, maxevents, int, timeout,
		      const sigset_t *, sigmask, size_t, sigsetsize)
{
	int r;
	struct uk_ofile *of = uk_fdtab_get(epfd);

	if (unlikely(!of))
		return -EBADF;
	r = uk_sys_epoll_pwait(of->file, events, maxevents,
			       timeout, sigmask, sigsetsize);
	uk_fdtab_ret(of);
	return r;
}

UK_SYSCALL_R_DEFINE(int, epoll_wait, int, epfd, struct epoll_event *, events,
		    int, maxevents, int, timeout)
{
	int r;
	struct uk_ofile *of = uk_fdtab_get(epfd);

	if (unlikely(!of))
		return -EBADF;
	r = uk_sys_epoll_pwait(of->file, events, maxevents,
			       timeout, NULL, 0);
	uk_fdtab_ret(of);
	return r;
}

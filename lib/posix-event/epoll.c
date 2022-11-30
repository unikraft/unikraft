/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/fdtab/fd.h>
#include <uk/fdtab/eventpoll.h>
#include <uk/alloc.h>
#include <uk/syscall.h>
#include <uk/config.h>

#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>

struct epoll_file {
	struct fdtab_file f_file;
	struct eventpoll f_ep;
};

#define to_efd(fp) \
	__containerof((fp), struct epoll_file, f_file)

static int epoll_fdtab_free(struct fdtab_file *fp)
{
	struct epoll_file *efd = to_efd(fp);
	struct eventpoll *ep = &efd->f_ep;

	/* Clean up interest lists and release fds */
	eventpoll_fini(ep);

	uk_free(uk_alloc_get_default(), efd);

	return 0;
}

static int epoll_fdtab_poll()
{
	return -EINVAL;
}

/* vnode operations */
static struct fdops epoll_fdops = {
	.fdop_poll = epoll_fdtab_poll,
	.fdop_free = epoll_fdtab_free,
};

static inline int is_epoll_file(struct fdtab_file *fp)
{
	return (fp->f_op == &epoll_fdops);
}

static inline int has_poll(struct fdtab_file *fp)
{
	return (fp->f_op->fdop_poll != NULL);
}

static int do_epoll_create(struct uk_alloc *a, int flags)
{
	int vfs_fd, ret;
	struct epoll_file *efd;
	struct fdtab_table *tab;
	struct fdtab_file *file;

	if (unlikely(flags & ~EPOLL_CLOEXEC))
		return -EINVAL;

	tab = fdtab_get_active();

	/* Reserve a file descriptor number */
	vfs_fd = fdtab_alloc_fd(tab);
	if (unlikely(vfs_fd < 0)) {
		ret = -ENFILE;
		goto ERR_EXIT;
	}

	/* Allocate file, vfs_file, and vnode */
	efd = uk_malloc(a, sizeof(struct epoll_file));
	if (unlikely(!efd)) {
		ret = -ENOMEM;
		goto ERR_MALLOC_FILE;
	}
	file = &efd->f_file;

	/* Initialize data structures */
	fdtab_file_init(file);
	file->fd = vfs_fd;
	file->f_op = &epoll_fdops;

	eventpoll_init(&efd->f_ep, a);

	/* Store within the vfs structure */
	ret = fdtab_install_fd(tab, vfs_fd, file);
	if (unlikely(ret))
		goto ERR_ALLOC_EFD;

	return vfs_fd;

ERR_ALLOC_EFD:
	uk_free(a, efd);
ERR_MALLOC_FILE:
	fdtab_put_fd(tab, vfs_fd);
ERR_EXIT:
	UK_ASSERT(ret < 0);
	return ret;
}

UK_SYSCALL_R_DEFINE(int, epoll_create1, int, flags)
{
	return do_epoll_create(uk_alloc_get_default(), flags);
}

UK_SYSCALL_R_DEFINE(int, epoll_create, int, size)
{
	/*
	 * size was used as hint for how much memory needs to be allocated. Not
	 * used nowadays but must still be greater than zero for compatibility
	 * reasons.
	 */
	if (unlikely(size <= 0))
		return -EINVAL;

	return do_epoll_create(uk_alloc_get_default(), 0);
}

static int do_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	struct fdtab_table *tab;
	struct fdtab_file *epf, *fp;
	struct eventpoll *ep;
	int ret;

	tab = fdtab_get_active();

	epf = fdtab_get_file(tab, epfd);
	if (unlikely(!epf)) {
		ret = -EBADF;
		goto EXIT;
	}

	fp = fdtab_get_file(tab, fd);
	if (unlikely(!fp)) {
		ret = -EBADF;
		goto ERR_PUT_EPF;
	}

	if (unlikely(!has_poll(fp))) {
		ret = -EPERM;
		goto ERR_PUT_F;
	}

	if (unlikely((epfd == fd) || (!is_epoll_file(epf)))) {
		ret = -EINVAL;
		goto ERR_PUT_F;
	}

	/* TODO: The current epoll implementation does not support adding
	 * an eventpoll fd to another eventpoll.
	 */
	if (unlikely(is_epoll_file(fp))) {
		uk_pr_err_once("No nested epoll support.\n");
		return -EBADF;
	}

	ep = &to_efd(epf)->f_ep;

	switch (op) {
	case EPOLL_CTL_ADD:
		ret = eventpoll_add(ep, fd, fp, event);
		break;
	case EPOLL_CTL_MOD:
		ret = eventpoll_mod(ep, fd, event);
		break;
	case EPOLL_CTL_DEL:
		ret = eventpoll_del(ep, fd);
		break;
	default:
		ret = -EINVAL;
		break;
	}

ERR_PUT_F:
	fdtab_put_file(fp);
ERR_PUT_EPF:
	fdtab_put_file(epf);
EXIT:
	return ret;
}

UK_SYSCALL_R_DEFINE(int, epoll_ctl, int, epfd, int, op, int, fd,
		    struct epoll_event *, event)
{
	return do_epoll_ctl(epfd, op, fd, event);
}

static int do_epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
			  const __nsec *timeout, const sigset_t *sigmask,
			  size_t sigsegsize __unused)
{
	struct fdtab_table *tab;
	struct fdtab_file *epf;
	struct eventpoll *ep;
	int ret;

	tab = fdtab_get_active();

	epf = fdtab_get_file(tab, epfd);
	if (unlikely(!epf)) {
		ret = -EBADF;
		goto EXIT;
	}

	if (unlikely((maxevents <= 0) || !is_epoll_file(epf))) {
		ret = -EINVAL;
		goto ERR_PUT_EPF;
	}

	if (unlikely(!events)) {
		ret = -EFAULT;
		goto ERR_PUT_EPF;
	}

	ep = &to_efd(epf)->f_ep;

	/* TODO: Implement atomic masking of signals */
	if (sigmask)
		uk_pr_warn_once("%s: signal masking not implemented.",
				__func__);

	ret = eventpoll_wait(ep, events, maxevents, timeout);

ERR_PUT_EPF:
	fdtab_put_file(epf);
EXIT:
	return ret;
}

UK_SYSCALL_R_DEFINE(int, epoll_wait, int, epfd, struct epoll_event *, events,
		    int, maxevents, int, timeout)
{
	__nsec timeout_ns;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	return do_epoll_pwait(epfd, events, maxevents,
			      (timeout >= 0) ? &timeout_ns : NULL,
			      NULL, 0);
}

UK_LLSYSCALL_R_DEFINE(int, epoll_pwait, int, epfd, struct epoll_event *, events,
		      int, maxevents, int, timeout, const sigset_t *, sigmask,
		      size_t, sigsetsize)
{
	__nsec timeout_ns;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	return do_epoll_pwait(epfd, events, maxevents,
			      (timeout >= 0) ? &timeout_ns : NULL,
			      sigmask, sigsetsize);
}

#if UK_LIBC_SYSCALLS
/* The actual system call implemented in Linux uses a different signature! We
 * thus provide the libc call here directly.
 */
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
		int timeout, const sigset_t *sigmask)
{
	__nsec timeout_ns;
	int ret;

	if (timeout >= 0)
		timeout_ns = ukarch_time_msec_to_nsec(timeout);

	ret = do_epoll_pwait(epfd, events, maxevents,
			     (timeout >= 0) ? &timeout_ns : NULL,
			     sigmask, sizeof(sigset_t));
	if (unlikely(ret < 0)) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
#endif /* UK_LIBC_SYSCALLS */

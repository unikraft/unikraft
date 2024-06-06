/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <fcntl.h>

#include <uk/atomic.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/file/nops.h>
#include <uk/posix-fd.h>
#include <uk/posix-pipe.h>
#include <uk/syscall.h>


#define PIPE_SIZE (1L << CONFIG_LIBPOSIX_PIPE_SIZE_ORDER)

#if CONFIG_LIBPOSIX_PIPE_PACKET
#define PIPE_MSGCOUNT CONFIG_LIBPOSIX_PIPE_MAX_PACKETS
#else /* !CONFIG_LIBPOSIX_PIPE_PACKET */
#define PIPE_MSGCOUNT 1
#endif

#define _PIPE_SZMASK (PIPE_SIZE - 1)
#define PIPE_IDX(x) ((x) & _PIPE_SZMASK)

#define PIPE_SPACE(start, lim) \
	(((start) <= (lim)) ? ((lim) - (start)) : (PIPE_SIZE - (start) + (lim)))
#define PIPE_USED(start, lim) \
	(((start) == (lim)) ? PIPE_SIZE : PIPE_SPACE(start, lim))

static const char PIPE_VOLID[] = "pipe_vol";

typedef __u32 pipeidx;

struct pipe_msg {
	struct pipe_msg *next;
	pipeidx start;
	pipeidx end;
};

struct pipe_node {
	struct pipe_msg *volatile head;
	struct pipe_msg *tail;
	struct pipe_msg *free;
	unsigned int flags;
	struct pipe_msg msgs[PIPE_MSGCOUNT];
	char buf[PIPE_SIZE];
};

#define PIPE_HUP    1
#define PIPE_FIN    2

struct pipe_alloc {
	struct uk_alloc *alloc;
	struct uk_file rf;
	struct uk_file wf;
	uk_file_refcnt rref;
	uk_file_refcnt wref;
	struct uk_file_state fstate;
	struct pipe_node node;
};


static void _pipebuf_read(const char *buf, pipeidx head, char *out, size_t n)
{
	if (head + n > PIPE_SIZE) {
		/* pipebuf not contiguous, need 2 copies */
		size_t l = PIPE_SIZE - head;

		memcpy(out, &buf[head], l);
		memcpy(&out[l], buf, n - l);
	} else {
		memcpy(out, &buf[head], n);
	}
}

static void _pipebuf_write(char *buf, pipeidx head, const char *in, size_t n)
{
	if (head + n > PIPE_SIZE) {
		/* pipebuf not contiguous, need 2 copies */
		size_t l = PIPE_SIZE - head;

		memcpy(&buf[head], in, l);
		memcpy(buf, &in[l], n - l);
	} else {
		memcpy(&buf[head], in, n);
	}
}

static void pipebuf_iovread(const char *buf, pipeidx head,
			    const struct iovec *iov, size_t n)
{
	int i;

	for (i = 0; iov[i].iov_len <= n; i++) {
		size_t len = iov[i].iov_len;

		_pipebuf_read(buf, head, (char *)iov[i].iov_base, len);
		n -= len;
		head = PIPE_IDX(head + len);
	}
	if (n)
		_pipebuf_read(buf, head, (char *)iov[i].iov_base, n);
}

static void pipebuf_iovwrite(char *buf, pipeidx head,
			     const struct iovec *iov, size_t n)
{
	int i;

	for (i = 0; iov[i].iov_len <= n; i++) {
		size_t len = iov[i].iov_len;

		_pipebuf_write(buf, head, (const char *)iov[i].iov_base, len);
		n -= len;
		head = PIPE_IDX(head + len);
	}
	if (n)
		_pipebuf_write(buf, head, (const char *)iov[i].iov_base, n);
}

static ssize_t _iovsz(const struct iovec *iov, int iovcnt)
{
	size_t ret = 0;

	for (int i = 0; i < iovcnt; i++)
		if (iov[i].iov_len) {
			if (likely(iov[i].iov_base))
				ret += iov[i].iov_len;
			else
				return -EFAULT;
		}
	return ret;
}

static ssize_t pipe_read(const struct uk_file *f,
			 const struct iovec *iov, int iovcnt,
			 off_t off, long flags __unused)
{
	ssize_t toread;
	struct pipe_node *d;
	struct pipe_msg *m;
	struct pipe_msg *prev;
	ssize_t canread;
	pipeidx start;

	UK_ASSERT(f->vol == PIPE_VOLID);
	if (unlikely(off))
		return -ESPIPE;

	toread = _iovsz(iov, iovcnt);
	if (unlikely(toread <= 0))
		return toread;

	d = (struct pipe_node *)f->node;
	m = d->head;
	for (;;) {
		if (!m) {
			uk_file_event_clear(f, UKFD_POLLIN);
			if (d->flags & PIPE_HUP)
				return 0;
			else
				return -EAGAIN;
		}

		if (m->next != m) { /* Packet msg */
			if (!uk_compare_exchange_n(&d->head, &m, m->next))
				continue;
			if (!m->next) {
				d->tail = NULL;
				uk_file_event_clear(f, UKFD_POLLIN);
			}

			start = m->start;
			canread = MIN(PIPE_USED(start, m->end), toread);
			UK_ASSERT(canread); /* 0-length packets not allowed */
			prev = uk_exchange_n(&d->free, m);
			m->next = prev;
		} else { /* Stream msg; always last in queue */
			ssize_t avail;

			start = m->start;
			if (start == PIPE_SIZE) {
				m = d->head;
				continue;
			}
			avail = PIPE_USED(start, m->end);
			canread = MIN(avail, toread);
			UK_ASSERT(canread);

			if (canread == avail) {
				if (!uk_compare_exchange_n(&d->head, &m, NULL))
					continue;
				while (!uk_compare_exchange_n(&m->start,
							      &start,
							      PIPE_SIZE));
				uk_file_event_clear(f, UKFD_POLLIN);

				canread = MIN(PIPE_USED(start, m->end), toread);

				d->tail = NULL;
				prev = uk_exchange_n(&d->free, m);
				m->next = prev;
			} else {
				pipeidx lim = PIPE_IDX(start + canread);

				UK_ASSERT(lim != start);
				if (!uk_compare_exchange_n(&m->start,
							   &start, lim)) {
					m = d->head;
					continue;
				}
			}
		}
		break;
	}
	UK_ASSERT(canread);
	pipebuf_iovread(d->buf, start, iov, canread);
	uk_file_event_set(f, UKFD_POLLOUT);

	return canread;
}

static ssize_t pipe_write(const struct uk_file *f,
			  const struct iovec *iov, int iovcnt,
			  off_t off, long flags)
{
	struct pipe_node *d;
	struct pipe_msg *tail;
	ssize_t towrite;
	ssize_t canwrite;
	ssize_t capacity;
	pipeidx whead;
	pipeidx wend;
	int empty = 0;

	UK_ASSERT(f->vol == PIPE_VOLID);
	if (unlikely(off))
		return -ESPIPE;

	d = (struct pipe_node *)f->node;
	if (unlikely(d->flags & PIPE_HUP))
		return -EPIPE;

	towrite = _iovsz(iov, iovcnt);
	if (unlikely(towrite <= 0))
		return towrite;

	tail = d->tail;
	if (!tail || tail->next != tail) { /* Empty or last msg is packet */
		struct pipe_msg *m;

		if (tail) {
			UK_ASSERT(d->head);
			UK_ASSERT(!tail->next);
			whead = tail->end;
			wend = d->head->start;
		} else {
			UK_ASSERT(!d->head);
			whead = 0;
			wend = PIPE_SIZE;
			empty = 1;
		}
		capacity = PIPE_SPACE(whead, wend);
		canwrite = MIN(capacity, towrite);
		if (!canwrite)
			goto out_full;
		wend = PIPE_IDX(whead + canwrite);

		m = d->free;
		if (!m)
			goto out_full;
		d->free = m->next;

		m->start = whead;
		m->end = wend;
		m->next = (flags & O_DIRECT) ? NULL : m;

		d->tail = m;
		if (tail)
			tail->next = m;
		else
			d->head = m;
	} else { /* Last msg is stream, extend */
		UK_ASSERT(d->head);
		whead = tail->end;
		wend = d->head->start;
		capacity = PIPE_SPACE(whead, wend);
		canwrite = MIN(towrite, capacity);
		if (!canwrite)
			goto out_full;
		tail->end = PIPE_IDX(whead + canwrite);
	}
	UK_ASSERT(canwrite);
	pipebuf_iovwrite(d->buf, whead, iov, canwrite);
	if (canwrite == capacity)
		uk_file_event_clear(f, UKFD_POLLOUT);
	if (empty)
		uk_file_event_set(f, UKFD_POLLIN);
	return canwrite;

out_full:
	uk_file_event_clear(f, UKFD_POLLOUT);
	return -EAGAIN;
}

static const struct uk_file_ops rpipe_ops = {
	.read = pipe_read,
	.write = uk_file_nop_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};

static const struct uk_file_ops wpipe_ops = {
	.read = uk_file_nop_read,
	.write = pipe_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = uk_file_nop_ctl
};


static void pipe_release(const struct uk_file *f, int what)
{
	struct pipe_node *d;

	UK_ASSERT(f->vol == PIPE_VOLID);
	d = (struct pipe_node *)f->node;
	if (what & UK_FILE_RELEASE_RES) {
		/* Atomically set the HUP flag */
		uk_or(&d->flags, PIPE_HUP);
		/* If was already set, we can free node */
		/* Update w/ EPOLL(HUP|IN) for read & EPOLLERR for write */
		uk_file_event_set(f, EPOLLHUP|EPOLLIN|EPOLLERR);
	}
	if (what & UK_FILE_RELEASE_OBJ) {
		/* Atomically set the FIN flag */
		if (uk_or(&d->flags, PIPE_FIN) & PIPE_FIN) {
			/* If was already set, we free everything */
			struct pipe_alloc *al = __containerof(f->state,
							      struct pipe_alloc,
							      fstate);

			uk_free(al->alloc, al);
		}
	}
}


/* File creation */

int uk_pipefile_create(struct uk_file *pipes[2])
{
	struct uk_alloc *a;
	struct pipe_alloc *al;

	a = uk_alloc_get_default();
	al = uk_malloc(a, sizeof(*al));
	if (unlikely(!al))
		return -ENOMEM;

	al->alloc = a;

	al->node.head = NULL;
	al->node.tail = NULL;
	al->node.flags = 0;
	al->node.free = &al->node.msgs[0];
	for (int i = 0; i < PIPE_MSGCOUNT - 1; i++)
		al->node.msgs[i].next = &al->node.msgs[i + 1];
	al->node.msgs[PIPE_MSGCOUNT - 1].next = NULL;

	al->fstate = UK_FILE_STATE_INIT_VALUE(al->fstate);
	al->rref = UK_FILE_REFCNT_INIT_VALUE(al->rref);
	al->wref = UK_FILE_REFCNT_INIT_VALUE(al->wref);
	al->rf = (struct uk_file){
		.vol = PIPE_VOLID,
		.node = &al->node,
		.refcnt = &al->rref,
		.state = &al->fstate,
		.ops = &rpipe_ops,
		._release = pipe_release
	};
	al->wf = (struct uk_file){
		.vol = PIPE_VOLID,
		.node = &al->node,
		.refcnt = &al->wref,
		.state = &al->fstate,
		.ops = &wpipe_ops,
		._release = pipe_release
	};
	uk_file_event_set(&al->wf, UKFD_POLLOUT);

	pipes[0] = &al->rf;
	pipes[1] = &al->wf;
	return 0;
}

/* Internal syscalls */

#define _OPEN_FLAGS (O_CLOEXEC|O_NONBLOCK|O_DIRECT)

int uk_sys_pipe(int pipefd[2], int flags)
{
	int r;
	struct uk_file *pipes[2];
	int oflags;
	int rpipe;

#if !CONFIG_LIBPOSIX_PIPE_PACKET
	if (unlikely(flags & O_DIRECT)) {
		uk_pr_warn_once("packet pipes requested but not enabled\n");
		return -EINVAL;
	}
#endif /* CONFIG_LIBPOSIX_PIPE_PACKET */

	r = uk_pipefile_create(pipes);
	if (unlikely(r))
		return r;

	oflags = (flags & _OPEN_FLAGS) | UKFD_O_NOSEEK;
	r = uk_fdtab_open(pipes[0], O_RDONLY|oflags);
	if (unlikely(r < 0))
		goto err_free;

	rpipe = r;

	r = uk_fdtab_open(pipes[1], O_WRONLY|oflags);
	if (unlikely(r < 0))
		goto err_close;

	pipefd[0] = rpipe;
	pipefd[1] = r;
	return  0;

err_close:
	uk_sys_close(rpipe);
err_free:
	uk_file_release(pipes[0]);
	uk_file_release(pipes[1]);
	return r;
}

/* Syscalls */

UK_SYSCALL_R_DEFINE(int, pipe, int *, pipefd)
{
	return uk_sys_pipe(pipefd, 0);
}

UK_SYSCALL_R_DEFINE(int, pipe2, int *, pipefd, int, flags)
{
	return uk_sys_pipe(pipefd, flags);
}

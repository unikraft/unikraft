/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>

#include <uk/assert.h>
#include <uk/init.h>
#include <uk/list.h>
#include <uk/semaphore.h>
#include <uk/isr/semaphore.h>
#include <uk/mutex.h>
#include <uk/sched.h>

#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/posix-fd.h>

#if CONFIG_LIBUKFS_DEVFS
#include <uk/devfs.h>
#include <uk/fs.h>
#include <uk/fs/prio.h>

#include <stdio.h>
#endif /* CONFIG_LIBUKFS_DEVFS */

/*
 * Semaphore upped from ISR context by async event callbacks; drained by the
 * evmon thread before iterating the async file list to deliver poll events.
 */
static struct uk_semaphore consf_async_sem;

/* List of consolefd_alloc instances backed by async console devices. */
static struct uk_mutex
consf_async_list_mtx = UK_MUTEX_INITIALIZER(consf_async_list_mtx);
static UK_LIST_HEAD(consf_async_list);

static const char SERIAL_VOLID[] = "serial_vol";

/* EOT: End Of Transmission character; ^D */
#define SERIAL_EOT 04

#define SERIAL_TERMIOS_IFLAGS (IUTF8|ICRNL)
#define SERIAL_TERMIOS_OFLAGS (0)
#define SERIAL_TERMIOS_CFLAGS (CREAD|CS8|B38400)
#if CONFIG_LIBUKFILE_CONSOLE_IN_NOECHOBACK
#define SERIAL_TERMIOS_LFLAGS (ICANON)
#else /* !CONFIG_LIBUKFILE_CONSOLE_IN_NOECHOBACK */
#define SERIAL_TERMIOS_LFLAGS (ICANON|ECHO)
#endif /* !CONFIG_LIBUKFILE_CONSOLE_IN_NOECHOBACK */

/* Linux fills in 19 control chars; copy exactly that many to and from the
 * caller's buffer, which the program sizes to its own libc's struct termios.
 */
#define KNCCS 19

/* Values taken from termios(3) from Linux man-pages 6.05 */
static const cc_t SERIAL_TERMIOS_CONTROL_CHARS[KNCCS] = {
	[VDISCARD] = 017,
	[VEOF] = SERIAL_EOT,
	[VEOL] = 0,
	[VEOL2] = 0,
	[VERASE] = 0177,
	[VINTR] = 003,
	[VKILL] = 025,
	[VLNEXT] = 026,
	[VMIN] = 1,
	[VQUIT] = 034,
	[VREPRINT] = 022,
	[VSTART] = 021,
	[VSTOP] = 023,
	[VSUSP] = 032,
	[VSWTC] = 0,
	[VTIME] = 0,
	[VWERASE] = 027,
};

struct consolefd_alloc {
	/* Allocator used to create this console file */
	struct uk_alloc *a;
	/* Console file refcount */
	uk_file_refcnt frefcnt;
	/* Console file state */
	struct uk_file_state fstate;
	/* Base file structure */
	struct uk_file f;
	/*
	 * Pending UKFD_POLL* bits to be delivered by the evmon thread.
	 * Written via atomic OR from ISR callbacks; consumed via atomic
	 * exchange by the evmon thread.
	 */
	__u32 pending_events;
	/* Entry in consf_async_list; only linked for async-backed files. */
	struct uk_list_head async_list;
	/* An async read returned the data before an EOT: the next returns 0 */
	int eot;
	/*
	 * Line settings, mutable: a program switches the terminal to raw
	 * mode (ICANON/ECHO off) to take over echo and line editing, then
	 * reads them back with TCGETS.  Echoing input regardless of ECHO
	 * shows every character twice for a program that does its own echo,
	 * an interactive shell's line editor, say.
	 */
	struct termios termios;
#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
	/* The console reached the end of its input, for good */
	int eof;
#endif /* CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
	/* Mutex to synchronize file read/write event management */
	struct uk_mutex mtx;
	/* Writes go to the console's standard error stream */
	int err;
	/*
	 * The file whose line settings and end of input this one shares:
	 * itself, or the console file a standard error file was made from,
	 * which it holds a reference to.
	 */
	struct consolefd_alloc *tty;
};

static inline struct consolefd_alloc *to_consf(const struct uk_file *f)
{
	return __containerof(f, struct consolefd_alloc, f);
}

/* The line settings of the terminal @f is on */
static inline struct termios *to_termios(const struct uk_file *f)
{
	return &to_consf(f)->tty->termios;
}

#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
/* Whether the console behind @f reached the end of its input */
static inline int *to_eof(const struct uk_file *f)
{
	return &to_consf(f)->tty->eof;
}
#endif /* CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */

/*
 * Writes to a console device, each newline as CR-LF where
 * CONFIG_LIBUKCONSOLE_ONLCR asks for it, unless this library is set to
 * write them as they are.  Either way a write stops at the first partial
 * one and returns how much of the buffer went out (see
 * uk_console_out_direct_onlcr()), so an async TX path resumes where the
 * queue filled.
 */
#if CONFIG_LIBUKFILE_CONSOLE_OUT_NOCARRIAGERETURN
#define consf_out_direct uk_console_out_direct
#define consf_err_direct uk_console_err_direct
#else /* !CONFIG_LIBUKFILE_CONSOLE_OUT_NOCARRIAGERETURN */
#define consf_out_direct uk_console_out_direct_onlcr
#define consf_err_direct uk_console_err_direct_onlcr
#endif /* !CONFIG_LIBUKFILE_CONSOLE_OUT_NOCARRIAGERETURN */

/* How a write to @f reaches its console */
static inline uk_console_out_func consf_writer(const struct uk_file *f)
{
	return to_consf(f)->err ? consf_err_direct : consf_out_direct;
}

/* Whether input read from @f is echoed to the console */
static inline int consf_echo(const struct uk_file *f)
{
	return to_termios(f)->c_lflag & ECHO;
}

static ssize_t serial_read(const struct uk_file *f,
			   const struct iovec *iov, size_t iovcnt,
			   size_t off, long flags __unused)
{
	struct uk_console *cons = f->node;
	ssize_t total = 0;

	UK_ASSERT(f->vol == SERIAL_VOLID);
	UK_ASSERT(f->node);

	if (unlikely(off))
		return -ESPIPE;

#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
	if (*to_eof(f))
		return 0;
#endif /* CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */

	if (!uk_file_poll_immediate(f, UKFD_POLLIN))
		return 0;

	for (size_t i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		char *last;
		int bytes_read;

		if (unlikely(!buf && len))
			return -EFAULT;

		bytes_read = uk_console_in_direct(cons, buf, len);
		if (!bytes_read)
			break;
		if (unlikely(bytes_read < 0))
			return bytes_read;

		total += bytes_read;

		last = buf + bytes_read - 1;
		if (*last == '\r')
			*last = '\n';

		/* EOT (^D) ends the input. Strip it from the data, as a
		 * terminal does, and have the next read return 0, the end of
		 * file: by clearing POLLIN or, with
		 * CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL, by remembering it.
		 */
		if (*last == SERIAL_EOT) {
			total--;
#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
			/* No more input will come: the file stays readable,
			 * as a file or a pipe at its end does, so poll() and
			 * select() report the end of file a read returns.
			 */
			*to_eof(f) = 1;
#else /* !CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
			uk_file_event_clear(f, UKFD_POLLIN);
#endif /* !CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
			if (bytes_read > 1 && consf_echo(f))
				consf_out_direct(cons, buf, bytes_read - 1);
			break;
		}

		/* Echo the input to the console (NOT stdout!), unless a
		 * program turned ECHO off to do its own
		 */
		if (consf_echo(f))
			consf_out_direct(cons, buf, bytes_read);

		if (*last == '\n')
			break;
	}

#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
	/* POLLIN stays set at the end, so it cannot mean "try again" */
	if (*to_eof(f))
		return total;
#endif /* CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
	if (total || !uk_file_poll_immediate(f, UKFD_POLLIN))
		return total;
	else
		return -EAGAIN;
}

/*
 * Read from an async-RX console file.
 *
 * Unlike serial_read, this variant does not assume that data is always
 * available.  UKFD_POLLIN is not pre-set at file creation time; instead the
 * evmon thread sets it when the driver's RX interrupt fires.
 *
 * UKFD_POLLIN is cleared BEFORE the actual read so the driver's in() op can
 * safely re-arm its interrupt internally (e.g. virtqueue_intr_enable).  Any
 * data that arrives after the drain re-triggers the interrupt, causing the
 * evmon to set POLLIN again on the next wakeup.
 */
static ssize_t serial_read_async(const struct uk_file *f,
				 const struct iovec *iov, size_t iovcnt,
				 size_t off, long flags __unused)
{
	struct uk_console *cons = f->node;
	struct consolefd_alloc *consf;
	ssize_t total = 0;
	int eot = 0;
	__ssz rc;

	UK_ASSERT(f->node);

	if (unlikely(off))
		return -ESPIPE;

	consf = to_consf(f);

	uk_mutex_lock(&consf->mtx);

#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
	if (*to_eof(f)) {
		uk_mutex_unlock(&consf->mtx);
		return 0;
	}
#endif /* CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */

	/* The previous read ended at an EOT: this one is the end of file */
	if (consf->eot) {
		consf->eot = 0;
		uk_mutex_unlock(&consf->mtx);
		return 0;
	}

	for (size_t i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		char *last;

		if (unlikely(!buf && len)) {
			uk_mutex_unlock(&consf->mtx);
			return -EFAULT;
		}

		/* Single non-blocking attempt — returns whatever the driver
		 * has buffered right now, not necessarily the full @len.
		 */
		rc = uk_console_in_direct(cons, buf, len);
		if (rc == 0) {
			uk_file_event_clear(f, UKFD_POLLIN);
			break;
		}
		if (unlikely(rc < 0)) {
			uk_mutex_unlock(&consf->mtx);
			return (ssize_t)rc;
		}

		total += rc;

		last = buf + rc - 1;
		if (*last == '\r')
			*last = '\n';

		/* EOT (^D) ends the input: strip it from the data, as a
		 * terminal does. What came before it is returned now and the
		 * end of file by the next read; POLLIN is still set, so a
		 * poll() does not wait for it.
		 */
		if (*last == SERIAL_EOT) {
			eot = 1;
			total--;
			rc--;
#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
			/* For good: readable from now on, at its end */
			*to_eof(f) = 1;
			uk_file_event_set(f, UKFD_POLLIN);
#else /* !CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
			if (total)
				consf->eot = 1;
#endif /* !CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
		}

		/* Echo only the bytes actually read, not the full iov len. */
		if (rc && consf_echo(f))
			consf_out_direct(cons, buf, (__sz)rc);

		if (eot || *last == '\n')
			break;
	}

	uk_mutex_unlock(&consf->mtx);

	/* If nothing was read (spurious wakeup), tell the caller to wait. */
	return (total || eot) ? total : -EAGAIN;
}

static ssize_t serial_write(const struct uk_file *f,
			    const struct iovec *iov, size_t iovcnt,
			    size_t off, long flags __unused)
{
	struct uk_console *cons = f->node;
	ssize_t total = 0;

	UK_ASSERT(f->vol == SERIAL_VOLID);
	UK_ASSERT(f->node);

	if (unlikely(off))
		return -ESPIPE;

	for (size_t i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		__ssz bytes_written;

		if (unlikely(!buf && len))
			return  -EFAULT;

		bytes_written = consf_writer(f)(cons, buf, len);
		if (unlikely(bytes_written < 0))
			return bytes_written;

		total += bytes_written;
		/* A partial write: the caller writes the rest */
		if ((__sz)bytes_written < len)
			break;
	}

	return total;
}

/*
 * Write to an async-TX console file.
 *
 * Unlike serial_write, this variant does not assume the TX queue is always
 * ready.  UKFD_POLLOUT is not pre-set at file creation time; instead the
 * evmon thread sets it when the driver's TX completion interrupt fires.
 *
 * If the TX queue becomes full mid-write (a partial write),
 * UKFD_POLLOUT is cleared so the caller blocks until the next TX completion
 * interrupt re-sets it via the async callback.
 */
static ssize_t serial_write_async(const struct uk_file *f,
				  const struct iovec *iov, size_t iovcnt,
				  size_t off, long flags __unused)
{
	struct uk_console *cons = f->node;
	struct consolefd_alloc *consf;
	ssize_t total = 0;
	__ssz rc;

	UK_ASSERT(f->node);

	if (unlikely(off))
		return -ESPIPE;

	consf = to_consf(f);

	uk_mutex_lock(&consf->mtx);

	for (size_t i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;

		if (unlikely(!buf && len))
			return -EFAULT;

		rc = consf_writer(f)(cons, buf, len);
		if (unlikely(rc < 0))
			return (ssize_t)rc;

		total += rc;
		if (!rc || (__sz)rc < len) {
			/* TX queue is full — clear POLLOUT and let the TX
			 * completion interrupt re-arm it via the async cb.
			 */
			uk_file_event_clear(f, UKFD_POLLOUT);
			break;
		}
	}

	uk_mutex_unlock(&consf->mtx);

	return total ? total : -EAGAIN;
}

static int serial_ctl(const struct uk_file *f __maybe_unused,
		      int fam, int req, uintptr_t arg1,
		      uintptr_t arg2 __unused, uintptr_t arg3 __unused)
{
	UK_ASSERT(f->vol == SERIAL_VOLID);

	switch (fam) {
	case UKFILE_CTL_IOCTL:
		switch (req) {
		/* Copy the four flag words and KNCCS control chars one field
		 * at a time: the caller's struct termios is its libc's, and a
		 * whole-struct copy could overrun a smaller one.
		 */
		case TCGETS:
		{
			const struct termios *t = to_termios(f);
			struct termios *tc = (struct termios *)arg1;

			tc->c_iflag = t->c_iflag;
			tc->c_oflag = t->c_oflag;
			tc->c_cflag = t->c_cflag;
			tc->c_lflag = t->c_lflag;
			memcpy(tc->c_cc, t->c_cc, KNCCS);
			return 0;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
		{
			const struct termios *tc = (const struct termios *)arg1;
			struct termios *t = to_termios(f);

			t->c_iflag = tc->c_iflag;
			t->c_oflag = tc->c_oflag;
			t->c_cflag = tc->c_cflag;
			t->c_lflag = tc->c_lflag;
			memcpy(t->c_cc, tc->c_cc, KNCCS);
			return 0;
		}
		case TIOCGWINSZ:
		{
			struct winsize *winsz = (struct winsize *)arg1;

			*winsz = (struct winsize){
				.ws_row = 24,
				.ws_col = 80
			};
			return 0;
		}
		/* Sending breaks not supported; no-op */
		case TCSBRK:
		case TCSBRKP:
		case TIOCSBRK:
		case TIOCCBRK:
			return 0;
		case TCXONC:
			uk_pr_warn_once("Serial file flow control stubbed\n");
			return 0;
		/* Input & Output queues always empty */
		case TIOCINQ:
		case TIOCOUTQ:
			*(int *)arg1 = 0;
			return 0;
		/* ... thus flushing is a no-op */
		case TCFLSH:
			return 0;
		/* Exclusive mode ignored; no-op */
		case TIOCEXCL:
		case TIOCNXCL:
			return 0;
		case TIOCGEXCL:
			*(int *)arg1 = 0;
			return 0;
		default:
			return -EINVAL;
		}
	default:
		return -ENOSYS;
	}
}

#define SERIAL_STATX_MASK \
	(UK_STATX_TYPE|UK_STATX_MODE|UK_STATX_NLINK|UK_STATX_INO|UK_STATX_SIZE)

static int serial_getstat(const struct uk_file *f __maybe_unused,
			  unsigned int mask __unused, struct uk_statx *arg)
{
	UK_ASSERT(f->vol == SERIAL_VOLID);

	/* Since all information is immediately available, ignore mask arg */
	arg->stx_mask = SERIAL_STATX_MASK;
	arg->stx_mode = S_IFCHR|0666;
	arg->stx_nlink = 1;
	/* A standard error file of its own differs from the console's, so
	 * programs comparing fd 1 and fd 2 tell them apart
	 */
	arg->stx_ino = to_consf(f)->err ? 2 : 1;
	arg->stx_size = 0;

	/* Following fields are always filled in, not in stx_mask */
	arg->stx_dev_major = 0;
	arg->stx_dev_minor = 5; /* Same value Linux returns for tty */
	arg->stx_rdev_major = 0;
	arg->stx_rdev_minor = 0;
	arg->stx_blksize = 0x1000;
	return 0;
}

static const struct uk_file_ops serial_ops = {
	.read = serial_read,
	.write = serial_write,
	.mem = uk_file_nop_mem,
	.getstat = serial_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = serial_ctl,
};

static const struct uk_file_ops serial_async_rx_ops = {
	.read = serial_read_async,
	.write = serial_write,
	.mem = uk_file_nop_mem,
	.getstat = serial_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = serial_ctl,
};

static const struct uk_file_ops serial_async_tx_ops = {
	.read = serial_read,
	.write = serial_write_async,
	.mem = uk_file_nop_mem,
	.getstat = serial_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = serial_ctl,
};

static const struct uk_file_ops serial_async_ops = {
	.read = serial_read_async,
	.write = serial_write_async,
	.mem = uk_file_nop_mem,
	.getstat = serial_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = serial_ctl,
};

static void consf_release(const struct uk_file *f, int what)
{
	struct consolefd_alloc *consf;

	UK_ASSERT(f->vol == SERIAL_VOLID);

	if (!(what & UK_FILE_RELEASE_OBJ))
		return;

	consf = to_consf(f);

	uk_list_del(&consf->async_list);

	if (consf->tty != consf)
		uk_file_release(&consf->tty->f);

	/* Free */
	uk_free(consf->a, consf);
}

/*
 * ISR-safe async event callbacks registered with async console devices.
 * The cookie is always the owning consolefd_alloc.  Each callback atomically
 * ORs the relevant poll bit into pending_events and wakes the evmon thread.
 */
__isr static void consf_async_in_cb(struct uk_console *dev __unused,
				    void *cookie)
{
	struct consolefd_alloc *consf = (struct consolefd_alloc *)cookie;

	uk_or(&consf->pending_events, UKFD_POLLIN);
	uk_semaphore_up_isr(&consf_async_sem);
}

__isr static void consf_async_out_cb(struct uk_console *dev __unused,
				     void *cookie)
{
	struct consolefd_alloc *consf = (struct consolefd_alloc *)cookie;

	uk_or(&consf->pending_events, UKFD_POLLOUT);
	uk_semaphore_up_isr(&consf_async_sem);
}

static const struct uk_file *consf_create(struct uk_console *cons,
					  struct consolefd_alloc *tty)
{
	struct consolefd_alloc *consf;
	const struct uk_file_ops *ops;
	__u32 init_events = 0;
	int rc;

	UK_ASSERT(cons);

	consf = uk_malloc(uk_alloc_get_default(), sizeof(*consf));
	if (unlikely(!consf)) {
		uk_pr_err("Could not allocate console file for %p\n", cons);
		return NULL;
	}

	consf->a = uk_alloc_get_default();
	consf->pending_events = 0;
	consf->err = !!tty;
	consf->tty = tty ? tty : consf;
	consf->eot = 0;
#if CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL
	consf->eof = 0;
#endif /* CONFIG_LIBUKFILE_CONSOLE_IN_EOF_FINAL */
	consf->termios = (struct termios){
		.c_iflag = SERIAL_TERMIOS_IFLAGS,
		.c_oflag = SERIAL_TERMIOS_OFLAGS,
		.c_cflag = SERIAL_TERMIOS_CFLAGS,
		.c_lflag = SERIAL_TERMIOS_LFLAGS,
	};
	memcpy(consf->termios.c_cc, SERIAL_TERMIOS_CONTROL_CHARS, KNCCS);
	UK_INIT_LIST_HEAD(&consf->async_list);

	/*
	 * Only pre-set poll flags for directions the device handles
	 * synchronously.  For async directions the evmon thread delivers
	 * the flag once the device's interrupt fires, so the file must not
	 * appear ready before there is actually data.
	 */
	if (!(cons->flags & UK_CONSOLE_FLAG_ASYNC_RX))
		init_events |= UKFD_POLLIN;
	if (!(cons->flags & UK_CONSOLE_FLAG_ASYNC_TX))
		init_events |= UKFD_POLLOUT;

	consf->fstate = UK_FILE_STATE_EVENTS_INIT_VALUE(consf->fstate,
							init_events);

	consf->frefcnt = UK_FILE_REFCNT_INIT_VALUE(consf->frefcnt);

	if ((cons->flags & UK_CONSOLE_FLAG_ASYNC_TX) &&
	    (cons->flags & UK_CONSOLE_FLAG_ASYNC_RX))
		ops = &serial_async_ops;
	else if (cons->flags & UK_CONSOLE_FLAG_ASYNC_TX)
		ops = &serial_async_tx_ops;
	else if (cons->flags & UK_CONSOLE_FLAG_ASYNC_RX)
		ops = &serial_async_rx_ops;
	else
		ops = &serial_ops;

	consf->f = (struct uk_file){
		.vol = SERIAL_VOLID,
		.node = cons,
		.refcnt = &consf->frefcnt,
		.state = &consf->fstate,
		.ops = ops,
		._release = consf_release,
	};

	/*
	 * For async-capable consoles, register the appropriate ISR callbacks
	 * and add the file to the global monitoring list so the evmon thread
	 * can deliver poll events after each interrupt.
	 */
	if ((cons->flags & UK_CONSOLE_FLAG_ASYNC_RX) ||
	    (cons->flags & UK_CONSOLE_FLAG_ASYNC_TX)) {
		if (cons->flags & UK_CONSOLE_FLAG_ASYNC_RX) {
			rc = uk_console_async_register_callback(
				cons, consf_async_in_cb, consf,
				UK_CONSOLE_ASYNC_EVENT_IN);
			if (unlikely(rc)) {
				uk_pr_err("Failed to register async RX cb: %d\n",
					  rc);
				uk_free(consf->a, consf);
				return __NULL;
			}
		}

		if (cons->flags & UK_CONSOLE_FLAG_ASYNC_TX) {
			rc = uk_console_async_register_callback(
				cons, consf_async_out_cb, consf,
				UK_CONSOLE_ASYNC_EVENT_OUT);
			if (unlikely(rc)) {
				uk_pr_err("Failed to register async TX cb: %d\n",
					  rc);
				/* Make sure we do not leak RX callback if it
				 * is both RX and TX. If it fails, just log it.
				 */
				if (cons->flags & UK_CONSOLE_FLAG_ASYNC_RX) {
					rc = uk_console_async_register_callback(
						cons, consf_async_in_cb, consf,
						UK_CONSOLE_ASYNC_EVENT_IN);
					if (unlikely(rc))
						uk_pr_err("Failed to release RX cb: %d\n",
							  rc);
				}

				uk_free(consf->a, consf);
				return __NULL;
			}
		}

		uk_mutex_lock(&consf_async_list_mtx);
		uk_list_add_tail(&consf->async_list, &consf_async_list);
		uk_mutex_unlock(&consf_async_list_mtx);
	}

	uk_mutex_init(&consf->mtx);

	if (tty)
		uk_file_acquire(&tty->f);

	return &consf->f;
}

const struct uk_file *uk_file_console_create(struct uk_console *cons)
{
	return consf_create(cons, __NULL);
}

const struct uk_file *uk_file_console_create_err(const struct uk_file *f)
{
	struct consolefd_alloc *tty;

	UK_ASSERT(f);

	if (f->vol != SERIAL_VOLID)
		return __NULL;

	tty = to_consf(f)->tty;
	return consf_create(f->node, tty);
}

/*
 * Event monitor thread.  Blocks until at least one async ISR callback ups
 * consf_async_sem, then atomically drains each async file's pending_events
 * and delivers them to the file event subsystem.
 */
__noreturn static void consf_evmon_thread(void *arg __unused)
{
	struct consolefd_alloc *consf;
	__u32 events;

	for (;;) {
		uk_semaphore_down_all(&consf_async_sem);

		uk_mutex_lock(&consf_async_list_mtx);
		uk_list_for_each_entry(consf, &consf_async_list, async_list) {
			/*
			 * Atomically exchange pending_events with 0 so that
			 * an ISR firing between the read and clear cannot
			 * silently lose a bit.
			 */
			events = uk_exchange_n(&consf->pending_events, 0);
			if (!events)
				continue;

			uk_mutex_lock(&consf->mtx);
			uk_file_event_set(&consf->f, events);
			uk_mutex_unlock(&consf->mtx);
		}
		uk_mutex_unlock(&consf_async_list_mtx);
	}
}

static int init_evmon(struct uk_init_ctx *ictx __unused)
{
	struct uk_thread *evmon;

	evmon = uk_sched_thread_create(uk_sched_current(),
				       consf_evmon_thread, __NULL,
				       "consf-evmon");
	if (unlikely(!evmon)) {
		uk_pr_err("Failed to start consf-evmon thread\n");
		return -ENOMEM;
	}

	uk_semaphore_init(&consf_async_sem, 0);

	return 0;
}

uk_early_initcall(init_evmon, 0x0);

#if CONFIG_LIBUKFS_DEVFS
static int consf_create_devnode(struct uk_console *cons,
				const char *name, __sz name_len)
{
	const struct uk_file *f;
	unsigned int mode;
	const void *r;

	UK_ASSERT(cons);
	UK_ASSERT(uk_fs_devfs_root);

	f = uk_file_console_create(cons);
	if (unlikely(PTRISERR(f))) {
		uk_pr_err("Failed to create console file for %s\n", name);
		return PTR2ERR(f);
	}

	mode = 0;
	if ((cons->flags & UK_CONSOLE_FLAG_STDOUT) || cons->ops->out)
		mode |= 0222;
	if ((cons->flags & UK_CONSOLE_FLAG_STDIN) || cons->ops->in)
		mode |= 0444;

	/* We do not clean up created files on error, as they will be
	 * dropped when the devfs root is released on system shutdown.
	 */
	r = uk_fs_createat(uk_fs_devfs_root,
			   name, name_len, mode, O_EXCL,
			   (union uk_fs_create_target){
				.file = f,
			   });
	uk_file_release(f);
	if (unlikely(PTRISERR(r))) {
		uk_pr_err("Failed to create /dev/%s: %d\n", name, PTR2ERR(r));
		return PTR2ERR(r);
	}

	return 0;
}

static int init_posix_tty_devfs_consnodes(struct uk_init_ctx *ictx __unused)
{
	__sz i, ccount, name_len, hvc_idx = 0, uart_idx = 0, fb_idx = 0;
	struct uk_console *cons;
	char name[32];  /* Somewhat pragmatic size for tty devfs nodes */
	int rc;

	ccount = uk_console_count();
	for (i = 0; i < ccount; i++) {
		cons = uk_console_get(i);
		UK_ASSERT(cons);

		switch (cons->dclass) {
		case UK_CONSOLE_CLASS_HVC:
			rc = snprintf(name, sizeof(name), "hvc%lu", hvc_idx);
			hvc_idx++;
			break;
		case UK_CONSOLE_CLASS_UART:
			rc = snprintf(name, sizeof(name), "ttyS%lu", uart_idx);
			uart_idx++;
			break;
		case UK_CONSOLE_CLASS_FB:
			rc = snprintf(name, sizeof(name), "tty%lu", fb_idx);
			fb_idx++;
			break;
		default:
			continue;
		}

		if (unlikely(rc < 0)) {
			uk_pr_err("Failed to build devfs node name\n");
			return rc;
		}

		UK_ASSERT((__sz)rc <= sizeof(name));
		name_len = (__sz)rc;

		rc = consf_create_devnode(cons, name, name_len);
		if (unlikely(rc)) {
			/* Not tracking previously allocated console files to
			 * avoid leaks because they will be automatically
			 * dropped on devfs drop during shutdown.
			 */
			uk_pr_err("Failed to create devfs node for %s\n",
				  name);
			return rc;
		}
	}

	return 0;
}

uk_rootfs_initcall_prio(init_posix_tty_devfs_consnodes, 0x0,
			UK_FS_PRIO_FSAVAIL);
#endif /* CONFIG_LIBUKFS_DEVFS */

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

#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/posix-fd.h>

static const char SERIAL_VOLID[] = "serial_vol";

/* EOT: End Of Transmission character; ^D */
#define SERIAL_EOT 04

#define SERIAL_TERMIOS_IFLAGS (IUTF8|ICRNL)
#define SERIAL_TERMIOS_OFLAGS (0)
#define SERIAL_TERMIOS_CFLAGS (CREAD|CS8|B38400)
#define SERIAL_TERMIOS_LFLAGS (ICANON|ECHO)

/* Linux only fills in 19 chars; bincompat might expect this */
#define KNCCS 19

/* Values taken from termios(3) from Linux man-pages 6.05 */
static const char SERIAL_TERMIOS_CONTROL_CHARS[KNCCS] = {
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

#if CONFIG_LIBUKFILE_CONSOLE_OUT_NOCARRIAGERETURN
static inline __ssz _console_out(struct uk_console *cons,
				 const char *buf, __sz len)
{
	return uk_console_out_direct(cons, buf, len);
}
#else /* !CONFIG_LIBUKFILE_CONSOLE_OUT_NOCARRIAGERETURN */
/* TODO: Some consoles require both a newline and a carriage return to
 * go to the start of the next line. This kind of behavior should be in
 * a single place in posix-tty. We keep this workaround until we have feature
 * in posix-tty that handles newline characters correctly.
 */
static inline __ssz _console_out(struct uk_console *cons,
				 const char *buf, __sz len)
{
	const char *next_nl = NULL;
	__sz l = len;
	__sz off = 0;
	__ssz rc = 0;

	if (unlikely(!len))
		return 0;
	if (unlikely(!buf))
		return -EINVAL;

	while (l > 0) {
		next_nl = memchr(buf, '\n', l);
		if (next_nl) {
			off = next_nl - buf;

			if ((rc = uk_console_out_direct(cons, buf, off)) < 0)
				return rc;
			if ((rc = uk_console_out_direct(cons, "\r\n", 2)) < 0)
				return rc;
			buf = next_nl + 1;
			l -= off + 1;
		} else {
			if ((rc = uk_console_out_direct(cons, buf, l)) < 0)
				return rc;
			break;
		}
	}

	return len;
}
#endif /* !CONFIG_LIBUKFILE_CONSOLE_OUT_NOCARRIAGERETURN */

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

#if !CONFIG_LIBUKFILE_CONSOLE_IN_NOECHOBACK
		/* Echo the input to the console (NOT stdout!) */
		_console_out(cons, buf, bytes_read);
#endif /* !CONFIG_LIBUKFILE_CONSOLE_IN_NOECHOBACK */

		if (*last == '\n')
			break;
		if (*last == SERIAL_EOT)
			uk_file_event_clear(f, UKFD_POLLIN);
	}

	if (total || !uk_file_poll_immediate(f, UKFD_POLLIN))
		return total;
	else
		return -EAGAIN;
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
		int bytes_written;

		if (unlikely(!buf && len))
			return  -EFAULT;

		bytes_written = _console_out(cons, buf, len);
		if (unlikely(bytes_written < 0))
			return bytes_written;

		total += bytes_written;
	}

	return total;
}

static int serial_ctl(const struct uk_file *f __maybe_unused,
		      int fam, int req, uintptr_t arg1,
		      uintptr_t arg2 __unused, uintptr_t arg3 __unused)
{
	UK_ASSERT(f->vol == SERIAL_VOLID);

	switch (fam) {
	case UKFILE_CTL_IOCTL:
		switch (req) {
		case TCGETS:
		{
			struct termios *tc = (struct termios *)arg1;

			tc->c_iflag = SERIAL_TERMIOS_IFLAGS;
			tc->c_oflag = SERIAL_TERMIOS_OFLAGS;
			tc->c_cflag = SERIAL_TERMIOS_CFLAGS;
			tc->c_lflag = SERIAL_TERMIOS_LFLAGS;
			memcpy(tc->c_cc, SERIAL_TERMIOS_CONTROL_CHARS, KNCCS);
			return 0;
		}
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			uk_pr_warn_once("Serial file settings stubbed\n");
			return 0;
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
	arg->stx_ino = 1;
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

struct consolefd_alloc {
	/* Allocator used to create this console file */
	struct uk_alloc *a;
	/* Console file refcount */
	uk_file_refcnt frefcnt;
	/* Console file state */
	struct uk_file_state fstate;
	/* Base file structure */
	struct uk_file f;
};

static inline struct consolefd_alloc *to_consf(const struct uk_file *f)
{
	return __containerof(f, struct consolefd_alloc, f);
}

static void consf_release(const struct uk_file *f, int what)
{
	struct consolefd_alloc *consf;

	UK_ASSERT(f->vol == SERIAL_VOLID);

	if (!(what & UK_FILE_RELEASE_OBJ))
		return;

	consf = to_consf(f);

	/* Free */
	uk_free(consf->a, consf);
}

const struct uk_file *uk_file_console_create(struct uk_console *cons)
{
	struct consolefd_alloc *consf;

	UK_ASSERT(cons);

	consf = uk_malloc(uk_alloc_get_default(), sizeof(*consf));
	if (unlikely(!consf)) {
		uk_pr_err("Could not allocate console file for %p\n", cons);
		return NULL;
	}

	consf->a = uk_alloc_get_default();
	consf->fstate = UK_FILE_STATE_EVENTS_INIT_VALUE(consf->fstate,
							UKFD_POLLIN |
							UKFD_POLLOUT);
	consf->frefcnt = UK_FILE_REFCNT_INIT_VALUE(consf->frefcnt);

	consf->f = (struct uk_file){
		.vol = SERIAL_VOLID,
		.node = cons,
		.refcnt = &consf->frefcnt,
		.state = &consf->fstate,
		.ops = &serial_ops,
		._release = consf_release,
	};

	return &consf->f;
}

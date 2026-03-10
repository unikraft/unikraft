/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <uk/assert.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/init.h>
#include <uk/posix-fd.h>
#include <uk/posix-serialfile.h>

#if CONFIG_LIBUKLIBPARAM
#include <uk/libparam.h>
#endif /* CONFIG_LIBUKLIBPARAM */

static struct uk_console *tty_cons;
static char *tty_cons_arg;

#if CONFIG_LIBUKLIBPARAM
UK_LIBPARAM_PARAM_ALIAS(console, &tty_cons_arg, charp,
			"Default application stdio console device based on devfs node");
#endif /* CONFIG_LIBUKLIBPARAM */

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

#if CONFIG_LIBPOSIX_TTY_OUT_NOCARRIAGERETURN
static inline __ssz _console_out(const char *buf, __sz len)
{
	return uk_console_out_direct(tty_cons, buf, len);
}
#else /* !CONFIG_LIBPOSIX_TTY_OUT_NOCARRIAGERETURN */
/* TODO: Some consoles require both a newline and a carriage return to
 * go to the start of the next line. This kind of behavior should be in
 * a single place in posix-tty. We keep this workaround until we have feature
 * in posix-tty that handles newline characters correctly.
 */
static inline __ssz _console_out(const char *buf, __sz len)
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
			if ((rc = uk_console_out_direct(tty_cons,
							buf, off)) < 0)
				return rc;
			if ((rc = uk_console_out_direct(tty_cons,
							"\r\n", 2)) < 0)
				return rc;
			buf = next_nl + 1;
			l -= off + 1;
		} else {
			if ((rc = uk_console_out_direct(tty_cons,
							buf, l)) < 0)
				return rc;
			break;
		}
	}

	return len;
}
#endif /* !CONFIG_LIBPOSIX_TTY_OUT_NOCARRIAGERETURN */

static ssize_t serial_read(const struct uk_file *f,
			   const struct iovec *iov, size_t iovcnt,
			   size_t off, long flags __unused)
{
	ssize_t total = 0;

	UK_ASSERT(f->vol == SERIAL_VOLID);
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

		bytes_read = uk_console_in_direct(tty_cons, buf, len);
		if (!bytes_read)
			break;
		if (unlikely(bytes_read < 0))
			return bytes_read;

		total += bytes_read;

		last = buf + bytes_read - 1;
		if (*last == '\r')
			*last = '\n';

#if !CONFIG_LIBPOSIX_TTY_IN_NOECHOBACK
		/* Echo the input to the console (NOT stdout!) */
		_console_out(buf, bytes_read);
#endif /* !CONFIG_LIBPOSIX_TTY_IN_NOECHOBACK */

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

static ssize_t serial_write(const struct uk_file *f __maybe_unused,
			    const struct iovec *iov, size_t iovcnt,
			    size_t off, long flags __unused)
{
	ssize_t total = 0;

	UK_ASSERT(f->vol == SERIAL_VOLID);
	if (unlikely(off))
		return -ESPIPE;

	for (size_t i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		int bytes_written;

		if (unlikely(!buf && len))
			return  -EFAULT;

		bytes_written = _console_out(buf, len);
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

static uk_file_refcnt serial_ref = UK_FILE_REFCNT_INITIALIZER(serial_ref);

static struct uk_file_state serial_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	serial_state, UKFD_POLLIN|UKFD_POLLOUT);

const struct uk_file uk_file_serial = {
	.vol = SERIAL_VOLID,
	.node = NULL,
	.ops = &serial_ops,
	.refcnt = &serial_ref,
	.state = &serial_state,
	._release = uk_file_static_release
};

static int init_tty_cons(struct uk_init_ctx *ictx __unused)
{
	__sz i, ccount, idx, cclass_str_len = 0;
	enum uk_console_devclass tty_dclass;
	struct uk_console *cons;
	const char *str;

	/* If no argument provided, assume the first registered one */
	if (!tty_cons_arg) {
		tty_cons = uk_console_get(0);
		return 0;
	}

	if (!strncmp(tty_cons_arg, "hvc", sizeof("hvc") - 1)) {
		tty_dclass = UK_CONSOLE_CLASS_HVC;
		cclass_str_len = sizeof("hvc") - 1;
	} else if (!strncmp(tty_cons_arg, "ttyS", sizeof("ttyS") - 1)) {
		tty_dclass = UK_CONSOLE_CLASS_UART;
		cclass_str_len = sizeof("ttyS") - 1;
	} else if (!strncmp(tty_cons_arg, "tty", sizeof("tty") - 1)) {
		tty_dclass = UK_CONSOLE_CLASS_FB;
		cclass_str_len = sizeof("tty") - 1;
	} else {
		uk_pr_err("Invalid console argument\n");
		return -EINVAL;
	}

	rc = sscanf(tty_cons_arg + cclass_str_len, "%lu", &idx);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to parse tty_console arg\n");
		return rc;
	}

	/**
	 * Now go through each registered console and find the idx'th console
	 * of class tty_cclass.
	 */
	ccount = uk_console_count();
	for (i = 0; i < ccount; i++) {
		cons = uk_console_get(i);
		UK_ASSERT(cons);

		 /* If not the class we care about, ignore and move on */
		if (cons->dclass != tty_dclass)
			continue;

		/**
		 * If idx is 0 and this is the first console of class
		 * tty_cclass then we are done - assign it.
		 * Otherwise we keep decrementing idx for each tty_cclass
		 * class registration we find and when it hits 0 then we know
		 * we found the user-intended idx'th console registration
		 * of this class.
		 */
		if (!idx) {
			tty_cons = cons;
			break;
		}
		idx--;
	}

	if (unlikely(!tty_cons)) {
		uk_pr_err("Requested console not found\n");
		return -ENOENT;
	}

	return 0;
}

uk_lib_initcall_prio(init_tty_cons, 0x0, UK_PRIO_LATEST);

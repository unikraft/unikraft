/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <termios.h>
#include <sys/ioctl.h>

#include <uk/assert.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/plat/console.h>
#include <uk/posix-fd.h>
#include <uk/posix-serialfile.h>

static const char SERIAL_VOLID[] = "serial_vol";

/* EOT: End Of Transmission character; ^D */
#define SERIAL_EOT 04

static ssize_t serial_read(const struct uk_file *f,
			   const struct iovec *iov, int iovcnt,
			   off_t off, long flags __unused)
{
	ssize_t total = 0;

	UK_ASSERT(f->vol == SERIAL_VOLID);
	if (unlikely(off != 0))
		return -EINVAL;

	if (!uk_file_poll_immediate(f, UKFD_POLLIN))
		return 0;

	for (int i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		char *last;
		int bytes_read;

		if (unlikely(!buf && len))
			return -EFAULT;

		bytes_read = ukplat_cink(buf, len);
		if (!bytes_read)
			break;
		if (unlikely(bytes_read < 0))
			return bytes_read;

		total += bytes_read;

		last = buf + bytes_read - 1;
		if (*last == '\r')
			*last = '\n';

		/* Echo the input to the console (NOT stdout!) */
		ukplat_coutk(buf, bytes_read);

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
			    const struct iovec *iov, int iovcnt,
			    off_t off, long flags __unused)
{
	ssize_t total = 0;

	UK_ASSERT(f->vol == SERIAL_VOLID);
	if (unlikely(off != 0))
		return -EINVAL;

	for (int i = 0; i < iovcnt; i++) {
		char *buf = iov[i].iov_base;
		size_t len = iov[i].iov_len;
		int bytes_written;

		if (unlikely(!buf && len))
			return  -EFAULT;

		bytes_written = ukplat_coutk(buf, len);
		if (unlikely(bytes_written < 0))
			return bytes_written;

		total += bytes_written;
	}

	return total;
}

static int serial_ctl(const struct uk_file *f, int fam, int req, uintptr_t arg1,
		      uintptr_t arg2 __unused, uintptr_t arg3 __unused)
{
	UK_ASSERT(f->vol == SERIAL_VOLID);

	switch (fam) {
	case UKFILE_CTL_IOCTL:
		switch (req) {
		case TIOCGWINSZ:
		{
			struct winsize *winsz = (struct winsize *)arg1;

			*winsz = (struct winsize){
				.ws_row = 24,
				.ws_col = 80
			};
			return 0;
		}
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
}

static const struct uk_file_ops serial_ops = {
	.read = serial_read,
	.write = serial_write,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = serial_ctl,
};

static uk_file_refcnt serial_ref = UK_FILE_REFCNT_INITIALIZER(serial_ref);

static struct uk_file_state serial_state = UK_FILE_STATE_EVENTS_INITIALIZER(
	serial_state, UKFD_POLLIN|UKFD_POLLOUT);

static const struct uk_file serial_file = {
	.vol = SERIAL_VOLID,
	.node = NULL,
	.ops = &serial_ops,
	.refcnt = &serial_ref,
	.state = &serial_state,
	._release = uk_file_static_release
};

const struct uk_file *uk_serialfile_create(void)
{
	const struct uk_file *f = &serial_file;

	uk_file_acquire(f);
	return f;
}

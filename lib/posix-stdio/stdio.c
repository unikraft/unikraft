/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
 *
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/plat/console.h>
#include <uk/syscall.h>
#include <uk/essentials.h>
#include <termios.h>
#include <unistd.h>
#include <uk/fdtab/uio.h>
#include <uk/fdtab/fd.h>
#include <uk/init.h>
#include <errno.h>

/*
 * When the syscall_shim library is not part of the build, there is warning
 * of implicit declaration of uk_syscall_r_dup2.
 * This declaration takes care of that when the syscall_shim library is not
 * part of the build.
 */

#if !CONFIG_LIBSYSCALL_SHIM
long uk_syscall_r_dup2(long oldfd, long newfd);
#endif /* !CONFIG_LIBSYSCALL_SHIM */

static int __write_fn(void *dst __unused, void *src, size_t *cnt)
{
	int ret = ukplat_coutk(src, *cnt);

	if (ret < 0)
		/* TODO: remove -1 when vfscore switches to negative
		 * error numbers
		 */
		return ret * -1;

	*cnt = (size_t) ret;
	return 0;
}

/* One function for stderr and stdout */
static int stdio_write(struct fdtab_file *fp __unused, struct uio *uio,
		       int ioflag __unused)
{
	if (uio->uio_offset != -1)
		return ESPIPE;

	return fdtab_uioforeach(__write_fn, NULL, uio->uio_resid, uio);
}


static int __read_fn(void *dst, void *src __unused, size_t *cnt)
{
	int bytes_read;
	size_t bytes_total = 0, count;
	char *buf = dst;

	count = *cnt;

	do {
		while ((bytes_read = ukplat_cink(buf,
			count - bytes_total)) <= 0)
			;

		buf = buf + bytes_read;
		*(buf - 1) = *(buf - 1) == '\r' ?
					'\n' : *(buf - 1);

		/* Echo the input */
		if (*(buf - 1) == '\177') {
			/* DELETE control character */
			if (buf - 1 != dst) {
				/* If this is not the first byte */
				ukplat_coutk("\b \b", 3);
				buf -= 1;
				bytes_total -= 1;
			}
			buf -= 1;
		} else {
			ukplat_coutk(buf - bytes_read, bytes_read);
			bytes_total += bytes_read;
		}

	} while (bytes_total < count && *(buf - 1) != '\n'
			&& *(buf - 1) != VEOF);

	*cnt = bytes_total;

	/* The INT_MIN here is a special return code. It makes the
	 * fdtab_uioforeach to quit from the loop. But this is not
	 * an error (for example a user hit Ctrl-C). That is why this
	 * special return value is fixed up to 0 in the stdio_read.
	 */
	if (*(buf - 1) == '\n' || *(buf - 1) == VEOF)
		return INT_MIN;

	return 0;
}

static int stdio_read(struct fdtab_file *file __unused,
		      struct uio *uio,
		      int ioflag __unused)
{
	int ret;

	if (uio->uio_offset)
		return ESPIPE;

	ret = fdtab_uioforeach(__read_fn, NULL, uio->uio_resid, uio);
	ret = (ret == INT_MIN) ? 0 : ret;

	return ret;
}

static int stdio_poll(struct fdtab_file *fp __unused,
		      unsigned int *revents __unused,
		      struct eventpoll_cb *cb __unused)
{
	return EINVAL;
}

static int stdio_free(struct fdtab_file *fp __unused)
{
	/* We ensure the refcount never reaches zero */
	UK_CRASH("unreachable");
}

static struct fdops stdio_fdops = {
	.fdop_free = stdio_free,
	.fdop_read = stdio_read,
	.fdop_write = stdio_write,
	.fdop_poll = stdio_poll,
};

static struct fdtab_file stdio_file = {
	.fd = 1,
	.f_flags = UK_FWRITE | UK_FREAD,
	/* reference count is 2 because close(0) is a valid
	 * operation. However it is not properly handled in the
	 * current implementation.
	 */
	.f_count = 2,
	.f_op = &stdio_fdops,
	.f_ep = UK_LIST_HEAD_INIT(stdio_file.f_ep)
};

static int init_stdio(void)
{
	int fd;
	struct fdtab_table *tab = fdtab_get_active();

	fd = fdtab_alloc_fd(tab);
	if (fd != 0) {
		uk_pr_crit("failed to allocate fd for stdin (fd=0)\n");
		return (fd < 0) ? fd : -EBADF;
	}
	fdtab_install_fd(tab, 0, &stdio_file);

	fd = uk_syscall_r_dup2(0, 1);
	if (fd != 1) {
		uk_pr_crit("failed to dup to stdout (fd=1)\n");
		return (fd < 0) ? fd : -EBADF;
	}

	fd = uk_syscall_r_dup2(0, 2);
	if (fd != 2) {
		uk_pr_crit("failed to dup to stderr (fd=2)\n");
		return (fd < 0) ? fd : -EBADF;
	}

	return 0;
}

uk_lib_initcall_prio(init_stdio, UK_PRIO_AFTER(POSIX_FDTAB_REGISTER_PRIO));

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Alexander Jung <a.jung@lancs.ac.uk>
 *
 * Copyright (c) 2021, Lancaster University. All rights reserved.
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

#include <uk/config.h>
#include <stdlib.h>
#include <string.h>
#include <uk/ctors.h>
#include <uk/print.h>
#include <vfscore/uio.h>
#include <devfs/device.h>
#include <errno.h>
#include <uk/plat/console.h>

#ifdef CONFIG_LIBDEVFS_DEV_STDOUT

#define DEV_STDOUT_NAME "stdout"

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
int dev_stdout_write(struct device *dev __unused, struct uio *uio,
		   int flags __unused)
{
	return vfscore_uioforeach(__write_fn, NULL, uio->uio_resid, uio);
}

int dev_stdout_open(struct device *device __unused, int mode __unused)
{
	return 0;
}

int dev_stdout_close(struct device *device __unused)
{
	return 0;
}

int dev_stdout_read(struct device *dev __unused, struct uio *uio,
		  int flags __unused)
{
	uio->uio_resid = uio->uio_iov->iov_len;
	return 0;
}

static struct devops stdout_devops = {
	.read = dev_stdout_read,
	.write = dev_stdout_write,
	.open = dev_stdout_open,
	.close = dev_stdout_close,
};

static struct driver drv_stdout = {
	.devops = &stdout_devops,
	.devsz = 0,
	.name = DEV_STDOUT_NAME
};

#endif /* CONFIG_LIBDEVFS_DEV_STDOUT */

static int devfs_register_stdout(void)
{
	struct device *dev;

#ifdef CONFIG_LIBDEVFS_DEV_STDOUT
	uk_pr_debug("Register '%s' to devfs\n", DEV_STDOUT_NAME);

	/* register /dev/stdout */
	dev = device_create(&drv_stdout, DEV_STDOUT_NAME, D_CHR);
	if (dev == NULL) {
		uk_pr_err("Failed to register '%s' to devfs\n",
			DEV_STDOUT_NAME);
		return -1;
	}
#endif /* LIBDEVFS_DEV_STDOUT */

	return 0;
}

devfs_initcall(devfs_register_stdout);

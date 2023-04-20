/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#include <stdlib.h>
#include <string.h>

#include <uk/config.h>
#include <uk/ctors.h>
#include <uk/print.h>
#include <vfscore/uio.h>
#include <devfs/device.h>

static int dev_null_write(struct device *dev __unused, struct uio *uio,
			  int flags __unused)
{
	uio->uio_resid = 0;
	return 0;
}

#ifdef CONFIG_LIBDEVFS_DEV_NULL
#define DEV_NULL_NAME "null"

static int dev_null_read(struct device *dev __unused, struct uio *uio,
			 int flags __unused)
{
	uio->uio_resid = uio->uio_iov->iov_len;
	return 0;
}

static struct devops null_devops = {
	.open = dev_noop_open,
	.close = dev_noop_close,
	.read = dev_null_read,
	.write = dev_null_write,
	.ioctl = dev_noop_ioctl,
};

static struct driver drv_null = {
	.devops = &null_devops,
	.devsz = 0,
	.name = DEV_NULL_NAME
};
#endif /* CONFIG_LIBDEVFS_DEV_NULL */

#ifdef CONFIG_LIBDEVFS_DEV_ZERO
#define DEV_ZERO_NAME "zero"

static int dev_zero_read(struct device *dev __unused, struct uio *uio,
			 int flags __unused)
{
	size_t count;
	char *buf;

	buf = uio->uio_iov->iov_base;
	count = uio->uio_iov->iov_len;

	memset(buf, 0, count);
	uio->uio_resid = 0;
	return 0;
}

static struct devops zero_devops = {
	.open = dev_noop_open,
	.close = dev_noop_close,
	.read = dev_zero_read,
	.write = dev_null_write,
	.ioctl = dev_noop_ioctl,
};

static struct driver drv_zero = {
	.devops = &zero_devops,
	.devsz = 0,
	.name = DEV_ZERO_NAME
};
#endif /* CONFIG_LIBDEVFS_DEV_ZERO */

static int devfs_register_null(void)
{
	int rc;

#ifdef CONFIG_LIBDEVFS_DEV_NULL
	uk_pr_debug("Register '%s' to devfs\n", DEV_NULL_NAME);

	/* register /dev/null */
	rc = device_create(&drv_null, DEV_NULL_NAME, D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to devfs: %d\n",
			  DEV_NULL_NAME, rc);
		return -rc;
	}
#endif /* CONFIG_LIBDEVFS_DEV_NULL */

#ifdef CONFIG_LIBDEVFS_DEV_ZERO
	uk_pr_debug("Register '%s' to devfs\n", DEV_ZERO_NAME);

	/* register /dev/zero */
	rc = device_create(&drv_zero, DEV_ZERO_NAME, D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to devfs: %d\n",
			  DEV_ZERO_NAME, rc);
		return -rc;
	}
#endif /* CONFIG_LIBDEVFS_DEV_ZERO */

	return 0;
}

devfs_initcall(devfs_register_null);

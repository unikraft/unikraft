/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@stud.acs.upb.ro>
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
#include <errno.h>
#include <uk/ctors.h>
#include <uk/print.h>
#include <uk/random.h>
#include <uk/arch/random.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <vfscore/uio.h>
#include <devfs/device.h>

#define DEV_RANDOM_NAME "random"
#define DEV_URANDOM_NAME "urandom"
#define DEV_HWRNG_NAME "hwrng"

/* TODO: Move this to random.c once ukarch_random is implemented as a driver */
__ssz uk_random_hwrng_fill_buffer(void *buf, __sz buflen)
{
	__sz step, chunk_size, i;
	__u64 rd;
	int ret;

	step = sizeof(__u64);
	chunk_size = buflen % step;

	for (i = 0; i < buflen - chunk_size; i += step) {
		ret = ukarch_random_seed_u64((__u64 *)((char *)buf + i));
		if (unlikely(ret))
			return ret;
	}

	/* fill the remaining bytes of the buffer */
	if (chunk_size > 0) {
		ret = ukarch_random_seed_u64(&rd);
		if (unlikely(ret))
			return ret;
		memcpy((char *)buf + i, &rd, chunk_size);
	}

	return buflen;
}

static int dev_random_read(struct device *dev __unused, struct uio *uio,
			   int flags __unused)
{
	__sz count;
	char *buf;

	buf = uio->uio_iov->iov_base;
	count = uio->uio_iov->iov_len;

	uk_random_fill_buffer(buf, count);

	uio->uio_resid = 0;
	return 0;
}

static int dev_random_write(struct device *dev __unused,
			    struct uio *uio __unused,
			    int flags __unused)
{
	return EPERM;
}

static int dev_hwrng_read(struct device *dev __unused, struct uio *uio,
			  int flags __unused)
{
	__sz count;
	char *buf;
	int ret;

	buf = uio->uio_iov->iov_base;
	count = uio->uio_iov->iov_len;

	ret = uk_random_hwrng_fill_buffer(buf, count);
	uio->uio_resid = count - ret;
	return ret;
}

static struct devops random_devops = {
	.open = dev_noop_open,
	.close = dev_noop_close,
	.read = dev_random_read,
	.write = dev_random_write,
	.ioctl = dev_noop_ioctl,
};

static struct driver drv_random = {
	.devops = &random_devops,
	.devsz = 0,
	.name = DEV_RANDOM_NAME
};

static struct driver drv_urandom = {
	.devops = &random_devops,
	.devsz = 0,
	.name = DEV_URANDOM_NAME
};

static struct devops hwrng_devops = {
	.open = dev_noop_open,
	.close = dev_noop_close,
	.read = dev_hwrng_read,
	.write = dev_random_write,
	.ioctl = dev_noop_ioctl,
};

static struct driver drv_hwrng = {
	.devops = &hwrng_devops,
	.devsz = 0,
	.name = DEV_HWRNG_NAME
};

static int devfs_register(struct uk_init_ctx *ictx __unused)
{
	int rc;

	uk_pr_info("Register '%s', %s, and '%s' to devfs\n",
		   DEV_URANDOM_NAME, DEV_RANDOM_NAME, DEV_HWRNG_NAME);

	/* register /dev/urandom */
	rc = device_create(&drv_urandom, DEV_URANDOM_NAME, D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to devfs: %d\n",
			  DEV_URANDOM_NAME, rc);
		return -rc;
	}

	/* register /dev/random */
	rc = device_create(&drv_random, DEV_RANDOM_NAME, D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to devfs: %d\n",
			  DEV_RANDOM_NAME, rc);
		return -rc;
	}

	/* register /dev/hwrng */
	rc = device_create(&drv_hwrng, DEV_HWRNG_NAME, D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register '%s' to devfs: %d\n",
			  DEV_HWRNG_NAME, rc);
		return -rc;
	}

	return 0;
}

devfs_initcall(devfs_register);

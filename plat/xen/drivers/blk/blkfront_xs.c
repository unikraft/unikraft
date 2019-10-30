/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#define _GNU_SOURCE /* for asprintf() */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <xenbus/client.h>
#include <xenbus/xs.h>
#include "blkfront_xb.h"

static int xs_read_backend_id(const char *nodename, domid_t *domid)
{
	char *path = NULL;
	int value, err;

	UK_ASSERT(nodename != NULL);

	err = asprintf(&path, "%s/backend-id", nodename);
	if (err <= 0) {
		uk_pr_err("Failed to allocate and format path: %d.\n", err);
		goto out;
	}

	err = xs_read_integer(XBT_NIL, path, &value);
	if (err == 0)
		*domid = (domid_t) value;

out:
	free(path);
	return err;
}

static int blkfront_xb_get_nb_max_queues(struct blkfront_dev *dev)
{
	int err = 0;
	struct xenbus_device *xendev;

	UK_ASSERT(dev != NULL);
	xendev = dev->xendev;

	err = xs_scanf(XBT_NIL, xendev->otherend, "multi-queue-max-queues",
				"%"PRIu16,
				&dev->nb_queues);
	if (err < 0) {
		uk_pr_err("Failed to read multi-queue-max-queues: %d\n", err);
		return err;
	}

	return 0;
}

int blkfront_xb_init(struct blkfront_dev *dev)
{
	struct xenbus_device *xendev;
	char *nodename;
	int err = 0;

	UK_ASSERT(dev != NULL);
	xendev = dev->xendev;

	err = xs_read_backend_id(xendev->nodename, &xendev->otherend_id);
	if (err)
		goto out;

	/* Get handle */
	nodename = strrchr(xendev->nodename, '/');
	if (!nodename) {
		err = -EINVAL;
		goto out;
	}

	dev->handle = strtoul(nodename + 1, NULL, 0);
	if (!dev->handle) {
		err = -EINVAL;
		goto out;
	}

	/* Read otherend path */
	xendev->otherend = xs_read(XBT_NIL, xendev->nodename, "backend");
	if (PTRISERR(xendev->otherend)) {
		uk_pr_err("Failed to read backend path: %d.\n", err);
		err = PTR2ERR(xendev->otherend);
		xendev->otherend = NULL;
		goto out;
	}

	err = blkfront_xb_get_nb_max_queues(dev);
	if (err) {
		uk_pr_err("Failed to read multi-queue-max-queues: %d\n", err);
		goto out;
	}
out:
	return err;
}

void blkfront_xb_fini(struct blkfront_dev *dev)
{
	struct xenbus_device *xendev;

	UK_ASSERT(dev != NULL);

	xendev = dev->xendev;
	if (xendev->otherend) {
		free(xendev->otherend);
		xendev->otherend = NULL;
	}
}

int blkfront_xb_write_nb_queues(struct blkfront_dev *dev)
{
	int err;
	struct xenbus_device *xendev;

	UK_ASSERT(dev);

	xendev = dev->xendev;
	err = xs_printf(XBT_NIL, xendev->nodename,
			"multi-queue-num-queues",
			"%u",
			dev->nb_queues);
	if (err < 0) {
		uk_pr_err("Failed to write multi-queue-num-queue: %d\n", err);
		goto out;
	}

	err = 0;

out:
	return err;
}

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <xenbus/xs.h>
#include <xenbus/client.h>

#include "9pfront_xb.h"

static int xs_read_backend_info(struct xenbus_device *xendev)
{
	int rc, val;
	char path[strlen(xendev->nodename) + sizeof("/backend-id")];

	/* Read backend id. */
	sprintf(path, "%s/backend-id", xendev->nodename);
	rc = xs_read_integer(XBT_NIL, path, &val);
	if (rc)
		goto out;
	xendev->otherend_id = (domid_t)val;

	/* Read backend path. */
	xendev->otherend = xs_read(XBT_NIL, xendev->nodename, "backend");
	if (PTRISERR(xendev->otherend)) {
		rc = PTR2ERR(xendev->otherend);
		xendev->otherend = NULL;
	}

out:
	return rc;
}

static int xs_read_backend_ring_info(struct xenbus_device *xendev,
				     int *nb_max_rings,
				     int *max_ring_page_order)
{
	int rc;
	char *int_str;

	/* Read max-rings. */
	int_str = xs_read(XBT_NIL, xendev->otherend, "max-rings");
	if (PTRISERR(int_str)) {
		rc = PTR2ERR(int_str);
		uk_pr_err("Error: %d\n", rc);
		goto out;
	}

	*nb_max_rings = strtol(int_str, NULL, 10);
	free(int_str);

	/* Read max-ring-page-order. */
	int_str = xs_read(XBT_NIL, xendev->otherend, "max-ring-page-order");
	if (PTRISERR(int_str)) {
		rc = PTR2ERR(int_str);
		uk_pr_err("Error: %d\n", rc);
		goto out;
	}

	*max_ring_page_order = strtol(int_str, NULL, 10);
	free(int_str);
	rc = 0;

out:
	return rc;
}

int p9front_xb_init(struct p9front_dev *p9fdev)
{
	struct xenbus_device *xendev;
	char *versions;
	int rc;

	UK_ASSERT(p9fdev != NULL);

	xendev = p9fdev->xendev;
	UK_ASSERT(xendev != NULL);

	/* Read backend node and backend id. */
	rc = xs_read_backend_info(xendev);
	if (rc) {
		uk_pr_err("Error initializing backend node and id.\n");
		goto out;
	}

	/* Check versions string. */
	versions = xs_read(XBT_NIL, xendev->otherend, "versions");
	if (PTRISERR(versions)) {
		uk_pr_err("Error reading backend version information.\n");
		rc = PTR2ERR(versions);
		goto out;
	}

	if (strcmp(versions, "1")) {
		uk_pr_err("Backend does not support xen protocol version 1.\n");
		free(versions);
		rc = -EINVAL;
		goto out;
	}
	free(versions);

	/* Read ring information. */
	rc = xs_read_backend_ring_info(xendev, &p9fdev->nb_max_rings,
				       &p9fdev->max_ring_page_order);
	if (rc) {
		uk_pr_err("Error reading backend ring information.\n");
		goto out;
	}

	/* Read tag. */
	p9fdev->tag = xs_read(XBT_NIL, xendev->nodename, "tag");
	if (PTRISERR(p9fdev->tag)) {
		uk_pr_err("Error reading 9pfs mount tag.\n");
		rc = PTR2ERR(p9fdev->tag);
		p9fdev->tag = NULL;
	}

out:
	return rc;
}

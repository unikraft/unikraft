/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/bus.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/assert.h>
#include <xenbus/xenbus.h>

static struct xenbus_handler xbh;


/* Helper functions for Xenbus related allocations */
void *uk_xb_malloc(size_t size)
{
	UK_ASSERT(xbh.a != NULL);
	return uk_malloc(xbh.a, size);
}

void *uk_xb_calloc(size_t nmemb, size_t size)
{
	UK_ASSERT(xbh.a != NULL);
	return uk_calloc(xbh.a, nmemb, size);
}

void uk_xb_free(void *ptr)
{
	UK_ASSERT(xbh.a != NULL);
	uk_free(xbh.a, ptr);
}

static int xenbus_probe(void)
{
	int err = 0;

	uk_printd(DLVL_INFO, "Probe Xenbus\n");

	/* TODO */

	return err;
}

static int xenbus_init(struct uk_alloc *a)
{
	struct xenbus_driver *drv, *drv_next;
	int ret = 0;

	UK_ASSERT(a != NULL);

	xbh.a = a;

	UK_TAILQ_FOREACH_SAFE(drv, &xbh.drv_list, next, drv_next) {
		if (drv->init) {
			ret = drv->init(a);
			if (ret == 0)
				continue;
			uk_printd(DLVL_ERR,
				"Failed to initialize driver %p: %d\n",
				drv, ret);
			UK_TAILQ_REMOVE(&xbh.drv_list, drv, next);
		}
	}

	return 0;
}

void _xenbus_register_driver(struct xenbus_driver *drv)
{
	UK_ASSERT(drv != NULL);
	UK_TAILQ_INSERT_TAIL(&xbh.drv_list, drv, next);
}

/*
 * Register this bus driver to libukbus:
 */
static struct xenbus_handler xbh = {
	.b.init  = xenbus_init,
	.b.probe = xenbus_probe,
	.drv_list = UK_TAILQ_HEAD_INITIALIZER(xbh.drv_list),
	.dev_list = UK_TAILQ_HEAD_INITIALIZER(xbh.dev_list),
};

UK_BUS_REGISTER(&xbh.b);

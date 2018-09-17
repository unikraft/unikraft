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
/* Internal API for Xenstore watches */

#include <stdio.h>
#include <string.h>
#include <uk/errptr.h>
#include "xs_watch.h"

/* Watches list */
static struct xenbus_watch_list xs_watch_list =
	UK_TAILQ_HEAD_INITIALIZER(xs_watch_list);

static int xs_watch_info_equal(const struct xs_watch_info *xswi,
	const char *path, const char *token)
{
	return (strcmp(xswi->path, path) == 0 &&
		strcmp(xswi->token, token) == 0);
}

struct xs_watch *xs_watch_create(const char *path)
{
	struct xs_watch *xsw;
	const int token_size = sizeof(xsw) * 2 + 1;
	char *tmpstr;
	int stringlen;

	UK_ASSERT(path != NULL);

	stringlen = token_size + strlen(path) + 1;

	xsw = uk_xb_malloc(sizeof(*xsw) + stringlen);
	if (!xsw)
		return ERR2PTR(-ENOMEM);

	ukarch_spin_lock_init(&xsw->base.lock);
	xsw->base.pending_events = 0;
	uk_waitq_init(&xsw->base.wq);

	/* set path */
	tmpstr = (char *) (xsw + 1);
	strcpy(tmpstr, path);
	xsw->xs.path = tmpstr;

	/* set token (watch address as string) */
	tmpstr += strlen(path) + 1;
	sprintf(tmpstr, "%lx", (long) xsw);
	xsw->xs.token = tmpstr;

	UK_TAILQ_INSERT_HEAD(&xs_watch_list, &xsw->base, watch_list);

	return xsw;
}

int xs_watch_destroy(struct xs_watch *watch)
{
	struct xenbus_watch *xbw;
	struct xs_watch *xsw;
	int err = -ENOENT;

	UK_ASSERT(watch != NULL);

	UK_TAILQ_FOREACH(xbw, &xs_watch_list, watch_list) {
		xsw = __containerof(xbw, struct xs_watch, base);

		if (xsw == watch) {
			UK_TAILQ_REMOVE(&xs_watch_list, xbw, watch_list);
			uk_xb_free(xsw);
			err = 0;
			break;
		}
	}

	return err;
}

struct xs_watch *xs_watch_find(const char *path, const char *token)
{
	struct xenbus_watch *xbw;
	struct xs_watch *xsw;

	UK_TAILQ_FOREACH(xbw, &xs_watch_list, watch_list) {
		xsw = __containerof(xbw, struct xs_watch, base);

		if (xs_watch_info_equal(&xsw->xs, path, token))
			return xsw;
	}

	return NULL;
}

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

#ifndef __XS_WATCH_H__
#define __XS_WATCH_H__

#include <xenbus/xenbus.h>

/* Xenstore watch info */
struct xs_watch_info {
	/**< Watched Xenstore path */
	char *path;
	/**< Watch identification token */
	char *token;
};

/* Xenstore watch */
struct xs_watch {
	struct xenbus_watch base;
	struct xs_watch_info xs;
};

/*
 * Create a Xenstore watch associated with a path.
 *
 * @param path Xenstore path
 * @return On success, returns a malloc'd Xenstore watch. On error, returns
 * a negative error number which should be checked using PTRISERR.
 */
struct xs_watch *xs_watch_create(const char *path);

/*
 * Destroy a previously created Xenstore watch.
 *
 * @param watch Xenstore watch
 * @return 0 on success, a negative errno value on error.
 */
int xs_watch_destroy(struct xs_watch *watch);

/*
 * Returns the Xenstore watch associated with path and token.
 *
 * @param path Watched path
 * @param token Watch token
 * @return On success returns the found watch. On error, returns NULL.
 */
struct xs_watch *xs_watch_find(const char *path, const char *token);

#endif /* __XS_WATCH_H__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Steven Smith (sos22@cam.ac.uk)
 *          Grzegorz Milos (gm281@cam.ac.uk)
 *          John D. Ramsdell
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2006, Cambridge University
 *               2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
/*
 * Ported from Mini-OS xenbus.c
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <uk/errptr.h>
#include <xen/io/xs_wire.h>
#include <xenbus/xs.h>
#include "xs_watch.h"
#include "xs_comms.h"


/* Helper macros for initializing xs requests from strings */
#define XS_IOVEC_STR_NULL(str) \
	((struct xs_iovec) { str, strlen(str) + 1 })
#define XS_IOVEC_STR(str) \
	((struct xs_iovec) { str, strlen(str) })


/* Common function used for sending requests when replies aren't handled */
static inline int xs_msg(enum xsd_sockmsg_type type, xenbus_transaction_t xbt,
		struct xs_iovec *reqs, int reqs_num)
{
	return xs_msg_reply(type, xbt, reqs, reqs_num, NULL);
}

char *xs_read(xenbus_transaction_t xbt, const char *path, const char *node)
{
	struct xs_iovec req, rep;
	char *fullpath, *value;
	int err;

	if (path == NULL)
		return ERR2PTR(-EINVAL);

	if (node != NULL) {
		err = asprintf(&fullpath, "%s/%s", path, node);
		if (err < 0) {
			value = ERR2PTR(-ENOMEM);
			goto out;
		}
	} else
		fullpath = (char *) path;

	req = XS_IOVEC_STR_NULL(fullpath);
	err = xs_msg_reply(XS_READ, xbt, &req, 1, &rep);
	if (err == 0)
		value = rep.data;
	else
		value = ERR2PTR(err);

	if (node != NULL)
		free(fullpath);
out:
	return value;
}

int xs_write(xenbus_transaction_t xbt, const char *path, const char *node,
	const char *value)
{
	struct xs_iovec req[2];
	char *fullpath;
	int err;

	if (path == NULL || value == NULL)
		return -EINVAL;

	if (node != NULL) {
		err = asprintf(&fullpath, "%s/%s", path, node);
		if (err < 0) {
			err = -ENOMEM;
			goto out;
		}
	} else
		fullpath = (char *) path;

	req[0] = XS_IOVEC_STR_NULL(fullpath);
	req[1] = XS_IOVEC_STR((char *) value);

	err = xs_msg(XS_WRITE, xbt, req, ARRAY_SIZE(req));

	if (node != NULL)
		free(fullpath);
out:
	return err;
}

/* Returns an array of strings out of the serialized reply */
static char **reply_to_string_array(struct xs_iovec *rep, int *size)
{
	int strings_num, offs, i;
	char *rep_strings, *strings, **res = NULL;

	rep_strings = rep->data;

	/* count the strings */
	for (offs = strings_num = 0; offs < (int) rep->len; offs++)
		strings_num += (rep_strings[offs] == 0);

	/* one alloc for both string addresses and contents */
	res = malloc((strings_num + 1) * sizeof(char *) + rep->len);
	if (!res)
		return ERR2PTR(-ENOMEM);

	/* copy the strings to the end of the array */
	strings = (char *) &res[strings_num + 1];
	memcpy(strings, rep_strings, rep->len);

	/* fill the string array */
	for (offs = i = 0; i < strings_num; i++) {
		char *string = strings + offs;
		int string_len = strlen(string);

		res[i] = string;

		offs += string_len + 1;
	}
	res[i] = NULL;

	if (size)
		*size = strings_num;

	return res;
}

char **xs_ls(xenbus_transaction_t xbt, const char *path)
{
	struct xs_iovec req, rep;
	char **res = NULL;
	int err;

	if (path == NULL)
		return ERR2PTR(-EINVAL);

	req = XS_IOVEC_STR_NULL((char *) path);
	err = xs_msg_reply(XS_DIRECTORY, xbt, &req, 1, &rep);
	if (err)
		return ERR2PTR(err);

	res = reply_to_string_array(&rep, NULL);
	free(rep.data);

	return res;
}

int xs_rm(xenbus_transaction_t xbt, const char *path)
{
	struct xs_iovec req;

	if (path == NULL)
		return -EINVAL;

	req = XS_IOVEC_STR_NULL((char *) path);

	return xs_msg(XS_RM, xbt, &req, 1);
}

/*
 * Permissions
 */

static const char xs_perm_tbl[] = {
	[XS_PERM_NONE]    = 'n',
	[XS_PERM_READ]    = 'r',
	[XS_PERM_WRITE]   = 'w',
	[XS_PERM_BOTH]    = 'b',
};

int xs_char_to_perm(char c, enum xs_perm *perm)
{
	int err = -EINVAL;

	if (perm == NULL)
		goto out;

	for (int i = 0; i < (int) ARRAY_SIZE(xs_perm_tbl); i++) {
		if (c == xs_perm_tbl[i]) {
			*perm = i;
			err = 0;
			break;
		}
	}

out:
	return err;
}

int xs_perm_to_char(enum xs_perm perm, char *c)
{
	if (c == NULL || perm >= ARRAY_SIZE(xs_perm_tbl))
		return -EINVAL;

	*c = xs_perm_tbl[perm];

	return 0;
}

int xs_str_to_perm(const char *str, domid_t *domid, enum xs_perm *perm)
{
	int err = 0;

	if (str == NULL || domid == NULL || perm == NULL) {
		err = -EINVAL;
		goto out;
	}

	err = xs_char_to_perm(str[0], perm);
	if (err)
		goto out;

	*domid = (domid_t) strtoul(&str[1], NULL, 10);

out:
	return err;
}

#define PERM_MAX_SIZE 32
char *xs_perm_to_str(domid_t domid, enum xs_perm perm)
{
	int err = 0;
	char permc, value[PERM_MAX_SIZE];

	err = xs_perm_to_char(perm, &permc);
	if (err)
		return NULL;

	snprintf(value, PERM_MAX_SIZE, "%c%hu", permc, domid);

	return strdup(value);
}

/*
 * Returns the ACL for input path. An extra number of empty entries may be
 * requested if caller intends to extend the list.
 */
static struct xs_acl *__xs_get_acl(xenbus_transaction_t xbt, const char *path,
	int extra)
{
	struct xs_acl *acl = NULL;
	struct xs_iovec req, rep;
	char **values;
	int values_num, err;

	if (path == NULL) {
		err = EINVAL;
		goto out;
	}

	req = XS_IOVEC_STR_NULL((char *) path);
	err = xs_msg_reply(XS_GET_PERMS, xbt, &req, 1, &rep);
	if (err)
		goto out;

	values = reply_to_string_array(&rep, &values_num);
	free(rep.data);
	if (PTRISERR(values)) {
		err = PTR2ERR(values);
		goto out;
	}

	acl = malloc(sizeof(struct xs_acl) +
		(values_num + extra) * sizeof(struct xs_acl_entry));
	if (acl == NULL) {
		err = ENOMEM;
		goto out_values;
	}

	/* set owner id and permissions for others */
	err = xs_str_to_perm(values[0],
		&acl->ownerid, &acl->others_perm);
	if (err)
		goto out_values;

	/* set ACL entries */
	acl->entries_num = values_num - 1;
	for (int i = 0; i < acl->entries_num; i++) {
		err = xs_str_to_perm(values[i + 1],
			&acl->entries[i].domid, &acl->entries[i].perm);
		if (err)
			goto out_values;
	}

out_values:
	free(values);
out:
	if (err) {
		if (acl)
			free(acl);
		acl = ERR2PTR(err);
	}
	return acl;
}

struct xs_acl *xs_get_acl(xenbus_transaction_t xbt, const char *path)
{
	return __xs_get_acl(xbt, path, 0);
}

int xs_set_acl(xenbus_transaction_t xbt, const char *path, struct xs_acl *acl)
{
	struct xs_iovec req[2 + acl->entries_num];
	char *s;
	int i, err;

	if (path == NULL || acl == NULL) {
		err = -EINVAL;
		goto out;
	}

	req[0] = XS_IOVEC_STR_NULL((char *) path);

	s = xs_perm_to_str(acl->ownerid, acl->others_perm);
	if (s == NULL) {
		err = -EINVAL;
		goto out;
	}

	req[1] = XS_IOVEC_STR_NULL(s);

	for (i = 0; i < acl->entries_num; i++) {
		struct xs_acl_entry *acle = &acl->entries[i];

		s = xs_perm_to_str(acle->domid, acle->perm);
		if (s == NULL) {
			err = -EINVAL;
			goto out_req;
		}

		req[i + 2] = XS_IOVEC_STR_NULL(s);
	}

	err = xs_msg(XS_SET_PERMS, xbt, req, ARRAY_SIZE(req));

out_req:
	for (i--; i > 0; i--)
		free(req[i].data);
out:
	return err;
}

int xs_get_perm(xenbus_transaction_t xbt, const char *path,
	domid_t domid, enum xs_perm *perm)
{
	struct xs_acl *acl;
	int err = 0;

	if (perm == NULL) {
		err = -EINVAL;
		goto out;
	}

	acl = xs_get_acl(xbt, path);
	if (PTRISERR(acl)) {
		err = PTR2ERR(acl);
		goto out;
	}

	if (acl->ownerid == domid) {
		*perm = XS_PERM_BOTH;
		goto out_acl;
	}

	for (int i = 0; i < acl->entries_num; i++) {
		struct xs_acl_entry *acle = &acl->entries[i];

		if (acle->domid == domid) {
			*perm = acle->perm;
			goto out_acl;
		}
	}

	*perm = acl->others_perm;

out_acl:
	free(acl);
out:
	return err;
}

static int acl_find_entry_index(struct xs_acl *acl, domid_t domid)
{
	struct xs_acl_entry *acle;
	int i;

	if (acl->ownerid == domid)
		/*
		 * let's say the function isn't called correctly considering
		 * that the owner domain has all the rights, all the time
		 */
		return -EINVAL;

	for (i = 0; i < acl->entries_num; i++) {
		acle = &acl->entries[i];
		if (acle->domid == domid)
			break;
	}

	if (i == acl->entries_num)
		/* no entry found for domid */
		return -ENOENT;

	return i;
}

int xs_set_perm(xenbus_transaction_t xbt, const char *path,
	domid_t domid, enum xs_perm perm)
{
	struct xs_acl *acl;
	struct xs_acl_entry *acle;
	int err, idx;

	UK_ASSERT(xbt != XBT_NIL);

	/* one extra entry in case a new one will be added */
	acl = __xs_get_acl(xbt, path, 1);
	if (PTRISERR(acl)) {
		err = PTR2ERR(acl);
		goto out;
	}

	idx = acl_find_entry_index(acl, domid);
	if (idx == -ENOENT) {
		/* new entry */
		acle = &acl->entries[acl->entries_num];
		acle->domid = domid;
		acle->perm = perm;
		acl->entries_num++;

	} else if (idx < 0) {
		/* some other error */
		err = idx;
		goto out_acl;

	} else {
		/* update entry */
		acle = &acl->entries[idx];
		acle->perm = perm;
	}

	err = xs_set_acl(xbt, path, acl);

out_acl:
	free(acl);
out:
	return err;
}

int xs_del_perm(xenbus_transaction_t xbt, const char *path,
	domid_t domid)
{
	struct xs_acl *acl;
	int idx, err = 0;

	UK_ASSERT(xbt != XBT_NIL);

	acl = __xs_get_acl(xbt, path, 0);
	if (PTRISERR(acl)) {
		err = PTR2ERR(acl);
		goto out;
	}

	idx = acl_find_entry_index(acl, domid);
	if (idx < 0) {
		err = idx;
		goto out_acl;
	}

	/* remove entry */
	acl->entries_num--;
	memmove(&acl->entries[idx], &acl->entries[idx + 1],
		(acl->entries_num - idx) * sizeof(struct xs_acl_entry));

	err = xs_set_acl(xbt, path, acl);

out_acl:
	free(acl);
out:
	return err;
}

/*
 * Watches
 */

struct xenbus_watch *xs_watch_path(xenbus_transaction_t xbt, const char *path)
{
	struct xs_watch *xsw;
	struct xs_iovec req[2];
	int err;

	if (path == NULL)
		return ERR2PTR(-EINVAL);

	xsw = xs_watch_create(path);
	if (PTRISERR(xsw))
		return (struct xenbus_watch *) xsw;

	req[0] = XS_IOVEC_STR_NULL(xsw->xs.path);
	req[1] = XS_IOVEC_STR_NULL(xsw->xs.token);

	err = xs_msg(XS_WATCH, xbt, req, ARRAY_SIZE(req));
	if (err) {
		xs_watch_destroy(xsw);
		return ERR2PTR(err);
	}

	return &xsw->base;
}

int xs_unwatch(xenbus_transaction_t xbt, struct xenbus_watch *watch)
{
	struct xs_watch *xsw, *_xsw;
	struct xs_iovec req[2];
	int err;

	if (watch == NULL) {
		err = -EINVAL;
		goto out;
	}

	xsw = __containerof(watch, struct xs_watch, base);

	_xsw = xs_watch_find(xsw->xs.path, xsw->xs.token);
	if (_xsw != xsw) {
		/* this watch was not registered */
		err = -ENOENT;
		goto out;
	}

	req[0] = XS_IOVEC_STR_NULL(xsw->xs.path);
	req[1] = XS_IOVEC_STR_NULL(xsw->xs.token);

	err = xs_msg(XS_UNWATCH, xbt, req, ARRAY_SIZE(req));
	if (err)
		goto out;

	err = xs_watch_destroy(xsw);

out:
	return err;
}

/*
 * Transactions
 */

int xs_transaction_start(xenbus_transaction_t *xbt)
{
	/*
	 * xenstored becomes angry if you send a length 0 message,
	 * so just shove a nul terminator on the end
	 */
	struct xs_iovec req, rep;
	int err;

	if (xbt == NULL)
		return -EINVAL;

	req = XS_IOVEC_STR_NULL("");
	err = xs_msg_reply(XS_TRANSACTION_START, 0, &req, 1, &rep);
	if (err)
		return err;

	*xbt = strtoul(rep.data, NULL, 10);
	free(rep.data);

	return err;
}

int xs_transaction_end(xenbus_transaction_t xbt, int abort)
{
	struct xs_iovec req;

	req.data = abort ? "F" : "T";
	req.len = 2;

	return xs_msg(XS_TRANSACTION_END, xbt, &req, 1);
}

/*
 * Misc
 */

/* Send a debug message to xenbus. Can block. */
int xs_debug_msg(const char *msg)
{
	struct xs_iovec req[3], rep;
	int err;

	if (msg == NULL)
		return -EINVAL;

	req[0] = XS_IOVEC_STR_NULL("print");
	req[1] = XS_IOVEC_STR((char *) msg);
	req[2] = XS_IOVEC_STR_NULL("");

	err = xs_msg_reply(XS_DEBUG, XBT_NIL, req, ARRAY_SIZE(req), &rep);
	if (err)
		goto out;

	uk_pr_debug("Got a debug reply %s\n", (char *) rep.data);
	free(rep.data);

out:
	return err;
}

int xs_read_integer(xenbus_transaction_t xbt, const char *path, int *value)
{
	char *value_str;

	if (path == NULL || value == NULL)
		return -EINVAL;

	value_str = xs_read(xbt, path, NULL);
	if (PTRISERR(value_str))
		return PTR2ERR(value_str);

	*value = atoi(value_str);

	free(value_str);

	return 0;
}

int xs_scanf(xenbus_transaction_t xbt, const char *dir, const char *node,
	const char *fmt, ...)
{
	char *val;
	va_list args;
	int err = 0;

	if (fmt == NULL)
		return -EINVAL;

	val = xs_read(xbt, dir, node);
	if (PTRISERR(val)) {
		err = PTR2ERR(val);
		goto out;
	}

	va_start(args, fmt);
	err = vsscanf(val, fmt, args);
	va_end(args);

	free(val);

out:
	return err;
}

int xs_printf(xenbus_transaction_t xbt, const char *dir, const char *node,
	const char *fmt, ...)
{
#define VAL_SIZE 256
	char val[VAL_SIZE];
	va_list args;
	int err = 0, _err;

	if (fmt == NULL)
		return -EINVAL;

	va_start(args, fmt);
	_err = vsnprintf(val, VAL_SIZE, fmt, args);
	va_end(args);

	/* send to Xenstore if vsnprintf was successful */
	if (_err > 0)
		err = xs_write(xbt, dir, node, val);

	/*
	 * if message sent to Xenstore was successful,
	 * return the number of characters
	 */
	if (err == 0)
		err = _err;

	return err;
}

domid_t xs_get_self_id(void)
{
	char *domid_str;
	domid_t domid;

	domid_str = xs_read(XBT_NIL, "domid", NULL);
	if (PTRISERR(domid_str))
		UK_CRASH("Error reading domain id.");

	domid = (domid_t) strtoul(domid_str, NULL, 10);

	free(domid_str);

	return domid;
}

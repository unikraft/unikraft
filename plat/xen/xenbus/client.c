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
 * Client interface between the device and the Xenbus driver.
 * Ported from Mini-OS xenbus.c
 */

#include <stdio.h>
#include <string.h>
#include <uk/errptr.h>
#include <uk/wait.h>
#include <xenbus/xs.h>
#include <xenbus/client.h>


#define XENBUS_STATE_ENTRY(name) \
	[XenbusState##name] = #name

static const char *const xb_state_tbl[] = {
	XENBUS_STATE_ENTRY(Unknown),
	XENBUS_STATE_ENTRY(Initialising),
	XENBUS_STATE_ENTRY(InitWait),
	XENBUS_STATE_ENTRY(Initialised),
	XENBUS_STATE_ENTRY(Connected),
	XENBUS_STATE_ENTRY(Closing),
	XENBUS_STATE_ENTRY(Closed),
	XENBUS_STATE_ENTRY(Reconfiguring),
	XENBUS_STATE_ENTRY(Reconfigured),
};

const char *xenbus_state_to_str(XenbusState state)
{
	return (state < ARRAY_SIZE(xb_state_tbl)) ?
		xb_state_tbl[state] : "INVALID";
}

#define XENBUS_DEVTYPE_ENTRY(name) \
	[xenbus_dev_##name] = #name

static const char *const xb_devtype_tbl[] = {
	XENBUS_DEVTYPE_ENTRY(none),
	XENBUS_DEVTYPE_ENTRY(vbd),
	XENBUS_DEVTYPE_ENTRY(9pfs),
};

const char *xenbus_devtype_to_str(enum xenbus_dev_type devtype)
{
	return (devtype < ARRAY_SIZE(xb_devtype_tbl)) ?
		xb_devtype_tbl[devtype] : "INVALID";
}

enum xenbus_dev_type xenbus_str_to_devtype(const char *devtypestr)
{
	for (int i = 0; i < (int) ARRAY_SIZE(xb_devtype_tbl); i++) {
		if (!strcmp(xb_devtype_tbl[i], devtypestr))
			return (enum xenbus_dev_type) i;
	}

	return xenbus_dev_none;
}

/*
 * Watches
 */

int xenbus_watch_wait_event(struct xenbus_watch *watch)
{
	if (watch == NULL)
		return -EINVAL;

	while (1) {
		ukarch_spin_lock(&watch->lock);

		if (watch->pending_events > 0)
			break;

		ukarch_spin_unlock(&watch->lock);

		uk_waitq_wait_event(&watch->wq,
			(watch->pending_events > 0));
	}

	watch->pending_events--;
	ukarch_spin_unlock(&watch->lock);

	return 0;
}

int xenbus_watch_notify_event(struct xenbus_watch *watch)
{
	if (watch == NULL)
		return -EINVAL;

	ukarch_spin_lock(&watch->lock);
	watch->pending_events++;
	uk_waitq_wake_up(&watch->wq);
	ukarch_spin_unlock(&watch->lock);

	return 0;
}

XenbusState xenbus_read_driver_state(const char *path)
{
	char state_path[strlen(path) + sizeof("/state")];
	XenbusState state = XenbusStateUnknown;

	sprintf(state_path, "%s/state", path);
	xs_read_integer(XBT_NIL, state_path, (int *) &state);

	return state;
}

int xenbus_switch_state(xenbus_transaction_t xbt,
	struct xenbus_device *xendev, XenbusState state)
{
	char state_path[strlen(xendev->nodename) + sizeof("/state")];
	char new_state_str[2];
	XenbusState crnt_state;
	int need_transaction_end = 0; /* non-zero if local transaction */
	int abort;
	int err;

	if (xendev == NULL)
		return -EINVAL;

	sprintf(state_path, "%s/state", xendev->nodename);

	do {
		abort = 1;

		if (xbt == XBT_NIL) {
			err = xs_transaction_start(&xbt);
			if (err)
				goto exit;
			need_transaction_end = 1;
		}

		/* check if state is already set */
		err = xs_read_integer(xbt, xendev->nodename,
			(int *) &crnt_state);
		if (err || crnt_state == state)
			goto exit;

		/* set new state */
		snprintf(new_state_str, sizeof(new_state_str), "%d", state);
		err = xs_write(xbt, state_path, NULL, new_state_str);

		abort = 0;
exit:
		if (need_transaction_end) {
			int _err;

			_err = xs_transaction_end(xbt, abort);
			if (!err)
				err = _err;
			xbt = XBT_NIL;
		}
	} while (err == -EAGAIN);

	if (err)
		uk_pr_err("Error switching state to %s: %d\n",
			xenbus_state_to_str(state), err);

	return err;
}

int xenbus_wait_for_state_change(const char *path, XenbusState *state,
	struct xenbus_watch *watch)
{
	XenbusState crnt_state;
	int err = 0, watch_is_local = 0;

	if (path == NULL || state == NULL) {
		err = -EINVAL;
		goto out;
	}

	if (watch == NULL) {
		/* create a local watch */
		watch = xs_watch_path(XBT_NIL, path);
		if (PTRISERR(watch)) {
			err = PTR2ERR(watch);
			goto out;
		}
		watch_is_local = 1;
	}

	for (;;) {
		err = xs_read_integer(XBT_NIL, path, (int *) &crnt_state);
		if (err)
			break;

		if (crnt_state != *state) {
			*state = crnt_state;
			break;
		}

		xenbus_watch_wait_event(watch);
	}

out:
	if (watch_is_local)
		xs_unwatch(XBT_NIL, watch);

	return err;
}

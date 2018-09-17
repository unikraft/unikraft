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
/*
 * Client interface between the device and the Xenbus driver.
 * Ported from Mini-OS xenbus.c
 */

#ifndef __XENBUS_CLIENT_H__
#define __XENBUS_CLIENT_H__

#include <xenbus/xenbus.h>

/*
 * Returns the name of the state for tracing/debugging purposes.
 *
 * @param state The Xenbus state
 * @return A string representing the state name
 */
const char *xenbus_state_to_str(XenbusState state);

/*
 * Converts a device type value to name
 *
 * @param devtype The Xenbus device type
 * @return A string representing the device type name
 */
const char *xenbus_devtype_to_str(enum xenbus_dev_type devtype);

/*
 * Converts a device type name to value
 *
 * @param devtypestr The Xenbus device type name
 * @return The Xenbus device type
 */
enum xenbus_dev_type xenbus_str_to_devtype(const char *devtypestr);


/*
 * Watches
 */

/*
 * Waits for a watch event. Called by a client driver.
 *
 * @param watch Xenbus watch
 * @return 0 on success, a negative errno value on error.
 */
int xenbus_watch_wait_event(struct xenbus_watch *watch);

/*
 * Notifies a client driver waiting for watch events.
 *
 * @param watch Xenbus watch
 * @return 0 on success, a negative errno value on error.
 */
int xenbus_watch_notify_event(struct xenbus_watch *watch);

/*
 * Driver states
 */

/*
 * Returns the driver state found at the given Xenstore path.
 *
 * @param path Xenstore path
 * @return The Xenbus driver state
 */
XenbusState xenbus_read_driver_state(const char *path);

/*
 * Changes the state of a Xen PV driver
 *
 * @param xbt Xenbus transaction id
 * @param xendev Xenbus device
 * @param state The new Xenbus state
 * @return 0 on success, a negative errno value on error.
 */
int xenbus_switch_state(xenbus_transaction_t xbt,
	struct xenbus_device *xendev, XenbusState state);

/*
 * Waits for the driver state found at the given Xenstore path to change by
 * using watches.
 *
 * @param path Xenstore path
 * @param state The returned Xenbus state
 * @param watch Xenbus watch. It may be NULL, in which case a local watch
 * will be created.
 * @return 0 on success, a negative errno value on error.
 */
int xenbus_wait_for_state_change(const char *path, XenbusState *state,
	struct xenbus_watch *watch);

#endif /* __XENBUS_CLIENT_H__ */

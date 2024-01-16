/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2020, University Politehnica of Bucharest. All rights reserved.
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
#define _GNU_SOURCE
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/xenbus/xs.h>
#include <uk/xenbus/client.h>
#include "netfront_xb.h"


static int netfront_xb_wait_be_connect(struct netfront_dev *nfdev);
static int netfront_xb_wait_be_disconnect(struct netfront_dev *nfdev);


static int xs_read_backend_id(const char *nodename, domid_t *domid)
{
	char path[strlen(nodename) + sizeof("/backend-id")];
	int value, rc;

	snprintf(path, sizeof(path), "%s/backend-id", nodename);

	rc = xs_read_integer(XBT_NIL, path, &value);
	if (!rc)
		*domid = (domid_t) value;

	return rc;
}

static void xs_econf_fini(struct xs_econf *econf,
		struct uk_alloc *a)
{
	if (econf->ipv4addr) {
		uk_free(a, econf->ipv4addr);
		econf->ipv4addr = NULL;
	}
	if (econf->ipv4mask) {
		uk_free(a, econf->ipv4mask);
		econf->ipv4mask = NULL;
	}
	if (econf->ipv4gw) {
		uk_free(a, econf->ipv4gw);
		econf->ipv4gw = NULL;
	}
}

static int xs_econf_init(struct xs_econf *econf, char *ip_str,
		struct uk_alloc *a)
{
	int rc = -1;
	char *ip_addr = NULL, *ip_gw_str = NULL, *ip_mask_str = NULL;

	/* IP */
	ip_addr = strtok(ip_str, " ");
	if (ip_addr == NULL)
		goto out_err;
	econf->ipv4addr = uk_malloc(a, strlen(ip_addr) + 1);
	if (!econf->ipv4addr) {
		uk_pr_err("Error allocating ip config.\n");
		goto out_err;
	}
	memcpy(econf->ipv4addr, ip_addr, strlen(ip_addr) + 1);

	/* Mask */
	ip_mask_str = strtok(NULL, " ");
	if (ip_mask_str == NULL)
		goto out_err;
	econf->ipv4mask = uk_malloc(a, strlen(ip_mask_str) + 1);
	if (!econf->ipv4mask) {
		uk_pr_err("Error allocating ip mask config.\n");
		goto out_err;
	}
	memcpy(econf->ipv4mask, ip_mask_str, strlen(ip_mask_str) + 1);

	/* Gateway */
	ip_gw_str = strtok(NULL, " ");
	if (ip_gw_str == NULL)
		goto out_err;
	econf->ipv4gw = uk_malloc(a, strlen(ip_gw_str) + 1);
	if (!econf->ipv4gw) {
		uk_pr_err("Error allocating ip gateway config.\n");
		goto out_err;
	}
	memcpy(econf->ipv4gw, ip_gw_str, strlen(ip_gw_str) + 1);

	rc = 0;
out:
	return rc;
out_err:
	xs_econf_fini(econf, a);
	goto out;
}

int netfront_xb_init(struct netfront_dev *nfdev, struct uk_alloc *a)
{
	struct xenbus_device *xendev;
	char *mac_str, *p, *ip_str, *int_str;
	int rc;

	UK_ASSERT(nfdev != NULL);

	xendev = nfdev->xendev;
	UK_ASSERT(xendev != NULL);
	UK_ASSERT(xendev->nodename != NULL);

	rc = xs_read_backend_id(xendev->nodename, &xendev->otherend_id);
	if (rc)
		goto out;

	/* read backend path */
	xendev->otherend = xs_read(XBT_NIL, xendev->nodename, "backend");
	if (PTRISERR(xendev->otherend)) {
		uk_pr_err("Error reading backend path.\n");
		rc = PTR2ERR(xendev->otherend);
		xendev->otherend = NULL;
		goto out;
	}

	/* read MAC address */
	mac_str = xs_read(XBT_NIL, xendev->nodename, "mac");
	if (PTRISERR(mac_str)) {
		uk_pr_err("Error reading MAC address.\n");
		rc = PTR2ERR(mac_str);
		goto no_conf;
	}
	uk_pr_debug("\tMAC via XenStore: %s\n", mac_str);

	p = mac_str;
	for (int i = 0; i < UK_NETDEV_HWADDR_LEN; i++) {
		nfdev->hw_addr.addr_bytes[i] = (uint8_t) strtoul(p, &p, 16);
		p++;
	}
	free(mac_str);

	/* read IP address */
	ip_str = xs_read(XBT_NIL, xendev->otherend, "ip");
	if (PTRISERR(ip_str)) {
		uk_pr_debug("No IP address information found on XenStore\n");
		memset(&nfdev->econf, 0, sizeof(struct xs_econf));
	} else {
		uk_pr_debug("\tIP via XenStore: %s\n", ip_str);
		rc = xs_econf_init(&nfdev->econf, ip_str, a);
		free(ip_str);
		if (rc)
			goto no_conf;
	}

	/* maximum queues number */
	int_str = xs_read(XBT_NIL, xendev->otherend,
		"multi-queue-max-queues");
	if (!PTRISERR(int_str)) {
		nfdev->max_queue_pairs = (uint16_t) strtoul(int_str, NULL, 10);
		free(int_str);
	}

	/* spit event channels */
	int_str = xs_read(XBT_NIL, xendev->otherend,
		"feature-split-event-channels");
	if (!PTRISERR(int_str)) {
		nfdev->split_evtchn = (bool) strtoul(int_str, NULL, 10);
		free(int_str);
	}

	/* TODO netmap */

out:
	return rc;
no_conf:
	if (xendev->otherend) {
		free(xendev->otherend);
		xendev->otherend = NULL;
	}
	goto out;
}

void netfront_xb_fini(struct netfront_dev *nfdev, struct uk_alloc *a)
{
	struct xenbus_device *xendev;

	UK_ASSERT(nfdev != NULL);

	xendev = nfdev->xendev;
	UK_ASSERT(xendev != NULL);

	xs_econf_fini(&nfdev->econf, a);

	if (xendev->otherend) {
		free(xendev->otherend);
		xendev->otherend = NULL;
	}
}

static int xs_write_queue(struct netfront_dev *nfdev, uint16_t queue_id,
		xenbus_transaction_t xbt, int write_hierarchical)
{
	struct xenbus_device *xendev = nfdev->xendev;
	struct uk_netdev_tx_queue *txq = &nfdev->txqs[queue_id];
	struct uk_netdev_rx_queue *rxq = &nfdev->rxqs[queue_id];
	char *path;
	int rc;

	if (write_hierarchical) {
		rc = asprintf(&path, "%s/queue-%u", xendev->nodename, queue_id);
		if (rc < 0)
			goto out;
	} else
		path = xendev->nodename;

	rc = xs_printf(xbt, path, "tx-ring-ref", "%u", txq->ring_ref);
	if (rc < 0)
		goto out_path;

	rc = xs_printf(xbt, path, "rx-ring-ref", "%u", rxq->ring_ref);
	if (rc < 0)
		goto out_path;

	if (nfdev->split_evtchn) {
		/* split event channels */
		rc = xs_printf(xbt, path, "event-channel-tx", "%u",
			txq->evtchn);
		if (rc < 0)
			goto out_path;

		rc = xs_printf(xbt, path, "event-channel-rx", "%u",
			rxq->evtchn);
		if (rc < 0)
			goto out_path;
	} else {
		/* shared event channel */
		rc = xs_printf(xbt, path, "event-channel", "%u",
			txq->evtchn);
		if (rc < 0)
			goto out_path;
	}

	rc = 0;

out_path:
	if (write_hierarchical)
		free(path);
out:
	return rc;
}

static void xs_delete_queue(struct netfront_dev *nfdev, uint16_t queue_id,
		xenbus_transaction_t xbt, int write_hierarchical)
{
	struct xenbus_device *xendev = nfdev->xendev;
	char *dir, *path;
	int rc;

	if (write_hierarchical) {
		rc = asprintf(&dir, "%s/queue-%u", xendev->nodename, queue_id);
		if (rc < 0)
			return;
	} else
		dir = xendev->nodename;

	rc = asprintf(&path, "%s/tx-ring-ref", dir);
	if (rc < 0)
		goto out;
	xs_rm(xbt, path);
	free(path);

	rc = asprintf(&path, "%s/rx-ring-ref", dir);
	if (rc < 0)
		goto out;
	xs_rm(xbt, path);
	free(path);

	if (nfdev->split_evtchn) {
		/* split event channels */
		rc = asprintf(&path, "%s/event-channel-tx", dir);
		if (rc < 0)
			goto out;
		xs_rm(xbt, path);
		free(path);
		rc = asprintf(&path, "%s/event-channel-rx", dir);
		if (rc < 0)
			goto out;
		xs_rm(xbt, path);
		free(path);
	} else {
		/* shared event channel */
		rc = asprintf(&path, "%s/event-channel", dir);
		if (rc < 0)
			goto out;
		xs_rm(xbt, path);
		free(path);
	}

out:
	if (write_hierarchical)
		free(dir);
}

static int netfront_xb_front_init(struct netfront_dev *nfdev,
		xenbus_transaction_t xbt)
{
	struct xenbus_device *xendev = nfdev->xendev;
	int rc, i;

	rc = xs_printf(xbt, xendev->nodename, "multi-queue-num-queues",
			"%u", nfdev->rxqs_num);
	if (rc < 0)
		goto out;

	if (nfdev->rxqs_num == 1) {
		rc = xs_write_queue(nfdev, 0, xbt, 0);
		if (rc)
			goto out;
	} else {
		for (i = 0; i < nfdev->rxqs_num; i++) {
			rc = xs_write_queue(nfdev, i, xbt, 1);
			if (rc)
				goto out;
		}
	}

	rc = xs_printf(xbt, xendev->nodename, "request-rx-copy", "%u", 1);
	if (rc < 0)
		goto out;

	rc = 0;

out:
	return rc;
}

static void netfront_xb_front_fini(struct netfront_dev *nfdev,
		xenbus_transaction_t xbt)
{
	int i;

	if (nfdev->rxqs_num == 1)
		xs_delete_queue(nfdev, 0, xbt, 0);
	else {
		for (i = 0; i < nfdev->rxqs_num; i++)
			xs_delete_queue(nfdev, i, xbt, 1);
	}
}

int netfront_xb_connect(struct netfront_dev *nfdev)
{
	struct xenbus_device *xendev;
	xenbus_transaction_t xbt;
	int rc;

	UK_ASSERT(nfdev != NULL);

	xendev = nfdev->xendev;
	UK_ASSERT(xendev != NULL);

again:
	rc = xs_transaction_start(&xbt);
	if (rc)
		goto abort_transaction;

	rc = netfront_xb_front_init(nfdev, xbt);
	if (rc)
		goto abort_transaction;

	rc = uk_xenbus_switch_state(xbt, xendev, XenbusStateConnected);
	if (rc)
		goto abort_transaction;

	rc = xs_transaction_end(xbt, 0);
	if (rc == -EAGAIN)
		goto again;

	rc = netfront_xb_wait_be_connect(nfdev);
	if (rc)
		netfront_xb_front_fini(nfdev, xbt);

	return rc;

abort_transaction:
	xs_transaction_end(xbt, 1);

	return rc;
}

int netfront_xb_disconnect(struct netfront_dev *nfdev)
{
	struct xenbus_device *xendev;
	int rc;

	UK_ASSERT(nfdev != NULL);

	xendev = nfdev->xendev;
	UK_ASSERT(xendev != NULL);

	uk_pr_debug("Close network: backend at %s\n", xendev->otherend);

	rc = uk_xenbus_switch_state(XBT_NIL, xendev, XenbusStateClosing);
	if (rc)
		goto out;

	rc = netfront_xb_wait_be_disconnect(nfdev);
	if (rc)
		goto out;

	netfront_xb_front_fini(nfdev, XBT_NIL);

out:
	return rc;
}

static int be_watch_start(struct xenbus_device *xendev, const char *path)
{
	struct xenbus_watch *watch;

	watch = xs_watch_path(XBT_NIL, path);
	if (PTRISERR(watch))
		return PTR2ERR(watch);

	xendev->otherend_watch = watch;

	return 0;
}

static int be_watch_stop(struct xenbus_device *xendev)
{
	return xs_unwatch(XBT_NIL, xendev->otherend_watch);
}

#define WAIT_BE_STATE_CHANGE_WHILE_COND(state_cond) \
	do { \
		rc = xs_read_integer(XBT_NIL, be_state_path, \
			(int *) &be_state); \
		if (rc) \
			goto out; \
		while (!rc && (state_cond)) \
			rc = uk_xenbus_wait_for_state_change(be_state_path, \
				&be_state, xendev->otherend_watch); \
		if (rc) \
			goto out; \
	} while (0)

static int netfront_xb_wait_be_connect(struct netfront_dev *nfdev)
{
	struct xenbus_device *xendev = nfdev->xendev;
	char be_state_path[strlen(xendev->otherend) + sizeof("/state")];
	XenbusState be_state;
	int rc;

	sprintf(be_state_path, "%s/state", xendev->otherend);

	rc = be_watch_start(xendev, be_state_path);
	if (rc)
		goto out;

	WAIT_BE_STATE_CHANGE_WHILE_COND(be_state < XenbusStateConnected);

	if (be_state != XenbusStateConnected) {
		uk_pr_err("Backend not available, state=%s\n",
				uk_xenbus_state_to_str(be_state));
		be_watch_stop(xendev);
	}

out:
	return rc;
}

static int netfront_xb_wait_be_disconnect(struct netfront_dev *nfdev)
{
	struct xenbus_device *xendev = nfdev->xendev;
	char be_state_path[strlen(xendev->otherend) + sizeof("/state")];
	XenbusState be_state;
	int rc;

	sprintf(be_state_path, "%s/state", xendev->otherend);

	WAIT_BE_STATE_CHANGE_WHILE_COND(be_state < XenbusStateClosing);

	rc = uk_xenbus_switch_state(XBT_NIL, xendev, XenbusStateClosed);
	if (rc)
		goto out;

	WAIT_BE_STATE_CHANGE_WHILE_COND(be_state < XenbusStateClosed);

	rc = uk_xenbus_switch_state(XBT_NIL, xendev, XenbusStateInitialising);
	if (rc)
		goto out;

	WAIT_BE_STATE_CHANGE_WHILE_COND(be_state < XenbusStateInitWait ||
			be_state >= XenbusStateClosed);

	be_watch_stop(xendev);

out:
	return rc;
}

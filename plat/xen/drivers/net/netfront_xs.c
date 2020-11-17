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
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <xenbus/xs.h>
#include "netfront_xb.h"


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
	uk_pr_info("\tMAC %s\n", mac_str);

	p = mac_str;
	for (int i = 0; i < UK_NETDEV_HWADDR_LEN; i++) {
		nfdev->hw_addr.addr_bytes[i] = (uint8_t) strtoul(p, &p, 16);
		p++;
	}
	free(mac_str);

	/* read IP address */
	ip_str = xs_read(XBT_NIL, xendev->otherend, "ip");
	if (PTRISERR(ip_str)) {
		uk_pr_err("Error reading IP address.\n");
		rc = PTR2ERR(ip_str);
		goto no_conf;
	}
	uk_pr_info("\tIP: %s\n", ip_str);

	rc = xs_econf_init(&nfdev->econf, ip_str, a);
	if (rc)
		goto no_conf;
	free(ip_str);

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

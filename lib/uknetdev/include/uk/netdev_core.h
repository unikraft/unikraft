/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2017 Intel Corporation
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
/* Derived from DPDK rte_ethdev_core.h - DPDK.org 18.02.2 */
#ifndef __UK_NETDEV_CORE__
#define __UK_NETDEV_CORE__

#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <uk/config.h>
#include <uk/netbuf.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/essentials.h>

/**
 * Unikraft network API common declarations.
 *
 * This header contains all API data types. Some of them are part of the
 * public API and some are part of the internal API.
 *
 * The device data and operations are separated. This split allows the
 * function pointer and driver data to be per-process, while the actual
 * configuration data for the device is shared.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct uk_netdev;
UK_TAILQ_HEAD(uk_netdev_list, struct uk_netdev);


/**
 * Enum to describe possible states of an Unikraft network device.
 */
enum uk_netdev_state {
	UK_NETDEV_INVALID = 0,
	UK_NETDEV_UNCONFIGURED,
	UK_NETDEV_CONFIGURED,
	UK_NETDEV_RUNNING,
};


/**
 * @internal
 * libuknetdev internal data associated with each network device.
 */
struct uk_netdev_data {
	enum uk_netdev_state state;

	const uint16_t       id;    /**< ID is assigned during registration */
	const char           *drv_name;
};

/**
 * NETDEV
 * A structure used to interact with a network device.
 */
struct uk_netdev {
	/** Pointer to API-internal state data. */
	struct uk_netdev_data       *_data;

	UK_TAILQ_ENTRY(struct uk_netdev) _list;
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_NETDEV_CORE__ */

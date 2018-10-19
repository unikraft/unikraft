/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2010-2017 Intel Corporation
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
#ifndef __UK_NETDEV__
#define __UK_NETDEV__

#include <uk/netdev_core.h>
#include <uk/assert.h>
#include <uk/errptr.h>

/**
 * Unikraft Network API
 *
 * The Unikraft netdev API provides a generalized interface between network
 * device drivers and network stack implementations or low-level network
 * applications.
 *
 * Most API interfaces take as parameter a reference to the corresponding
 * Unikraft Network Device (struct uk_netdev) which can be initially obtained
 * by its ID by calling uk_netdev_get(). The network application should store
 * this reference and use it for subsequent API calls.
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the number of registered network devices.
 *
 * @return
 *   - (unsigned int): number of network devices.
 */
unsigned int uk_netdev_count(void);

/**
 * Get the reference to a network device based on its ID.
 * Valid IDs are in the range of 0 to (n-1) where n is
 * the number of available network devices.
 *
 * @param id
 *   The identifier of the network device to configure.
 * @return
 *   - (NULL): device not available
 *   - (struct uk_netdev *): reference to network device
 */
struct uk_netdev *uk_netdev_get(unsigned int id);

/**
 * Returns the id of a network device
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (>=0): Device ID
 */
uint16_t uk_netdev_id_get(struct uk_netdev *dev);

/**
 * Returns the driver name of a network device.
 * The name might be set to NULL
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (NULL): if no name is defined.
 *   - (const char *): Reference to string if name is available.
 */
const char *uk_netdev_drv_name_get(struct uk_netdev *dev);

/**
 * Returns the current state of a network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (enum uk_netdev_state): current device state
 */
enum uk_netdev_state uk_netdev_state_get(struct uk_netdev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __UK_NETDEV__ */

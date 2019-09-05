/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#ifndef __UK_9PDEV_TRANS__
#define __UK_9PDEV_TRANS__

#include <stdbool.h>
#include <uk/config.h>
#include <uk/9pdev_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A structure used to describe a transport. */
struct uk_9pdev_trans {
	/*
	 * Transport name (e.g. "virtio", "xen"). This field is reserved for
	 * future use, when multiple transport options are available on the
	 * same platform, such as RDMA or TCP, in addition to the platform
	 * specific transport.
	 */
	const char                              *name;
	/* Supported operations. */
	const struct uk_9pdev_trans_ops         *ops;
	/* Allocator used for devices which use this transport layer. */
	struct uk_alloc                         *a;
	/* @internal Entry in the list of available transports. */
	struct uk_list_head                     _list;
};

/**
 * Adds a transport to the available transports list for Unikraft 9P Devices.
 * This should be called once per driver (once for virtio, once for xen, etc.).
 *
 * @param trans
 *   Pointer to the transport structure.
 * @return
 *   - (0): Successful.
 *   - (< 0): Failed to register the transport layer.
 */
int uk_9pdev_trans_register(struct uk_9pdev_trans *trans);

/**
 * Looks up a transport layer by its name.
 *
 * @param name
 *   The transport layer name.
 * @return
 *   The 9P transport with the given name, or NULL if missing.
 */
struct uk_9pdev_trans *uk_9pdev_trans_by_name(const char *name);

/**
 * Gets the default transport layer.
 *
 * @return
 *   The default 9P transport, or NULL if missing.
 */
struct uk_9pdev_trans *uk_9pdev_trans_get_default(void);

/**
 * Sets the default transport layer.
 *
 * @param trans
 *   The default 9P transport.
 */
void uk_9pdev_trans_set_default(struct uk_9pdev_trans *trans);

#ifdef __cplusplus
}
#endif

#endif /* __UK_9PDEV_TRANS__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * All rights reserved.
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
#ifndef __BLKFRONT_XB_H__
#define __BLKFRONT_XB_H__

/**
 * Blkfront interface for xenstore operations.
 *
 * This header contains all the functions needed by the blkfront driver
 * in order to access Xenstore data.
 */

#include "blkfront.h"

/*
 * Get initial info from the xenstore.
 * Ex: backend path, handle, max-queues.
 *
 * Return 0 on success, a negative errno value on error.
 */
int blkfront_xb_init(struct blkfront_dev *dev);

/*
 * It deallocates the xendev structure members allocated during initialization.
 */
void blkfront_xb_fini(struct blkfront_dev *dev);

/**
 * Write nb of queues for further use to Xenstore.
 * Return 0 on success, a negative errno value on error.
 */
int blkfront_xb_write_nb_queues(struct blkfront_dev *dev);

/**
 * Write ring entries to Xenstore.
 * Device changes its state to Connected.
 * It waits until the backend is connected.
 *
 * Return 0 on success, a negative errno value on error.
 */
int blkfront_xb_connect(struct blkfront_dev *dev);

/**
 * Reinitialize the connection with the backend.
 * The following states are:
 *	Connected -> Closing -> Closed -> Initializing.
 * Delete ring entries
 *
 * Return 0 on success, a negative errno value on error.
 */
int blkfront_xb_disconnect(struct blkfront_dev *dev);

#endif /* __BLKFRONT_XB_H__ */

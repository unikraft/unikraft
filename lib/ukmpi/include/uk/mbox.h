/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
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


#ifndef __UK_MBOX_H__
#define __UK_MBOX_H__

#include <uk/config.h>

#if CONFIG_LIBUKMPI_MBOX
#include <errno.h>
#include <uk/semaphore.h>
#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct uk_mbox;

struct uk_mbox *uk_mbox_create(struct uk_alloc *a, size_t size);
void uk_mbox_free(struct uk_alloc *a, struct uk_mbox *m);

void uk_mbox_post(struct uk_mbox *m, void *msg);
int uk_mbox_post_try(struct uk_mbox *m, void *msg);
__nsec uk_mbox_post_to(struct uk_mbox *m, void *msg, __nsec timeout);

void uk_mbox_recv(struct uk_mbox *m, void **msg);
int uk_mbox_recv_try(struct uk_mbox *m, void **msg);
__nsec uk_mbox_recv_to(struct uk_mbox *m, void **msg, __nsec timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CONFIG_LIBMPI_UKMBOX */
#endif /* __UK_MBOX_H__ */

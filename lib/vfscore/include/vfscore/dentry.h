/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
 *
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

#ifndef _OSV_DENTRY_H
#define _OSV_DENTRY_H 1

#include <osv/mutex.h>
#include <bsd/sys/sys/queue.h>

struct vnode;

struct dentry {
	LIST_ENTRY(dentry) d_link;	/* link for hash list */
	int		d_refcnt;	/* reference count */
	char		*d_path;	/* pointer to path in fs */
	struct vnode	*d_vnode;
	struct mount	*d_mount;
	struct dentry   *d_parent; /* pointer to parent */
	LIST_ENTRY(dentry) d_names_link; /* link fo vnode::d_names */
	mutex_t		d_lock;
	LIST_HEAD(, dentry) d_children;
	LIST_ENTRY(dentry) d_children_link;
};

#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>

using dentry_ref = boost::intrusive_ptr<dentry>;

extern "C" {
	void dref(struct dentry* dp);
	void drele(struct dentry* dp);
};

inline void intrusive_ptr_add_ref(dentry* dp) { dref(dp); }
inline void intrusive_ptr_release(dentry* dp) { drele(dp); }

#endif

#endif /* _OSV_DENTRY_H */

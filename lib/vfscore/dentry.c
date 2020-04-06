/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define _BSD_SOURCE

#include <string.h>
#include <stdlib.h>

#include <uk/list.h>
#include <vfscore/dentry.h>
#include <vfscore/vnode.h>
#include <uk/mutex.h>
#include "vfs.h"

#define DENTRY_BUCKETS 32

static struct uk_hlist_head dentry_hash_table[DENTRY_BUCKETS];
static UK_HLIST_HEAD(fake);
static struct uk_mutex dentry_hash_lock = UK_MUTEX_INITIALIZER(dentry_hash_lock);

/*
 * Get the hash value from the mount point and path name.
 * XXX: replace with a better hash for 64-bit pointers.
 */
static unsigned int
dentry_hash(struct mount *mp, const char *path)
{
	unsigned int val = 0;

	if (path) {
		while (*path) {
			val = ((val << 5) + val) + *path++;
		}
	}
	return (val ^ (unsigned long) mp) & (DENTRY_BUCKETS - 1);
}


struct dentry *
dentry_alloc(struct dentry *parent_dp, struct vnode *vp, const char *path)
{
	struct mount *mp = vp->v_mount;
	struct dentry *dp = (struct dentry*)calloc(sizeof(*dp), 1);

	if (!dp) {
		return NULL;
	}

	dp->d_path = strdup(path);
	if (!dp->d_path) {
		free(dp);
		return NULL;
	}

	vref(vp);

	dp->d_refcnt = 1;
	dp->d_vnode = vp;
	dp->d_mount = mp;
	UK_INIT_LIST_HEAD(&dp->d_child_list);

	if (parent_dp) {
		dref(parent_dp);

		uk_mutex_lock(&parent_dp->d_lock);
		// Insert dp into its parent's children list.
		uk_list_add(&dp->d_child_link, &parent_dp->d_child_list);
		uk_mutex_unlock(&parent_dp->d_lock);
	}
	dp->d_parent = parent_dp;

	vn_add_name(vp, dp);

	uk_mutex_lock(&dentry_hash_lock);
	uk_hlist_add_head(&dp->d_link,
			  &dentry_hash_table[dentry_hash(mp, path)]);
	uk_mutex_unlock(&dentry_hash_lock);
	return dp;
};

struct dentry *
dentry_lookup(struct mount *mp, char *path)
{
	struct dentry *dp;

	uk_mutex_lock(&dentry_hash_lock);
	uk_hlist_for_each_entry(dp, &dentry_hash_table[dentry_hash(mp, path)], d_link) {
		if (dp->d_mount == mp && !strncmp(dp->d_path, path, PATH_MAX)) {
			dp->d_refcnt++;
			uk_mutex_unlock(&dentry_hash_lock);
			return dp;
		}
	}
	uk_mutex_unlock(&dentry_hash_lock);
	return NULL;                /* not found */
}

static void dentry_children_remove(struct dentry *dp)
{
	struct dentry *entry = NULL;

	uk_mutex_lock(&dp->d_lock);
	uk_list_for_each_entry(entry, &dp->d_child_list, d_child_link) {
		UK_ASSERT(entry);
		UK_ASSERT(entry->d_refcnt > 0);
		uk_hlist_del(&entry->d_link);
	}
	uk_mutex_unlock(&dp->d_lock);

}

int
dentry_move(struct dentry *dp, struct dentry *parent_dp, char *path)
{
	struct dentry *old_pdp = dp->d_parent;
	char *old_path = dp->d_path;
	char *new_path = strdup(path);

	if (!new_path) {
		// Fail before changing anything to the VFS
		return ENOMEM;
	}

	if (old_pdp) {
		uk_mutex_lock(&old_pdp->d_lock);
		// Remove dp from its old parent's children list.
		uk_list_del(&dp->d_child_link);
		uk_mutex_unlock(&old_pdp->d_lock);
	}

	if (parent_dp) {
		dref(parent_dp);

		uk_mutex_lock(&parent_dp->d_lock);
		// Insert dp into its new parent's children list.
		uk_list_add(&dp->d_child_link, &parent_dp->d_child_list);
		uk_mutex_unlock(&parent_dp->d_lock);
	}

	uk_mutex_lock(&dentry_hash_lock);
	// Remove all dp's child dentries from the hashtable.
	dentry_children_remove(dp);
	// Remove dp with outdated hash info from the hashtable.
	uk_hlist_del(&dp->d_link);
	// Update dp.
	dp->d_path = new_path;

	dp->d_parent = parent_dp;
	// Insert dp updated hash info into the hashtable.
	uk_hlist_add_head(&dp->d_link,
			  &dentry_hash_table[dentry_hash(dp->d_mount, path)]);
	uk_mutex_unlock(&dentry_hash_lock);

	if (old_pdp) {
		drele(old_pdp);
	}

	free(old_path);
	return 0;
}

void
dentry_remove(struct dentry *dp)
{
	uk_mutex_lock(&dentry_hash_lock);
	uk_hlist_del(&dp->d_link);
	/* put it on a fake list for drele() to work*/
	uk_hlist_add_head(&dp->d_link, &fake);
	uk_mutex_unlock(&dentry_hash_lock);
}

void
dref(struct dentry *dp)
{
	UK_ASSERT(dp);
	UK_ASSERT(dp->d_refcnt > 0);

	uk_mutex_lock(&dentry_hash_lock);
	dp->d_refcnt++;
	uk_mutex_unlock(&dentry_hash_lock);
}

void
drele(struct dentry *dp)
{
	UK_ASSERT(dp);
	UK_ASSERT(dp->d_refcnt > 0);

	uk_mutex_lock(&dentry_hash_lock);
	if (--dp->d_refcnt) {
		uk_mutex_unlock(&dentry_hash_lock);
		return;
	}
	uk_hlist_del(&dp->d_link);
	vn_del_name(dp->d_vnode, dp);

	uk_mutex_unlock(&dentry_hash_lock);

	if (dp->d_parent) {
		uk_mutex_lock(&dp->d_parent->d_lock);
		// Remove dp from its parent's children list.
		uk_list_del(&dp->d_child_link);
		uk_mutex_unlock(&dp->d_parent->d_lock);

		drele(dp->d_parent);
	}

	vrele(dp->d_vnode);

	free(dp->d_path);
	free(dp);
}

void
dentry_init(void)
{
	int i;

	for (i = 0; i < DENTRY_BUCKETS; i++) {
		UK_INIT_HLIST_HEAD(&dentry_hash_table[i]);
	}
}

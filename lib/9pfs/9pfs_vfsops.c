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
 */

#define _GNU_SOURCE

#include <uk/config.h>
#include <uk/errptr.h>
#include <uk/9p.h>
#include <uk/9pdev_trans.h>
#include <vfscore/mount.h>
#include <vfscore/dentry.h>
#include <stdlib.h>
#include <string.h>

#include "9pfs.h"

extern struct vnops uk_9pfs_vnops;

static int uk_9pfs_mount(struct mount *mp, const char *dev, int flags,
			 const void *data);

static int uk_9pfs_unmount(struct mount *mp, int flags);

#define uk_9pfs_sync		((vfsop_sync_t)vfscore_nullop)
#define uk_9pfs_vget		((vfsop_vget_t)vfscore_nullop)
#define uk_9pfs_statfs		((vfsop_statfs_t)vfscore_nullop)

struct vfsops uk_9pfs_vfsops = {
	.vfs_mount	= uk_9pfs_mount,
	.vfs_unmount	= uk_9pfs_unmount,
	.vfs_sync	= uk_9pfs_sync,
	.vfs_vget	= uk_9pfs_vget,
	.vfs_statfs	= uk_9pfs_statfs,
	.vfs_vnops	= &uk_9pfs_vnops
};

static struct vfscore_fs_type uk_9pfs_fs = {
	.vs_name	= "9pfs",
	.vs_init	= NULL,
	.vs_op		= &uk_9pfs_vfsops
};

UK_FS_REGISTER(uk_9pfs_fs);

static const char *uk_9pfs_proto_str[UK_9P_PROTO_MAX] = {
	[UK_9P_PROTO_2000U] = "9P2000.u",
	[UK_9P_PROTO_2000L] = "9P2000.L"
};

static int uk_9pfs_parse_option(struct uk_9pfs_mount_data *md,
		const char *option)
{
	if (strncmp(option, "version=", 8) == 0) {
		const char *protoname = option + 8;
		enum uk_9pfs_proto i;

		for (i = 0; i < UK_9P_PROTO_MAX; i++)
			if (strcmp(protoname, uk_9pfs_proto_str[i]) == 0) {
				md->proto = i;
				return 0;
			}

		return -EINVAL;
	} else if (strncmp(option, "uname=", 6) == 0) {
		free(md->uname);
		md->uname = strdup(option + 6);
		if (!md->uname)
			return -ENOMEM;
	} else if (strncmp(option, "aname=", 6) == 0) {
		free(md->aname);
		md->aname = strdup(option + 6);
		if (!md->aname)
			return -ENOMEM;
	}

	return 0;
}

static int uk_9pfs_parse_options(struct uk_9pfs_mount_data *md,
				 const void *data __unused)
{
	int rc = 0;
	char *options = NULL, *options_tok = NULL, *options_save = NULL;
	char *option;

	md->trans = uk_9pdev_trans_get_default();
	if (!md->trans) {
		rc = ENODEV;
		goto err;
	}

	md->proto = UK_9P_PROTO_2000L;
	md->uname = strdup("");
	md->aname = strdup("");

	/*
	 * musl/nolibc strtok_r resets saveptr at the end, so we need to feed
	 * the option string only once; otherwise strtok_r would loop over the
	 * options string forever
	 */
	if (data)
		options = options_tok = strdup(data);

	while ((option = strtok_r(options_tok, ",", &options_save))) {
		options_tok = NULL;
		rc = uk_9pfs_parse_option(md, option);
		if (rc < 0) {
			rc = -rc;
			goto err;
		}
	}

	if (!md->uname || !md->aname) {
		rc = ENOMEM;
		goto err;
	}

	free(options);
	return rc;

err:
	free(md->uname);
	md->uname = NULL;
	free(md->aname);
	md->aname = NULL;
	free(options);
	return rc;
}

static int uk_9pfs_mount(struct mount *mp, const char *dev,
			int flags __unused, const void *data)
{
	struct uk_9pfs_mount_data *md;
	struct uk_9preq *version_req;
	struct uk_9p_str rcvd_version;
	struct uk_9pfid *rootfid;
	int version_accepted;
	int rc;

	/* Set data as null, vnop_inactive() checks this for the root fid. */
	mp->m_root->d_vnode->v_data = NULL;

	/* Allocate mount data, parse options. */
	md = malloc(sizeof(*md));
	if (!md)
		return ENOMEM;

	rc = uk_9pfs_parse_options(md, data);
	if (rc)
		goto out_free_mdata;

	mp->m_data = md;

	/* Establish connection with the given 9P endpoint. */
	md->dev = uk_9pdev_connect(md->trans, dev, data, NULL);
	if (PTRISERR(md->dev)) {
		rc = -PTR2ERR(md->dev);
		goto out_free_mdata;
	}

	/* Create a new 9pfs session via a VERSION message. */
	version_req = uk_9p_version(md->dev, uk_9pfs_proto_str[md->proto],
			&rcvd_version);
	if (PTRISERR(version_req)) {
		rc = -PTR2ERR(version_req);
		goto out_disconnect;
	}

	version_accepted = uk_9p_str_equal(&rcvd_version,
			uk_9pfs_proto_str[md->proto]);
	uk_9pdev_req_remove(md->dev, version_req);

	if (!version_accepted) {
		rc = EIO;
		uk_pr_warn("Could not negotiate protocol %s\n",
				uk_9pfs_proto_str[md->proto]);
		goto out_disconnect;
	}

	/* Create root fid. */
	rootfid = uk_9p_attach(md->dev, UK_9P_NOFID, md->uname,
			md->aname, UK_9P_NONUNAME);
	if (PTRISERR(rootfid)) {
		rc = -PTR2ERR(rootfid);
		goto out_disconnect;
	}

	rc = uk_9pfs_allocate_vnode_data(mp->m_root->d_vnode, rootfid);
	if (rc != 0) {
		rc = -rc;
		goto out_disconnect;
	}

	return 0;

out_disconnect:
	uk_9pdev_disconnect(md->dev);
out_free_mdata:
	free(md);
	return rc;
}

static void uk_9pfs_release_tree_fids(struct dentry *d)
{
	struct dentry *p;

	uk_list_for_each_entry(p, &d->d_child_list, d_child_link) {
		uk_9pfs_release_tree_fids(p);
		drele(p);
	}
}

static int uk_9pfs_unmount(struct mount *mp, int flags __unused)
{
	struct uk_9pfs_mount_data *md = UK_9PFS_MD(mp);

	uk_9pfs_release_tree_fids(mp->m_root);
	vfscore_release_mp_dentries(mp);
	uk_9pdev_disconnect(md->dev);
	free(md->uname);
	free(md->aname);
	free(md);

	return 0;
}

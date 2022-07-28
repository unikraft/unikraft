/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Felipe Huici <felipe.huici@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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

#include "user.h"

#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/syscall.h>
#include <uk/assert.h>

#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

/* Group members */
static char *g_members__[] = { UK_DEFAULT_USER, NULL };

/* Default group */
static struct group g__ = {
	.gr_name = UK_DEFAULT_GROUP,
	.gr_passwd = UK_DEFAULT_PASS,
	.gr_gid = UK_DEFAULT_GID,
	.gr_mem = g_members__,
};

static __uk_tls struct group_entry {
	struct group *group;

	UK_SLIST_ENTRY(struct group_entry) entries;
} *groups_iter;

UK_SLIST_HEAD(uk_group_entry_list, struct group_entry);

static struct uk_group_entry_list groups;

void pu_init_groups(void)
{
	static struct group_entry ge;

	ge.group = &g__;
	UK_SLIST_INIT(&groups);
	UK_SLIST_INSERT_HEAD(&groups, &ge, entries);
}

void setgrent(void)
{
	groups_iter = UK_SLIST_FIRST(&groups);
}

void endgrent(void)
{
	/* Nothing to do */
}

struct group *getgrent(void)
{
	struct group *grp;

	if (groups_iter) {
		grp = groups_iter->group;
		groups_iter = UK_SLIST_NEXT(groups_iter, entries);
	} else
		grp = NULL;

	return grp;
}

UK_SYSCALL_R_DEFINE(gid_t, getgid)
{
	return UK_DEFAULT_GID;
}

UK_SYSCALL_R_DEFINE(int, setgid, gid_t, gid)
{
	/* We allow only UK_DEFAULT_GID */
	if (unlikely(gid != UK_DEFAULT_GID))
		return -EINVAL;

	return 0;
}

/* not a syscall */
int issetugid(void)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(gid_t, getegid)
{
	return UK_DEFAULT_GID;
}

/* not a syscall */
int setegid(gid_t egid)
{
	/* We allow only UK_DEFAULT_GID */
	if (unlikely(egid != UK_DEFAULT_GID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setregid, gid_t, rgid, gid_t, egid)
{
	/* We allow only UK_DEFAULT_GID */
	if (unlikely(rgid != UK_DEFAULT_GID || egid != UK_DEFAULT_GID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, getresgid, gid_t *, rgid, gid_t *, egid, gid_t *, sgid)
{
	if (unlikely(!rgid || !egid || !sgid))
		return -EFAULT;

	*rgid = *egid = *sgid = UK_DEFAULT_GID;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setresgid, gid_t, rgid, gid_t, egid, gid_t, sgid)
{
	/* We allow only UK_DEFAULT_GID */
	if (unlikely(rgid != UK_DEFAULT_GID ||
		     egid != UK_DEFAULT_GID ||
		     sgid != UK_DEFAULT_GID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setfsgid, uid_t, fsgid)
{
	return UK_DEFAULT_GID;
}

/* Maximum number of supplementary groups (Linux 2.6.4+) */
#ifndef NGROUPS_MAX
#define NGROUPS_MAX		(1 << 16)
#endif

/* not a syscall */
int initgroups(const char *user __unused, gid_t group __unused)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, getgroups, int, size, gid_t *, list)
{
	/* We only have one group and it is not specified if the effective GID
	 * (which is this group) should be part of the list. So we can safely
	 * return an empty list
	 */
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setgroups, size_t, size, const gid_t *, list)
{
	size_t i;

	if (unlikely(size > NGROUPS_MAX))
		return -EINVAL;

	/* Since we have only one group, we just check if the caller tries to
	 * set any invalid groups. Since the only valid group ID is also the
	 * effective group it, it is fine to collapse to an empty list.
	 */
	for (i = 0; i < size; i++)
		if (unlikely(list[i] != UK_DEFAULT_GID))
			return -EINVAL;

	return 0;
}

static int getgr_r(struct group *ingrp, struct group *grp, char *buf,
		   size_t buflen, struct group **result)
{
	size_t needed;
	int i, members;

	UK_ASSERT(grp);
	UK_ASSERT(buf);
	UK_ASSERT(result);

	if (unlikely(!ingrp)) {
		*result = NULL;
		return 0;
	}

	/* check if provided buffer is large enough */
	needed = strlen(ingrp->gr_name) + 1 +
		 strlen(ingrp->gr_passwd) + 1;

	members = 0;
	while (ingrp->gr_mem[members])
		needed += strlen(ingrp->gr_mem[members++]) + 1;

	needed += members + 1 * sizeof(char *);

	if (unlikely(buflen < needed)) {
		*result = NULL;
		return ERANGE; /* must be positive */
	}

	/* set name */
	grp->gr_name = buf;
	buf = pu_cpystr(ingrp->gr_name, buf);

	/* set passwd */
	grp->gr_passwd = buf;
	buf = pu_cpystr(ingrp->gr_passwd, buf);

	/* set gid */
	grp->gr_gid = ingrp->gr_gid;

	/* set members */
	grp->gr_mem = (char **) buf;
	buf += (members + 1) * sizeof(char *);
	for (i = 0; i < members; i++) {
		grp->gr_mem[i] = buf;
		buf = pu_cpystr(ingrp->gr_mem[i], buf);
	}
	grp->gr_mem[members] = NULL;

	*result = grp;
	return 0;
}

struct group *getgrnam(const char *name)
{
	struct group *res;

	if (name && !strcmp(name, g__.gr_name))
		res = &g__;
	else {
		res = NULL;
		errno = ENOENT;
	}

	return res;
}

int getgrnam_r(const char *name, struct group *grp,
	       char *buf, size_t buflen, struct group **result)
{
	return getgr_r(getgrnam(name), grp, buf, buflen, result);
}

struct group *getgrgid(gid_t gid)
{
	struct group *res;

	if (gid == g__.gr_gid)
		res = &g__;
	else {
		res = NULL;
		errno = ENOENT;
	}

	return res;
}

int getgrgid_r(gid_t gid, struct group *grp, char *buf, size_t buflen,
	       struct group **result)
{
	return getgr_r(getgrgid(gid), grp, buf, buflen, result);
}

int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
	struct passwd *pwd;
	int ngrps;

	UK_ASSERT(ngroups);
	UK_ASSERT(groups);

	pwd = getpwnam(user);
	if (!pwd)
		return -1;

	ngrps = (group == pwd->pw_gid) ? 1 : 2;

	/* Check if the grouplist is large enough */
	if (*ngroups < ngrps) {
		*ngroups = ngrps;
		return -1;
	}

	groups[0] = pwd->pw_gid;
	if (group != pwd->pw_gid)
		groups[1] = group;

	*ngroups = ngrps;

	return ngrps;
}

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
	setgrent();
}

struct group *getgrent(void)
{
	struct group *res;

	if (groups_iter) {
		res = groups_iter->group;
		groups_iter = UK_SLIST_NEXT(groups_iter, entries);
	} else
		res = NULL;

	return res;

}

UK_SYSCALL_R_DEFINE(gid_t, getgid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setgid, gid_t, gid)
{
	return 0;
}

int issetugid(void)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(gid_t, getegid)
{
	return 0;
}

int setegid(gid_t egid __unused)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setregid, gid_t, rgid, gid_t, egid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, getresgid, gid_t*, rgid, gid_t*, egid, gid_t*, sgid)
{
	if (!rgid || !egid || !sgid) {
		return -EFAULT;
	}

	*rgid = *egid = *sgid = UK_DEFAULT_GID;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setresgid, gid_t, rgid, gid_t, egid, gid_t, sgid)
{
	/* We allow only UK_DEFAULT_GID */
	if (rgid != UK_DEFAULT_GID || egid != UK_DEFAULT_GID ||
			sgid != UK_DEFAULT_GID) {
		return -EINVAL;
	}

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setfsgid, uid_t, fsgid)
{
	return 0;
}

int initgroups(const char *user __unused, gid_t group __unused)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, getgroups, int, size, gid_t*, list)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setgroups, size_t, size, const gid_t*, list)
{
	return 0;
}

int getgrnam_r(const char *name, struct group *grp,
		char *buf, size_t buflen, struct group **result)
{
	size_t needed;
	int i, members;

	if (!name || strcmp(name, g__.gr_name)) {
		*result = NULL;
		return 0;
	}

	/* check if provided buffer is big enough */
	needed = strlen(g__.gr_name) + 1
			+ strlen(g__.gr_passwd) + 1
			+ sizeof(g__.gr_gid);
	i = 0;
	while (g__.gr_mem[i])
		needed += strlen(g__.gr_mem[i++]) + 1;
	members = i;

	if (buflen < needed) {
		*result = NULL;
		return ERANGE;
	}

	/* set name */
	strlcpy(buf, g__.gr_name, strlen(g__.gr_name) + 1);
	grp->gr_name = buf;
	buf += strlen(g__.gr_name) + 1;

	/* set passwd */
	strlcpy(buf, g__.gr_passwd, strlen(g__.gr_passwd) + 1);
	grp->gr_passwd = buf;
	buf += strlen(g__.gr_passwd) + 1;

	/* set gid */
	grp->gr_gid = g__.gr_gid;

	/* set members */
	i = 0;
	grp->gr_mem = (char **) buf;
	buf += (members + 1) * sizeof(char *);
	while (g__.gr_mem[i]) {
		strlcpy(buf, g__.gr_mem[i], strlen(g__.gr_mem[i]) + 1);
		grp->gr_mem[i] = buf;
		buf += strlen(g__.gr_mem[i]) + 1;
		i++;
	}
	grp->gr_mem[i] = NULL;

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

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Felipe Huici <felipe.huici@neclab.eu>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
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

#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/ctors.h>
#include <uk/print.h>
#include <uk/user.h>

#define UK_DEFAULT_UID    0
#define UK_DEFAULT_GID    0
#define UK_DEFAULT_USER   "root"
#define UK_DEFAULT_GROUP  "root"
#define UK_DEFAULT_PASS   "x"

static __uk_tls struct passwd_entry {
	struct passwd *passwd;

	UK_SLIST_ENTRY(struct passwd_entry) entries;
} *iter;

UK_SLIST_HEAD(uk_entry_list, struct passwd_entry);

static struct uk_entry_list passwds;

static void init_groups(void);

/*
 * TODO make passwd management consistent with group management
 */

static void init_posix_user(void)
{
	static struct passwd_entry p1;
	static struct passwd passwd = {
		.pw_name = UK_DEFAULT_USER,
		.pw_passwd = UK_DEFAULT_PASS,
		.pw_uid = UK_DEFAULT_UID,
		.pw_gid = UK_DEFAULT_GID,
		.pw_gecos = UK_DEFAULT_USER,
		.pw_dir = "/",
		.pw_shell = NULL,
	};

	p1.passwd = &passwd;

	UK_SLIST_INIT(&passwds);
	UK_SLIST_INSERT_HEAD(&passwds, &p1, entries);

	init_groups();
}
UK_CTOR_FUNC(2, init_posix_user);

uid_t getuid(void)
{
	return 0;
}

int setuid(uid_t uid __unused)
{
	return 0;
}

uid_t geteuid(void)
{
	return 0;
}

int seteuid(uid_t euid __unused)
{
	return 0;
}

int getresuid(uid_t *ruid __unused, uid_t *euid __unused, uid_t *suid __unused)
{
	return 0;
}

int setresuid(uid_t ruid __unused, uid_t euid __unused, uid_t suid __unused)
{
	return 0;
}

int setreuid(uid_t ruid __unused, uid_t euid __unused)
{
	return 0;
}

char *getlogin(void)
{
	return 0;
}

void setpwent(void)
{
	iter = UK_SLIST_FIRST(&passwds);
}

void endpwent(void)
{
}

struct passwd *getpwnam(const char *name)
{
	struct passwd *pwd;

	setpwent();
	while ((pwd = getpwent()) && strcmp(pwd->pw_name, name))
		;
	endpwent();

	return pwd;
}

struct passwd *getpwuid(uid_t uid)
{
	struct passwd *pwd;

	setpwent();
	while ((pwd = getpwent()) && pwd->pw_uid != uid)
		;
	endpwent();

	return pwd;
}

int getpwnam_r(const char *name __unused, struct passwd *pwd __unused,
	char *buf __unused, size_t buflen __unused,
	struct passwd **result __unused)
{
	return 0;
}

int getpwuid_r(uid_t uid __unused, struct passwd *pwd __unused,
	char *buf __unused, size_t buflen __unused,
	struct passwd **result __unused)
{
	return 0;
}

struct passwd *getpwent(void)
{
	struct passwd *pwd;

	if (iter == NULL)
		return NULL;

	pwd = iter->passwd;

	iter = UK_SLIST_NEXT(iter, entries);

	return pwd;
}

gid_t getgid(void)
{
	return 0;
}

int setgid(gid_t gid __unused)
{
	return 0;
}

int issetugid(void)
{
	return 0;
}

gid_t getegid(void)
{
	return 0;
}

int setegid(gid_t egid __unused)
{
	return 0;
}

int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
	if (!rgid || !egid || !sgid) {
		errno = EFAULT;
		return -1;
	}

	*rgid = *egid = *sgid = UK_DEFAULT_GID;

	return 0;
}

int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	/* We allow only UK_DEFAULT_GID */
	if (rgid != UK_DEFAULT_GID || egid != UK_DEFAULT_GID ||
			sgid != UK_DEFAULT_GID) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int setregid(gid_t rgid __unused, gid_t egid __unused)
{
	return 0;
}

int initgroups(const char *user __unused, gid_t group __unused)
{
	return 0;
}

int getgroups(int size __unused, gid_t list[] __unused)
{
	return 0;
}

int setgroups(size_t size __unused, const gid_t *list __unused)
{
	return 0;
}

/* Group members */
static char *g_members__[] = { UK_DEFAULT_USER, NULL };

/* Group entry */
static struct group g__ = {
	.gr_name = UK_DEFAULT_GROUP,
	.gr_passwd = UK_DEFAULT_PASS,
	.gr_gid = UK_DEFAULT_GID,
	.gr_mem = g_members__,
};

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

static __uk_tls struct group_entry {
	struct group *group;

	UK_SLIST_ENTRY(struct group_entry) entries;
} *groups_iter;

UK_SLIST_HEAD(uk_group_entry_list, struct group_entry);

static struct uk_group_entry_list groups;

static void init_groups(void)
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

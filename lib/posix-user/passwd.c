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

/* Default passwd */
static struct passwd pw__ = {
	.pw_name = UK_DEFAULT_USER,
	.pw_passwd = UK_DEFAULT_PASS,
	.pw_uid = UK_DEFAULT_UID,
	.pw_gid = UK_DEFAULT_GID,
	.pw_gecos = UK_DEFAULT_USER,
	.pw_dir = "/",
	.pw_shell = "",
};

static __uk_tls struct passwd_entry {
	struct passwd *passwd;

	UK_SLIST_ENTRY(struct passwd_entry) entries;
} *passwds_iter;

UK_SLIST_HEAD(uk_entry_list, struct passwd_entry);

static struct uk_entry_list passwds;

void pu_init_passwds(void)
{
	static struct passwd_entry pe;

	pe.passwd = &pw__;
	UK_SLIST_INIT(&passwds);
	UK_SLIST_INSERT_HEAD(&passwds, &pe, entries);
}

void setpwent(void)
{
	passwds_iter = UK_SLIST_FIRST(&passwds);
}

void endpwent(void)
{
	/* Nothing to do */
}

struct passwd *getpwent(void)
{
	struct passwd *pwd;

	if (passwds_iter) {
		pwd = passwds_iter->passwd;
		passwds_iter = UK_SLIST_NEXT(passwds_iter, entries);
	} else
		pwd = NULL;

	return pwd;
}

UK_SYSCALL_R_DEFINE(uid_t, getuid)
{
	return UK_DEFAULT_UID;
}

UK_SYSCALL_R_DEFINE(int, setuid, uid_t, uid)
{
	/* We allow only UK_DEFAULT_UID */
	if (unlikely(uid != UK_DEFAULT_UID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(uid_t, geteuid)
{
	return UK_DEFAULT_UID;
}

/* not a syscall */
int seteuid(uid_t euid)
{
	/* We allow only UK_DEFAULT_UID */
	if (unlikely(euid != UK_DEFAULT_UID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setreuid, uid_t, ruid, uid_t, euid)
{
	/* We allow only UK_DEFAULT_UID */
	if (unlikely(ruid != UK_DEFAULT_UID || euid != UK_DEFAULT_UID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, getresuid, uid_t *, ruid, uid_t *, euid, uid_t *, suid)
{
	if (unlikely(!ruid || !euid || !suid))
		return -EFAULT;

	*ruid = *euid = *suid = UK_DEFAULT_UID;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setresuid, uid_t, ruid, uid_t, euid, uid_t, suid)
{
	/* We allow only UK_DEFAULT_UID */
	if (unlikely(ruid != UK_DEFAULT_UID ||
		     euid != UK_DEFAULT_UID ||
		     suid != UK_DEFAULT_UID))
		return -EINVAL;

	return 0;
}

UK_SYSCALL_R_DEFINE(int, setfsuid, uid_t, fsuid)
{
	return UK_DEFAULT_UID;
}

/* not a syscall */
char *getlogin(void)
{
	return UK_DEFAULT_USER;
}

int getlogin_r(char *buf, size_t bufsize)
{
	UK_ASSERT(buf);

	if (unlikely(bufsize < sizeof(UK_DEFAULT_USER))) {
		errno = ERANGE;
		return -1;
	}

	memcpy(buf, UK_DEFAULT_USER, sizeof(UK_DEFAULT_USER));
	return 0;
}

static int getpw_r(struct passwd *inpwd, struct passwd *pwd, char *buf,
		   size_t buflen, struct passwd **result)
{
	size_t needed;

	UK_ASSERT(pwd);
	UK_ASSERT(buf);
	UK_ASSERT(result);

	if (unlikely(!inpwd)) {
		*result = NULL;
		return 0;
	}

	/* check if provided buffer is large enough */
	needed = strlen(inpwd->pw_name) + 1 +
		 strlen(inpwd->pw_passwd) + 1 +
		 strlen(inpwd->pw_gecos) + 1 +
		 strlen(inpwd->pw_dir) + 1 +
		 strlen(inpwd->pw_shell) + 1;

	if (unlikely(buflen < needed)) {
		*result = NULL;
		return ERANGE; /* must be positive */
	}

	/* set name */
	pwd->pw_name = buf;
	buf = pu_cpystr(inpwd->pw_name, buf);

	/* set passwd */
	pwd->pw_passwd = buf;
	buf = pu_cpystr(inpwd->pw_passwd, buf);

	/* set uid & gid */
	pwd->pw_uid = inpwd->pw_uid;
	pwd->pw_gid = inpwd->pw_gid;

	/* set gecos */
	pwd->pw_gecos = buf;
	buf = pu_cpystr(inpwd->pw_gecos, buf);

	/* set dir */
	pwd->pw_dir = buf;
	buf = pu_cpystr(inpwd->pw_dir, buf);

	/* set shell */
	pwd->pw_shell = buf;
	pu_cpystr(inpwd->pw_shell, buf);

	*result = pwd;
	return 0;
}

struct passwd *getpwnam(const char *name)
{
	struct passwd *pwd;

	if (name && !strcmp(name, pw__.pw_name))
		pwd = &pw__;
	else {
		pwd = NULL;
		errno = ENOENT;
	}

	return pwd;
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen,
	       struct passwd **result)
{
	return getpw_r(getpwnam(name), pwd, buf, buflen, result);
}

struct passwd *getpwuid(uid_t uid)
{
	struct passwd *pwd;

	if (uid == pw__.pw_uid)
		pwd = &pw__;
	else {
		pwd = NULL;
		errno = ENOENT;
	}

	return pwd;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen,
	       struct passwd **result)
{
	return getpw_r(getpwuid(uid), pwd, buf, buflen, result);
}

/* TODO: Find better place (perhaps posix-process) */
UK_SYSCALL_R_DEFINE(int, capget, void*, hdrp, void*, datap)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, capset, void*, hdrp, void*, datap)
{
	return 0;
}

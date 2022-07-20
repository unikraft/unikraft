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

static __uk_tls struct passwd_entry {
	struct passwd *passwd;

	UK_SLIST_ENTRY(struct passwd_entry) entries;
} *iter;

UK_SLIST_HEAD(uk_entry_list, struct passwd_entry);

static struct uk_entry_list passwds;

void pu_init_passwds(void)
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
}

void setpwent(void)
{
	iter = UK_SLIST_FIRST(&passwds);
}

void endpwent(void)
{
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

UK_SYSCALL_R_DEFINE(uid_t, getuid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setuid, uid_t, uid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(uid_t, geteuid)
{
	return 0;
}

int seteuid(uid_t euid __unused)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setreuid, uid_t, ruid, uid_t, euid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, getresuid, uid_t*, ruid, uid_t*, euid, uid_t*, suid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setresuid, uid_t, ruid, uid_t, euid, uid_t, suid)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, setfsuid, uid_t, fsuid)
{
	return 0;
}

char *getlogin(void)
{
	return 0;
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

int getpwnam_r(const char *name __unused, struct passwd *pwd __unused,
	char *buf __unused, size_t buflen __unused,
	struct passwd **result __unused)
{
	return 0;
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

int getpwuid_r(uid_t uid __unused, struct passwd *pwd __unused,
	char *buf __unused, size_t buflen __unused,
	struct passwd **result __unused)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, capset, void*, hdrp, void*, datap)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, capget, void*, hdrp, void*, datap)
{
	return 0;
}

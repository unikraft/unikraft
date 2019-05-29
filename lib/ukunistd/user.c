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
#include <pwd.h>
#include <sys/types.h>
#include <uk/essentials.h>

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

struct passwd *getpwnam(const char *name __unused)
{
	return NULL;
}

struct passwd *getpwuid(uid_t uid __unused)
{
	return NULL;
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
	return NULL;
}

void setpwent(void)
{
}

void endpwent(void)
{
}

gid_t getgid(void)
{
	return 0;
}

int setgid(gid_t gid __unused)
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

int getresgid(gid_t *rgid __unused, gid_t *egid __unused, gid_t *sgid __unused)
{
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

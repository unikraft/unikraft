/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@stud.acs.upb.ro>
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __PWD_H__
#define __PWD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct passwd {
	char	*pw_name;	/* user name */
	char	*pw_passwd;	/* user password */
	uid_t	pw_uid;		/* user ID */
	gid_t	pw_gid;		/* group ID */
	char	*pw_gecos;	/* user information */
	char	*pw_dir;	/* home directory */
	char	*pw_shell;	/* shell program */
};

struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);

int getpwnam_r(const char *name, struct passwd *pwd,
		char *buf, size_t buflen, struct passwd **result);
int getpwuid_r(uid_t uid, struct passwd *pwd,
		char *buf, size_t buflen, struct passwd **result);

struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

#ifdef __cplusplus
}
#endif

#endif /* __PWD_H__ */

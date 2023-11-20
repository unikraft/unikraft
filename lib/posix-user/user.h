/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
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

#ifndef __LIBPOSIX_USER_H__
#define __LIBPOSIX_USER_H__

#include <uk/assert.h>
#include <uk/config.h>

#define UK_DEFAULT_UID		CONFIG_LIBPOSIX_USER_UID
#define UK_DEFAULT_GID		CONFIG_LIBPOSIX_USER_GID
#define UK_DEFAULT_USER		CONFIG_LIBPOSIX_USER_USERNAME
#define UK_DEFAULT_GROUP	CONFIG_LIBPOSIX_USER_GROUPNAME
#define UK_DEFAULT_PASS		""

/**
 * Initializes the `passwds` list and inserts in it the first entry.
 * This entry contains the default values defined above, such that
 * name => `UK_DEFAULT_USER`, passwd => `UK_DEFAULT_PASS`,
 * uid => `UK_DEFAULT_UID`, gid => `UK_DEFAULT_GID`,
 * gecos => `UK_DEFAULT_USER`, dir = "/", shell = "".
 */
void pu_init_passwds(void);
/**
 * Initializes the `groups` list and inserts in it the first entry.
 * This entry contains the default values defined above, such that
 * name => `UK_DEFAULT_GROUP`, passwd => `UK_DEFAULT_PASS`,
 * uid => `UK_DEFAULT_GID`, mem => `{ `UK_DEFAULT_USER` }`.
 */
void pu_init_groups(void);

/**
 * Copies `src` string at the end of the `dest` string,
 * also adding `\0` at the end.
 *
 * @param src
 *   Source string
 * @param dest
 *	 Destination string
 * @return
 *   Destination string, containing the source string at the end.
 */
static inline char *pu_cpystr(char *src, char *dest)
{
	UK_ASSERT(src);
	UK_ASSERT(dest);

	while (*src)
		*dest++ = *src++;

	*dest++ = '\0';
	return dest;
}

#endif /* __LIBPOSIX_USER_H__ */

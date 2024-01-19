/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon@unikraft.io>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2023, Unikraft GmbH. All rights reserved.
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

#include <uk/argparse.h>
#include <uk/assert.h>
#include <string.h> /* strchr */


static void left_shift(char *buf, __sz index, __sz maxlen)
{
	while(buf[index] != '\0' && index < maxlen) {
		buf[index] = buf[index + 1];
		index++;
	}
}

int uk_argnparse(char *argb, __sz maxlen, char *argv[], int maxcount)
{
	int argc = 0;
	int prev_wspace = 1;
	int prev_escape = 0;
	char in_quote = '\0';
	__sz i;

	UK_ASSERT(argb != __NULL);
	UK_ASSERT(argv != __NULL);
	UK_ASSERT(maxcount >= 0);

	for (i = 0; i < maxlen && argc < maxcount; ++i) {
		switch (argb[i]) {
		/* end of string */
		case '\0':
			goto out;

		/* white spaces */
		case ' ':
		case '\r':
		case '\n':
		case '\t':
		case '\v':
			if (!in_quote && !prev_escape) {
				argb[i] = '\0';
				prev_wspace = 1;
				break;
			}
			goto regularchar;

		/* quotes */
		case '\'':
		case '"':
			if (prev_escape) {
				if (!in_quote) {
					/* escaping removes special meaning */
					goto regularchar;
				} else if (argb[i] == '"') {
					/* \" -> " */
					left_shift(argb, i - 1, maxlen + 1);
					--i;
					prev_escape = 0;
					goto regularchar;
				}
			}
			if (!in_quote) {
				in_quote = argb[i];
				left_shift(argb, i, maxlen);
				--i;
				break;
			}
			if (in_quote == argb[i]) {
				in_quote = '\0';
				left_shift(argb, i, maxlen);
				--i;
				break;
			}
			goto regularchar;

		default:
		regularchar:
			/* any character */
			if (prev_wspace) {
				argv[argc++] = &argb[i];
				prev_wspace = 0;
				prev_escape = 0;
			}

			/* escape character handling */
			if (argb[i] == '\\' && in_quote != '\'') {
				if (prev_escape) {
					/* double escape: \\ -> \ */
					left_shift(argb, i, maxlen);
					--i;
					prev_escape = 0;
				} else {
					prev_escape = 1;
				}
			} else if (prev_escape) {
				/* any character after escape symbol */
				if (!in_quote) {
					/* remove escape symbol */
					left_shift(argb, i - 1, maxlen + 1);
					--i;
				}
				prev_escape = 0;
			}
			break;
		}
	}

out:
	return argc;
}

__sz uk_nextarg_r(const char **argptr, int separator)
{
	const char *nsep;
	const char *arg;
	__sz arglen;

	UK_ASSERT(argptr);

	if (!*argptr || (*argptr)[0] == '\0') {
		/* We likely got called again after we already
		 * returned the last argument
		 */
		*argptr = __NULL;
		return 0;
	}

	arg = *argptr;
	nsep = strchr(*argptr, separator);
	if (!nsep) {
		/* No next separator, we hit the last argument */
		*argptr = __NULL;

		/* Rest of C-string is last argument */
		return strlen(arg);
	}

	/* Compute the len of current argument */
	arglen = (__sz)((__uptr)nsep - (__uptr)arg);

	/* Skip separator on argptr for subsequent calls */
	*argptr = nsep + 1;
	return arglen;
}

char *uk_nextarg(char **argptr, int separator)
{
	__sz arglen;
	char *arg;

	UK_ASSERT(argptr);

	arg = *argptr;
	arglen = uk_nextarg_r((const char **)argptr, separator);

	/* Return NULL if we are at the end of parsing */
	if (arglen == 0 && *argptr == __NULL)
		return __NULL;

	/* Overwrite separator with terminating character */
	arg[arglen] = '\0';
	return arg;
}

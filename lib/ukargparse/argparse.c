/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/argparse.h>
#include <uk/assert.h>


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
			if (!in_quote) {
				argb[i] = '\0';
				prev_wspace = 1;
			}
			break;

		/* quotes */
		case '\'':
		case '"':
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
			
			/* Fall through */
		default:
			/* any character */
			if (prev_wspace) {
				argv[argc++] = &argb[i];
				prev_wspace = 0;
			}
			break;
		}
	}

out:
	return argc;
}

/*	$OpenBSD: wcio.h,v 1.1 2005/06/17 20:40:32 espie Exp $	*/
/* $NetBSD: wcio.h,v 1.3 2003/01/18 11:30:00 thorpej Exp $ */

/*-
 * Copyright (c)2001 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Citrus$
 */

#ifndef _WCIO_H_
#define _WCIO_H_

/* minimal requirement of SUSv2 */
#define WCIO_UNGETWC_BUFSIZE 1

struct wchar_io_data {
	mbstate_t wcio_mbstate_in;
	mbstate_t wcio_mbstate_out;

	wchar_t wcio_ungetwc_buf[WCIO_UNGETWC_BUFSIZE];
	size_t wcio_ungetwc_inbuf;

	int wcio_mode; /* orientation */
};

#define WCIO_GET(fp) \
	(_EXT(fp) ? &(_EXT(fp)->_wcio) : (struct wchar_io_data *)0)

#define _SET_ORIENTATION(fp, mode) \
do {\
	struct wchar_io_data *_wcio = WCIO_GET(fp); \
	if (_wcio && _wcio->wcio_mode == 0) \
		_wcio->wcio_mode = (mode);\
} while (0)

#define WCIO_INIT(fp) \
	memset(WCIO_GET(fp), 0, sizeof(struct wchar_io_data))

#endif /*_WCIO_H_*/

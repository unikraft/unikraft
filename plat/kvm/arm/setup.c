/* SPDX-License-Identifier: ISC */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2018 Arm Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <libfdt.h>
#include <kvm/console.h>
#include <uk/assert.h>

void *_libkvmplat_dtb;

#define MAX_CMDLINE_SIZE 1024
static char cmdline[MAX_CMDLINE_SIZE];

static void _init_dtb(void *dtb_pointer)
{
	int ret;

	if ((ret = fdt_check_header(dtb_pointer)))
		UK_CRASH("Invalid DTB: %s\n", fdt_strerror(ret));

	_libkvmplat_dtb = dtb_pointer;
	uk_printd(DLVL_INFO, "Found device tree on: %p\n", dtb_pointer);
}

static void _dtb_get_cmdline(char *cmdline, size_t maxlen)
{
	int fdtchosen, len;
	const char *fdtcmdline;

	/* TODO: Proper error handling */
	fdtchosen = fdt_path_offset(_libkvmplat_dtb, "/chosen");
	if (!fdtchosen)
		goto enocmdl;
	fdtcmdline = fdt_getprop(_libkvmplat_dtb, fdtchosen, "bootargs", &len);
	if (!fdtcmdline || (len <= 0))
		goto enocmdl;

	strncpy(cmdline, fdtcmdline, MIN(maxlen, (unsigned int) len));
	/* ensure null termination */
	cmdline[((unsigned int) len - 1) <= (maxlen - 1) ?
		((unsigned int) len - 1) : (maxlen - 1)] = '\0';

	uk_printd(DLVL_INFO, "Command line: %s\n", cmdline);
	return;

enocmdl:
	uk_printd(DLVL_INFO, "No command line found\n");
	strcpy(cmdline, CONFIG_UK_NAME);
}

void _libkvmplat_start(void *dtb_pointer)
{
	_init_dtb(dtb_pointer);
	_libkvmplat_init_console();

	uk_printd(DLVL_INFO, "Entering from KVM (arm64)...\n");

	/* Get command line from DTB */
	_dtb_get_cmdline(cmdline, sizeof(cmdline));
}

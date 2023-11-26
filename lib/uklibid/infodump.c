/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/libid/info.h>
#include <uk/ctors.h>
#include <uk/print.h>
#include <uk/assert.h>

static void libinfo_bootdump(void)
{
	const struct uk_libid_info_hdr *hdr;
	const struct uk_libid_info_rec *rec;
	const char *libname, *version, *license;

	uk_pr_debug("Library info section .uk_libinfo %p - %p\n",
		    uk_libinfo_start, &uk_libinfo_end);
	uk_pr_info("Compiled-in libraries:\n");
	uk_libinfo_hdr_foreach(hdr) {
		if (hdr->version != UKLI_LAYOUT) {
			uk_pr_debug("Unknown library info layout at %p\n", hdr);
			continue;
		}

		libname = __NULL;
		version = "<n/a>";
		license = __NULL;
		uk_libinfo_rec_foreach(hdr, rec) {
			switch (rec->type) {
			case UKLI_REC_LIBNAME:
				libname = rec->data;
				break;
			case UKLI_REC_VERSION:
				version = rec->data;
				break;
			case UKLI_REC_LICENSE:
				version = rec->data;
				break;
			default:
				break;
			}
		}

		/* Print only per library information (skip global info) */
		if (libname) {
			uk_pr_info(" %s (version: %s", libname, version);
			if (license)
				uk_pr_info(", license: %s", license);
			uk_pr_info(")\n");
		}
	}
}

UK_CTOR(libinfo_bootdump);

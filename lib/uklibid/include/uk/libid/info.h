/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_LIBID_INFO_H__
#define __UK_LIBID_INFO_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

/*
 * Library information is stored in the special section " .uk_libinfo". For
 * each library there is one `struct uk_libid_info_hdr` generated. Each header
 * can contain multiple `struct uk_libid_info_rec` records. A library name
 * record (`UKLI_REC_LIBNAME`) is the only mandatory record type for each
 * library.
 * Additionally, a single header is created for global information, which
 * differs from the library headers in that it does not contain a record for a
 * library name.
 *
 * Layout for each information header:
 *
 *                              _____________
 *  +--------------------------+             \
 *  | struct uk_libid_info_hdr |              |
 *  |                          |               > hdr->len
 *  +--------------------------+              |
 *  | struct uk_libid_info_rec |              |
 *  +--------------------------+ -.           |
 *  | struct uk_libid_info_rec |   > rec->len |
 *  |                          |  |           |
 *  +--------------------------+ -`           |
 *  :          . . .           :              :
 *  +--------------------------+              |
 *  | struct uk_libid_info_rec |              |
 *  |                          |              |
 *  +--------------------------+ ____________/
 */

/*
 * NOTE: The layout version needs only be updated if the structs
 *       `uk_libid_info_rec` and `uk_libid_info_hdr` are updated.
 */
 #define UKLI_LAYOUT              0x0001

/*
 * NOTE: In order to keep backwards compatibility with pre-compiled
 *       libraries, this list should only be extended with new entries.
 */
#define UKLI_REC_LIBNAME          0x0001 /* C-string, absent for global hdr */
#define UKLI_REC_COMMENT          0x0002 /* C-string */
#define UKLI_REC_VERSION          0x0003 /* C-string */
#define UKLI_REC_LICENSE          0x0004 /* C-string */
#define UKLI_REC_GITDESC          0x0005 /* C-string */
#define UKLI_REC_UKVERSION        0x0006 /* C-string, Unikraft version only */
#define UKLI_REC_UKFULLVERSION    0x0007 /* C-string, UK version with gitsha */
#define UKLI_REC_UKCODENAME       0x0008 /* C-string */
#define UKLI_REC_UKCONFIG         0x0009 /* binary data */
#define UKLI_REC_UKCONFIGGZ       0x000A /* binary data */
#define UKLI_REC_COMPILER         0x000B /* C-string */
#define UKLI_REC_COMPILEDATE      0x000C /* C-string */
#define UKLI_REC_COMPILEDBY       0x000D /* C-string */
#define UKLI_REC_COMPILEDBYASSOC  0x000E /* C-string */
#define UKLI_REC_COMPILEOPTS      0x000F /* __u32 flags */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_libid_info_rec {
	__u16 type;    /* record type */
	__u32 len;     /* including record header */
	char  data[];  /* raw data */
} __packed __align(1);

/*
 * NOTE: Due to binary cross-compatibility, the sizes, offsets, data types and
 *       meanings must remain the same regardless of the layout version:
 *       `len` and `version`.
 */
struct uk_libid_info_hdr {
	__u32 len;     /* including header */
	__u16 version; /* layout version */
	struct uk_libid_info_rec recs[];
} __packed __align(1);

extern const struct uk_libid_info_hdr uk_libinfo_start[];
extern const struct uk_libid_info_hdr uk_libinfo_end;

#define uk_libinfo_hdr_foreach(hdr_itr)					       \
	for ((hdr_itr) = (const struct uk_libid_info_hdr *)uk_libinfo_start;   \
	     (hdr_itr) < &(uk_libinfo_end);				       \
	     (hdr_itr) = (const struct uk_libid_info_hdr *)((__uptr)(hdr_itr)  \
							    + (hdr_itr)->len))

#define uk_libinfo_rec_foreach(hdr, rec_itr)				       \
	for ((rec_itr) = (const struct uk_libid_info_rec *) &hdr->recs[0];     \
	     (__uptr)(rec_itr) < ((__uptr)hdr + hdr->len);		       \
	     (rec_itr) = (const struct uk_libid_info_rec *)((__uptr)(rec_itr)  \
							    + (rec_itr)->len))

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LIBID_INFO_H__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/config.h>
#include <uk/libid/info.h>
#include <uk/arch/types.h>

#define RECORD(type, data)				  \
	.align 1					; \
1:	.__u16 type			/* type */	; \
	.__u32 2f - 1b			/* len */	; \
	data				/* data[] */	; \
2:	.align 1

.pushsection .uk_libinfo, "a"
	.align 1
3:	.__u32 4f - 3b			/* len */
	.__u16 UKLI_LAYOUT		/* version */

#if __GLOBALINFO__
	/* Only one global header per unikernel is generated */
	/* NOTE: Global metadata do not have a libname record */
#if CONFIG_LIBUKLIBID_INFO_UKFULLVERSION
	RECORD(UKLI_REC_UKFULLVERSION, .asciz STRINGIFY(UK_FULLVERSION))
#else /* !CONFIG_LIBUKLIBID_INFO_UKFULLVERSION */
	RECORD(UKLI_REC_UKVERSION,     .asciz STRINGIFY(UK_VERSION))
#endif /* !CONFIG_LIBUKLIBID_INFO_UKFULLVERSION */
#if CONFIG_LIBUKLIBID_INFO_UKCODENAME
	RECORD(UKLI_REC_UKCODENAME,    .asciz STRINGIFY(UK_CODENAME))
#endif /* CONFIG_LIBUKLIBID_INFO_UKCODENAME */
#if CONFIG_LIBUKLIBID_INFO_COMPILER
	RECORD(UKLI_REC_COMPILER,
	       .asciz STRINGIFY(__LIBUKLIBID_COMPILER__))
#endif /* CONFIG_LIBUKLIBID_INFO_COMPILER */
#if CONFIG_LIBUKLIBID_INFO_UKCONFIGGZ
	RECORD(UKLI_REC_UKCONFIGGZ,    .incbin STRINGIFY(UK_CONFIGGZINC))
#endif /* CONFIG_LIBUKLIBID_INFO_UKCONFIGGZ */

#else /* !__GLOBALINFO__ */
	/* Records per library */
	/* NOTE: The libname record is mandatory */
	RECORD(UKLI_REC_LIBNAME,       .asciz STRINGIFY(__LIBNAME__))
#if CONFIG_LIBUKLIBID_INFO_LIB_UKVERSION
#if CONFIG_LIBUKLIBID_INFO_LIB_UKFULLVERSION
	RECORD(UKLI_REC_UKFULLVERSION, .asciz STRINGIFY(UK_FULLVERSION))
#else /* !CONFIG_LIBUKLIBID_INFO_LIB_UKVERSION */
	RECORD(UKLI_REC_UKVERSION,     .asciz STRINGIFY(UK_VERSION))
#endif /* !CONFIG_LIBUKLIBID_INFO_LIB_UKVERSION */
#endif /* CONFIG_LIBUKLIBID_INFO_LIB_UKVERSION */
#if CONFIG_LIBUKLIBID_INFO_LIB_UKCODENAME
	RECORD(UKLI_REC_UKCODENAME,    .asciz STRINGIFY(UK_CODENAME))
#endif /* CONFIG_LIBUKLIBID_INFO_LIB_UKCODENAME */
#if CONFIG_LIBUKLIBID_INFO_LIB_COMPILER
	RECORD(UKLI_REC_COMPILER,
	       .asciz STRINGIFY(__LIBUKLIBID_COMPILER__))
#endif /* CONFIG_LIBUKLIBID_INFO_COMPILER */
	RECORD(UKLI_REC_COMPILEOPTS,   .__u32 (
			(!(!__LIBUKLIBID_COMPILEOPT_PIE__) * UKLI_REC_CO_PIE) |\
			(!(!__LIBUKLIBID_COMPILEOPT_DCE__) * UKLI_REC_CO_DCE) |\
			(!(!__LIBUKLIBID_COMPILEOPT_LTO__) * UKLI_REC_CO_LTO)))
#endif /* !__GLOBALINFO__ */

4:	.align 1
.popsection

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/libparam.h>

SECTIONS {
	. = ALIGN(8);
	PROVIDE(UK_LIBPARAM_PARAMSECTION_STARTSYM = .);
	.UK_LIBPARAM_PARAMSECTION_NAME : {
		KEEP(*(SORT_BY_NAME(.UK_LIBPARAM_PARAMSECTION_NAME)));
	}
	PROVIDE(UK_LIBPARAM_PARAMSECTION_ENDSYM = .);
}
INSERT AFTER .rodata

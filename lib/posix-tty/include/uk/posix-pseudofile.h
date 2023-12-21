/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_PSEUDOFILE_H__
#define __UK_POSIX_PSEUDOFILE_H__

#include <uk/file.h>

const struct uk_file *uk_nullfile_create(void);
const struct uk_file *uk_voidfile_create(void);
const struct uk_file *uk_zerofile_create(void);

#endif /* __UK_POSIX_PSEUDOFILE_H__ */

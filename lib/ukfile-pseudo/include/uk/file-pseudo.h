/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Pseudofiles for Unikraft that ignore all writes */

#ifndef __UK_FILE_PSEUDO_H__
#define __UK_FILE_PSEUDO_H__

#include <uk/file.h>

/* Public references with static lifetimes */

/* Null file: read always at EOF */
extern const struct uk_file uk_file_null;
/* Zero file: always reads zero */
extern const struct uk_file uk_file_zero;
/* Void file: read always returns -EAGAIN */
extern const struct uk_file uk_file_void;

#endif /* __UK_FILE_PSEUDO_H__ */

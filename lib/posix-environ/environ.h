/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __ENVIRON_H__
#define __ENVIRON_H__

#include <uk/config.h>
#include <stdlib.h>

extern char **__environ;

int __putenv(char *s, size_t l, char *r);
void __env_rm_add(char *old, char *new);

#endif /* __ENVIRON_H__ */

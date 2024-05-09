/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __ENVIRON_H__
#define __ENVIRON_H__

#include <uk/config.h>
#include <stdlib.h>

/**
 * __environ - An array of key-value pairs that holds the environment variables
 */
extern char **__environ;

/**
 * Adds the new environment variable name-value pair in the `__environ` array
 * if it doesn't already exist, or replaces the old value with the new value
 * if it exists.
 *
 * @param s
 *   The name of the environment variable to be set
 * @param l
 *   The length of the environment variable name
 * @param r
 *   The value of the environment variable to be set
 * @return
 *   0 in case of success, -1 in case of error
 */
int __putenv(char *s, size_t l, char *r);

/**
 * Replaces the old value with the new value.
 *
 * @param old
 *   The old value to be replaced
 * @param new
 *   The new value to replace it with
 */
void __env_rm_add(char *old, char *new);

#endif /* __ENVIRON_H__ */

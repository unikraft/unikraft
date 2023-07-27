/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CONSOLE_H__
#define __UK_CONSOLE_H__

#include <uk/essentials.h>
#include <uk/plat/common/bootinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

/* console operations */
struct uk_console_ops {
	int (*coutk)(const char *buf, unsigned int len);
	int (*coutd)(const char *buf, unsigned int len);
	int (*cink)(char *buf, unsigned int maxlen);
	void (*init)(struct ukplat_bootinfo *bi);
};

/**
 * Outputs a string to kernel console
 * Note that printing does not stop on null termination
 * @param buf Buffer with characters
 * @param len Length of string buffer (if 0 str has to be zero-terminated),
 *            < 0 ignored
 * @return Number of printed characters, errno on < 0
 */
int uk_console_coutk(const char *buf, unsigned int len);

/**
 * Outputs a string to debug console
 * Note that printing does not stop on null termination
 * @param buf Buffer with characters
 * @param len Length of string buffer (if 0 str has to be zero-terminated)
 * @return Number of printed characters, errno on < 0
 */
int uk_console_coutd(const char *buf, unsigned int len);

/**
 * Reads characters from kernel console
 * Note that returned buf is not null terminated.
 * @param buf Target buffer
 * @param len Length of string buffer
 * @return Number of read characters, errno on < 0
 */
int uk_console_cink(char *buf, unsigned int maxlen);

/**
 * Initializes console
 *
 * @param bi Pointer to bootinfo
 */
void uk_console_init(struct ukplat_bootinfo *bi);

#ifdef __cplusplus
}
#endif

#endif /* __UK_CONSOLE_H__ */

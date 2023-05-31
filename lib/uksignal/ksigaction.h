/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_KSIGACTION_H__
#define __UK_KSIGACTION_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Imported from musl 1.2.3 */
/* In line with the Linux kernel struct for x86_64 and aarch64. */

struct k_sigaction {
        void (*handler)(int);
        unsigned long flags;
        void (*restorer)(void);
        unsigned mask[2];
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_KSIGACTION_H__ */

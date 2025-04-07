/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_BOOT_INIT_PRIV_H__
#define __UK_BOOT_INIT_PRIV_H__

/**
 * INTERNAL. main() wrapper
 *
 * Calls application's constructors follwed up by main().
 *
 * @param argc Arg count
 * @param argc Arg vector
 * @return The return value of main()
 */
int do_main(int argc, char *argv[]);

#if CONFIG_LIBUKBOOT_INIT
/**
 * INTERNAL. /sbin/init logic
 *
 * Spawns a new process that executes do_main(), fosters orphans,
 * reaps children, and triggers (graceful) shutdown in response of
 * SIGTERM.
 *
 * @param argc Arg count
 * @param argc Arg vector
 * @return The return value of main(), or SIGKILL | 0xff if the
 *         application did not exit gracefully.
 */
int do_init(int argc, char *argv[]);
#endif /* CONFIG_LIBUKBOOT_INIT */

#endif /* __UK_BOOT_INIT_PRIV_H__ */

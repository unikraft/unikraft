/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UK_PROCESS_H__
#define __UK_PROCESS_H__

#include <stdbool.h>
#include <stddef.h> /* NULL */
#include <sys/resource.h>
#include <sys/types.h> /* pid_t */

#include <uk/config.h>
#include <uk/essentials.h>

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#include <arch/clone.h>
#include <uk/event.h>
#include <uk/prio.h>
#include <uk/thread.h>

#include <uk/process/events.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING  */

int uk_sys_prlimit64(int pid, unsigned int resource,
		     struct rlimit *new_limit, struct rlimit *old_limit);

static inline int uk_sys_getrlimit(int resource, struct rlimit *rlim)
{
	return uk_sys_prlimit64(0, resource, NULL, rlim);
}

static inline int uk_sys_setrlimit(int resource, const struct rlimit *rlim)
{
	return uk_sys_prlimit64(0, resource,
				DECONST(struct rlimit *, rlim), NULL);
}

pid_t uk_sys_gettid(void);
pid_t uk_sys_getppid(void);
pid_t uk_sys_getpid(void);

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

/* Creates a pthread and attaches it to the current process
 *
 * DO NOT USE. This is only necessary when loading an ELF via initrd,
 * where we want to POSIXise the kernel thread, as initrd does not
 * support paths and therefore we cannot execve(). For more details
 * see the implementation of app-elfloader.
 */
int uk_posix_process_create_pthread(struct uk_thread *thread);

#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#if CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS

/**
 * Main-like function that calls exit() when done instead of returning.
 *
 * Parameters are passed as void *, and are to be cast by the implementation to:
 * - argc: int
 * - argv: char * []
 */
typedef __noreturn void (*uk_posix_process_mainlike_func)(void *argc,
							  void *argv);

/**
 * Spawn a process that jumps into function.
 *
 * DO NOT USE. This is only necessary when we create a new process
 * for main() in multiprocess.
 *
 * @param fn      Function to jump to.
 * @param argc    Arg count
 * @param argv    Arg vector
 * @return        Child pid to parent, or negative value on failure.
 */
pid_t uk_posix_process_run(uk_posix_process_mainlike_func fn,
			   int argc, const char **argv);
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */

#endif /* __UK_PROCESS_H__ */

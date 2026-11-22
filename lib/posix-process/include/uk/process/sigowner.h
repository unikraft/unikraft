/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __UK_PROCESS_SIGOWNER_H__
#define __UK_PROCESS_SIGOWNER_H__

#include <sys/types.h>

/* A positive owner names a process, a negative owner names a group. */
int uk_sigowner_acquire(pid_t owner);
void uk_sigowner_release(pid_t owner);
/* The caller keeps the owner identity pinned for the duration of delivery. */
int uk_sigowner_sigio(pid_t owner);

#endif /* __UK_PROCESS_SIGOWNER_H__ */

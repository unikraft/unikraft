/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_POSIX_PROCESS_PREPARED_H__
#define __UK_POSIX_PROCESS_PREPARED_H__

#include <uk/config.h>

#if CONFIG_LIBPOSIX_PROCESS_PREPARE && CONFIG_LIBUKVMEM_PROCESS_VAS
#include <sys/types.h>
#include <uk/alloc.h>
#include <uk/thread.h>
#include <uk/vmem.h>

/**
 * Prepare an initial process on a fresh owned address space.
 *
 * The child must be a new, unscheduled, blocked UK thread with UKTLS created
 * by the current parent. The caller exclusively owns it and keeps the parent
 * alive. Descriptor and directory state and the signal mask are inherited.
 * Normal clone callbacks clear caught signal actions. Exec callbacks close
 * CLOEXEC descriptors before returning. The process receives a fresh brk area
 * when enabled. No PID is published and no thread is registered or woken.
 *
 * On success return the reserved PID and one owned VAS reference in *vas.
 * The child and its process hold separate VAS references. The caller may load
 * the image and build its user stack before publication. On failure *vas is
 * NULL; the caller must release the child, including partially initialized
 * UKTLS resources. Do not retry preparation on the same failed child.
 * Requires interrupts enabled and no caller locks. A modern per-process
 * descriptor table is required; unsupported table modes return -ENOTSUP.
 */
pid_t uk_posix_process_prepare_spawn(struct uk_alloc *a,
				     struct uk_thread *child,
				     struct uk_vas **vas);

/**
 * Publish a fully initialized prepared child without waking it.
 * It must remain blocked but may already be registered with a scheduler.
 * On failure it stays unpublished. On success use normal process lifetime
 * management instead of aborting it, then wake it when ready to run.
 * Requires interrupts enabled and exclusive ownership of the child.
 */
int uk_posix_process_publish_prepared(struct uk_thread *child);

/**
 * Abort a prepared child without publishing it or sending exit events.
 * The child must be unscheduled. The caller subsequently releases the UK
 * thread and any VAS reference returned by prepare_spawn(). NULL is accepted.
 * Published processes are rejected. Requires interrupts enabled.
 */
int uk_posix_process_abort_prepared(struct uk_thread *child);
#endif

#endif /* __UK_POSIX_PROCESS_PREPARED_H__ */

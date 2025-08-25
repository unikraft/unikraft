/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __POSIX_PROCESS_PROCESS_EVENTS_H__
#define __POSIX_PROCESS_PROCESS_EVENTS_H__

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>       /* CLONE_* constants */
#include <linux/sched.h> /* struct clone_args */

#include <uk/event.h>
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBPOSIX_PROCESS_MULTITHREADING

/* In case a libC is defining only a subset of our currently supported clone
 * flags, we provide here a completion of the list
 */
#ifndef CLONE_NEWTIME
#define CLONE_NEWTIME		0x00000080
#endif /* !CLONE_NEWTIME */
#ifndef CLONE_VM
#define CLONE_VM		0x00000100
#endif /* !CLONE_VM */
#ifndef CLONE_FS
#define CLONE_FS		0x00000200
#endif /* !CLONE_FS */
#ifndef CLONE_FILES
#define CLONE_FILES		0x00000400
#endif /* !CLONE_FILES */
#ifndef CLONE_SIGHAND
#define CLONE_SIGHAND		0x00000800
#endif /* !CLONE_SIGHAND */
#ifndef CLONE_PIDFD
#define CLONE_PIDFD		0x00001000
#endif /* !CLONE_PIDFD */
#ifndef CLONE_PTRACE
#define CLONE_PTRACE		0x00002000
#endif /* !CLONE_PTRACE */
#ifndef CLONE_VFORK
#define CLONE_VFORK		0x00004000
#endif /* !CLONE_VFORK */
#ifndef CLONE_PARENT
#define CLONE_PARENT		0x00008000
#endif /* !CLONE_PARENT */
#ifndef CLONE_THREAD
#define CLONE_THREAD		0x00010000
#endif /* !CLONE_THREAD */
#ifndef CLONE_NEWNS
#define CLONE_NEWNS		0x00020000
#endif /* !CLONE_NEWNS */
#ifndef CLONE_SYSVSEM
#define CLONE_SYSVSEM		0x00040000
#endif /* !CLONE_SYSVSEM */
#ifndef CLONE_SETTLS
#define CLONE_SETTLS		0x00080000
#endif /* !CLONE_SETTLS */
#ifndef CLONE_PARENT_SETTID
#define CLONE_PARENT_SETTID	0x00100000
#endif /* !CLONE_PARENT_SETTID */
#ifndef CLONE_CHILD_CLEARTID
#define CLONE_CHILD_CLEARTID	0x00200000
#endif /* !CLONE_CHILD_CLEARTID */
#ifndef CLONE_DETACHED
#define CLONE_DETACHED		0x00400000
#endif /* !CLONE_DETACHED */
#ifndef CLONE_UNTRACED
#define CLONE_UNTRACED		0x00800000
#endif /* !CLONE_UNTRACED */
#ifndef CLONE_CHILD_SETTID
#define CLONE_CHILD_SETTID	0x01000000
#endif /* !CLONE_CHILD_SETTID */
#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP		0x02000000
#endif /* !CLONE_NEWCGROUP */
#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS		0x04000000
#endif /* !CLONE_NEWUTS */
#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC		0x08000000
#endif /* !CLONE_NEWIPC */
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER		0x10000000
#endif /* !CLONE_NEWUSER */
#ifndef CLONE_NEWPID
#define CLONE_NEWPID		0x20000000
#endif /* !CLONE_NEWPID */
#ifndef CLONE_NEWNET
#define CLONE_NEWNET		0x40000000
#endif /* !CLONE_NEWNET */
#ifndef CLONE_IO
#define CLONE_IO		0x80000000
#endif /* !CLONE_IO */
#ifndef CLONE_CLEAR_SIGHAND
#define CLONE_CLEAR_SIGHAND	0x100000000ULL
#endif

/**
 * Data delivered to the handlers of POSIX_PROCESS_CLONE_EVENT
 * When this event is raised for PID 1, ppid and parent are set
 * to zero and NULL respectively.
 */
struct posix_process_clone_event_data {
	const struct clone_args *cl_args;
	size_t cl_args_len;
	struct uk_thread *child;
	struct uk_thread *parent;
	pid_t ppid;
	pid_t pid;
	pid_t tid;
};

/**
 * POSIX_PROCESS_CLONE_EVENT handler function
 *
 * @param event_data Pointer to event data. Handler should cast to
 *		     struct posix_process_clone_event_data.
 * @return UK_EVENT_HANDLED_CONT if the event is handled,
 *	   UK_EVENT_NOT_HANDLED if the event is not handled,
 *	   or negative value on error.
 */
typedef int (*posix_process_clone_handler_fn)(void *event_data);

struct posix_process_clonetab_entry {
	__u64 flags_mask;
};

#define __POSIX_PROCESS_CLONETAB_ENTRY(arg_flags_mask, handler_fn)	    \
	static const struct posix_process_clonetab_entry		    \
	__used __section(".posix_process_clonetab") __align(8)		    \
		__posix_process_clonetab ## _ ## handler_fn = { \
		.flags_mask = (arg_flags_mask),				    \
	}

#define _POSIX_PROCESS_CLONETAB_ENTRY(flags_mask, handler_fn)	\
	__POSIX_PROCESS_CLONETAB_ENTRY(flags_mask, handler_fn)

/**
 * Registers a handler that is called when a pprocess / pthread is created.
 *
 * @param flags_mask	clone() flags managed by this handler. This is merely
 *                      a declaration from the handler's side that assists
 *                      libposix-process perform runtime checks that at least
 *                      one handler has been registered for each flag requested
 *                      by clone.
 * @param handler_fn	Handler that is called during clone. The handler is
 *                      called regardless of the flags passed in flags_mask.
 * @parm prio		Call order priority for this handler.
 */
#define POSIX_PROCESS_CLONE_HANDLER_PRIO(flags_mask, handler_fn, prio)	    \
		_POSIX_PROCESS_CLONETAB_ENTRY(flags_mask, handler_fn);	    \
		UK_EVENT_HANDLER_PRIO(POSIX_PROCESS_CLONE_EVENT,	    \
				      handler_fn, prio)			    \

/**
 * Same as POSIX_PROCESS_CLONE_HANDLER_PRIO() with prio set to UK_PRIO_LATEST
 */
#define POSIX_PROCESS_CLONE_HANDLER(flags_mask, handler_fn)		    \
	POSIX_PROCESS_CLONE_HANDLER_PRIO(flags_mask, handler_fn, UK_PRIO_LATEST)

#if CONFIG_LIBPOSIX_PROCESS_EXECVE

/**
 * Data delivered to the handlers of POSIX_PROCESS_EXECVE_EVENT
 */
struct posix_process_execve_event_data {
	struct uk_thread *thread;
	pid_t pid;
	pid_t tid;
};

/**
 * POSIX_PROCESS_EXECVE_EVENT handler function
 *
 * @param event_data Pointer to event data. Handler should cast to
 *		     struct posix_process_execve_event_data.
 * @return UK_EVENT_HANDLED_CONT if the event is handled,
 *	   UK_EVENT_NOT_HANDLED if the event is not handled,
 *	   or negative value on error.
 */
typedef int (*posix_process_execve_handler_fn)(void *event_data);

/**
 * Registers a handler that is called when a pprocess calls execve().
 *
 * @param init_fn   Handler function
 * @param prio	    Call order priority for this handler
 */
#define POSIX_PROCESS_EXECVE_HANDLER_PRIO(handler_fn, prio)		    \
	UK_EVENT_HANDLER_PRIO(POSIX_PROCESS_EXECVE_EVENT,		    \
			      handler_fn, prio)

/**
 * Same as POSIX_PROCESS_EXECVE_HANDLER_PRIO() with prio set to UK_PRIO_LATEST
 */
#define POSIX_PROCESS_EXECVE_HANDLER(handler_fn)			    \
	POSIX_PROCESS_EXECVE_HANDLER_PRIO(handler_fn, UK_PRIO_LATEST)

#endif /* CONFIG_LIBPOSIX_PROCESS_EXECVE */

/**
 * Data delivered to the handlers of POSIX_PROCESS_PTEXIT_EVENT.
 */
struct posix_process_ptexit_event_data {
	struct uk_thread *thread;
	pid_t tid;
};

/**
 * POSIX_PROCESS_PTEXIT_EVENT handler function
 *
 * Notice that the exit event may be issued as a means of rollback upon
 * failure during process creation. Handlers shall not assume a complete
 * process state.
 *
 * @param event_data Pointer to event data. Handler should cast to
 *		     struct posix_process_ptexit_event_data.
 * @return UK_EVENT_HANDLED_CONT if the event is handled,
 *	   UK_EVENT_NOT_HANDLED if the event is not handled.
 *	   Handlers of this event cannot return an error.
 */
typedef void (*posix_process_ptexit_handler_fn)(void *event_data);

/**
 * Registers a handler that is called when a pthread is terminated.
 *
 * @param init_fn   Handler function
 * @param prio	    Call order priority for this handler
 */
#define POSIX_PROCESS_PTEXIT_HANDLER_PRIO(handler_fn, prio)		    \
	UK_EVENT_HANDLER_PRIO(POSIX_PROCESS_PT_EXIT_EVENT,		    \
			      handler_fn, prio)

/**
 * Same as POSIX_PROCESS_EXIT_HANDLER_PRIO() with prio set to UK_PRIO_LATEST
 */
#define POSIX_PROCESS_PTEXIT_HANDLER(handler_fn)			    \
	POSIX_PROCESS_PTEXIT_HANDLER_PRIO(handler_fn, UK_PRIO_LATEST)

/**
 * Data delivered to the handlers of POSIX_PROCESS_PPEXIT_EVENT.
 */
struct posix_process_ppexit_event_data {
	pid_t pid;
};

/**
 * POSIX_PROCESS_PPEXIT_EVENT handler function
 *
 * Notice that the exit event may be issued as a means of rollback upon
 * failure during process creation. Handlers shall not assume a complete
 * process state.
 *
 * @param event_data Pointer to event data. Handler should cast to
 *		     struct posix_process_ppexit_event_data.
 * @return UK_EVENT_HANDLED_CONT if the event is handled,
 *	   UK_EVENT_NOT_HANDLED if the event is not handled.
 *	   Handlers of this event cannot return an error.
 */
typedef void (*posix_process_ppexit_handler_fn)(void *event_data);

/**
 * Registers a handler that is called when a pprocess is terminated.
 *
 * @param init_fn   Handler function
 * @param prio	    Call order priority for this handler
 */
#define POSIX_PROCESS_PPEXIT_HANDLER_PRIO(handler_fn, prio)		    \
	UK_EVENT_HANDLER_PRIO(POSIX_PROCESS_PPEXIT_EVENT,		    \
			      handler_fn, prio)

/**
 * Same as POSIX_PROCESS_PPEXIT_HANDLER_PRIO() with prio set to UK_PRIO_LATEST
 */
#define POSIX_PROCESS_PPEXIT_HANDLER(handler_fn)			    \
	POSIX_PROCESS_PPEXIT_HANDLER_PRIO(handler_fn, UK_PRIO_LATEST)

#endif /* CONFIG_LIBPOSIX_PROCESS_MULTITHREADING */

#ifdef __cplusplus
}
#endif
#endif /* __POSIX_PROCESS_EVENTS_H__ */

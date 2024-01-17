/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Thread definitions
 * Ported from Mini-OS
 */
#ifndef __UK_THREAD_H__
#define __UK_THREAD_H__

#include <stdint.h>
#include <stdbool.h>
#include <uk/alloc.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/time.h>
#include <uk/arch/ctx.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/tls.h>
#include <uk/wait_types.h>
#include <uk/list.h>
#include <uk/prio.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_sched;

typedef void (*uk_thread_dtor_t)(struct uk_thread *);
typedef void (*uk_thread_gc_t)(struct uk_thread *, void *);

typedef void (*uk_thread_fn0_t)(void) __noreturn;
typedef void (*uk_thread_fn1_t)(void *) __noreturn;
typedef void (*uk_thread_fn2_t)(void *, void *) __noreturn;

struct uk_thread {
	struct ukarch_ctx    ctx;	/**< Architecture context */
	struct ukarch_ectx *ectx;	/**< Extended context (FPU, VPU, ...) */
	uintptr_t           tlsp;	/**< Current active TLS pointer */
	__uptr            uktlsp;	/**< Unikraft TLS pointer */
	__uptr		   auxsp;	/**< Unikraft Auxiliary Stack Pointer */

	UK_TAILQ_ENTRY(struct uk_thread) queue;
	uint32_t flags;
	__snsec wakeup_time;
	struct uk_sched *sched;

	struct {
		struct uk_alloc *t_a;
		void            *stack;
		struct uk_alloc *stack_a;
		void            *uktls;
		struct uk_alloc *uktls_a;
		void            *auxstack;
		struct uk_alloc *auxstack_a;
	} _mem;				/**< Associated allocs (internal!) */
	uk_thread_gc_t _gc_fn;		/**< Extra gc function (internal!) */
	void *_gc_argp;			/**< Argument for gc fn (internal!) */

	uk_thread_dtor_t dtor;		/**< User provided destructor */
	void *priv;			/**< Private field, free for use */

	__nsec exec_time;		/**< Time the thread was scheduled */
	const char *name;		/**< Reference to thread name */
	UK_TAILQ_ENTRY(struct uk_thread) thread_list;
};

UK_TAILQ_HEAD(uk_thread_list, struct uk_thread);

#define uk_thread_terminate(thread) \
	uk_sched_thread_terminate(thread)
#define uk_thread_exit() \
	uk_sched_thread_exit()

/* managed by sched.c */
extern UKPLAT_PER_LCPU_DEFINE(struct uk_thread *, __uk_sched_thread_current);

static inline
struct uk_thread *uk_thread_current(void)
{
	return ukplat_per_lcpu_current(__uk_sched_thread_current);
}

/*
 * STATES OF THREADS
 * =================
 *
 * The state of a thread can be determined by checking for the
 * `UK_THREADF_RUNNABLE` and `UK_THREADF_EXITED` flags. A thread can then be in
 * one of three states: BLOCKED, RUNNABLE, EXITED
 * (state flags legend: [<R=Runnable><E=Exited>]):
 *
 *     uk_thread create()/init()
 *           |     |      _____
 *           |     |     /     \
 *           |     v    v       |
 *           |    BLOCKED [--]  |
 *           |  /  |            |
 *            \|   | wake()     | block()
 *             \   |            |
 *             |\  |            |
 *             | v v            |
 * terminate() |  RUNNABLE [R-] |
 *             |   |     \_____/
 *             |   |
 *             |   | terminate()
 *             |   v
 *              > EXITED [*E]
 *                 |
 *                 | release()
 *                 v
 *                (X)
 *
 * NOTE: Depending on the API used for thread creation, a thread is created in
 *       runnable or blocked state.
 * NOTE: The thread initialization callbacks are called on any of the thread
 *       creation/initialization APIs.
 *       The termination callbacks are called when the thread transitions to the
 *       exited state. Calling `uk_thread_release()` on a non-exited thread will
 *       transition the thread to the exited state first and will cause the
 *       termination callbacks as well.
 */

#define UK_THREADF_ECTX       (0x001)	/**< Extended context available */
#define UK_THREADF_AUXSP      (0x002)	/**< Thread has auxiliary stack */
#define UK_THREADF_UKTLS      (0x004)	/**< Unikraft allocated TLS */
#define UK_THREADF_RUNNABLE   (0x008)
#define UK_THREADF_EXITED     (0x010)
/*
 *  A flag used for marking that a thread could potentially
 *  be added to the run queue. A thread marked as such must
 *  neither be running (except when it's about to be scheduled
 *  out from the CPU during a context switch), nor be already
 *  present in the run queue.
 */
#define UK_THREADF_QUEUEABLE  (0x020)

#define uk_thread_is_exited(t)   ((t)->flags & UK_THREADF_EXITED)
#define uk_thread_is_runnable(t) (!uk_thread_is_exited(t) \
				  && ((t)->flags & UK_THREADF_RUNNABLE))
#define uk_thread_is_blocked(t)  ((t)->flags &				\
				  (UK_THREADF_EXITED | UK_THREADF_RUNNABLE) == \
				  0x0)
#define uk_thread_is_queueable(t) ((t)->flags & UK_THREADF_QUEUEABLE)

#define uk_thread_set_runnable(t) \
	do { (t)->flags |= UK_THREADF_RUNNABLE; } while (0)
#define uk_thread_set_blocked(t) \
	do { (t)->flags &= ~UK_THREADF_RUNNABLE; } while (0)
#define uk_thread_set_queueable(t) \
	do { (t)->flags |= UK_THREADF_QUEUEABLE; } while (0)
#define uk_thread_clear_queueable(t) \
	do { (t)->flags &= ~UK_THREADF_QUEUEABLE; } while (0)
/* NOTE: Setting a thread as EXITED cannot be undone. */
/* NOTE: Never change the EXIT flag manually. Trnasition to exit state reqiures
 * the terminate funcrtiomns to be called.
 */
void uk_thread_set_exited(struct uk_thread *t);

/*
 * WARNING: The following functions allow threads being created without extended
 *          context (ectx) and without or a custom TLS. Such threads are
 *          intended for very special cases. Threads without ectx must be
 *          ISR-safe routines or need to be never interrupted until completed.
 *          Threads with a custom or without TLS cannot execute standard
 *          Unikraft code that potentially makes use of the system TLS.
 */

/**
 * Initializes a given uk_thread structure. Such a thread can then be
 * assigned to a scheduler. No register initialization (reset)
 * is done.
 * NOTE: On releasing, only the destructor `dtor` is called,
 *       no memory is released.
 *
 * @param t
 *   Reference to uk_thread structure to initialize
 * @param ip
 *   Instruction pointer. When `ip != NULL`, the flag
 *   UK_THREADF_RUNNABLE is set.
 * @param sp
 *   Stack pointer
 * @param auxsp
 *   Auxiliary stack pointer
 * @param tlsp
 *   Architecture pointer to TLS. If set to NULL, the thread cannot
 *   access thread-local variables.
 * @param is_uktls
 *   Indicates that the given TLS pointer (`tlsp` must be != 0x0)
 *   points to a TLS derived from the Unikraft system TLS template.
 * @param ectx
 *   Reference to memory for saving/restoring extended CPU context
 *   (e.g., floating point, vector registers). If set to `NULL`, no
 *   extended context is saved nor restored on thread switches.
 *   Executed functions must be ISR-safe. `UK_THREADF_ECTX` is set
 *   when ectx is given.
 * @param name
 *   Optional reference to a name for the thread
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (0):  Successfully initialized
 *   - (<0): Negative value with error code
 */
int uk_thread_init_bare(struct uk_thread *t,
			uintptr_t ip,
			uintptr_t sp,
			uintptr_t auxsp,
			uintptr_t tlsp,
			bool is_uktls,
			struct ukarch_ectx *ectx,
			const char *name,
			void *priv,
			uk_thread_dtor_t dtor);

/**
 * Initializes a given uk_thread structure. Such a thread can then be
 * assigned to a scheduler. When the thread starts, the general-purpose
 * registers are reset. The thread is set with
 * `UK_THREADF_RUNNABLE`.
 * NOTE: On releasing, only the destructor `dtor` is called,
 *       no memory is released.
 *
 * @param t
 *   Reference to uk_thread structure to initialize
 * @param fn
 *   Thread entry function (required)
 * @param sp
 *   Architecture stack pointer (stack is required)
 * @param auxsp
 *   Auxiliary stack pointer
 * @param tlsp
 *   Architecture pointer to TLS. If set to NULL, the thread cannot
 *   access thread-local variables.
 * @param is_uktls
 *   Indicates that the given TLS pointer (`tlsp` must be != 0x0)
 *   points to a TLS derived from the Unikraft system TLS template.
 * @param ectx
 *   Reference to memory for saving/restoring extended CPU context
 *   (e.g., floating point, vector registers). If set to `NULL`, no
 *   extended context is saved nor restored on thread switches.
 *   Executed functions must be ISR-safe. `UK_THREADF_ECTX` is set
 *   when ectx is given.
 * @param name
 *   Optional reference to a name for the thread
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (0):  Successfully initialized
 *   - (<0): Negative value with error code
 */
int uk_thread_init_bare_fn0(struct uk_thread *t,
			    uk_thread_fn0_t fn,
			    uintptr_t sp,
			    uintptr_t auxsp,
			    uintptr_t tlsp,
			    bool is_uktls,
			    struct ukarch_ectx *ectx,
			    const char *name,
			    void *priv,
			    uk_thread_dtor_t dtor);

/**
 * Similar to `uk_thread_init_bare_fn0()` but with a thread function accepting
 * one argument
 */
int uk_thread_init_bare_fn1(struct uk_thread *t,
			    uk_thread_fn1_t fn,
			    void *argp,
			    uintptr_t sp,
			    uintptr_t auxsp,
			    uintptr_t tlsp,
			    bool is_uktls,
			    struct ukarch_ectx *ectx,
			    const char *name,
			    void *priv,
			    uk_thread_dtor_t dtor);

/**
 * Similar to `uk_thread_init_bare_fn0()` but with a thread function accepting
 * two arguments
 */
int uk_thread_init_bare_fn2(struct uk_thread *t,
			    uk_thread_fn2_t fn,
			    void *argp0, void *argp1,
			    uintptr_t sp,
			    uintptr_t auxsp,
			    uintptr_t tlsp,
			    bool is_uktls,
			    struct ukarch_ectx *ectx,
			    const char *name,
			    void *priv,
			    uk_thread_dtor_t dtor);

/**
 * Initializes a `uk_thread` structure and allocates stack and optionally TLS.
 * Such a thread can then be assigned to a scheduler. When the thread starts,
 * the general-purpose registers are reset.
 *
 * @param t
 *   Reference to uk_thread structure to initialize
 * @param fn
 *   Thread entry function (required)
 * @param a_stack
 *   Reference to an allocator for allocating a stack (required)
 * @param stack_len
 *   Size of the thread stack. If set to 0, a default stack size is used
 *   for the allocation.
 * @param a_auxstack
 *   Reference to an allocator for allocating an auxiliary stack
 * @param auxstack_len
 *   Size of the thread auxiliary stack. If set to 0, a default stack size is
 *   used for the allocation.
 * @param a_uktls
 *   Reference to an allocator for allocating (Unikraft) thread local storage.
 *   In case `custom_ectx` is not set, space for extended CPU context state
 *   is allocated together with the TLS. If `NULL` is passed, a thread without
 *   TLS is allocated (and without ectx if `custom_ectx` is not set
 *   (see arguments `custom_ectx`, `ectx`).
 * @param custom_ectx
 *   Do not allocate ectx together with TLS. Use next parameter as reference
 *   to load and store ectx.
 * @param ectx
 *   This parameter is only used if `custom_ectx` is set. If set to `NULL`
 *   no memory is available for saving/restoring extended CPU context state
 *   (e.g., floating point, vector registers). In such a case, no ectx can
 *   be saved nor restored on thread switches.
 *   Executed functions must be ISR-safe and UK_THREADF_ECTX is not set.
 * @param name
 *   Optional name for the thread, can be `NULL`
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (0):  Successfully initialized
 *   - (<0): Negative value with error code
 */
int uk_thread_init_fn0(struct uk_thread *t,
		       uk_thread_fn0_t fn,
		       struct uk_alloc *a_stack,
		       size_t stack_len,
		       struct uk_alloc *a_auxstack,
		       size_t auxstack_len,
		       struct uk_alloc *a_uktls,
		       bool custom_ectx,
		       struct ukarch_ectx *ectx,
		       const char *name,
		       void *priv,
		       uk_thread_dtor_t dtor);

/**
 * Similar to `uk_thread_init_fn0()` but with a thread function accepting
 * one argument
 */
int uk_thread_init_fn1(struct uk_thread *t,
		       uk_thread_fn1_t fn,
		       void *argp,
		       struct uk_alloc *a_stack,
		       size_t stack_len,
		       struct uk_alloc *a_auxstack,
		       size_t auxstack_len,
		       struct uk_alloc *a_uktls,
		       bool custom_ectx,
		       struct ukarch_ectx *ectx,
		       const char *name,
		       void *priv,
		       uk_thread_dtor_t dtor);

/**
 * Similar to `uk_thread_init_fn0()` but with a thread function accepting
 * two arguments
 */
int uk_thread_init_fn2(struct uk_thread *t,
		       uk_thread_fn2_t fn,
		       void *argp0, void *argp1,
		       struct uk_alloc *a_stack,
		       size_t stack_len,
		       struct uk_alloc *a_auxstack,
		       size_t auxstack_len,
		       struct uk_alloc *a_uktls,
		       bool custom_ectx,
		       struct ukarch_ectx *ectx,
		       const char *name,
		       void *priv,
		       uk_thread_dtor_t dtor);

/**
 * Allocates a bare uk_thread structure. Such a thread can then be assigned
 * to a scheduler. No register initialization (reset) is done.
 *
 * @param a
 *   Reference to an allocator
 * @param ip
 *   Instruction pointer. When `ip != NULL`, the flag
 *   `UK_THREADF_RUNNABLE` is set.
 * @param sp
 *   Stack pointer
 * @param auxsp
 *   Auxiliary stack pointer
 * @param tlsp
 *   Architecture pointer to TLS. If set to NULL, the thread cannot
 *   access thread-local variables
 * @param is_uktls
 *   Indicates that the given TLS pointer (`tlsp` must be != 0x0)
 *   points to a TLS derived from the Unikraft system TLS template.
 * @param no_ectx
 *   If set, no memory is allocated for saving/restoring extended CPU
 *   context state (e.g., floating point, vector registers). In such a
 *   case, no extended context is saved nor restored on thread switches.
 *   Executed functions must be ISR-safe.
 * @param name
 *   Optional name for the thread
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (NULL): Allocation failed
 *   - Reference to initialized uk_thread
 */
struct uk_thread *uk_thread_create_bare(struct uk_alloc *a,
					uintptr_t ip,
					uintptr_t sp,
					uintptr_t auxsp,
					uintptr_t tlsp,
					bool is_uktls,
					bool no_ectx,
					const char *name,
					void *priv,
					uk_thread_dtor_t dtor);

/**
 * Allocates a uk_thread container with stack and TLS. The entry point is not
 * set and the thread is not marked as runnable. Such a thread should only be
 * assigned to a scheduler after an entry point is configured (`thread->ctx`)
 * and the thread was marked as runnable.
 *
 * @param a
 *   Reference to an allocator (required)
 * @param a_stack
 *   Reference to an allocator for allocating a stack
 *   Set to `NULL` to continue without a stack.
 * @param stack_len
 *   Size of the thread stack. If set to 0, a default stack size is used
 *   for the stack allocation.
 * @param a_auxstack
 *   Reference to an allocator for allocating an auxiliary stack
 * @param auxstack_len
 *   Size of the thread auxiliary stack. If set to 0, a default stack size is
 *   used for the allocation.
 * @param a_uktls
 *   Reference to an allocator for allocating (Unikraft) thread local storage.
 *   If `NULL` is passed, a thread without TLS is allocated.
 * @param no_ectx
 *   If set, no memory is allocated for saving/restoring extended CPU
 *   context state (e.g., floating point, vector registers). In such a
 *   case, no extended context is saved nor restored on thread switches.
 *   Executed functions must be ISR-safe.
 * @param name
 *   Optional name for the thread
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (NULL): Allocation failed
 *   - Reference to initialized uk_thread
 */
struct uk_thread *uk_thread_create_container(struct uk_alloc *a,
					     struct uk_alloc *a_stack,
					     size_t stack_len,
					     struct uk_alloc *a_auxstack,
					     size_t auxstack_len,
					     struct uk_alloc *a_uktls,
					     bool no_ectx,
					     const char *name,
					     void *priv,
					     uk_thread_dtor_t dtor);

/**
 * Allocates a uk_thread container. Stack and TLS are given by the caller.
 * The entry point is not set and the thread is not marked as runnable.
 * Such a thread should only be assigned to a scheduler after stack and
 * entry point is configured (`thread->ctx`) and the thread was marked as
 * runnable.
 *
 * @param a
 *   Reference t o an allocator (required)
 * @param sp
 *   Stack pointer
 * @param auxsp
 *   Auxiliary stack pointer
 * @param tlsp
 *   Architecture pointer to TLS. If set to NULL, the thread cannot
 *   access thread-local variables
 * @param is_uktls
 *   Indicates that the given TLS pointer (`tlsp` must be != 0x0)
 *   points to a TLS derived from the Unikraft system TLS template.
 * @param no_ectx
 *   If set, no memory is allocated for saving/restoring extended CPU
 *   context state (e.g., floating point, vector registers). In such a
 *   case, no extended context is saved nor restored on thread switches.
 *   Executed functions must be ISR-safe.
 * @param name
 *   Optional name for the thread
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (NULL): Allocation failed
 *   - Reference to initialized uk_thread
 */
struct uk_thread *uk_thread_create_container2(struct uk_alloc *a,
					      uintptr_t sp,
					      uintptr_t auxsp,
					      uintptr_t tlsp,
					      bool is_uktls,
					      bool no_ectx,
					      const char *name,
					      void *priv,
					      uk_thread_dtor_t dtor);

/*
 * Helper functions for initializing a thread container and setting it
 * as runnable
 */
void uk_thread_container_init_bare(struct uk_thread *t, uintptr_t ip);
void uk_thread_container_init_fn0(struct uk_thread *t, uk_thread_fn0_t fn);
void uk_thread_container_init_fn1(struct uk_thread *t, uk_thread_fn1_t fn,
						       void *argp);
void uk_thread_container_init_fn2(struct uk_thread *t, uk_thread_fn2_t fn,
						       void *argp0,
						       void *argp1);

/**
 * Allocates a raw uk_thread structure. Such a thread can then be assigned
 * to a scheduler. When the thread starts, the general-purpose registers are
 * reset.
 *
 * @param a
 *   Reference to an allocator (required)
 * @param fn
 *   Thread entry function (required)
 * @param a_stack
 *   Reference to an allocator for allocating a stack (required)
 * @param stack_len
 *   Size of the thread stack. If set to 0, a default stack size is used
 *   for the stack allocation.
 * @param a_auxstack
 *   Reference to an allocator for allocating an auxiliary stack
 * @param auxstack_len
 *   Size of the thread auxiliary stack. If set to 0, a default stack size is
 *   used for the allocation.
 * @param a_uktls
 *   Reference to an allocator for allocating (Unikraft) thread local storage.
 *   If `NULL` is passed, a thread without TLS is allocated.
 * @param no_ectx
 *   If set, no memory is allocated for saving/restoring extended CPU
 *   context state (e.g., floating point, vector registers). In such a
 *   case, no extended context is saved nor restored on thread switches.
 *   Executed functions must be ISR-safe.
 * @param name
 *   Optional name for the thread
 * @param priv
 *   Reference to external data that corresponds to this thread
 * @param dtor
 *   Destructor that is called when this thread is released
 * @return
 *   - (NULL): Allocation failed
 *   - Reference to initialized uk_thread
 */
struct uk_thread *uk_thread_create_fn0(struct uk_alloc *a,
				       uk_thread_fn0_t fn,
				       struct uk_alloc *a_stack,
				       size_t stack_len,
				       struct uk_alloc *a_auxstack,
				       size_t auxstack_len,
				       struct uk_alloc *a_uktls,
				       bool no_ectx,
				       const char *name,
				       void *priv,
				       uk_thread_dtor_t dtor);

/**
 * Similar to `uk_thread_create_fn0()` but with a thread function accepting
 * one argument
 */
struct uk_thread *uk_thread_create_fn1(struct uk_alloc *a,
				       uk_thread_fn1_t fn, void *argp,
				       struct uk_alloc *a_stack,
				       size_t stack_len,
				       struct uk_alloc *a_auxstack,
				       size_t auxstack_len,
				       struct uk_alloc *a_uktls,
				       bool no_ectx,
				       const char *name,
				       void *priv,
				       uk_thread_dtor_t dtor);

/**
 * Similar to `uk_thread_create_fn0()` but with a thread function accepting
 * two arguments
 */
struct uk_thread *uk_thread_create_fn2(struct uk_alloc *a,
				       uk_thread_fn2_t fn,
				       void *argp0, void *argp1,
				       struct uk_alloc *a_stack,
				       size_t stack_len,
				       struct uk_alloc *a_auxstack,
				       size_t auxstack_len,
				       struct uk_alloc *a_uktls,
				       bool no_ectx,
				       const char *name,
				       void *priv,
				       uk_thread_dtor_t dtor);

void uk_thread_release(struct uk_thread *t);
void uk_thread_block_until(struct uk_thread *thread, __snsec until);
void uk_thread_block_timeout(struct uk_thread *thread, __nsec nsec);
void uk_thread_block(struct uk_thread *thread);
void uk_thread_wake(struct uk_thread *thread);

/**
 * Macro to access a Unikraft thread-local (UKTLS) variable of a
 * (foreign) thread.
 * This macro computes the offset of the TLS variable address to the
 * architecture TLS pointer. Since each UKTLS has the same structure,
 * the offset from the tlsp to the variable should be the same
 * regardless of which TLS we are looking at.
 * Because we get the absolute address of a variable with the `&`
 * operand, we need to subtract the current TLS pointer to retrieve
 * the variable offset.
 */
#define uk_thread_uktls_var(thread, variable)				\
	(*({								\
		__uptr _curr_tlsp = ukplat_tlsp_get();			\
		__sptr _offset;						\
		typeof(variable) *_ref;					\
									\
		UK_ASSERT((thread)->flags & UK_THREADF_UKTLS);		\
									\
		_offset = _curr_tlsp - (__uptr) &(variable);		\
		_ref = (typeof(_ref)) ((thread)->uktlsp - _offset);	\
		_ref;							\
	}))

/**
 * Thread initialization callback
 * A thread initialization callback is called during thread creation
 * from the parent context. Libraries can register callbacks with
 * the `UK_THREAD_INIT*()` macros.
 *
 * @param parent
 *  Parent thread of created one. Please note that the parent can be NULL.
 *  This happens when scheduling is currently initializing and the main
 *  threads are set up (e.g., "main", "idle").
 * @param child
 *  The child thread that is created.
 *
 * @return
 *  (>=0): Success
 *  (<0): Failure, thread creation will be aborted
 */
typedef int  (*uk_thread_init_func_t)(struct uk_thread *child,
				      struct uk_thread *parent);

/**
 * Thread termination callback
 * A thread termination callback is called when a threads terminates or gets
 * terminated (transition into EXITED state) and before the thread resources
 * are released.
 * Libraries can register callbacks with the `UK_THREAD_INIT*()` macros.
 *
 * @param child
 *  The thread that is going to be removed from the system.
 */
typedef void (*uk_thread_term_func_t)(struct uk_thread *child);

struct uk_thread_inittab_entry {
	uint32_t flags;
	uk_thread_init_func_t init;
	uk_thread_term_func_t term;
};

#define UK_THREAD_INITF_ECTX  (UK_THREADF_ECTX)
#define UK_THREAD_INITF_AUXSP (UK_THREADF_AUXSP)
#define UK_THREAD_INITF_UKTLS (UK_THREADF_UKTLS)
#define UK_THREAD_INITF_ALL   (UK_THREAD_INITF_ECTX | UK_THREAD_INITF_AUXSP | \
			       UK_THREAD_INITF_UKTLS)

#define __UK_THREAD_INITTAB_ENTRY(init_fn, term_fn, prio, arg_flags)	\
	static const struct uk_thread_inittab_entry			\
	__used __section(".uk_thread_inittab" # prio) __align(8)	\
		__uk_thread_inittab ## prio ## _ ## init_fn ## _ ## term_fn = {\
		.flags = (arg_flags),					\
		.init = (init_fn),					\
		.term = (term_fn)					\
	}

#define _UK_THREAD_INITTAB_ENTRY(init_fn, term_fn, prio, flags)		\
	__UK_THREAD_INITTAB_ENTRY(init_fn, term_fn, prio, flags)

/**
 * Registers a thread initialization function that is
 * called during thread creation
 *
 * @param fn
 *   initialization function to be called (uk_thread_init_func_t)
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest))
 *   Use the UK_PRIO_AFTER() helper macro for computing priority dependencies.
 *   Note: Any other value for level will be ignored
 */
#define UK_THREAD_INIT_PRIO_FLAGS(init_fn, term_fn, prio, flags)	\
	_UK_THREAD_INITTAB_ENTRY(init_fn, term_fn, prio,		\
				 ((flags) & (UK_THREAD_INITF_ALL)))

#define UK_THREAD_INIT_PRIO(init_fn, term_fn, prio)			\
	UK_THREAD_INIT_PRIO_FLAGS(init_fn, term_fn, prio, (UK_THREAD_INITF_ALL))

#define UK_THREAD_INIT(init_fn, term_fn)				\
	UK_THREAD_INIT_PRIO_FLAGS(init_fn, term_fn, UK_PRIO_LATEST,	\
				  (UK_THREAD_INITF_ALL))

#ifdef __cplusplus
}
#endif

#endif /* __UK_THREAD_H__ */

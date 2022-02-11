/* SPDX-License-Identifier: MIT */
/*
 * Authors: Rolf Neugebauer
 *          Grzegorz Milos
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2003-2005, Intel Research Cambridge
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, NEC Laboratories GmbH, NEC Corrporation,
 *                     All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Some thread definitions were derived from Mini-OS
 */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <uk/plat/config.h>
#include <uk/plat/time.h>
#include <uk/plat/tls.h>
#include <uk/thread.h>
#include <uk/sched.h>
#include <uk/wait.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/arch/tls.h>

#ifdef CONFIG_LIBNEWLIBC
static void reent_init(struct _reent *reent)
{
	_REENT_INIT_PTR(reent);
#if 0
	/* TODO initialize basic signal handling */
	_init_signal_r(myreent);
#endif
}

struct _reent *__getreent(void)
{
	struct _reent *_reent;
	struct uk_sched *s = uk_sched_get_default();

	if (!s || !uk_sched_started(s))
		_reent = _impure_ptr;
	else
		_reent = &uk_thread_current()->reent;

	return _reent;
}
#endif /* CONFIG_LIBNEWLIBC */

extern const struct uk_thread_inittab_entry _uk_thread_inittab_start[];
extern const struct uk_thread_inittab_entry _uk_thread_inittab_end;

#define uk_thread_inittab_foreach(itr)					\
	for ((itr) = DECONST(struct uk_thread_inittab_entry*,		\
			     _uk_thread_inittab_start);			\
	     (itr) < &(_uk_thread_inittab_end);				\
	     (itr)++)

#define uk_thread_inittab_foreach_reverse2(itr, start)			\
	for ((itr) = (start);						\
	     (itr) >= _uk_thread_inittab_start;				\
	     (itr)--)

#define uk_thread_inittab_foreach_reverse(itr)				\
	uk_thread_inittab_foreach_reverse2((itr),			\
			(DECONST(struct uk_thread_inittab_entry*,	\
				 (&_uk_thread_inittab_end))) - 1)

struct _inittab_call_init_args {
	uk_thread_init_func_t init;
	struct uk_thread *child;
	struct uk_thread *parent;
};

static int _inittab_call_init(void *argp)
{
	const struct _inittab_call_init_args *args
		= (const struct _inittab_call_init_args *) argp;
	int ret;

	UK_ASSERT(args);
	UK_ASSERT(args->init);

	ret = (args->init)(args->child, args->parent);
	return ret;
}

struct _inittab_call_term_args {
	uk_thread_term_func_t term;
	struct uk_thread *child;
};

static int _inittab_call_term(void *argp)
{
	const struct _inittab_call_term_args *args
		= (const struct _inittab_call_term_args *) argp;

	UK_ASSERT(args);
	UK_ASSERT(args->term);

	(args->term)(args->child);
	return 0;
}

/** Iterates over registered thread initialization functions */
static int _uk_thread_call_inittab(struct uk_thread *child)
{
	struct uk_thread *parent = uk_thread_current();
	struct uk_thread_inittab_entry *itr;
	struct _inittab_call_init_args init_args;
	struct _inittab_call_term_args term_args;
	int ret = 0;

	/* Either we run without scheduling or we need to make sure that we have ectx */
	UK_ASSERT(!parent || parent->flags & UK_THREADF_ECTX);

	/* Go over thread initialization functions that match with
	 * child's ECTX and UKTLS feature requirements
	 */
	init_args.child  = child;
	init_args.parent = parent;
	uk_thread_inittab_foreach(itr) {
		if (unlikely(!itr->init))
			continue;
		if (unlikely((itr->flags & child->flags) != itr->flags)) {
			uk_pr_debug("uk_thread %p (%s) init cb: Skip %p() due to feature mismatch: %c%c required (has %c%c)\n",
				    child, child->name
				     ? child->name : "<unnamed>",
				    *itr->init,
				    (itr->flags & UK_THREAD_INITF_ECTX)
				     ? 'E' : '-',
				    (itr->flags & UK_THREAD_INITF_UKTLS)
				     ? 'T' : '-',
				    (child->flags & UK_THREADF_ECTX)
				     ? 'E' : '-',
				    (child->flags & UK_THREADF_UKTLS)
				     ? 'T' : '-');
			continue;
		}

		/* NOTE: We execute the init function with child's TLS enabled. */
		uk_pr_debug("uk_thread %p (%s) init: Call initialization %p()...\n",
			    child, child->name ? child->name : "<unnamed>",
			    *itr->init);
		init_args.init = itr->init;
		ret = ukplat_tlsp_exec(child->uktlsp, _inittab_call_init, &init_args);
		if (ret < 0)
			goto err;
	}
	goto out;

err:
	/* Run termination functions starting from one level before the failed
	 * one for cleanup
	 */
	term_args.child = child;
	uk_thread_inittab_foreach_reverse2(itr, itr - 2) {
		if (unlikely(!itr->term
			     || (itr->flags & child->flags) != itr->flags))
			continue;
		term_args.term = itr->term;
		ukplat_tlsp_exec(child->uktlsp, _inittab_call_term, &term_args);
	}
out:
	return ret;
}

/** Iterates over registered thread termination functions */
static int _uk_thread_call_termtab(struct uk_thread *child)
{
	struct uk_thread *parent = uk_thread_current();
	struct uk_thread_inittab_entry *itr;
	struct _inittab_call_term_args term_args;
	int ret = 0;

	/* Either we run without scheduling or we need to make sure that we have ectx */
	UK_ASSERT(!parent || parent->flags & UK_THREADF_ECTX);

	/* Go over thread termination functions that match with
	 * child's ECTX and UKTLS feature requirements
	 */
	term_args.child = child;
        uk_thread_inittab_foreach_reverse(itr) {
		if (unlikely(!itr->term))
			continue;
		if (unlikely((itr->flags & child->flags) != itr->flags)) {
			uk_pr_debug("uk_thread %p (%s) term cb: Skip %p() due to feature mismatch: %c%c required (has %c%c)\n",
				    child, child->name
				     ? child->name : "<unnamed>",
				    *itr->term,
				    (itr->flags & UK_THREAD_INITF_ECTX)
				     ? 'E' : '-',
				    (itr->flags & UK_THREAD_INITF_UKTLS)
				     ? 'T' : '-',
				    (child->flags & UK_THREADF_ECTX)
				     ? 'E' : '-',
				    (child->flags & UK_THREADF_UKTLS)
				     ? 'T' : '-');
			continue;
		}

		uk_pr_debug("uk_thread %p (%s) term: Call termination %p()...\n",
			    child, child->name
			    ? child->name : "<unnamed>",
			    *itr->term);
		term_args.term = itr->term;
		ukplat_tlsp_exec(child->uktlsp, _inittab_call_term, &term_args);
	}

	return ret;
}

static void _uk_thread_struct_init(struct uk_thread *t,
				   uintptr_t tlsp,
				   bool is_uktls,
				   struct ukarch_ectx *ectx,
				   const char *name,
				   void *priv,
				   uk_thread_dtor_t dtor)
{
	/* TLS pointer required if is_uktls is set */
	UK_ASSERT(!is_uktls || tlsp);

	memset(t, 0x0, sizeof(*t));

	t->ectx = ectx;
	t->tlsp = tlsp;
	t->name = name;
	t->priv = priv;
	t->dtor = dtor;

	if (tlsp && is_uktls) {
		t->flags |= UK_THREADF_UKTLS;
	}
	if (ectx) {
		ukarch_ectx_init(t->ectx);
		t->flags |= UK_THREADF_ECTX;
	}

	uk_waitq_init(&t->waiting_threads);

#ifdef CONFIG_LIBNEWLIBC
	/* TODO: Move newlibc reent initialization to newlib as
	 *       thread initialization function and use TLS
	 */
	reent_init(&t->reent);
#endif

	uk_pr_debug("uk_thread %p (%s): ctx:%p, ectx:%p, tlsp:%p\n",
		    t, t->name ? t->name : "<unnamed>",
		    &t->ctx, t->ectx, (void *) t->tlsp);
}

int uk_thread_init_bare(struct uk_thread *t,
			uintptr_t ip,
			uintptr_t sp,
			uintptr_t tlsp,
			bool is_uktls,
			struct ukarch_ectx *ectx,
			const char *name,
			void *priv,
			uk_thread_dtor_t dtor)
{
	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());

	_uk_thread_struct_init(t, tlsp, is_uktls, ectx, name, priv, dtor);
	ukarch_ctx_init_bare(&t->ctx, sp, ip);

	if (ip)
		t->flags |= UK_THREADF_RUNNABLE;

	return _uk_thread_call_inittab(t);
}

int uk_thread_init_bare_fn0(struct uk_thread *t,
			    uk_thread_fn0_t fn,
			    uintptr_t sp,
			    uintptr_t tlsp,
			    bool is_uktls,
			    struct ukarch_ectx *ectx,
			    const char *name,
			    void *priv,
			    uk_thread_dtor_t dtor)
{
	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(sp); /* stack pointer is required for ctx_entry */
	UK_ASSERT(fn);

	_uk_thread_struct_init(t, tlsp, is_uktls, ectx, name, priv, dtor);
	ukarch_ctx_init_entry0(&t->ctx, sp, 0,
			       (ukarch_ctx_entry0) fn);
	t->flags |= UK_THREADF_RUNNABLE;

	return _uk_thread_call_inittab(t);
}

int uk_thread_init_bare_fn1(struct uk_thread *t,
			    uk_thread_fn1_t fn,
			    void *argp,
			    uintptr_t sp,
			    uintptr_t tlsp,
			    bool is_uktls,
			    struct ukarch_ectx *ectx,
			    const char *name,
			    void *priv,
			    uk_thread_dtor_t dtor)
{
	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(sp); /* stack pointer is required for ctx_entry */
	UK_ASSERT(fn);

	_uk_thread_struct_init(t, tlsp, is_uktls, ectx, name, priv, dtor);
	ukarch_ctx_init_entry1(&t->ctx, sp, 0,
			       (ukarch_ctx_entry1) fn,
			       (long) argp);
	t->flags |= UK_THREADF_RUNNABLE;

	return _uk_thread_call_inittab(t);
}

int uk_thread_init_bare_fn2(struct uk_thread *t,
			    uk_thread_fn2_t fn,
			    void *argp0, void *argp1,
			    uintptr_t sp,
			    uintptr_t tlsp,
			    bool is_uktls,
			    struct ukarch_ectx *ectx,
			    const char *name,
			    void *priv,
			    uk_thread_dtor_t dtor)
{
	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(sp); /* stack pointer is required for ctx_entry */
	UK_ASSERT(fn);

	_uk_thread_struct_init(t, tlsp, is_uktls, ectx, name, priv, dtor);
	ukarch_ctx_init_entry2(&t->ctx, sp, 0,
			       (ukarch_ctx_entry2) fn,
			       (long) argp0, (long) argp1);
	t->flags |= UK_THREADF_RUNNABLE;

	return _uk_thread_call_inittab(t);
}

/** Initializes uk_thread struct and allocates stack & TLS */
static int _uk_thread_struct_init_alloc(struct uk_thread *t,
					struct uk_alloc *a_stack,
					size_t stack_len,
					struct uk_alloc *a_tls,
					bool custom_ectx,
					struct ukarch_ectx *ectx,
					const char *name,
					void *priv,
					uk_thread_dtor_t dtor)
{
	void *stack = NULL;
	void *tls = NULL;
	uintptr_t tlsp = 0x0;

	if (a_stack && stack_len) {
		stack = uk_malloc(a_stack, stack_len);
		if (!stack)
			goto err_out;
	}

	if (a_tls) {
		if (!custom_ectx) {
			/* Allocate TLS and ectx together */
			tls = uk_memalign(a_tls,
					  ukarch_tls_area_align(),
					  ukarch_tls_area_size()
					  + ukarch_ectx_size()
					  + ukarch_ectx_align());
			if (!tls)
				goto err_free_stack;

			/* When custom_ectx is not set, we ignore user's
			 * ectx argument and overwrite it...
			 */
			ectx = (struct ukarch_ectx *) ALIGN_UP(
				(uintptr_t) tls + ukarch_tls_area_size(),
				ukarch_ectx_align());
		} else {
			tls = uk_memalign(a_tls, ukarch_tls_area_align(),
					  ukarch_tls_area_size());
			if (!tls)
				goto err_free_stack;
		}

		tlsp = ukarch_tls_pointer(tls);
	}

	_uk_thread_struct_init(t, tlsp, !(!tlsp), ectx, name, priv, dtor);

	/* Set uk_thread fields related to stack and TLS */
	if (stack) {
		t->_mem.stack = stack;
		t->_mem.stack_a = a_stack;
	}

	if (tls) {
		ukarch_tls_area_copy(tls);

		t->_mem.tls = tls;
		t->_mem.tls_a = a_tls;
		t->flags |= UK_THREADF_UKTLS;
	} else {
		tlsp = 0x0;
	}

	return 0;

err_free_stack:
	if (stack)
		uk_free(a_stack, stack);
err_out:
	return -ENOMEM;
}

/** Reverts `_uk_thread_struct_init_alloc()` */
void _uk_thread_struct_free_alloc(struct uk_thread *t)
{

	UK_ASSERT(t);

	/* Free memory that was allocated by us */
	if (t->_mem.tls_a   && t->_mem.tls) {
		uk_free(t->_mem.tls_a,   t->_mem.tls);
		t->_mem.tls_a   = NULL;
		t->_mem.tls     = NULL;
	}
	if (t->_mem.stack_a && t->_mem.stack) {
		uk_free(t->_mem.stack_a, t->_mem.stack);
		t->_mem.stack_a = NULL;
		t->_mem.stack   = NULL;
	}
}

int uk_thread_init_fn0(struct uk_thread *t,
		       uk_thread_fn0_t fn,
		       struct uk_alloc *a_stack,
		       size_t stack_len,
		       struct uk_alloc *a_tls,
		       bool custom_ectx,
		       struct ukarch_ectx *ectx,
		       const char *name,
		       void *priv,
		       uk_thread_dtor_t dtor)
{
	int ret;

	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(fn);

	ret = _uk_thread_struct_init_alloc(t, a_stack, stack_len,
					   a_tls, custom_ectx, ectx, name,
					   priv, dtor);
	if (ret < 0)
		goto err_out;

	ukarch_ctx_init_entry0(&t->ctx,
			       ukarch_gen_sp(t->_mem.stack, stack_len),
			       0, (ukarch_ctx_entry0) fn);
	t->flags |= UK_THREADF_RUNNABLE;

	ret = _uk_thread_call_inittab(t);
	if (ret < 0)
		goto err_free_alloc;
	return 0;

err_free_alloc:
	_uk_thread_struct_free_alloc(t);
err_out:
	return ret;
}

int uk_thread_init_fn1(struct uk_thread *t,
		       uk_thread_fn1_t fn,
		       void *argp,
		       struct uk_alloc *a_stack,
		       size_t stack_len,
		       struct uk_alloc *a_tls,
		       bool custom_ectx,
		       struct ukarch_ectx *ectx,
		       const char *name,
		       void *priv,
		       uk_thread_dtor_t dtor)
{
	int ret;

	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(fn);

	ret = _uk_thread_struct_init_alloc(t, a_stack, stack_len,
					   a_tls, custom_ectx, ectx, name,
					   priv, dtor);
	if (ret < 0)
		goto err_out;

	ukarch_ctx_init_entry1(&t->ctx,
			       ukarch_gen_sp(t->_mem.stack, stack_len),
			       0, (ukarch_ctx_entry1) fn, (long) argp);
	t->flags |= UK_THREADF_RUNNABLE;

	ret = _uk_thread_call_inittab(t);
	if (ret < 0)
		goto err_free_alloc;
	return 0;

err_free_alloc:
	_uk_thread_struct_free_alloc(t);
err_out:
	return ret;
}

int uk_thread_init_fn2(struct uk_thread *t,
		       uk_thread_fn2_t fn,
		       void *argp0, void *argp1,
		       struct uk_alloc *a_stack,
		       size_t stack_len,
		       struct uk_alloc *a_tls,
		       bool custom_ectx,
		       struct ukarch_ectx *ectx,
		       const char *name,
		       void *priv,
		       uk_thread_dtor_t dtor)
{
	int ret;

	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(fn);

	ret = _uk_thread_struct_init_alloc(t, a_stack, stack_len,
					   a_tls, custom_ectx, ectx, name,
					   priv, dtor);
	if (ret < 0)
		goto err_out;

	ukarch_ctx_init_entry2(&t->ctx,
			       ukarch_gen_sp(t->_mem.stack, stack_len),
			       0, (ukarch_ctx_entry2) fn,
			       (long) argp0, (long) argp1);
	t->flags |= UK_THREADF_RUNNABLE;

	ret = _uk_thread_call_inittab(t);
	if (ret < 0)
		goto err_free_alloc;
	return 0;

err_free_alloc:
	_uk_thread_struct_free_alloc(t);
err_out:
	return ret;
}

struct uk_thread *uk_thread_create_bare(struct uk_alloc *a,
					uintptr_t ip,
					uintptr_t sp,
					uintptr_t tlsp,
					bool is_uktls,
					bool no_ectx,
					const char *name,
					void *priv,
					uk_thread_dtor_t dtor)
{
	struct uk_thread *t;
	size_t alloc_size = sizeof(*t);

	/* NOTE: We place space for extended context after
	 *       struct uk_thread within the same allocation
	 */
	if (!no_ectx)
		alloc_size += ukarch_ectx_size() + ukarch_ectx_align();

	t = uk_malloc(a, alloc_size);
	if (!t)
		return NULL;

	uk_thread_init_bare(t, ip, sp, tlsp, is_uktls,
			    (struct ukarch_ectx *) ALIGN_UP((uintptr_t) t
							    + sizeof(*t),
							   ukarch_ectx_align()),
			    name, priv, dtor);
	t->_mem.t_a = a; /* Save allocator reference for releasing */

	return t;
}

/** Allocates `struct uk_thread` along with stack and TLS */
static struct uk_thread *_uk_thread_alloc(struct uk_alloc *a,
					  struct uk_alloc *a_stack,
					  size_t stack_len,
					  struct uk_alloc *a_tls,
					  bool no_ectx,
					  const char *name,
					  void *priv,
					  uk_thread_dtor_t dtor)
{
	struct uk_thread *t;
	size_t t_size;
	struct ukarch_ectx *ectx = NULL;

	/* NOTE: We place space for extended context after
	 *       struct uk_thread within the same allocation
	 *       when no TLS was requested but ectx support
	 */
	t_size = sizeof(*t);
	if (!no_ectx && !a_tls)
		t_size += ukarch_ectx_size() + ukarch_ectx_align();

	t = uk_malloc(a, t_size);
	if (!t)
		goto err_out;

	if (!no_ectx && !a_tls)
		ectx = (struct ukarch_ectx *) ALIGN_UP((uintptr_t) t
						       + sizeof(*t),
						       ukarch_ectx_align());

	if (_uk_thread_struct_init_alloc(t,
					 a_stack, stack_len,
					 a_tls,
					 !(!ectx),
					 ectx,
					 name, priv, dtor) < 0)
		goto err_free_thread;
	t->_mem.t_a = a;
	return t;

err_free_thread:
	uk_free(a, t);
err_out:
	return NULL;
}

/** Reverts `_uk_thread_alloc()` */
static void _uk_thread_free(struct uk_thread *t)
{
	UK_ASSERT(t);

	_uk_thread_struct_free_alloc(t);
	uk_free(t->_mem.t_a, t);
}

struct uk_thread *uk_thread_create_fn0(struct uk_alloc *a,
				       uk_thread_fn0_t fn,
				       struct uk_alloc *a_stack,
				       size_t stack_len,
				       struct uk_alloc *a_tls,
				       bool no_ectx,
				       const char *name,
				       void *priv,
				       uk_thread_dtor_t dtor)
{
	struct uk_thread *t;
	int ret;

	UK_ASSERT(fn);
	UK_ASSERT(a_stack); /* A stack is required for ctx initialization */

	stack_len = (!!stack_len) ? stack_len : STACK_SIZE;
	t = _uk_thread_alloc(a,
			     a_stack, stack_len,
			     a_tls,
			     no_ectx, name, priv, dtor);
	if (!t)
		goto err_out;

	ukarch_ctx_init_entry0(&t->ctx,
			       ukarch_gen_sp(t->_mem.stack, stack_len),
			       0,
			       fn);
	t->flags |= UK_THREADF_RUNNABLE;

	ret = _uk_thread_call_inittab(t);
	if (ret < 0)
		goto err_free_alloc;
	return t;

err_free_alloc:
	_uk_thread_free(t);
err_out:
	return NULL;
}

struct uk_thread *uk_thread_create_fn1(struct uk_alloc *a,
				       uk_thread_fn1_t fn,
				       void *argp,
				       struct uk_alloc *a_stack,
				       size_t stack_len,
				       struct uk_alloc *a_tls,
				       bool no_ectx,
				       const char *name,
				       void *priv,
				       uk_thread_dtor_t dtor)
{
	struct uk_thread *t;
	int ret;

	UK_ASSERT(fn);
	UK_ASSERT(a_stack); /* A stack is required for ctx initialization */

	stack_len = (!!stack_len) ? stack_len : STACK_SIZE;
	t = _uk_thread_alloc(a,
			     a_stack, stack_len,
			     a_tls,
			     no_ectx, name, priv, dtor);
	if (!t)
		goto err_out;

	ukarch_ctx_init_entry1(&t->ctx,
			       ukarch_gen_sp(t->_mem.stack, stack_len),
			       0,
			       (ukarch_ctx_entry1) fn, (long) argp);
	t->flags |= UK_THREADF_RUNNABLE;

	ret = _uk_thread_call_inittab(t);
	if (ret < 0)
		goto err_free_alloc;
	return t;

err_free_alloc:
	_uk_thread_free(t);
err_out:
	return NULL;
}

struct uk_thread *uk_thread_create_fn2(struct uk_alloc *a,
				       uk_thread_fn2_t fn,
				       void *argp0, void *argp1,
				       struct uk_alloc *a_stack,
				       size_t stack_len,
				       struct uk_alloc *a_tls,
				       bool no_ectx,
				       const char *name,
				       void *priv,
				       uk_thread_dtor_t dtor)
{
	struct uk_thread *t;
	int ret;

	UK_ASSERT(fn);
	UK_ASSERT(a_stack); /* A stack is required for ctx initialization */

	stack_len = (!!stack_len) ? stack_len : STACK_SIZE;
	t = _uk_thread_alloc(a,
			     a_stack, stack_len,
			     a_tls,
			     no_ectx, name, priv, dtor);
	if (!t)
		goto err_out;

	ukarch_ctx_init_entry2(&t->ctx,
			       ukarch_gen_sp(t->_mem.stack, stack_len),
			       0,
			       (ukarch_ctx_entry2) fn,
			       (long) argp0, (long) argp1);
	t->flags |= UK_THREADF_RUNNABLE;

	ret = _uk_thread_call_inittab(t);
	if (ret < 0)
		goto err_free_alloc;
	return t;

err_free_alloc:
	_uk_thread_free(t);
err_out:
	return NULL;
}

void uk_thread_release(struct uk_thread *t)
{
	struct uk_alloc *a;
	struct uk_alloc *stack_a;
	struct uk_alloc *tls_a;
	void *stack;
	void *tls;

	UK_ASSERT(t);
	UK_ASSERT(t != uk_thread_current());
	UK_ASSERT(!t->sched); /* Thread must be disconnected from scheduler */

	_uk_thread_call_termtab(t);

	/* Take copies of associated allocation information.
	 * The destructor provided might free the struct before
	 * we take action.
	 */
	a = t->_mem.t_a;
	stack_a = t->_mem.stack_a;
	stack   = t->_mem.stack;
	tls_a   = t->_mem.tls_a;
	tls     = t->_mem.tls;

	if (t->dtor)
		t->dtor(t);

	/* Free memory that was allocated by us */
	if (tls_a   && tls)
		uk_free(tls_a,   tls);
	if (stack_a && stack)
		uk_free(stack_a, stack);
	if (a)
		uk_free(a, t);
}

static void uk_thread_block_until(struct uk_thread *thread, __snsec until)
{
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();
	thread->wakeup_time = until;
	clear_runnable(thread);
	uk_sched_thread_blocked(thread->sched, thread);
	ukplat_lcpu_restore_irqf(flags);
}

void uk_thread_block_timeout(struct uk_thread *thread, __nsec nsec)
{
	__snsec until = (__snsec) ukplat_monotonic_clock() + nsec;

	uk_thread_block_until(thread, until);
}

void uk_thread_block(struct uk_thread *thread)
{
	uk_thread_block_until(thread, 0LL);
}

void uk_thread_wake(struct uk_thread *thread)
{
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();
	if (!is_runnable(thread)) {
		uk_sched_thread_woken(thread->sched, thread);
		thread->wakeup_time = 0LL;
		set_runnable(thread);
	}
	ukplat_lcpu_restore_irqf(flags);
}

void uk_thread_exit(struct uk_thread *thread)
{
	UK_ASSERT(thread);

	set_exited(thread);

	if (!thread->detached)
		uk_waitq_wake_up(&thread->waiting_threads);

	uk_pr_debug("Thread \"%s\" exited.\n", thread->name);
}

int uk_thread_wait(struct uk_thread *thread)
{
	UK_ASSERT(thread);

	/* TODO critical region */

	if (thread->detached)
		return -EINVAL;

	uk_waitq_wait_event(&thread->waiting_threads, is_exited(thread));

	thread->detached = true;

	uk_sched_thread_destroy(thread->sched, thread);

	return 0;
}

int uk_thread_detach(struct uk_thread *thread)
{
	UK_ASSERT(thread);

	thread->detached = true;

	return 0;
}

int uk_thread_set_prio(struct uk_thread *thread, prio_t prio)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_set_prio(thread->sched, thread, prio);
}

int uk_thread_get_prio(const struct uk_thread *thread, prio_t *prio)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_get_prio(thread->sched, thread, prio);
}

int uk_thread_set_timeslice(struct uk_thread *thread, int timeslice)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_set_timeslice(thread->sched, thread, timeslice);
}

int uk_thread_get_timeslice(const struct uk_thread *thread, int *timeslice)
{
	if (!thread)
		return -EINVAL;

	return uk_sched_thread_get_timeslice(thread->sched, thread, timeslice);
}

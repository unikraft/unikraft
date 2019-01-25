/* SPDX-License-Identifier: BSD-3-Clause */
/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)queue.h	8.5 (Berkeley) 8/20/94
 * $FreeBSD$
 */
/*
 * Generated automatically by bsd-sys-queue-h-seddery to
 *  - introduce UK_ and UK_ namespace prefixes
 *  - turn "struct type" into "type" so that type arguments
 *     to the macros are type names not struct tags
 *  - remove the reference to sys/cdefs.h, which is not needed
 *
 * The purpose of this seddery is to allow the resulting file to be
 * freely included by software which might also want to include other
 * list macros; to make it usable when struct tags are not being used
 * or not known; to make it more portable.
 */

#ifndef UK__SYS_QUEUE_H_
#define	UK__SYS_QUEUE_H_

/* #include <sys/cdefs.h> */

/*
 * This file defines four types of data structures: singly-linked lists,
 * singly-linked tail queues, lists and tail queues.
 *
 * A singly-linked list is headed by a single forward pointer. The elements
 * are singly linked for minimum space and pointer manipulation overhead at
 * the expense of O(n) removal for arbitrary elements. New elements can be
 * added to the list after an existing element or at the head of the list.
 * Elements being removed from the head of the list should use the explicit
 * macro for this purpose for optimum efficiency. A singly-linked list may
 * only be traversed in the forward direction.  Singly-linked lists are ideal
 * for applications with large datasets and few or no removals or for
 * implementing a LIFO queue.
 *
 * A singly-linked tail queue is headed by a pair of pointers, one to the
 * head of the list and the other to the tail of the list. The elements are
 * singly linked for minimum space and pointer manipulation overhead at the
 * expense of O(n) removal for arbitrary elements. New elements can be added
 * to the list after an existing element, at the head of the list, or at the
 * end of the list. Elements being removed from the head of the tail queue
 * should use the explicit macro for this purpose for optimum efficiency.
 * A singly-linked tail queue may only be traversed in the forward direction.
 * Singly-linked tail queues are ideal for applications with large datasets
 * and few or no removals or for implementing a FIFO queue.
 *
 * A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked
 * so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before
 * or after an existing element or at the head of the list. A list
 * may be traversed in either direction.
 *
 * A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may be traversed in either direction.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 *
 * Below is a summary of implemented functions where:
 *  +  means the macro is available
 *  -  means the macro is not available
 *  s  means the macro is available but is slow (runs in O(n) time)
 *
 *				UK_SLIST	UK_LIST	UK_STAILQ	UK_TAILQ
 * _HEAD			+	+	+	+
 * _CLASS_HEAD			+	+	+	+
 * _HEAD_INITIALIZER		+	+	+	+
 * _ENTRY			+	+	+	+
 * _CLASS_ENTRY			+	+	+	+
 * _INIT			+	+	+	+
 * _EMPTY			+	+	+	+
 * _FIRST			+	+	+	+
 * _NEXT			+	+	+	+
 * _PREV			-	+	-	+
 * _LAST			-	-	+	+
 * _FOREACH			+	+	+	+
 * _FOREACH_FROM		+	+	+	+
 * _FOREACH_SAFE		+	+	+	+
 * _FOREACH_FROM_SAFE		+	+	+	+
 * _FOREACH_REVERSE		-	-	-	+
 * _FOREACH_REVERSE_FROM	-	-	-	+
 * _FOREACH_REVERSE_SAFE	-	-	-	+
 * _FOREACH_REVERSE_FROM_SAFE	-	-	-	+
 * _INSERT_HEAD			+	+	+	+
 * _INSERT_BEFORE		-	+	-	+
 * _INSERT_AFTER		+	+	+	+
 * _INSERT_TAIL			-	-	+	+
 * _CONCAT			s	s	+	+
 * _REMOVE_AFTER		+	-	+	-
 * _REMOVE_HEAD			+	-	+	-
 * _REMOVE			s	+	s	+
 * _SWAP			+	+	+	+
 *
 */
#if (defined(_KERNEL) && defined(INVARIANTS))
    #include <uk/assert.h>
#endif
#ifdef UK_QUEUE_MACRO_DEBUG
#warn Use UK_QUEUE_MACRO_DEBUG_TRACE and/or UK_QUEUE_MACRO_DEBUG_TRASH
#define	UK_QUEUE_MACRO_DEBUG_TRACE
#define	UK_QUEUE_MACRO_DEBUG_TRASH
#endif

#ifdef UK_QUEUE_MACRO_DEBUG_TRACE
/* Store the last 2 places the queue element or head was altered */
struct UK__qm_trace {
	unsigned long	 lastline;
	unsigned long	 prevline;
	const char	*lastfile;
	const char	*prevfile;
};

#define	UK__TRACEBUF	struct UK__qm_trace trace;
#define	UK__TRACEBUF_INITIALIZER	{ __LINE__, 0, __FILE__, 0 } ,

#define	UK__QMD_TRACE_HEAD(head) do {					\
	(head)->trace.prevline = (head)->trace.lastline;		\
	(head)->trace.prevfile = (head)->trace.lastfile;		\
	(head)->trace.lastline = __LINE__;				\
	(head)->trace.lastfile = __FILE__;				\
} while (0)

#define	UK__QMD_TRACE_ELEM(elem) do {					\
	(elem)->trace.prevline = (elem)->trace.lastline;		\
	(elem)->trace.prevfile = (elem)->trace.lastfile;		\
	(elem)->trace.lastline = __LINE__;				\
	(elem)->trace.lastfile = __FILE__;				\
} while (0)

#else	/* !UK_QUEUE_MACRO_DEBUG_TRACE */
#define	UK__QMD_TRACE_ELEM(elem)
#define	UK__QMD_TRACE_HEAD(head)
#define	UK__TRACEBUF
#define	UK__TRACEBUF_INITIALIZER
#endif	/* UK_QUEUE_MACRO_DEBUG_TRACE */

#ifdef UK_QUEUE_MACRO_DEBUG_TRASH
#define	UK__TRASHIT(x)		do {(x) = (void *)-1;} while (0)
#define	UK__QMD_IS_TRASHED(x)	((x) == (void *)(intptr_t)-1)
#else	/* !UK_QUEUE_MACRO_DEBUG_TRASH */
#define	UK__TRASHIT(x)
#define	UK__QMD_IS_TRASHED(x)	0
#endif	/* UK_QUEUE_MACRO_DEBUG_TRASH */

#if defined(UK_QUEUE_MACRO_DEBUG_TRACE) || defined(UK_QUEUE_MACRO_DEBUG_TRASH)
#define	UK__QMD_SAVELINK(name, link)	void **name = (void *)&(link)
#else	/* !UK_QUEUE_MACRO_DEBUG_TRACE && !UK_QUEUE_MACRO_DEBUG_TRASH */
#define	UK__QMD_SAVELINK(name, link)
#endif	/* UK_QUEUE_MACRO_DEBUG_TRACE || UK_QUEUE_MACRO_DEBUG_TRASH */

#ifdef __cplusplus
/*
 * In C++ there can be structure lists and class lists:
 */
#define	UK_QUEUE_TYPEOF(type) type
#else
#define	UK_QUEUE_TYPEOF(type) type
#endif

/*
 * Singly-linked List declarations.
 */
#define	UK_SLIST_HEAD(name, type)						\
struct name {								\
	type *slh_first;	/* first element */			\
}

#define	UK_SLIST_CLASS_HEAD(name, type)					\
struct name {								\
	class type *slh_first;	/* first element */			\
}

#define	UK_SLIST_HEAD_INITIALIZER(head)					\
	{ 0 }

#define	UK_SLIST_ENTRY(type)						\
struct {								\
	type *sle_next;	/* next element */			\
}

#define	UK_SLIST_CLASS_ENTRY(type)						\
struct {								\
	class type *sle_next;		/* next element */		\
}

/*
 * Singly-linked List functions.
 */
#if (defined(_KERNEL) && defined(INVARIANTS))
#define	UK__QMD_SLIST_CHECK_PREVPTR(prevp, elm) do {			\
	if (*(prevp) != (elm))						\
		UK_CRASH("Bad prevptr *(%p) == %p != %p",			\
		    (prevp), *(prevp), (elm));				\
} while (0)
#else
#define	UK__QMD_SLIST_CHECK_PREVPTR(prevp, elm)
#endif

#define UK_SLIST_CONCAT(head1, head2, type, field) do {			\
	UK_QUEUE_TYPEOF(type) *curelm = UK_SLIST_FIRST(head1);		\
	if (curelm == 0) {						\
		if ((UK_SLIST_FIRST(head1) = UK_SLIST_FIRST(head2)) != 0)	\
			UK_SLIST_INIT(head2);				\
	} else if (UK_SLIST_FIRST(head2) != 0) {			\
		while (UK_SLIST_NEXT(curelm, field) != 0)		\
			curelm = UK_SLIST_NEXT(curelm, field);		\
		UK_SLIST_NEXT(curelm, field) = UK_SLIST_FIRST(head2);		\
		UK_SLIST_INIT(head2);					\
	}								\
} while (0)

#define	UK_SLIST_EMPTY(head)	((head)->slh_first == 0)

#define	UK_SLIST_FIRST(head)	((head)->slh_first)

#define	UK_SLIST_FOREACH(var, head, field)					\
	for ((var) = UK_SLIST_FIRST((head));				\
	    (var);							\
	    (var) = UK_SLIST_NEXT((var), field))

#define	UK_SLIST_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : UK_SLIST_FIRST((head)));		\
	    (var);							\
	    (var) = UK_SLIST_NEXT((var), field))

#define	UK_SLIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = UK_SLIST_FIRST((head));				\
	    (var) && ((tvar) = UK_SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_SLIST_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : UK_SLIST_FIRST((head)));		\
	    (var) && ((tvar) = UK_SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_SLIST_FOREACH_PREVPTR(var, varp, head, field)			\
	for ((varp) = &UK_SLIST_FIRST((head));				\
	    ((var) = *(varp)) != 0;					\
	    (varp) = &UK_SLIST_NEXT((var), field))

#define	UK_SLIST_INIT(head) do {						\
	UK_SLIST_FIRST((head)) = 0;					\
} while (0)

#define	UK_SLIST_INSERT_AFTER(slistelm, elm, field) do {			\
	UK_SLIST_NEXT((elm), field) = UK_SLIST_NEXT((slistelm), field);	\
	UK_SLIST_NEXT((slistelm), field) = (elm);				\
} while (0)

#define	UK_SLIST_INSERT_HEAD(head, elm, field) do {			\
	UK_SLIST_NEXT((elm), field) = UK_SLIST_FIRST((head));			\
	UK_SLIST_FIRST((head)) = (elm);					\
} while (0)

#define	UK_SLIST_NEXT(elm, field)	((elm)->field.sle_next)

#define	UK_SLIST_REMOVE(head, elm, type, field) do {			\
	UK__QMD_SAVELINK(oldnext, (elm)->field.sle_next);			\
	if (UK_SLIST_FIRST((head)) == (elm)) {				\
		UK_SLIST_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		UK_QUEUE_TYPEOF(type) *curelm = UK_SLIST_FIRST(head);		\
		while (UK_SLIST_NEXT(curelm, field) != (elm))		\
			curelm = UK_SLIST_NEXT(curelm, field);		\
		UK_SLIST_REMOVE_AFTER(curelm, field);			\
	}								\
	UK__TRASHIT(*oldnext);						\
} while (0)

#define UK_SLIST_REMOVE_AFTER(elm, field) do {				\
	UK_SLIST_NEXT(elm, field) =					\
	    UK_SLIST_NEXT(UK_SLIST_NEXT(elm, field), field);			\
} while (0)

#define	UK_SLIST_REMOVE_HEAD(head, field) do {				\
	UK_SLIST_FIRST((head)) = UK_SLIST_NEXT(UK_SLIST_FIRST((head)), field);	\
} while (0)

#define	UK_SLIST_REMOVE_PREVPTR(prevp, elm, field) do {			\
	UK__QMD_SLIST_CHECK_PREVPTR(prevp, elm);				\
	*(prevp) = UK_SLIST_NEXT(elm, field);				\
	UK__TRASHIT((elm)->field.sle_next);					\
} while (0)

#define UK_SLIST_SWAP(head1, head2, type) do {				\
	UK_QUEUE_TYPEOF(type) *swap_first = UK_SLIST_FIRST(head1);		\
	UK_SLIST_FIRST(head1) = UK_SLIST_FIRST(head2);			\
	UK_SLIST_FIRST(head2) = swap_first;				\
} while (0)

/*
 * Singly-linked Tail queue declarations.
 */
#define	UK_STAILQ_HEAD(name, type)						\
struct name {								\
	type *stqh_first;/* first element */			\
	type **stqh_last;/* addr of last next element */		\
}

#define	UK_STAILQ_CLASS_HEAD(name, type)					\
struct name {								\
	class type *stqh_first;	/* first element */			\
	class type **stqh_last;	/* addr of last next element */		\
}

#define	UK_STAILQ_HEAD_INITIALIZER(head)					\
	{ 0, &(head).stqh_first }

#define	UK_STAILQ_ENTRY(type)						\
struct {								\
	type *stqe_next;	/* next element */			\
}

#define	UK_STAILQ_CLASS_ENTRY(type)					\
struct {								\
	class type *stqe_next;	/* next element */			\
}

/*
 * Singly-linked Tail queue functions.
 */
#define	UK_STAILQ_CONCAT(head1, head2) do {				\
	if (!UK_STAILQ_EMPTY((head2))) {					\
		*(head1)->stqh_last = (head2)->stqh_first;		\
		(head1)->stqh_last = (head2)->stqh_last;		\
		UK_STAILQ_INIT((head2));					\
	}								\
} while (0)

#define	UK_STAILQ_EMPTY(head)	((head)->stqh_first == 0)

#define	UK_STAILQ_FIRST(head)	((head)->stqh_first)

#define	UK_STAILQ_FOREACH(var, head, field)				\
	for((var) = UK_STAILQ_FIRST((head));				\
	   (var);							\
	   (var) = UK_STAILQ_NEXT((var), field))

#define	UK_STAILQ_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : UK_STAILQ_FIRST((head)));		\
	   (var);							\
	   (var) = UK_STAILQ_NEXT((var), field))

#define	UK_STAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = UK_STAILQ_FIRST((head));				\
	    (var) && ((tvar) = UK_STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_STAILQ_FOREACH_FROM_SAFE(var, head, field, tvar)		\
	for ((var) = ((var) ? (var) : UK_STAILQ_FIRST((head)));		\
	    (var) && ((tvar) = UK_STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_STAILQ_INIT(head) do {						\
	UK_STAILQ_FIRST((head)) = 0;					\
	(head)->stqh_last = &UK_STAILQ_FIRST((head));			\
} while (0)

#define	UK_STAILQ_INSERT_AFTER(head, tqelm, elm, field) do {		\
	if ((UK_STAILQ_NEXT((elm), field) = UK_STAILQ_NEXT((tqelm), field)) == 0)\
		(head)->stqh_last = &UK_STAILQ_NEXT((elm), field);		\
	UK_STAILQ_NEXT((tqelm), field) = (elm);				\
} while (0)

#define	UK_STAILQ_INSERT_HEAD(head, elm, field) do {			\
	if ((UK_STAILQ_NEXT((elm), field) = UK_STAILQ_FIRST((head))) == 0)	\
		(head)->stqh_last = &UK_STAILQ_NEXT((elm), field);		\
	UK_STAILQ_FIRST((head)) = (elm);					\
} while (0)

#define	UK_STAILQ_INSERT_TAIL(head, elm, field) do {			\
	UK_STAILQ_NEXT((elm), field) = 0;				\
	*(head)->stqh_last = (elm);					\
	(head)->stqh_last = &UK_STAILQ_NEXT((elm), field);			\
} while (0)

#define	UK_STAILQ_LAST(head, type, field)				\
	(UK_STAILQ_EMPTY((head)) ? 0 :				\
	    __containerof((head)->stqh_last,			\
	    UK_QUEUE_TYPEOF(type), field.stqe_next))

#define	UK_STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)

#define	UK_STAILQ_REMOVE(head, elm, type, field) do {			\
	UK__QMD_SAVELINK(oldnext, (elm)->field.stqe_next);			\
	if (UK_STAILQ_FIRST((head)) == (elm)) {				\
		UK_STAILQ_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		UK_QUEUE_TYPEOF(type) *curelm = UK_STAILQ_FIRST(head);	\
		while (UK_STAILQ_NEXT(curelm, field) != (elm))		\
			curelm = UK_STAILQ_NEXT(curelm, field);		\
		UK_STAILQ_REMOVE_AFTER(head, curelm, field);		\
	}								\
	UK__TRASHIT(*oldnext);						\
} while (0)

#define UK_STAILQ_REMOVE_AFTER(head, elm, field) do {			\
	if ((UK_STAILQ_NEXT(elm, field) =					\
	     UK_STAILQ_NEXT(UK_STAILQ_NEXT(elm, field), field)) == 0)	\
		(head)->stqh_last = &UK_STAILQ_NEXT((elm), field);		\
} while (0)

#define	UK_STAILQ_REMOVE_HEAD(head, field) do {				\
	if ((UK_STAILQ_FIRST((head)) =					\
	     UK_STAILQ_NEXT(UK_STAILQ_FIRST((head)), field)) == 0)		\
		(head)->stqh_last = &UK_STAILQ_FIRST((head));		\
} while (0)

#define UK_STAILQ_SWAP(head1, head2, type) do {				\
	UK_QUEUE_TYPEOF(type) *swap_first = UK_STAILQ_FIRST(head1);		\
	UK_QUEUE_TYPEOF(type) **swap_last = (head1)->stqh_last;		\
	UK_STAILQ_FIRST(head1) = UK_STAILQ_FIRST(head2);			\
	(head1)->stqh_last = (head2)->stqh_last;			\
	UK_STAILQ_FIRST(head2) = swap_first;				\
	(head2)->stqh_last = swap_last;					\
	if (UK_STAILQ_EMPTY(head1))					\
		(head1)->stqh_last = &UK_STAILQ_FIRST(head1);		\
	if (UK_STAILQ_EMPTY(head2))					\
		(head2)->stqh_last = &UK_STAILQ_FIRST(head2);		\
} while (0)


/*
 * List declarations.
 */
#define	UK_COMPAT_LIST_HEAD(name, type)					\
struct name {								\
	type *lh_first;	/* first element */			\
}

#define	UK_LIST_CLASS_HEAD(name, type)					\
struct name {								\
	class type *lh_first;	/* first element */			\
}

#define	UK_LIST_HEAD_INITIALIZER(head)					\
	{ 0 }

#define	UK_LIST_ENTRY(type)						\
struct {								\
	type *le_next;	/* next element */			\
	type **le_prev;	/* address of previous next element */	\
}

#define	UK_LIST_CLASS_ENTRY(type)						\
struct {								\
	class type *le_next;	/* next element */			\
	class type **le_prev;	/* address of previous next element */	\
}

/*
 * List functions.
 */

#if (defined(_KERNEL) && defined(INVARIANTS))
/*
 * UK__QMD_LIST_CHECK_HEAD(UK_LIST_HEAD *head, UK_LIST_ENTRY NAME)
 *
 * If the list is non-empty, validates that the first element of the list
 * points back at 'head.'
 */
#define	UK__QMD_LIST_CHECK_HEAD(head, field) do {				\
	if (UK_LIST_FIRST((head)) != 0 &&				\
	    UK_LIST_FIRST((head))->field.le_prev !=			\
	     &UK_LIST_FIRST((head)))					\
            	UK_CRASH("Bad list head %p first->prev != head", (head));	\
} while (0)

/*
 * UK__QMD_LIST_CHECK_NEXT(TYPE *elm, UK_LIST_ENTRY NAME)
 *
 * If an element follows 'elm' in the list, validates that the next element
 * points back at 'elm.'
 */
#define	UK__QMD_LIST_CHECK_NEXT(elm, field) do {				\
	if (UK_LIST_NEXT((elm), field) != 0 &&				\
	    UK_LIST_NEXT((elm), field)->field.le_prev !=			\
	     &((elm)->field.le_next))					\
	        UK_CRASH("Bad link elm %p next->prev != elm", (elm));	\
} while (0)

/*
 * UK__QMD_LIST_CHECK_PREV(TYPE *elm, UK_LIST_ENTRY NAME)
 *
 * Validates that the previous element (or head of the list) points to 'elm.'
 */
#define	UK__QMD_LIST_CHECK_PREV(elm, field) do {				\
	if (*(elm)->field.le_prev != (elm))				\
		UK_CRASH("Bad link elm %p prev->next != elm", (elm));	\
} while (0)
#else
#define	UK__QMD_LIST_CHECK_HEAD(head, field)
#define	UK__QMD_LIST_CHECK_NEXT(elm, field)
#define	UK__QMD_LIST_CHECK_PREV(elm, field)
#endif /* (_KERNEL && INVARIANTS) */

#define UK_LIST_CONCAT(head1, head2, type, field) do {			      \
	UK_QUEUE_TYPEOF(type) *curelm = UK_LIST_FIRST(head1);			      \
	if (curelm == 0) {						      \
		if ((UK_LIST_FIRST(head1) = UK_LIST_FIRST(head2)) != 0) {	      \
			UK_LIST_FIRST(head2)->field.le_prev =		      \
			    &UK_LIST_FIRST((head1));			      \
			UK_LIST_INIT(head2);				      \
		}							      \
	} else if (UK_LIST_FIRST(head2) != 0) {				      \
		while (UK_LIST_NEXT(curelm, field) != 0)		      \
			curelm = UK_LIST_NEXT(curelm, field);		      \
		UK_LIST_NEXT(curelm, field) = UK_LIST_FIRST(head2);		      \
		UK_LIST_FIRST(head2)->field.le_prev = &UK_LIST_NEXT(curelm, field); \
		UK_LIST_INIT(head2);					      \
	}								      \
} while (0)

#define	UK_LIST_EMPTY(head)	((head)->lh_first == 0)

#define	UK_LIST_FIRST(head)	((head)->lh_first)

#define	UK_LIST_FOREACH(var, head, field)					\
	for ((var) = UK_LIST_FIRST((head));				\
	    (var);							\
	    (var) = UK_LIST_NEXT((var), field))

#define	UK_LIST_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : UK_LIST_FIRST((head)));		\
	    (var);							\
	    (var) = UK_LIST_NEXT((var), field))

#define	UK_LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = UK_LIST_FIRST((head));				\
	    (var) && ((tvar) = UK_LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_LIST_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : UK_LIST_FIRST((head)));		\
	    (var) && ((tvar) = UK_LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_LIST_INIT(head) do {						\
	UK_LIST_FIRST((head)) = 0;					\
} while (0)

#define	UK_LIST_INSERT_AFTER(listelm, elm, field) do {			\
	UK__QMD_LIST_CHECK_NEXT(listelm, field);				\
	if ((UK_LIST_NEXT((elm), field) = UK_LIST_NEXT((listelm), field)) != 0)\
		UK_LIST_NEXT((listelm), field)->field.le_prev =		\
		    &UK_LIST_NEXT((elm), field);				\
	UK_LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &UK_LIST_NEXT((listelm), field);		\
} while (0)

#define	UK_LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	UK__QMD_LIST_CHECK_PREV(listelm, field);				\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	UK_LIST_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &UK_LIST_NEXT((elm), field);		\
} while (0)

#define	UK_LIST_INSERT_HEAD(head, elm, field) do {				\
	UK__QMD_LIST_CHECK_HEAD((head), field);				\
	if ((UK_LIST_NEXT((elm), field) = UK_LIST_FIRST((head))) != 0)	\
		UK_LIST_FIRST((head))->field.le_prev = &UK_LIST_NEXT((elm), field);\
	UK_LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &UK_LIST_FIRST((head));			\
} while (0)

#define	UK_LIST_NEXT(elm, field)	((elm)->field.le_next)

#define	UK_LIST_PREV(elm, head, type, field)			\
	((elm)->field.le_prev == &UK_LIST_FIRST((head)) ? 0 :	\
	    __containerof((elm)->field.le_prev,			\
	    UK_QUEUE_TYPEOF(type), field.le_next))

#define	UK_LIST_REMOVE(elm, field) do {					\
	UK__QMD_SAVELINK(oldnext, (elm)->field.le_next);			\
	UK__QMD_SAVELINK(oldprev, (elm)->field.le_prev);			\
	UK__QMD_LIST_CHECK_NEXT(elm, field);				\
	UK__QMD_LIST_CHECK_PREV(elm, field);				\
	if (UK_LIST_NEXT((elm), field) != 0)				\
		UK_LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = UK_LIST_NEXT((elm), field);		\
	UK__TRASHIT(*oldnext);						\
	UK__TRASHIT(*oldprev);						\
} while (0)

#define UK_LIST_SWAP(head1, head2, type, field) do {			\
	UK_QUEUE_TYPEOF(type) *swap_tmp = UK_LIST_FIRST(head1);		\
	UK_LIST_FIRST((head1)) = UK_LIST_FIRST((head2));			\
	UK_LIST_FIRST((head2)) = swap_tmp;					\
	if ((swap_tmp = UK_LIST_FIRST((head1))) != 0)			\
		swap_tmp->field.le_prev = &UK_LIST_FIRST((head1));		\
	if ((swap_tmp = UK_LIST_FIRST((head2))) != 0)			\
		swap_tmp->field.le_prev = &UK_LIST_FIRST((head2));		\
} while (0)

/*
 * Tail queue declarations.
 */
#define	UK_TAILQ_HEAD(name, type)						\
struct name {								\
	type *tqh_first;	/* first element */			\
	type **tqh_last;	/* addr of last next element */		\
	UK__TRACEBUF							\
}

#define	UK_TAILQ_CLASS_HEAD(name, type)					\
struct name {								\
	class type *tqh_first;	/* first element */			\
	class type **tqh_last;	/* addr of last next element */		\
	UK__TRACEBUF							\
}

#define	UK_TAILQ_HEAD_INITIALIZER(head)					\
	{ 0, &(head).tqh_first, UK__TRACEBUF_INITIALIZER }

#define	UK_TAILQ_ENTRY(type)						\
struct {								\
	type *tqe_next;	/* next element */			\
	type **tqe_prev;	/* address of previous next element */	\
	UK__TRACEBUF							\
}

#define	UK_TAILQ_CLASS_ENTRY(type)						\
struct {								\
	class type *tqe_next;	/* next element */			\
	class type **tqe_prev;	/* address of previous next element */	\
	UK__TRACEBUF							\
}

/*
 * Tail queue functions.
 */
#if (defined(_KERNEL) && defined(INVARIANTS))
/*
 * UK__QMD_TAILQ_CHECK_HEAD(UK_TAILQ_HEAD *head, UK_TAILQ_ENTRY NAME)
 *
 * If the tailq is non-empty, validates that the first element of the tailq
 * points back at 'head.'
 */

#define	UK__QMD_TAILQ_CHECK_HEAD(head, field) do {				\
	if (!UK_TAILQ_EMPTY(head) &&					\
	    UK_TAILQ_FIRST((head))->field.tqe_prev !=			\
	     &UK_TAILQ_FIRST((head)))					\
		UK_CRASH("Bad tailq head %p first->prev != head", (head));	\
} while (0)

/*
 * UK__QMD_TAILQ_CHECK_TAIL(UK_TAILQ_HEAD *head, UK_TAILQ_ENTRY NAME)
 *
 * Validates that the tail of the tailq is a pointer to pointer to 0.
 */
#define	UK__QMD_TAILQ_CHECK_TAIL(head, field) do {				\
	if (*(head)->tqh_last != 0)					\
	    	UK_CRASH("Bad tailq NEXT(%p->tqh_last) != 0", (head)); 	\
} while (0)

/*
 * UK__QMD_TAILQ_CHECK_NEXT(TYPE *elm, UK_TAILQ_ENTRY NAME)
 *
 * If an element follows 'elm' in the tailq, validates that the next element
 * points back at 'elm.'
 */
#define	UK__QMD_TAILQ_CHECK_NEXT(elm, field) do {				\
	if (UK_TAILQ_NEXT((elm), field) != 0 &&				\
	    UK_TAILQ_NEXT((elm), field)->field.tqe_prev !=			\
	     &((elm)->field.tqe_next))					\
		UK_CRASH("Bad link elm %p next->prev != elm", (elm));	\
} while (0)

/*
 * UK__QMD_TAILQ_CHECK_PREV(TYPE *elm, UK_TAILQ_ENTRY NAME)
 *
 * Validates that the previous element (or head of the tailq) points to 'elm.'
 */
#define	UK__QMD_TAILQ_CHECK_PREV(elm, field) do {				\
	if (*(elm)->field.tqe_prev != (elm))				\
		UK_CRASH("Bad link elm %p prev->next != elm", (elm));	\
} while (0)
#else
#define	UK__QMD_TAILQ_CHECK_HEAD(head, field)
#define	UK__QMD_TAILQ_CHECK_TAIL(head, headname)
#define	UK__QMD_TAILQ_CHECK_NEXT(elm, field)
#define	UK__QMD_TAILQ_CHECK_PREV(elm, field)
#endif /* (_KERNEL && INVARIANTS) */

#define	UK_TAILQ_CONCAT(head1, head2, field) do {				\
	if (!UK_TAILQ_EMPTY(head2)) {					\
		*(head1)->tqh_last = (head2)->tqh_first;		\
		(head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;	\
		(head1)->tqh_last = (head2)->tqh_last;			\
		UK_TAILQ_INIT((head2));					\
		UK__QMD_TRACE_HEAD(head1);					\
		UK__QMD_TRACE_HEAD(head2);					\
	}								\
} while (0)

#define	UK_TAILQ_EMPTY(head)	((head)->tqh_first == 0)

#define	UK_TAILQ_FIRST(head)	((head)->tqh_first)

#define	UK_TAILQ_FOREACH(var, head, field)					\
	for ((var) = UK_TAILQ_FIRST((head));				\
	    (var);							\
	    (var) = UK_TAILQ_NEXT((var), field))

#define	UK_TAILQ_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : UK_TAILQ_FIRST((head)));		\
	    (var);							\
	    (var) = UK_TAILQ_NEXT((var), field))

#define	UK_TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = UK_TAILQ_FIRST((head));				\
	    (var) && ((tvar) = UK_TAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_TAILQ_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : UK_TAILQ_FIRST((head)));		\
	    (var) && ((tvar) = UK_TAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	UK_TAILQ_FOREACH_REVERSE(var, head, headname, field)		\
	for ((var) = UK_TAILQ_LAST((head), headname);			\
	    (var);							\
	    (var) = UK_TAILQ_PREV((var), headname, field))

#define	UK_TAILQ_FOREACH_REVERSE_FROM(var, head, headname, field)		\
	for ((var) = ((var) ? (var) : UK_TAILQ_LAST((head), headname));	\
	    (var);							\
	    (var) = UK_TAILQ_PREV((var), headname, field))

#define	UK_TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)	\
	for ((var) = UK_TAILQ_LAST((head), headname);			\
	    (var) && ((tvar) = UK_TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))

#define	UK_TAILQ_FOREACH_REVERSE_FROM_SAFE(var, head, headname, field, tvar) \
	for ((var) = ((var) ? (var) : UK_TAILQ_LAST((head), headname));	\
	    (var) && ((tvar) = UK_TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))

#define	UK_TAILQ_INIT(head) do {						\
	UK_TAILQ_FIRST((head)) = 0;					\
	(head)->tqh_last = &UK_TAILQ_FIRST((head));			\
	UK__QMD_TRACE_HEAD(head);						\
} while (0)

#define	UK_TAILQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	UK__QMD_TAILQ_CHECK_NEXT(listelm, field);				\
	if ((UK_TAILQ_NEXT((elm), field) = UK_TAILQ_NEXT((listelm), field)) != 0)\
		UK_TAILQ_NEXT((elm), field)->field.tqe_prev = 		\
		    &UK_TAILQ_NEXT((elm), field);				\
	else {								\
		(head)->tqh_last = &UK_TAILQ_NEXT((elm), field);		\
		UK__QMD_TRACE_HEAD(head);					\
	}								\
	UK_TAILQ_NEXT((listelm), field) = (elm);				\
	(elm)->field.tqe_prev = &UK_TAILQ_NEXT((listelm), field);		\
	UK__QMD_TRACE_ELEM(&(elm)->field);					\
	UK__QMD_TRACE_ELEM(&(listelm)->field);				\
} while (0)

#define	UK_TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	UK__QMD_TAILQ_CHECK_PREV(listelm, field);				\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	UK_TAILQ_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &UK_TAILQ_NEXT((elm), field);		\
	UK__QMD_TRACE_ELEM(&(elm)->field);					\
	UK__QMD_TRACE_ELEM(&(listelm)->field);				\
} while (0)

#define	UK_TAILQ_INSERT_HEAD(head, elm, field) do {			\
	UK__QMD_TAILQ_CHECK_HEAD(head, field);				\
	if ((UK_TAILQ_NEXT((elm), field) = UK_TAILQ_FIRST((head))) != 0)	\
		UK_TAILQ_FIRST((head))->field.tqe_prev =			\
		    &UK_TAILQ_NEXT((elm), field);				\
	else								\
		(head)->tqh_last = &UK_TAILQ_NEXT((elm), field);		\
	UK_TAILQ_FIRST((head)) = (elm);					\
	(elm)->field.tqe_prev = &UK_TAILQ_FIRST((head));			\
	UK__QMD_TRACE_HEAD(head);						\
	UK__QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#define	UK_TAILQ_INSERT_TAIL(head, elm, field) do {			\
	UK__QMD_TAILQ_CHECK_TAIL(head, field);				\
	UK_TAILQ_NEXT((elm), field) = 0;				\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &UK_TAILQ_NEXT((elm), field);			\
	UK__QMD_TRACE_HEAD(head);						\
	UK__QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#define	UK_TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))

#define	UK_TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define	UK_TAILQ_PREV(elm, headname, field)				\
	(*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define	UK_TAILQ_REMOVE(head, elm, field) do {				\
	UK__QMD_SAVELINK(oldnext, (elm)->field.tqe_next);			\
	UK__QMD_SAVELINK(oldprev, (elm)->field.tqe_prev);			\
	UK__QMD_TAILQ_CHECK_NEXT(elm, field);				\
	UK__QMD_TAILQ_CHECK_PREV(elm, field);				\
	if ((UK_TAILQ_NEXT((elm), field)) != 0)				\
		UK_TAILQ_NEXT((elm), field)->field.tqe_prev = 		\
		    (elm)->field.tqe_prev;				\
	else {								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
		UK__QMD_TRACE_HEAD(head);					\
	}								\
	*(elm)->field.tqe_prev = UK_TAILQ_NEXT((elm), field);		\
	UK__TRASHIT(*oldnext);						\
	UK__TRASHIT(*oldprev);						\
	UK__QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#define UK_TAILQ_SWAP(head1, head2, type, field) do {			\
	UK_QUEUE_TYPEOF(type) *swap_first = (head1)->tqh_first;		\
	UK_QUEUE_TYPEOF(type) **swap_last = (head1)->tqh_last;		\
	(head1)->tqh_first = (head2)->tqh_first;			\
	(head1)->tqh_last = (head2)->tqh_last;				\
	(head2)->tqh_first = swap_first;				\
	(head2)->tqh_last = swap_last;					\
	if ((swap_first = (head1)->tqh_first) != 0)			\
		swap_first->field.tqe_prev = &(head1)->tqh_first;	\
	else								\
		(head1)->tqh_last = &(head1)->tqh_first;		\
	if ((swap_first = (head2)->tqh_first) != 0)			\
		swap_first->field.tqe_prev = &(head2)->tqh_first;	\
	else								\
		(head2)->tqh_last = &(head2)->tqh_first;		\
} while (0)

#endif /* !UK__SYS_QUEUE_H_ */

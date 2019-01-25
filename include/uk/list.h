/* SPDX-License-Identifier: BSD-3-Clause */
/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013-2016 Mellanox Technologies, Ltd.
 * Copyright (c) 2018 NEC Europe Ltd., NEC Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */
#ifndef _LINUX_LIST_H_
#define _LINUX_LIST_H_

#include <uk/arch/atomic.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_LIST_HEAD_INIT(name) { &(name), &(name) }

#define UK_LIST_HEAD(name) \
	struct uk_list_head name = UK_LIST_HEAD_INIT(name)

struct uk_list_head {
	struct uk_list_head *next;
	struct uk_list_head *prev;
};

static inline void
UK_INIT_LIST_HEAD(struct uk_list_head *list)
{
	list->next = list->prev = list;
}

static inline int
uk_list_empty(const struct uk_list_head *head)
{
	return (head->next == head);
}

static inline int
uk_list_empty_careful(const struct uk_list_head *head)
{
	struct uk_list_head *next = head->next;

	return ((next == head) && (next == head->prev));
}

static inline void
__uk_list_del(struct uk_list_head *prev, struct uk_list_head *next)
{
	next->prev = prev;
	UK_WRITE_ONCE(prev->next, next);
}

static inline void
__uk_list_del_entry(struct uk_list_head *entry)
{
	__uk_list_del(entry->prev, entry->next);
}

static inline void
uk_list_del(struct uk_list_head *entry)
{
	__uk_list_del(entry->prev, entry->next);
}

static inline void
uk_list_replace(struct uk_list_head *old_entry, struct uk_list_head *new_entry)
{
	new_entry->next = old_entry->next;
	new_entry->next->prev = new_entry;
	new_entry->prev = old_entry->prev;
	new_entry->prev->next = new_entry;
}

static inline void
uk_list_replace_init(struct uk_list_head *old_entry,
		     struct uk_list_head *new_entry)
{
	uk_list_replace(old_entry, new_entry);
	UK_INIT_LIST_HEAD(old_entry);
}

static inline void
__uk_list_add(struct uk_list_head *new_entry, struct uk_list_head *prev,
	      struct uk_list_head *next)
{
	next->prev = new_entry;
	new_entry->next = next;
	new_entry->prev = prev;
	prev->next = new_entry;
}

static inline void
uk_list_del_init(struct uk_list_head *entry)
{
	uk_list_del(entry);
	UK_INIT_LIST_HEAD(entry);
}

#define	uk_list_entry(ptr, type, field)	__containerof(ptr, type, field)

#define	uk_list_first_entry(ptr, type, member) \
	uk_list_entry((ptr)->next, type, member)

#define	uk_list_last_entry(ptr, type, member)	\
	uk_list_entry((ptr)->prev, type, member)

#define	uk_list_first_entry_or_null(ptr, type, member) \
	(!uk_list_empty(ptr) ? uk_list_first_entry(ptr, type, member) : NULL)

#define	uk_list_next_entry(ptr, member)					\
	uk_list_entry(((ptr)->member.next), typeof(*(ptr)), member)

#define	uk_list_safe_reset_next(ptr, n, member) \
	((n) = uk_list_next_entry(ptr, member))

#define	uk_list_prev_entry(ptr, member)					\
	uk_list_entry(((ptr)->member.prev), typeof(*(ptr)), member)

#define	uk_list_for_each(p, head)				\
	for (p = (head)->next; p != (head); p = (p)->next)

#define	uk_list_for_each_safe(p, n, head)				\
	for (p = (head)->next, n = (p)->next; p != (head); p = n, n = (p)->next)

#define uk_list_for_each_entry(p, h, field)				\
	for (p = uk_list_entry((h)->next, typeof(*p), field);		\
	     &(p)->field != (h);					\
	     p = uk_list_entry((p)->field.next, typeof(*p), field))

#define uk_list_for_each_entry_safe(p, n, h, field)			\
	for (p = uk_list_entry((h)->next, typeof(*p), field),		\
		     n = uk_list_entry((p)->field.next, typeof(*p), field); \
	     &(p)->field != (h);					\
	     p = n, n = uk_list_entry(n->field.next, typeof(*n), field))

#define	uk_list_for_each_entry_from(p, h, field) \
	for ( ; &(p)->field != (h); \
	    p = uk_list_entry((p)->field.next, typeof(*p), field))

#define	uk_list_for_each_entry_continue(p, h, field)			\
	for (p = uk_list_next_entry((p), field); &(p)->field != (h);	\
	    p = uk_list_next_entry((p), field))

#define	uk_list_for_each_entry_safe_from(pos, n, head, member)		\
	for (n = uk_list_entry((pos)->member.next, typeof(*pos), member); \
	     &(pos)->member != (head);					\
	     pos = n, n = uk_list_entry(n->member.next, typeof(*n), member))

#define	uk_list_for_each_entry_reverse(p, h, field)			\
	for (p = uk_list_entry((h)->prev, typeof(*p), field);		\
	     &(p)->field != (h);					\
	     p = uk_list_entry((p)->field.prev, typeof(*p), field))

#define	uk_list_for_each_entry_safe_reverse(p, n, h, field)		\
	for (p = uk_list_entry((h)->prev, typeof(*p), field),		\
		     n = uk_list_entry((p)->field.prev, typeof(*p), field); \
	     &(p)->field != (h);					\
	     p = n, n = uk_list_entry(n->field.prev, typeof(*n), field))

#define	uk_list_for_each_entry_continue_reverse(p, h, field)		\
	for (p = uk_list_entry((p)->field.prev, typeof(*p), field);	\
	     &(p)->field != (h);					\
	     p = uk_list_entry((p)->field.prev, typeof(*p), field))

#define	uk_list_for_each_prev(p, h) for (p = (h)->prev; p != (h); p = (p)->prev)

static inline void
uk_list_add(struct uk_list_head *new_entry, struct uk_list_head *head)
{
	__uk_list_add(new_entry, head, head->next);
}

static inline void
uk_list_add_tail(struct uk_list_head *new_entry, struct uk_list_head *head)
{
	__uk_list_add(new_entry, head->prev, head);
}

static inline void
uk_list_move(struct uk_list_head *list, struct uk_list_head *head)
{
	uk_list_del(list);
	uk_list_add(list, head);
}

static inline void
uk_list_move_tail(struct uk_list_head *entry, struct uk_list_head *head)
{
	uk_list_del(entry);
	uk_list_add_tail(entry, head);
}

static inline void
__uk_list_splice(const struct uk_list_head *list, struct uk_list_head *prev,
		 struct uk_list_head *next)
{
	struct uk_list_head *first;
	struct uk_list_head *last;

	if (uk_list_empty(list))
		return;
	first = list->next;
	last = list->prev;
	first->prev = prev;
	prev->next = first;
	last->next = next;
	next->prev = last;
}

static inline void
uk_list_splice(const struct uk_list_head *list, struct uk_list_head *head)
{
	__uk_list_splice(list, head, head->next);
}

static inline void
uk_list_splice_tail(struct uk_list_head *list, struct uk_list_head *head)
{
	__uk_list_splice(list, head->prev, head);
}

static inline void
uk_list_splice_init(struct uk_list_head *list, struct uk_list_head *head)
{
	__uk_list_splice(list, head, head->next);
	UK_INIT_LIST_HEAD(list);
}

static inline void
uk_list_splice_tail_init(struct uk_list_head *list, struct uk_list_head *head)
{
	__uk_list_splice(list, head->prev, head);
	UK_INIT_LIST_HEAD(list);
}


struct uk_hlist_head {
	struct uk_hlist_node *first;
};

struct uk_hlist_node {
	struct uk_hlist_node *next, **pprev;
};

#define	UK_HLIST_HEAD_INIT { }
#define	UK_HLIST_HEAD(name) struct uk_hlist_head name = UK_HLIST_HEAD_INIT
#define	UK_INIT_HLIST_HEAD(head) ((head)->first = NULL)
#define	UK_INIT_HLIST_NODE(node)					\
do {									\
	(node)->next = NULL;						\
	(node)->pprev = NULL;						\
} while (0)

static inline int
uk_hlist_unhashed(const struct uk_hlist_node *h)
{
	return !h->pprev;
}

static inline int
uk_hlist_empty(const struct uk_hlist_head *h)
{
	return !UK_READ_ONCE(h->first);
}

static inline void
uk_hlist_del(struct uk_hlist_node *n)
{
	UK_WRITE_ONCE(*(n->pprev), n->next);
	if (n->next != NULL)
		n->next->pprev = n->pprev;
}

static inline void
uk_hlist_del_init(struct uk_hlist_node *n)
{
	if (uk_hlist_unhashed(n))
		return;
	uk_hlist_del(n);
	UK_INIT_HLIST_NODE(n);
}

static inline void
uk_hlist_add_head(struct uk_hlist_node *n, struct uk_hlist_head *h)
{
	n->next = h->first;
	if (h->first != NULL)
		h->first->pprev = &n->next;
	UK_WRITE_ONCE(h->first, n);
	n->pprev = &h->first;
}

static inline void
uk_hlist_add_before(struct uk_hlist_node *n, struct uk_hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	UK_WRITE_ONCE(*(n->pprev), n);
}

static inline void
uk_hlist_add_behind(struct uk_hlist_node *n, struct uk_hlist_node *prev)
{
	n->next = prev->next;
	UK_WRITE_ONCE(prev->next, n);
	n->pprev = &prev->next;

	if (n->next != NULL)
		n->next->pprev = &n->next;
}

static inline void
uk_hlist_move_list(struct uk_hlist_head *old_entry,
		   struct uk_hlist_head *new_entry)
{
	new_entry->first = old_entry->first;
	if (new_entry->first)
		new_entry->first->pprev = &new_entry->first;
	old_entry->first = NULL;
}

static inline int uk_list_is_singular(const struct uk_list_head *head)
{
	return !uk_list_empty(head) && (head->next == head->prev);
}

static inline void __uk_list_cut_position(struct uk_list_head *list,
		struct uk_list_head *head, struct uk_list_head *entry)
{
	struct uk_list_head *new_first = entry->next;

	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

static inline void uk_list_cut_position(struct uk_list_head *list,
		struct uk_list_head *head, struct uk_list_head *entry)
{
	if (uk_list_empty(head))
		return;
	if (uk_list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		UK_INIT_LIST_HEAD(list);
	else
		__uk_list_cut_position(list, head, entry);
}

static inline int uk_list_is_last(const struct uk_list_head *list,
				const struct uk_list_head *head)
{
	return list->next == head;
}

#define	uk_hlist_entry(ptr, type, field)	__containerof(ptr, type, field)

#define	uk_hlist_for_each(p, head)			\
	for (p = (head)->first; p; p = (p)->next)

#define	uk_hlist_for_each_safe(p, n, head)				\
	for (p = (head)->first; p && ({ n = (p)->next; 1; }); p = n)

#define	uk_hlist_entry_safe(ptr, type, member) \
	((ptr) ? uk_hlist_entry(ptr, type, member) : NULL)

#define	uk_hlist_for_each_entry(pos, head, member)		\
	for (pos = uk_hlist_entry_safe((head)->first,		\
				       typeof(*(pos)), member);	\
	     pos;						\
	     pos = uk_hlist_entry_safe((pos)->member.next,	\
				       typeof(*(pos)), member))

#define	uk_hlist_for_each_entry_continue(pos, member)		\
	for (pos = uk_hlist_entry_safe((pos)->member.next,	\
				       typeof(*(pos)),		\
				       member);			\
	     (pos);						\
	     pos = uk_hlist_entry_safe((pos)->member.next,	\
				       typeof(*(pos)), member))

#define	uk_hlist_for_each_entry_from(pos, member)		\
	for (; (pos);						\
	     pos = uk_hlist_entry_safe((pos)->member.next,	\
				       typeof(*(pos)),		\
				       member))

#define	uk_hlist_for_each_entry_safe(pos, n, head, member)		\
	for (pos = uk_hlist_entry_safe((head)->first, typeof(*(pos)), member); \
	     (pos) && ({ n = (pos)->member.next; 1; });			\
	     pos = uk_hlist_entry_safe(n, typeof(*(pos)), member))

#ifdef __cplusplus
}
#endif

/* TODO: get rid of the old linked list implementation */
#include <uk/compat_list.h>

#endif /* _LINUX_LIST_H_ */

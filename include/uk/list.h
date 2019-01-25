/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013-2016 Mellanox Technologies, Ltd.
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

#ifndef prefetch
#define	prefetch(x)
#endif

#define LINUX_LIST_HEAD_INIT(name) { &(name), &(name) }

#define LINUX_LIST_HEAD(name) \
	struct list_head name = LINUX_LIST_HEAD_INIT(name)

#ifndef LIST_HEAD_DEF
#define	LIST_HEAD_DEF
struct list_head {
	struct list_head *next;
	struct list_head *prev;
};
#endif

static inline void
INIT_LIST_HEAD(struct list_head *list)
{

	list->next = list->prev = list;
}

static inline int
list_empty(const struct list_head *head)
{

	return (head->next == head);
}

static inline int
list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;

	return ((next == head) && (next == head->prev));
}

static inline void
__list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	UK_WRITE_ONCE(prev->next, next);
}

static inline void
__list_del_entry(struct list_head *entry)
{

	__list_del(entry->prev, entry->next);
}

static inline void
list_del(struct list_head *entry)
{

	__list_del(entry->prev, entry->next);
}

static inline void
list_replace(struct list_head *old_entry, struct list_head *new_entry)
{
	new_entry->next = old_entry->next;
	new_entry->next->prev = new_entry;
	new_entry->prev = old_entry->prev;
	new_entry->prev->next = new_entry;
}

static inline void
list_replace_init(struct list_head *old_entry, struct list_head *new_entry)
{
	list_replace(old_entry, new_entry);
	INIT_LIST_HEAD(old_entry);
}

static inline void
linux_list_add(struct list_head *new_entry, struct list_head *prev,
    struct list_head *next)
{

	next->prev = new_entry;
	new_entry->next = next;
	new_entry->prev = prev;
	prev->next = new_entry;
}

static inline void
list_del_init(struct list_head *entry)
{

	list_del(entry);
	INIT_LIST_HEAD(entry);
}

#define	list_entry(ptr, type, field)	__containerof(ptr, type, field)

#define	list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define	list_last_entry(ptr, type, member)	\
	list_entry((ptr)->prev, type, member)

#define	list_first_entry_or_null(ptr, type, member) \
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)

#define	list_next_entry(ptr, member)					\
	list_entry(((ptr)->member.next), typeof(*(ptr)), member)

#define	list_safe_reset_next(ptr, n, member) \
	(n) = list_next_entry(ptr, member)

#define	list_prev_entry(ptr, member)					\
	list_entry(((ptr)->member.prev), typeof(*(ptr)), member)

#define	list_for_each(p, head)						\
	for (p = (head)->next; p != (head); p = (p)->next)

#define	list_for_each_safe(p, n, head)					\
	for (p = (head)->next, n = (p)->next; p != (head); p = n, n = (p)->next)

#define list_for_each_entry(p, h, field)				\
	for (p = list_entry((h)->next, typeof(*p), field); &(p)->field != (h); \
	    p = list_entry((p)->field.next, typeof(*p), field))

#define list_for_each_entry_safe(p, n, h, field)			\
	for (p = list_entry((h)->next, typeof(*p), field),		\
	    n = list_entry((p)->field.next, typeof(*p), field); &(p)->field != (h);\
	    p = n, n = list_entry(n->field.next, typeof(*n), field))

#define	list_for_each_entry_from(p, h, field) \
	for ( ; &(p)->field != (h); \
	    p = list_entry((p)->field.next, typeof(*p), field))

#define	list_for_each_entry_continue(p, h, field)			\
	for (p = list_next_entry((p), field); &(p)->field != (h);	\
	    p = list_next_entry((p), field))

#define	list_for_each_entry_safe_from(pos, n, head, member)			\
	for (n = list_entry((pos)->member.next, typeof(*pos), member);		\
	     &(pos)->member != (head);						\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define	list_for_each_entry_reverse(p, h, field)			\
	for (p = list_entry((h)->prev, typeof(*p), field); &(p)->field != (h); \
	    p = list_entry((p)->field.prev, typeof(*p), field))

#define	list_for_each_entry_safe_reverse(p, n, h, field)		\
	for (p = list_entry((h)->prev, typeof(*p), field),		\
	    n = list_entry((p)->field.prev, typeof(*p), field); &(p)->field != (h); \
	    p = n, n = list_entry(n->field.prev, typeof(*n), field))

#define	list_for_each_entry_continue_reverse(p, h, field) \
	for (p = list_entry((p)->field.prev, typeof(*p), field); &(p)->field != (h); \
	    p = list_entry((p)->field.prev, typeof(*p), field))

#define	list_for_each_prev(p, h) for (p = (h)->prev; p != (h); p = (p)->prev)

static inline void
list_add(struct list_head *new_entry, struct list_head *head)
{

	linux_list_add(new_entry, head, head->next);
}

static inline void
list_add_tail(struct list_head *new_entry, struct list_head *head)
{

	linux_list_add(new_entry, head->prev, head);
}

static inline void
list_move(struct list_head *list, struct list_head *head)
{

	list_del(list);
	list_add(list, head);
}

static inline void
list_move_tail(struct list_head *entry, struct list_head *head)
{

	list_del(entry);
	list_add_tail(entry, head);
}

static inline void
linux_list_splice(const struct list_head *list, struct list_head *prev,
    struct list_head *next)
{
	struct list_head *first;
	struct list_head *last;

	if (list_empty(list))
		return;
	first = list->next;
	last = list->prev;
	first->prev = prev;
	prev->next = first;
	last->next = next;
	next->prev = last;
}

static inline void
list_splice(const struct list_head *list, struct list_head *head)
{

	linux_list_splice(list, head, head->next);
}

static inline void
list_splice_tail(struct list_head *list, struct list_head *head)
{

	linux_list_splice(list, head->prev, head);
}

static inline void
list_splice_init(struct list_head *list, struct list_head *head)
{

	linux_list_splice(list, head, head->next);
	INIT_LIST_HEAD(list);
}

static inline void
list_splice_tail_init(struct list_head *list, struct list_head *head)
{

	linux_list_splice(list, head->prev, head);
	INIT_LIST_HEAD(list);
}

#undef LIST_HEAD
#define LIST_HEAD(name)	struct list_head name = { &(name), &(name) }


struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#define	HLIST_HEAD_INIT { }
#define	HLIST_HEAD(name) struct hlist_head name = HLIST_HEAD_INIT
#define	INIT_HLIST_HEAD(head) (head)->first = NULL
#define	INIT_HLIST_NODE(node)						\
do {									\
	(node)->next = NULL;						\
	(node)->pprev = NULL;						\
} while (0)

static inline int
hlist_unhashed(const struct hlist_node *h)
{

	return !h->pprev;
}

static inline int
hlist_empty(const struct hlist_head *h)
{

	return !UK_READ_ONCE(h->first);
}

static inline void
hlist_del(struct hlist_node *n)
{

	UK_WRITE_ONCE(*(n->pprev), n->next);
	if (n->next != NULL)
		n->next->pprev = n->pprev;
}

static inline void
hlist_del_init(struct hlist_node *n)
{

	if (hlist_unhashed(n))
		return;
	hlist_del(n);
	INIT_HLIST_NODE(n);
}

static inline void
hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{

	n->next = h->first;
	if (h->first != NULL)
		h->first->pprev = &n->next;
	UK_WRITE_ONCE(h->first, n);
	n->pprev = &h->first;
}

static inline void
hlist_add_before(struct hlist_node *n, struct hlist_node *next)
{

	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	UK_WRITE_ONCE(*(n->pprev), n);
}

static inline void
hlist_add_behind(struct hlist_node *n, struct hlist_node *prev)
{

	n->next = prev->next;
	UK_WRITE_ONCE(prev->next, n);
	n->pprev = &prev->next;

	if (n->next != NULL)
		n->next->pprev = &n->next;
}

static inline void
hlist_move_list(struct hlist_head *old_entry, struct hlist_head *new_entry)
{

	new_entry->first = old_entry->first;
	if (new_entry->first)
		new_entry->first->pprev = &new_entry->first;
	old_entry->first = NULL;
}

static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct list_head *list,
		struct list_head *head, struct list_head *entry)
{
	struct list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

static inline void list_cut_position(struct list_head *list,
		struct list_head *head, struct list_head *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline int list_is_last(const struct list_head *list,
				const struct list_head *head)
{
	return list->next == head;
}

#define	hlist_entry(ptr, type, field)	__containerof(ptr, type, field)

#define	hlist_for_each(p, head)						\
	for (p = (head)->first; p; p = (p)->next)

#define	hlist_for_each_safe(p, n, head)					\
	for (p = (head)->first; p && ({ n = (p)->next; 1; }); p = n)

#define	hlist_entry_safe(ptr, type, member) \
	((ptr) ? hlist_entry(ptr, type, member) : NULL)

#define	hlist_for_each_entry(pos, head, member)				\
	for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define	hlist_for_each_entry_continue(pos, member)			\
	for (pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member); \
	     (pos);							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define	hlist_for_each_entry_from(pos, member)				\
	for (; (pos);								\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define	hlist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member); \
	     (pos) && ({ n = (pos)->member.next; 1; });			\
	     pos = hlist_entry_safe(n, typeof(*(pos)), member))

#ifdef __cplusplus
}
#endif

/* TODO: get rid of the old linked list implementation */
#include <uk/compat_list.h>

#endif /* _LINUX_LIST_H_ */

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
#ifndef _LINUX_RCULIST_H_
#define _LINUX_RCULIST_H_

#include <uk/atomic.h>
#include <uk/list.h>
#include <uk/rcu.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_POISON2  ((void *) 0x00200200)

static inline void INIT_LIST_HEAD_RCU(struct uk_list_head *list)
{
	UK_WRITE_ONCE(list->next, list);
	UK_WRITE_ONCE(list->prev, list);
}

#define list_next_rcu(list)	(*((struct uk_list_head **)(&(list)->next)))
#define list_tail_rcu(head)	(*((struct uk_list_head **)(&(head)->prev)))


static inline void __list_add_rcu(struct uk_list_head *new,
		struct uk_list_head *prev, struct uk_list_head *next)
{
	
	new->next = next;
	new->prev = prev;
	rcu_assign_pointer(list_next_rcu(prev), new);
	next->prev = new;
}

static inline void list_add_rcu(struct uk_list_head *new, struct uk_list_head *head)
{
	__list_add_rcu(new, head, head->next);
}

static inline void list_add_tail_rcu(struct uk_list_head *new,
					struct uk_list_head *head)
{
	__list_add_rcu(new, head->prev, head);
}

static inline void list_del_rcu(struct uk_list_head *entry)
{
	__uk_list_del_entry(entry);
	entry->prev = LIST_POISON2;
}


static inline void list_replace_rcu(struct uk_list_head *old,
				struct uk_list_head *new)
{
	new->next = old->next;
	new->prev = old->prev;
	rcu_assign_pointer(list_next_rcu(new->prev), new);
	new->next->prev = new;
	old->prev = LIST_POISON2;
}

static inline void __list_splice_init_rcu(struct uk_list_head *list,
					  struct uk_list_head *prev,
					  struct uk_list_head *next,
					  void (*sync)(void))
{
	struct uk_list_head *first = list->next;
	struct uk_list_head *last = list->prev;

	INIT_LIST_HEAD_RCU(list);

	sync();
	UK_ACCESS_ONCE(*first);
	UK_ACCESS_ONCE(*last);

	last->next = next;
	rcu_assign_pointer(list_next_rcu(prev), first);
	first->prev = prev;
	next->prev = last;
}

static inline void list_splice_init_rcu(struct uk_list_head *list,
					struct uk_list_head *head,
					void (*sync)(void))
{
	if (!uk_list_empty(list))
		__list_splice_init_rcu(list, head, head->next, sync);
}

static inline void list_splice_tail_init_rcu(struct uk_list_head *list,
					     struct uk_list_head *head,
					     void (*sync)(void))
{
	if (!uk_list_empty(list))
		__list_splice_init_rcu(list, head->prev, head, sync);
}

#ifdef __cplusplus
}
#endif


#endif /* _LINUX_RCULIST_H_ */

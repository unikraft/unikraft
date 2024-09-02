#ifndef _RCULIST_H_
#define _RCULIST_H_

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


#endif /* _RCULIST_H_ */

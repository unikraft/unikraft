/* SPDX-License-Identifier: BSD-3-Clause */
/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013-2018 Mellanox Technologies, Ltd.
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
#ifndef _LINUX_RADIX_TREE_H_
#define _LINUX_RADIX_TREE_H_


/*
 * Radix tree illustration (index=11100000):
                                                                    +-------------+                                                                    
                                                                    |*00* 01 10 11|                                                                    
                                                                    +-------------+                                                                    
                                                                  /---  /   \  ---\                                                                  
                                                            /-----     /     \     -----\                                                            
                                                      /-----          /       \          -----\                                                      
                                                /-----               /         \               -----\                                                
                                         +-------------+     +-----------+     +-----------+     +-----------+                                         
                                         |*00* 01 10 11|     |00 01 10 11|     |00 01 10 11|     |00 01 10 11|                                         
                                         +-------------+     +-----------+     +-----------+     +-----------+                                         
                                         /-                                                              -\                                          
                                      /--                                                                  --\                                       
                                    /-                                                                        --\                                    
                                 /--                                                                             --\                                 
                           +-------------+                                                                     +-----------+                           
                           |00 01 *10* 11|                              .........                              |00 01 10 11|                           
                           +-------------+                                                                     +-----------+                           
                         /---  /   \  ---\                                                                 /---  /   \  ---\                         
                   /-----     /     \     -----\                                                     /-----     /     \     -----\                   
             /-----          /       \          -----\                                         /-----          /       \          -----\             
       /-----               /         \               -----\                             /-----               /         \               -----\       
+-----------+     +-----------+     +-------------+     +-----------+               +-----------+     +-----------+     +-----------+     +-----------+
|00 01 10 11|     |00 01 10 11|     |00 01 10 *11*|     |00 01 10 11|               |00 01 10 11|     |00 01 10 11|     |00 01 10 11|     |00 01 10 11|
+-----------+     +-----------+     +-------------+     +-----------+               +-----------+     +-----------+     +-----------+     +-----------+
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <uk/bitops.h>
#include <uk/bitmap.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/plat/spinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	UK_RADIX_TREE_MAP_SHIFT	6 /* each layer represents 6 bits of the index */
#define	UK_RADIX_TREE_MAP_SIZE	(1UL << UK_RADIX_TREE_MAP_SHIFT) /* max number of child nodes: 2^6 = 64 1000000 */
#define	UK_RADIX_TREE_MAP_MASK	(UK_RADIX_TREE_MAP_SIZE - 1UL) /* mask: 2^6 - 1 = 63 111111 */
#define	UK_RADIX_TREE_MAX_HEIGHT \
	howmany(sizeof(long) * NBBY, UK_RADIX_TREE_MAP_SHIFT)

#define	UK_RADIX_TREE_ENTRY_MASK 3UL
#define	UK_RADIX_TREE_EXCEPTIONAL_ENTRY 2UL
#define	UK_RADIX_TREE_EXCEPTIONAL_SHIFT 2

struct uk_radix_tree_node {
	void		*slots[UK_RADIX_TREE_MAP_SIZE];
	int		count;
};

struct uk_radix_tree_root {
	struct uk_radix_tree_node	*rnode;
	int			height;
};

struct uk_radix_tree_iter {
	unsigned long index;
};

#define	UK_RADIX_TREE_INIT()						\
	    { .rnode = NULL, .height = 0 };
#define	UK_INIT_RADIX_TREE(root)					\
	    { (root)->rnode = NULL; (root)->height = 0; }
#define	UK_RADIX_TREE(name)						\
	    struct uk_radix_tree_root name = UK_RADIX_TREE_INIT()

#define	uk_radix_tree_for_each_slot(slot, root, iter, start) \
	for ((iter)->index = (start);			  \
	     uk_radix_tree_iter_find(root, iter, &(slot)); (iter)->index++)

static inline int
uk_radix_tree_exception(void *arg)
{
	return ((uintptr_t)arg & UK_RADIX_TREE_ENTRY_MASK);
}

static inline unsigned long
uk_radix_max(struct uk_radix_tree_root *root)
{
	return ((1UL << (root->height * UK_RADIX_TREE_MAP_SHIFT)) - 1UL);
}

static inline int
uk_radix_pos(long id, int height)
{
	return (id >> (UK_RADIX_TREE_MAP_SHIFT * height)) & UK_RADIX_TREE_MAP_MASK;
}

static inline void
uk_radix_tree_clean_root_node(struct uk_radix_tree_root *root)
{
	/* Check if the root node should be freed */
	if (root->rnode->count == 0) {
		free(root->rnode);
		root->rnode = NULL;
		root->height = 0;
	}
}

static inline void *
uk_radix_tree_lookup(struct uk_radix_tree_root *root, unsigned long index)
{
	struct uk_radix_tree_node *node;
	void *item;
	int height;

	item = NULL;
	node = root->rnode;
	height = root->height - 1;
	if (index > uk_radix_max(root))
		goto out;
	while (height && node)
		node = node->slots[uk_radix_pos(index, height--)];
	if (node)
		item = node->slots[uk_radix_pos(index, 0)];

out:
	return (item);
}

static inline bool
uk_radix_tree_iter_find(struct uk_radix_tree_root *root, struct uk_radix_tree_iter *iter,
    void ***pppslot)
{
	struct uk_radix_tree_node *node;
	unsigned long index = iter->index;
	int height;

restart:
	node = root->rnode;
	if (node == NULL)
		return (false);
	height = root->height - 1;
	if (height == -1 || index > uk_radix_max(root))
		return (false);
	do {
		unsigned long mask = UK_RADIX_TREE_MAP_MASK << (UK_RADIX_TREE_MAP_SHIFT * height);
		unsigned long step = 1UL << (UK_RADIX_TREE_MAP_SHIFT * height);
		int pos = uk_radix_pos(index, height);
		struct uk_radix_tree_node *next;

		/* track last slot */
		*pppslot = node->slots + pos;

		next = node->slots[pos];
		if (next == NULL) {
			index += step;
			index &= -step;
			if ((index & mask) == 0)
				goto restart;
		} else {
			node = next;
			height--;
		}
	} while (height != -1);
	iter->index = index;
	return (true);
}

static inline void *
uk_radix_tree_delete(struct uk_radix_tree_root *root, unsigned long index)
{
	struct uk_radix_tree_node *stack[UK_RADIX_TREE_MAX_HEIGHT];
	struct uk_radix_tree_node *node;
	void *item;
	int height;
	int idx;

	item = NULL;
	node = root->rnode;
	height = root->height - 1;
	if (index > uk_radix_max(root))
		goto out;
	/*
	 * Find the node and record the path in stack.
	 */
	while (height && node) {
		stack[height] = node;
		node = node->slots[uk_radix_pos(index, height--)];
	}
	idx = uk_radix_pos(index, 0);
	if (node)
		item = node->slots[idx];
	/*
	 * If we removed something reduce the height of the tree.
	 */
	if (item)
		for (;;) {
			node->slots[idx] = NULL;
			node->count--;
			if (node->count > 0)
				break;
			free(node);
			if (node == root->rnode) {
				root->rnode = NULL;
				root->height = 0;
				break;
			}
			height++;
			node = stack[height];
			idx = uk_radix_pos(index, height);
		}
out:
	return (item);
}

static inline void
uk_radix_tree_iter_delete(struct uk_radix_tree_root *root,
    struct uk_radix_tree_iter *iter, void **slot)
{
	uk_radix_tree_delete(root, iter->index);
}

static inline int
uk_radix_tree_insert(struct uk_radix_tree_root *root, unsigned long index, void *item)
{
	struct uk_radix_tree_node *node;
	struct uk_radix_tree_node *temp[UK_RADIX_TREE_MAX_HEIGHT - 1];
	int height;
	int idx;

	/* bail out upon insertion of a NULL item */
	if (item == NULL)
		return (-EINVAL);

	/* get root node, if any */
	node = root->rnode;

	/* allocate root node, if any */
	if (node == NULL) {
		node = malloc(sizeof(*node));
		if (node == NULL)
			return (-ENOMEM);
		root->rnode = node;
		root->height++;
	}

	/* expand radix tree as needed */
	while (uk_radix_max(root) < index) {
		/* check if the radix tree is getting too big */
		if (root->height == UK_RADIX_TREE_MAX_HEIGHT) {
			uk_radix_tree_clean_root_node(root);
			return (-E2BIG);
		}

		/*
		 * If the root radix level is not empty, we need to
		 * allocate a new radix level:
		 */
		if (node->count != 0) {
			node = malloc(sizeof(*node));
			if (node == NULL) {
				/*
				 * Freeing the already allocated radix
				 * levels, if any, will be handled by
				 * the uk_radix_tree_delete() function.
				 * This code path can only happen when
				 * the tree is not empty.
				 */
				return (-ENOMEM);
			}
			node->slots[0] = root->rnode;
			node->count++;
			root->rnode = node;
		}
		root->height++;
	}

	/* get radix tree height index */
	height = root->height - 1;

	/* walk down the tree until the first missing node, if any */
	for ( ; height != 0; height--) {
		idx = uk_radix_pos(index, height);
		if (node->slots[idx] == NULL)
			break;
		node = node->slots[idx];
	}

	/* allocate the missing radix levels, if any */
	for (idx = 0; idx != height; idx++) {
		temp[idx] = malloc(sizeof(*node));
		if (temp[idx] == NULL) {
			while (idx--)
				free(temp[idx]);
			uk_radix_tree_clean_root_node(root);
			return (-ENOMEM);
		}
	}

	/* setup new radix levels, if any */
	for ( ; height != 0; height--) {
		idx = uk_radix_pos(index, height);
		node->slots[idx] = temp[height - 1];
		node->count++;
		node = node->slots[idx];
	}

	/*
	 * Insert and adjust count if the item does not already exist.
	 */
	idx = uk_radix_pos(index, 0);
	if (node->slots[idx]) {
		uk_pr_warn("radix tree insert failed: item already exists (node->slots[%d]=%p)\n", idx, node->slots[idx]);
		return (-EEXIST);
	}
	node->slots[idx] = item;
	node->count++;

	return (0);
}

static inline int
uk_radix_tree_store(struct uk_radix_tree_root *root, unsigned long index, void **ppitem)
{
	struct uk_radix_tree_node *node;
	struct uk_radix_tree_node *temp[UK_RADIX_TREE_MAX_HEIGHT - 1];
	void *pitem;
	int height;
	int idx;

	/*
	 * Inserting a NULL item means delete it. The old pointer is
	 * stored at the location pointed to by "ppitem".
	 */
	if (*ppitem == NULL) {
		*ppitem = uk_radix_tree_delete(root, index);
		return (0);
	}

	/* get root node, if any */
	node = root->rnode;

	/* allocate root node, if any */
	if (node == NULL) {
		node = malloc(sizeof(*node));
		if (node == NULL)
			return (-ENOMEM);
		root->rnode = node;
		root->height++;
	}

	/* expand radix tree as needed */
	while (uk_radix_max(root) < index) {
		/* check if the radix tree is getting too big */
		if (root->height == UK_RADIX_TREE_MAX_HEIGHT) {
			uk_radix_tree_clean_root_node(root);
			return (-E2BIG);
		}

		/*
		 * If the root radix level is not empty, we need to
		 * allocate a new radix level:
		 */
		if (node->count != 0) {
			node = malloc(sizeof(*node));
			if (node == NULL) {
				/*
				 * Freeing the already allocated radix
				 * levels, if any, will be handled by
				 * the uk_radix_tree_delete() function.
				 * This code path can only happen when
				 * the tree is not empty.
				 */
				return (-ENOMEM);
			}
			node->slots[0] = root->rnode;
			node->count++;
			root->rnode = node;
		}
		root->height++;
	}

	/* get radix tree height index */
	height = root->height - 1;

	/* walk down the tree until the first missing node, if any */
	for ( ; height != 0; height--) {
		idx = uk_radix_pos(index, height);
		if (node->slots[idx] == NULL)
			break;
		node = node->slots[idx];
	}

	/* allocate the missing radix levels, if any */
	for (idx = 0; idx != height; idx++) {
		temp[idx] = malloc(sizeof(*node));
		if (temp[idx] == NULL) {
			while (idx--)
				free(temp[idx]);
			uk_radix_tree_clean_root_node(root);
			return (-ENOMEM);
		}
	}

	/* setup new radix levels, if any */
	for ( ; height != 0; height--) {
		idx = uk_radix_pos(index, height);
		node->slots[idx] = temp[height - 1];
		node->count++;
		node = node->slots[idx];
	}

	/*
	 * Insert and adjust count if the item does not already exist.
	 */
	idx = uk_radix_pos(index, 0);
	/* swap */
	pitem = node->slots[idx];
	node->slots[idx] = *ppitem;
	*ppitem = pitem;

	if (pitem == NULL)
		node->count++;
	return (0);
}

#ifdef __cplusplus
}
#endif

#endif
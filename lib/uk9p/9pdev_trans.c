/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <errno.h>
#include <string.h>
#include <uk/config.h>
#include <uk/list.h>
#include <uk/assert.h>
#include <uk/9pdev_trans.h>

static UK_LIST_HEAD(uk_9pdev_trans_list);

static struct uk_9pdev_trans *uk_9pdev_trans_saved_default;

int uk_9pdev_trans_register(struct uk_9pdev_trans *trans)
{
	UK_ASSERT(trans);
	UK_ASSERT(trans->name);
	UK_ASSERT(trans->ops);
	UK_ASSERT(trans->ops->connect);
	UK_ASSERT(trans->ops->disconnect);
	UK_ASSERT(trans->ops->request);
	UK_ASSERT(trans->a);

	uk_list_add_tail(&trans->_list, &uk_9pdev_trans_list);

	if (!uk_9pdev_trans_saved_default)
		uk_9pdev_trans_saved_default = trans;

	uk_pr_info("Registered transport %s\n", trans->name);

	return 0;
}

struct uk_9pdev_trans *uk_9pdev_trans_by_name(const char *name)
{
	struct uk_9pdev_trans *t;

	uk_list_for_each_entry(t, &uk_9pdev_trans_list, _list) {
		if (!strcmp(t->name, name))
			return t;
	}

	return NULL;
}

struct uk_9pdev_trans *uk_9pdev_trans_get_default(void)
{
	return uk_9pdev_trans_saved_default;
}

void uk_9pdev_trans_set_default(struct uk_9pdev_trans *trans)
{
	UK_ASSERT(trans);
	uk_9pdev_trans_saved_default = trans;
}

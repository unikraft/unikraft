/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/vma_ops.h>

const struct uk_vma_ops uk_vma_rsvd_ops = {
	.get_base	= __NULL,
	.new		= __NULL,
	.destroy	= __NULL,
	.fault		= __NULL,
	.unmap		= uk_vma_op_unmap,	/* default */
	.split		= __NULL,
	.merge		= __NULL,
	.set_attr	= uk_vma_op_set_attr,	/* default */
	.advise		= __NULL,
};

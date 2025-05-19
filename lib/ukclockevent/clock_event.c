/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/clock_event.h>

UK_LIST_HEAD(uk_clock_event_list);

void uk_clock_event_register(struct uk_clock_event *ce)
{
	uk_list_add(&ce->clock_list, &uk_clock_event_list);
}

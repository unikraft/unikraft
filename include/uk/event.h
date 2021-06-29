/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author(s): Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
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
 */

#ifndef __UK_EVENT_H__
#define __UK_EVENT_H__

#include <uk/essentials.h>
#include <uk/prio.h>

#ifdef CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LIBUKDEBUG
#define __event_assert(x) UK_ASSERT(x)
#else
#define __event_assert(x)	\
	do {			\
	} while (0)
#endif /* CONFIG_LIBUKDEBUG */

/**
 * Signature for event handler functions.
 *
 * @param data
 *   Optional data parameter. The data is supplied when raising the event
 * @return
 *   One of the UK_EVENT_* macros on success, errno on < 0
 */
typedef int (*uk_event_handler_t)(void *data);

#define UK_EVENT_NOT_HANDLED 0  /* The handler did not handle the event */
#define UK_EVENT_HANDLED     1  /* The handler handled the event */

struct uk_event {
	const uk_event_handler_t *hlist_end;
};

/* Internal macros to create per-event sections. The linker will sort these
 * sections and merge them in the .uk_eventtab section in the final binary. So
 * that we are able to arbitrarly name events (including numbers, underscores,
 * etc.), we use the tilde (~) to mark header and end sections as well as to
 * separate the event name from handler priority. The tilde has a large ASCII
 * value (126) and thus allows us to keep sections in order, irrespective of
 * the chosen event name (and conceptionally also the priority's textual
 * representation).
 */
#define _EVT_EVENT(event)	uk_event_ ## event
#define _EVT_HLIST_END(event)	_uk_eventtab_ ## event ## __end

#define __EVT_SECTION_LABEL(section, label)				\
	__asm__ (							\
		".pushsection \".uk_eventtab_" section "\", \"a\"\n"	\
		#label ":\n"						\
		".popsection\n"						\
	)

#define _EVT_SECTION_LABEL(section, label)				\
	__EVT_SECTION_LABEL(section, label)

#define _EVT_SECTION_HLIST_END(event)					\
	_EVT_SECTION_LABEL(#event "~~", _EVT_HLIST_END(event))

#define _EVT_SECTION_HEADER(event)					\
	extern const uk_event_handler_t _EVT_HLIST_END(event)[];	\
	const struct uk_event						\
	__used __section(".uk_eventtab_" #event "~")			\
	_EVT_EVENT(event) = {						\
		_EVT_HLIST_END(event)					\
	}

#define _EVT_IMPORT_EVENT(event)					\
	extern const struct uk_event _EVT_EVENT(event)

#define _EVT_HLIST_START(e_ptr)						\
	((const uk_event_handler_t *)((e_ptr) + 1))

/**
 * Register an event. If the event should be raised from different libraries
 * export its symbol as 'uk_event_name'.
 *
 * @param name
 *   Name of the event. The name may contain any characters that can be used
 *   in C identifiers.
 */
#define _UK_EVENT(name)							\
	_EVT_SECTION_HEADER(name);					\
	_EVT_SECTION_HLIST_END(name)

#define UK_EVENT(name)							\
	_UK_EVENT(name)

/**
 * Register an event handler that is called when the specified event is raised.
 * Previous handlers may already handled the event, in which case the handler
 * might not be called.
 *
 * @param event
 *   Event which to register the handler for
 * @param fn
 *   Handler function to be called
 * @param prio
 *   Priority level (0 (earliest) to 9 (latest))
 *   Use the UK_PRIO_AFTER() helper macro for computing priority dependencies.
 */
#define __UK_EVENT_HANDLER_PRIO(event, fn, prio)			\
	static const uk_event_handler_t					\
	__used __section(".uk_eventtab_" #event "~" #prio)		\
	_uk_eventtab_ ## event ## _ ## prio ## _ ## fn = (fn)

#define _UK_EVENT_HANDLER_PRIO(event, fn, prio)				\
	__UK_EVENT_HANDLER_PRIO(event, fn, prio)

#define UK_EVENT_HANDLER_PRIO(event, fn, prio)				\
	_UK_EVENT_HANDLER_PRIO(event, fn, prio)

/* Similar interface without priority.*/
#define UK_EVENT_HANDLER(event, fn)					\
	UK_EVENT_HANDLER_PRIO(event, fn, UK_PRIO_LATEST)

/**
 * Helper macro for iterating over event handlers
 *
 * @param itr
 *   Iterator variable (uk_event_handler_t *) which points to the individual
 *   handlers during iteration
 * @param e_ptr
 *   Pointer to uk_event
 */
#define uk_event_handler_foreach(itr, e_ptr)				\
	for ((itr) = DECONST(uk_event_handler_t*, _EVT_HLIST_START(e_ptr));\
	     (itr) < ((e_ptr)->hlist_end);				\
	     (itr)++)

/**
 * Returns a pointer to the event with the given name.
 *
 * @param name
 *   Name of the event to retrieve a pointer to
 * @return
 *   Pointer to the event (struct uk_event *)
 */
#define UK_EVENT_PTR(name)						\
	(&_EVT_EVENT(name))

/**
 * Raise an event by pointer and invoke the handler chain until the first
 * handler successfully handled the event.
 *
 * @param event
 *   Pointer to the event to raise.
 * @param data
 *   Optional data supplied to the event handlers
 * @return
 *   One of the UK_EVENT_* macros on success, errno on < 0
 */
static inline int uk_raise_event_ptr(const struct uk_event *e, void *data)
{
	const uk_event_handler_t *itr;
	int ret = UK_EVENT_NOT_HANDLED;

	__event_assert(e);

	uk_event_handler_foreach(itr, e) {
		__event_assert(*itr);
		ret = ((*itr)(data));
		if (ret)
			break;
	}

	return ret;
}

/**
 * Helper macro to raise an event by name. Invokes the handler chain until the
 * first handler successfully handled the event.
 *
 * @param event
 *   Name of the event to raise.
 * @param data
 *   Optional data supplied to the event handlers
 * @return
 *   One of the UK_EVENT_* macros on success, errno on < 0
 */
#define uk_raise_event(event, data)					\
	({	_EVT_IMPORT_EVENT(event);				\
		uk_raise_event_ptr(UK_EVENT_PTR(event), data);	})

#ifdef __cplusplus
}
#endif

#endif /* __UK_EVENT_H__ */

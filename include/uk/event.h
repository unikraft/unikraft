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

/**
 * ## Overview
 *
 * `uk_event` decouples the definition of an event and the definition of
 * callback routines that should be called whenever the event ocurrs. This way
 * a library (A) can provide an event and a library (B) can register handlers
 * for this event without creating a dependency (A)->(B). Handler registration
 * is done at link time by collecting all handler functions for a particular
 * event and putting their addresses into an event-specific function table.
 * Handlers can indicate that they have handled the event, in which case no
 * other handlers are called, if desired. Handlers can also be assigned a
 * priority class to enforce a certain execution order.
 *
 * ## Using Events
 *
 * In the following, we define an event `myevent` in library (A). We encapsulate
 * additional event-specific information in `struct myevent_data`. Library (B)
 * includes the declaration of `struct myevent_data` and registers three
 * handler functions, where `myhandler1` and `myhandler2` allow other handlers
 * to be called afterwards and `myhandler3` terminates handler invocation.
 * Note: Since there are no guarantees in which order handlers of the same
 * priority class may be called, it is only guaranteed that `handler1` will be
 * the first of libB's handlers that is invoked. However, `handler2` may or may
 * not be called depending on if `handler3` is executed before.
 *
 * **libA/include/uk/myevent.h**:
 * ```C
 * struct myevent_data {
 *       int dummy;
 * };
 * ```
 * **libA/myevent.c:**
 * ```C
 * #include <uk/event.h>
 * #include <uk/myevent.h>
 *
 * UK_EVENT(myevent);
 *
 * void some_function(void)
 * {
 *       struct myevent_data = { .dummy = 42 };
 *       int rc;
 *
 *       rc = uk_raise_event(myevent, &myevent_data);
 *       if (rc < 0)
 *               uk_pr_crit("an event handler returned an error!\n");
 *       else if (!rc)
 *               uk_pr_err("myevent not handled!\n");
 * }
 * ```
 * **libB/myhandler.c:**
 * ```C
 * #include <uk/event.h>
 * #include <uk/myevent.h>
 *
 * int handler1(void *arg)
 * {
 *       struct myevent_data *data = (struct myevent_data *)arg;
 *       ...
 *       return UK_EVENT_HANDLED_CONT;
 * }
 *
 * int handler2(void *arg)
 * {
 *       return UK_EVENT_HANDLED_CONT;
 * }
 *
 * int handler3(void *arg)
 * {
 *       return UK_EVENT_HANDLED;
 * }
 *
 * UK_EVENT_HANDLER_PRIO(event, handler1, UK_PRIO_EARLIEST);
 * UK_EVENT_HANDLER_PRIO(event, handler2, UK_PRIO_LATEST);
 * UK_EVENT_HANDLER_PRIO(event, handler3, UK_PRIO_LATEST);
 * ```
 */

#include <uk/essentials.h>
#include <uk/prio.h>
#include <uk/config.h>

#ifdef CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#endif /* CONFIG_LIBUKDEBUG */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LIBUKDEBUG
#define __uk_event_assert(x) UK_ASSERT(x)
#else
#define __uk_event_assert(x)	\
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

#define UK_EVENT_NOT_HANDLED	0  /* Event not handled. Try next handler. */
#define UK_EVENT_HANDLED	1  /* Event handled. Stop calling handlers. */
#define UK_EVENT_HANDLED_CONT	2  /* Event handled. Call next handler. */

struct uk_event {
	const uk_event_handler_t *hlist_end;
};

/* Internal macros to create per-event sections. The linker will sort these
 * sections and merge them in the .uk_eventtab section in the final binary. So
 * that we are able to arbitrarily name events (including numbers, underscores,
 * etc.) and still guarantee proper ordering in the presents of multiple events,
 * we use the tilde (~) to mark header and end sections as well as to separate
 * the event name from handler priority. The tilde has a large ASCII value (126)
 * and thus allows us to keep sections in order, irrespective of the chosen
 * event name (and conceptionally also the priority's textual representation).
 */
#define _UK_EVT_EVENT(event)		uk_event_ ## event
#define _UK_EVT_HLIST_END(event)	_uk_event_ ## event ## _end

#define __UK_EVT_SECTION_LABEL(section, label)				\
	__asm__ (							\
		".pushsection \".uk_event_" section "\", \"a\"\n"	\
		#label ":\n"						\
		".popsection\n"						\
	)

#define _UK_EVT_SECTION_LABEL(section, label)				\
	__UK_EVT_SECTION_LABEL(section, label)

#define _UK_EVT_SECTION_HLIST_END(event)				\
	_UK_EVT_SECTION_LABEL(#event "~~", _UK_EVT_HLIST_END(event))

#define _UK_EVT_SECTION_HEADER(event)					\
	extern const uk_event_handler_t _UK_EVT_HLIST_END(event)[];	\
	struct uk_event							\
	__used __section(".uk_event_" #event "~")			\
	_UK_EVT_EVENT(event) = {					\
		_UK_EVT_HLIST_END(event)				\
	}

#define _UK_EVT_IMPORT_EVENT(event)					\
	extern struct uk_event _UK_EVT_EVENT(event)

#define _UK_EVT_HLIST_START(e_ptr)					\
	((const uk_event_handler_t *)((e_ptr) + 1))

/**
 * Register an event. If the event should be raised from different libraries
 * export its symbol as 'uk_event_name'.
 *
 * @param name
 *   Name of the event. The name may contain any characters that can be used
 *   in C identifiers.
 */
#define UK_EVENT(name)							\
	_UK_EVT_SECTION_HEADER(name);					\
	_UK_EVT_SECTION_HLIST_END(name)

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
 *   Priority level (0 (earliest) to 9 (latest)). Use the UK_PRIO_AFTER()
 *   helper macro for computing priority dependencies. NOTE: There is no
 *   guarantee in the execution order of handlers of the same priority level
 */
#define _UK_EVENT_HANDLER_PRIO(event, fn, prio)				\
	static const uk_event_handler_t					\
	__used __section(".uk_event_" #event "~" #prio)			\
	_uk_event_ ## event ## _ ## prio ## _ ## fn = (fn)

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
	for ((itr) = _UK_EVT_HLIST_START(e_ptr);			\
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
	(&_UK_EVT_EVENT(name))

/**
 * Raise an event by pointer and invoke the handler chain until the first
 * handler successfully handled the event or returned a negative error code.
 *
 * @param e
 *   Pointer to the event to raise.
 * @param data
 *   Optional data supplied to the event handlers
 * @returns
 *   A negative error value if a handler returns one. Event processing
 *   immediately stops in this case. Otherwise:
 *   - UK_EVENT_HANDLED if a handler indicated that it successfully handled
 *     the event and event processing should stop with this handler.
 *   - UK_EVENT_HANDLED_CONT if at least one handler indicated that it
 *     successfully handled the event but event handling can continue, and no
 *     other handler returned UK_EVENT_HANDLED.
 *   - UK_EVENT_NOT_HANDLED if no handler handled the event.
 */
static inline int uk_raise_event_ptr(struct uk_event *e, void *data)
{
	const uk_event_handler_t *itr;
	int rc;
	int ret = UK_EVENT_NOT_HANDLED;

	uk_event_handler_foreach(itr, e) {
		__uk_event_assert(*itr);
		rc = ((*itr)(data));
		if (unlikely(rc < 0))
			return rc;

		if (rc == UK_EVENT_HANDLED)
			return UK_EVENT_HANDLED;

		if (rc == UK_EVENT_HANDLED_CONT)
			ret = UK_EVENT_HANDLED_CONT;
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
	({	_UK_EVT_IMPORT_EVENT(event);				\
		uk_raise_event_ptr(UK_EVENT_PTR(event), data);	})

#ifdef __cplusplus
}
#endif

#undef __uk_event_assert
#endif /* __UK_EVENT_H__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SIGNAL_H__
#error Do not include this header directly
#endif

#include <uk/bitops.h>

/* Standard Signals arm64
 *
 * SIGHUP	1	Term
 * SIGINT	2	Term
 * SIGQUIT	3	Core
 * SIGILL	4	Core
 * SIGTRAP	5	Core
 * SIGABRT	6	Core
 * SIGIOT	6	Core
 * SIGBUS	7	Core
 * SIGFPE	8	Core
 * SIGKILL	9	Term
 * SIGUSR1	10	Term
 * SIGSEGV	11	Core
 * SIGUSR2	12	Term
 * SIGPIPE	13	Term
 * SIGALRM	14	Term
 * SIGTERM	15	Term
 * SIGSTKFLT	16	Term
 * SIGCHLD	17	Ignore
 * SIGCONT	18	Cont
 * SIGSTOP	19	Stop
 * SIGTSTP	20	Stop
 * SIGTTIN	21	Stop
 * SIGTTOU	22	Stop
 * SIGURG	23	Ignore
 * SIGXCPU	24	Core
 * SIGXFSZ	25	Core
 * SIGVTALRM	26	Term
 * SIGPROF	27	Term
 * SIGWINCH	28	Ignore
 * SIGIO	29	Term
 * SIGPOLL	29	Term
 * SIGPWR	30	Term
 * SIGSYS	31	Core
 */
#define SIGACT_CONT_MASK						\
	UK_BIT(SIGCONT)

#define SIGACT_CORE_MASK						\
	(UK_BIT(SIGQUIT) | UK_BIT(SIGILL) | UK_BIT(SIGTRAP) |		\
	 UK_BIT(SIGABRT) | UK_BIT(SIGBUS) | UK_BIT(SIGFPE) |		\
	 UK_BIT(SIGSEGV) | UK_BIT(SIGXCPU) | UK_BIT(SIGXFSZ) |		\
	 UK_BIT(SIGSYS))

#define SIGACT_IGN_MASK							\
	(UK_BIT(SIGCHLD) | UK_BIT(SIGURG) | UK_BIT(SIGWINCH))

#define SIGACT_STOP_MASK						\
	(UK_BIT(SIGSTOP) | UK_BIT(SIGTSTP) | UK_BIT(SIGTTIN) |		\
	 UK_BIT(SIGTTOU))

#define SIGACT_TERM_MASK						\
	(UK_BIT(SIGHUP) | UK_BIT(SIGINT) | UK_BIT(SIGKILL) |		\
	 UK_BIT(SIGUSR1) | UK_BIT(SIGUSR2) | UK_BIT(SIGPIPE) |		\
	 UK_BIT(SIGALRM) | UK_BIT(SIGTERM) | UK_BIT(SIGSTKFLT) |	\
	 UK_BIT(SIGVTALRM) | UK_BIT(SIGPROF) | UK_BIT(SIGPOLL) |	\
	 UK_BIT(SIGPWR))


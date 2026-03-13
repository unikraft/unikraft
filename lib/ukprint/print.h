/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PRINT_INT_H__
#define __UK_PRINT_INT_H__

#include <uk/arch/time.h>
#include <uk/config.h>
#include <uk/essentials.h>

#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
#define UK_PRINT_MSG_ARGS_SRCNAME(_msg, _srcname, _srcline)	\
	(_msg).srcname = (_srcname);				\
	(_msg).srcline = (_srcline)
#else
#define UK_PRINT_MSG_ARGS_SRCNAME(_msg, _srcname, _srcline)
#endif /* CONFIG_LIBUKPRINT_PRINT_SRCNAME */

#if CONFIG_LIBUKPRINT_PRINT_CALLER
#define UK_PRINT_MSG_ARGS_CALLER(_msg)				\
	(_msg).retaddr = __return_addr(0);			\
	(_msg).frameaddr = __frame_addr(0)
#else
#define UK_PRINT_MSG_ARGS_CALLER(_msg)
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */

#define UK_PRINT_MSG_ALLOC(_msg, _flags, _libid, _srcname, _srcline,	\
			   _fmt, _ap)					\
	do {								\
		(_msg) = (struct uk_print_msg) {			\
			.flags = (_flags),				\
			.libid = (_libid),				\
			.fmt = (_fmt),					\
		};							\
		UK_PRINT_MSG_ARGS_SRCNAME((_msg), (_srcname),		\
					  (_srcline));			\
		UK_PRINT_MSG_ARGS_CALLER((_msg));			\
		va_copy((_msg).ap, (_ap));				\
	} while (0)

#define UK_PRINT_MSG_FREE(_msg)						\
		va_end((_msg).ap)					\

extern unsigned int uk_print_console_lvl;

struct uk_print_msg {
	int flags;
	__u16 libid;
#if CONFIG_LIBUKPRINT_PRINT_SRCNAME
	const char *srcname;
	unsigned int srcline;
#endif /* CONFIG_LIBUKPRINT_PRINT_SRCNAME */
#if CONFIG_LIBUKPRINT_PRINT_TIME
	__nsec timestamp;
#endif /* CONFIG_LIBUKPRINT_PRINT_TIME */
#if CONFIG_LIBUKPRINT_PRINT_THREAD
	const char *threadname; /* may be NULL */
#endif /* CONFIG_LIBUKPRINT_PRINT_THREAD */
#if CONFIG_LIBUKPRINT_PRINT_CALLER
	__uptr retaddr;
	__uptr frameaddr;
#endif /* CONFIG_LIBUKPRINT_PRINT_CALLER */
	const char *fmt;
	va_list ap;
#if CONFIG_LIBUKPRINT_LOGBUF
	__sz len;
	char msg[];
#endif /* CONFIG_LIBUKPRINT_LOGBUF */
};

#if CONFIG_LIBUKCONSOLE
void uk_print_console_write(struct uk_print_msg *msg);
__isr void uk_print_console_write_isr(struct uk_print_msg *msg);
#endif /* CONFIG_LIBUKCONSOLE */

#endif /* __UK_PRINT_INT_H__ */

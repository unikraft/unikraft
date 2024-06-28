/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#define _GNU_SOURCE
#include <uk/config.h>

#include <string.h>
#include <fcntl.h>

#include <sys/stat.h>
#ifdef CONFIG_LIBPOSIX_SYSINFO
#include <sys/utsname.h>
#endif /* CONFIG_LIBPOSIX_SYSINFO */

#include <uk/arch/types.h>
#include <uk/plat/console.h> /* ANSI definitions */
#include <uk/arch/limits.h>
#include <uk/syscall.h>
#include <uk/errptr.h>
#include <uk/streambuf.h>
#include <uk/assert.h>
#include <uk/essentials.h>

/* Syntax highlighting */
#define __SHCC_ANSI_SYSCALL  UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
#define __SHCC_ANSI_CHARP    UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_GREEN)
#define __SHCC_ANSI_VALUE    UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_CYAN)
#define __SHCC_ANSI_MACRO    UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_YELLOW)
#define __SHCC_ANSI_FLAGS    UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_YELLOW)
#define __SHCC_ANSI_TYPE     __SHCC_ANSI_RESET /* default color */
#define __SHCC_ANSI_PROPERTY __SHCC_ANSI_RESET /* default color */
#define __SHCC_ANSI_COMMENT  UK_ANSI_MOD_BOLD \
			     UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLACK)
#define __SHCC_ANSI_ERROR    UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define __SHCC_ANSI_OK       __SHCC_ANSI_RESET /* default color */
#define __SHCC_ANSI_RESET    UK_ANSI_MOD_RESET

#define uk_streambuf_shcc(sb, fmtf, typename)				\
	do {								\
		if ((fmtf) & UK_PRSYSCALL_FMTF_ANSICOLOR)		\
			uk_streambuf_strcpy((sb), __SHCC_ANSI_ ## typename); \
	} while (0)

/* Flag decoding */
#define PR_FLAG(sb, fmtf, orig_seek, prefix, flagname, flags)		\
	do {								\
		if ((flags) & (prefix##flagname)) {			\
			if (uk_streambuf_seek((sb)) > (orig_seek))	\
				uk_streambuf_strcpy((sb), "|");		\
			uk_streambuf_shcc((sb), (fmtf), FLAGS);		\
			uk_streambuf_strcpy((sb), STRINGIFY(prefix)	\
						  STRINGIFY(flagname));	\
			uk_streambuf_shcc((sb), (fmtf), RESET);		\
			(flags) &= ~((typeof(flags)) (prefix##flagname)); \
		}							\
	} while (0)
#define PR_FLAG_END(sb, fmtf, orig_seek, flags)				\
	do {								\
		if (flags != 0) {					\
			/* unknown flags */				\
			if (uk_streambuf_seek((sb)) > (orig_seek))	\
				uk_streambuf_strcpy((sb), "|");		\
			uk_streambuf_shcc((sb), (fmtf), FLAGS);		\
			uk_streambuf_printf((sb), "0x%x", (flags));	\
			uk_streambuf_shcc((sb), (fmtf), RESET);		\
		} else if (uk_streambuf_seek((sb)) == (orig_seek)) {	\
			uk_streambuf_shcc((sb), (fmtf), VALUE);		\
			uk_streambuf_strcpy((sb), "0x0");		\
			uk_streambuf_shcc((sb), (fmtf), RESET);		\
		}							\
	} while (0)

/* Type decoding */
#define PR_TYPE(sb, fmtf, prefix, typename)				\
	case prefix##typename:						\
		uk_streambuf_shcc((sb), (fmtf), MACRO);			\
		uk_streambuf_strcpy((sb), STRINGIFY(prefix)		\
					 STRINGIFY(typename));		\
		uk_streambuf_shcc((sb), (fmtf), RESET);			\
		break;
#define PR_TYPE_DEFAULT(sb, fmtf, prefix, arg)				\
	default:							\
		uk_streambuf_shcc((sb), (fmtf), MACRO);			\
		uk_streambuf_printf((sb), STRINGIFY(prefix)		\
					 "0x%lx", (long) arg);		\
		uk_streambuf_shcc((sb), (fmtf), RESET);			\
		break;

#ifdef CONFIG_LIBSYSCALL_SHIM_STRACE_PRINT_TYPE
#define PR_PARAM_TYPE(sb, fmtf, type_prefix)				\
	do {								\
		uk_streambuf_shcc((sb), (fmtf), TYPE);			\
		uk_streambuf_strcpy((sb), type_prefix);			\
		uk_streambuf_shcc((sb), (fmtf), RESET);			\
		uk_streambuf_strcpy((sb), ":");				\
	} while (0)
#else
#define PR_PARAM_TYPE(sb, fmtf, type_prefix) do { } while (0)
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_PRINT_TYPE */

/* Helper to generate a typed value string */
#define PR_PARAM(sb, fmtf, type_prefix, ...)				\
	do {								\
		PR_PARAM_TYPE((sb), (fmtf), type_prefix);		\
		uk_streambuf_shcc((sb), (fmtf), VALUE);			\
		uk_streambuf_printf((sb), __VA_ARGS__);			\
		uk_streambuf_shcc((sb), (fmtf), RESET);			\
	} while (0)

/* Helper to generate a comment */
#define PR_COMMENT(sb, fmtf, ...)					\
	do {								\
		uk_streambuf_shcc((sb), (fmtf), COMMENT);		\
		uk_streambuf_strcpy((sb), "/* ");			\
		uk_streambuf_printf((sb), __VA_ARGS__);			\
		uk_streambuf_strcpy((sb), "*/");			\
		uk_streambuf_shcc((sb), (fmtf), RESET);			\
	} while (0)

/* Helper macros for expanding code depending on variadic arguments */
#define __VARG_EXPAND_IDX77 0
#define __VARG_EXPAND_IDX76 1
#define __VARG_EXPAND_IDX75 2
#define __VARG_EXPAND_IDX74 3
#define __VARG_EXPAND_IDX73 4
#define __VARG_EXPAND_IDX72 5
#define __VARG_EXPAND_IDX71 6
#define __VARG_EXPAND_IDX66 0
#define __VARG_EXPAND_IDX65 1
#define __VARG_EXPAND_IDX64 2
#define __VARG_EXPAND_IDX63 3
#define __VARG_EXPAND_IDX62 4
#define __VARG_EXPAND_IDX61 5
#define __VARG_EXPAND_IDX55 0
#define __VARG_EXPAND_IDX54 1
#define __VARG_EXPAND_IDX53 2
#define __VARG_EXPAND_IDX52 3
#define __VARG_EXPAND_IDX51 4
#define __VARG_EXPAND_IDX44 0
#define __VARG_EXPAND_IDX43 1
#define __VARG_EXPAND_IDX42 2
#define __VARG_EXPAND_IDX41 3
#define __VARG_EXPAND_IDX33 0
#define __VARG_EXPAND_IDX32 1
#define __VARG_EXPAND_IDX31 2
#define __VARG_EXPAND_IDX22 0
#define __VARG_EXPAND_IDX21 1
#define __VARG_EXPAND_IDX11 0
#define __VARG_EXPAND_IDX(x, y) UK_CONCAT(UK_CONCAT(__VARG_EXPAND_IDX, x), y)

#define __VARG_EXPAND0(x, ...)
#define __VARG_EXPAND1(x, m, earg, succ, type)				\
	m(__VARG_EXPAND_IDX(x, 1), earg, succ, type)
#define __VARG_EXPAND2(x, m, earg, succ, type, ...)			\
	m(__VARG_EXPAND_IDX(x, 2), earg, succ, type)			\
	__VARG_EXPAND1(x, m, earg, succ, __VA_ARGS__)
#define __VARG_EXPAND3(x, m, earg, succ, type, ...)			\
	m(__VARG_EXPAND_IDX(x, 3), earg, succ, type)			\
	__VARG_EXPAND2(x, m, earg, succ, __VA_ARGS__)
#define __VARG_EXPAND4(x, m, earg, succ, type, ...)			\
	m(__VARG_EXPAND_IDX(x, 4), earg, succ, type)			\
	__VARG_EXPAND3(x, m, earg, succ, __VA_ARGS__)
#define __VARG_EXPAND5(x, m, earg, succ, type, ...)			\
	m(__VARG_EXPAND_IDX(x, 5), earg, succ, type)			\
	__VARG_EXPAND4(x, m, earg, succ, __VA_ARGS__)
#define __VARG_EXPAND6(x, m, earg, succ, type, ...)			\
	m(__VARG_EXPAND_IDX(x, 6), earg, succ, type)			\
	__VARG_EXPAND5(x, m, earg, succ, __VA_ARGS__)
#define __VARG_EXPAND7(x, m, earg, succ, type, ...)			\
	m(__VARG_EXPAND_IDX(x, 7), earg, succ, type)			\
	__VARG_EXPAND6(x, m, earg, succ, __VA_ARGS__)
#define __VARG_EXPANDx(nr_args, ...)					\
	UK_CONCAT(__VARG_EXPAND, nr_args)(nr_args, __VA_ARGS__)
#define _VARG_EXPAND(...) __VARG_EXPANDx(__VA_ARGS__)
#define VARG_EXPAND(m, earg, succ, ...); \
	_VARG_EXPAND(UK_NARGS(__VA_ARGS__), m, earg, succ, __VA_ARGS__)

/* Like VARG_EXPAND() but takes two variadic arguments per expansion step  */
#define __VARG2_EXPAND_IDX1414 0
#define __VARG2_EXPAND_IDX1412 1
#define __VARG2_EXPAND_IDX1410 2
#define __VARG2_EXPAND_IDX148  3
#define __VARG2_EXPAND_IDX146  4
#define __VARG2_EXPAND_IDX144  5
#define __VARG2_EXPAND_IDX142  6
#define __VARG2_EXPAND_IDX1212 0
#define __VARG2_EXPAND_IDX1210 1
#define __VARG2_EXPAND_IDX128  2
#define __VARG2_EXPAND_IDX126  3
#define __VARG2_EXPAND_IDX124  4
#define __VARG2_EXPAND_IDX122  5
#define __VARG2_EXPAND_IDX1010 0
#define __VARG2_EXPAND_IDX108  1
#define __VARG2_EXPAND_IDX106  2
#define __VARG2_EXPAND_IDX104  3
#define __VARG2_EXPAND_IDX102  4
#define __VARG2_EXPAND_IDX88   0
#define __VARG2_EXPAND_IDX86   1
#define __VARG2_EXPAND_IDX84   2
#define __VARG2_EXPAND_IDX82   3
#define __VARG2_EXPAND_IDX66   0
#define __VARG2_EXPAND_IDX64   1
#define __VARG2_EXPAND_IDX62   2
#define __VARG2_EXPAND_IDX44   0
#define __VARG2_EXPAND_IDX42   1
#define __VARG2_EXPAND_IDX22   0
#define __VARG2_EXPAND_IDX(x, y) UK_CONCAT(UK_CONCAT(__VARG2_EXPAND_IDX, x), y)

#define  __VARG2_EXPAND0(x, ...)
#define  __VARG2_EXPAND2(x, m, earg, succ, type, param)			\
	m(__VARG2_EXPAND_IDX(x, 2), earg, succ, type, param)
#define  __VARG2_EXPAND4(x, m, earg, succ, type, param, ...)		\
	m(__VARG2_EXPAND_IDX(x, 4), earg, succ, type, param)		\
	__VARG2_EXPAND2(x, m, earg, succ, __VA_ARGS__)
#define  __VARG2_EXPAND6(x, m, earg, succ, type, param, ...)		\
	m(__VARG2_EXPAND_IDX(x, 6), earg, succ, type, param)		\
	__VARG2_EXPAND4(x, m, earg, succ, __VA_ARGS__)
#define  __VARG2_EXPAND8(x, m, earg, succ, type, param, ...)		\
	m(__VARG2_EXPAND_IDX(x, 8), earg, succ, type, param)		\
	__VARG2_EXPAND6(x, m, earg, succ, __VA_ARGS__)
#define __VARG2_EXPAND10(x, m, earg, succ, type, param, ...)		\
	m(__VARG2_EXPAND_IDX(x, 10), earg, succ, type, param)		\
	__VARG2_EXPAND8(x, m, earg, succ, __VA_ARGS__)
#define __VARG2_EXPAND12(x, m, earg, succ, type, param, ...)		\
	m(__VARG2_EXPAND_IDX(x, 12), earg, succ, type, param)		\
	__VARG2_EXPAND10(x, m, earg, succ, __VA_ARGS__)
#define __VARG2_EXPAND14(x, m, earg, succ, type, param, ...)		\
	m(__VARG2_EXPAND_IDX(x, 14), earg, succ, type, param)		\
	__VARG2_EXPAND12(x, m, earg, succ, __VA_ARGS__)
#define __VARG2_EXPANDx(nr_args, ...)					\
	UK_CONCAT(__VARG2_EXPAND, nr_args)(nr_args, __VA_ARGS__)
#define _VARG2_EXPAND(...) __VARG2_EXPANDx(__VA_ARGS__)
#define VARG2_EXPAND(m, earg, succ, ...); \
	_VARG2_EXPAND(UK_NARGS(__VA_ARGS__), m, earg, succ, __VA_ARGS__)

/* Helper for defining structs */
#define PT_STRUCT(name) PT_STRUCT_ ## name

/* Helpers for pretty printing structs.
 * For structs as well as strings we expect to get a pointer per default.
 * However, we might also have embedded values. To be able to pass these as
 * long param, we simply always pass pointers to struct members and usually
 * just dereference with PT_REF again.
 */
#define __PR_STRUCT_FIELD_PRINT(i, earg, succ, type, arg)		\
	do {								\
		if ((i) != 0)						\
			uk_streambuf_strcpy((earg).sb, ", ");		\
		uk_streambuf_strcpy((earg).sb, STRINGIFY(arg) "=");	\
		pr_param((earg).sb, fmtf,				\
			 ((type) & PT_SVAL) ? (type) : (type)		\
				| PT_REF  /* Dereference */		\
				| PT_SVAL /* Suppress <ref>-text */,	\
			 (long) &(earg).s->arg,				\
			 succ);						\
	} while (0);

#define PR_STRUCT(sb, fmtf, name, flags, param, more, succ, ...)	\
	do {								\
		struct {						\
			struct name *s;					\
			struct uk_streambuf *sb;			\
		} s_##name = {						\
			(struct name *)(param),				\
			(sb),						\
		};							\
		PR_PARAM_TYPE((sb), (fmtf), STRINGIFY(name));		\
		if ((param) == 0) {					\
			uk_streambuf_shcc((sb), (fmtf), MACRO);		\
			uk_streambuf_strcpy((sb), "NULL");		\
			uk_streambuf_shcc((sb), (fmtf), RESET);		\
		} else if (!((flags) & PT_OUT) || succ)	{		\
			uk_streambuf_strcpy((sb), "{");			\
			VARG2_EXPAND(__PR_STRUCT_FIELD_PRINT, s_##name,	\
				     succ, __VA_ARGS__);		\
			if (more)					\
				uk_streambuf_strcpy((sb), ", ...");	\
			uk_streambuf_strcpy((sb), "}");			\
		} else {						\
			uk_streambuf_shcc((sb), (fmtf), VALUE);		\
			uk_streambuf_printf((sb), "0x%lx",		\
				 (unsigned long) (param));		\
			uk_streambuf_shcc((sb), (fmtf), RESET);		\
		}							\
	} while (0)

/*
 * Parameter types (`PT_*`)
 *
 * Layout of `enum param_type`:
 * We limit the use of enum param_type to 32-bits, this way we reserve a range
 * for parameters and flags and can keep this implementation architecture width
 * independent.
 * Additional parameter flags, can be OR'ed to param type.
 *
 *   16-bits        4-bits     12-bits
 * +--------------+----------+-----------------+
 * |    pt_arg    | pt_flags |     pt_type     |
 * +--------------+----------+-----------------+
 */
#define PT_FLAGMASK 0xF000
#define PT_TYPEMASK 0x0FFF
#define PT_ARGSHIFT 16

/* Flag definition */
#define PT_REF      0x1000 /* reference to type
			    * NOTE: If type is already a reference,
			    * specifying PT_REF causes a double-ref
			    */
#define PT_OUT      0x2000 /* output parameter */
#define PT_SVAL     0x4000 /* Parameter is embedded in a struct of a type
			    * which per-default is expected to be a pointer
			    * (e.g., an PT_STRUCT or PT_CHARP).
			    */

/*
 * Type definition (because of the binary representation,
 * at most 16384 different types are supported)
 */
enum param_type {
	PT_UNKNOWN = 0x0,
	PT_BOOL, /* Boolean: true, false */
	PT_DEC, /* Signed decimal */
	PT_UDEC, /* Unsigned decimal */
	PT_HEX, /* Hexadecimal */
	PT_OCTAL, /* Octal */
	PT_CHARP, /* Reference to '\0' terminated string (char *) */
	_PT_BUFP, /* Dump of reference to memory buffer, use PT_BUFP(len) */
	PT_VADDR, /* Virtual address */
	PT_FD, /* File descriptor number */
	PT_DIRFD, /* File descriptor number of directory */
	PT_PID, /* PID number */
	PT_OFLAGS,
	PT_OKFLAGS,
	PT_PROTFLAGS,
	PT_MAPFLAGS,
	PT_FUTEXOP,
	PT_CLOCKID,
	PT_SOCKETAF,
	PT_SOCKETTYPE,
	PT_MSGFLAGS,
	PT_CLONEFLAGS,
	PT_STRUCT(timespec),
	PT_STRUCT(stat),
#ifdef CONFIG_LIBPOSIX_SYSINFO
	PT_STRUCT(utsname),
#endif /* CONFIG_LIBPOSIX_SYSINFO */
};
#define PT_BUFP(len)							\
	(long)(_PT_BUFP | ((MIN((unsigned long) __U16_MAX,		\
				(unsigned long) len)) << PT_ARGSHIFT))

/* Maximum number of PT_BUFP bytes to actually print */
#define __PR_BUFP_MAXLEN 24

/* Aliases */
#define PT_STATUS PT_BOOL
#define PT_PATH PT_CHARP
#define PT_TID PT_PID

/*
 * Individual parameter type formats
 */
static inline void param_dirfd(struct uk_streambuf *sb, int fmtf, int dirfd)
{
	if (dirfd == -100) {
		uk_streambuf_shcc(sb, fmtf, MACRO);
		uk_streambuf_strcpy(sb, "AT_FDCWD");
		uk_streambuf_shcc(sb, fmtf, RESET);
	} else {
		PR_PARAM(sb, fmtf, "dirfd", "%d", dirfd);

		/* TODO: Print path as comment */
	}
}

static inline void param_fd(struct uk_streambuf *sb, int fmtf, int fd)
{
	PR_PARAM(sb, fmtf, "fd", "%d", fd);

	/* TODO: Print file constructor/path (socket/file) as comment */
}

static inline void param_pid(struct uk_streambuf *sb, int fmtf, int pid)
{
	PR_PARAM(sb, fmtf, "pid", "%d", pid);

	/* TODO: PID of corresponding process group as comment */
}

static inline void param_oflags(struct uk_streambuf *sb, int fmtf, int oflags)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	uk_streambuf_shcc(sb, fmtf, FLAGS);
	uk_streambuf_strcpy(sb, "O_RDONLY");
	uk_streambuf_shcc(sb, fmtf, RESET);

	PR_FLAG(sb, fmtf, orig_seek, O_, APPEND,    oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, ASYNC,     oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, CLOEXEC,   oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, CREAT,     oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, DIRECT,    oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, DIRECTORY, oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, DSYNC,     oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, EXCL,      oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, LARGEFILE, oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, NOATIME,   oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, NOCTTY,    oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, NOFOLLOW,  oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, NONBLOCK,  oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, PATH,      oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, SYNC,      oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, TMPFILE,   oflags);
	PR_FLAG(sb, fmtf, orig_seek, O_, TRUNC,     oflags);
	PR_FLAG_END(sb, fmtf, orig_seek, oflags);
}

static inline void param_okflag(struct uk_streambuf *sb, int fmtf, int okflags)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	if (okflags == 0) {
		uk_streambuf_shcc(sb, fmtf, FLAGS);
		uk_streambuf_strcpy(sb, "F_OK");
		uk_streambuf_shcc(sb, fmtf, RESET);
		return;
	}
	PR_FLAG(sb, fmtf, orig_seek, R_, OK, okflags);
	PR_FLAG(sb, fmtf, orig_seek, W_, OK, okflags);
	PR_FLAG(sb, fmtf, orig_seek, X_, OK, okflags);
	PR_FLAG_END(sb, fmtf, orig_seek, okflags);
}

#if CONFIG_LIBPOSIX_MMAP || CONFIG_LIBUKMMAP
#include <sys/mman.h>

static inline void param_protflags(struct uk_streambuf *sb, int fmtf,
				   int protflags)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	if (protflags == 0) {
		uk_streambuf_shcc(sb, fmtf, FLAGS);
		uk_streambuf_strcpy(sb, "PROT_NONE");
		uk_streambuf_shcc(sb, fmtf, RESET);
		return;
	}
	PR_FLAG(sb, fmtf, orig_seek, PROT_, EXEC,  protflags);
	PR_FLAG(sb, fmtf, orig_seek, PROT_, READ,  protflags);
	PR_FLAG(sb, fmtf, orig_seek, PROT_, WRITE, protflags);
	PR_FLAG_END(sb, fmtf, orig_seek, protflags);
}

static inline void param_mapflags(struct uk_streambuf *sb, int fmtf,
				  int mapflags)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	if (mapflags == 0) {
		uk_streambuf_shcc(sb, fmtf, FLAGS);
		uk_streambuf_strcpy(sb, "MAP_NONE");
		uk_streambuf_shcc(sb, fmtf, RESET);
		return;
	}
	PR_FLAG(sb, fmtf, orig_seek, MAP_, SHARED,     mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, PRIVATE,    mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, ANONYMOUS,  mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, DENYWRITE,  mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, EXECUTABLE, mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, FILE,       mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, FIXED,      mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, GROWSDOWN,  mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, HUGETLB,    mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, LOCKED,     mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, NONBLOCK,   mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, POPULATE,   mapflags);
	PR_FLAG(sb, fmtf, orig_seek, MAP_, STACK,      mapflags);
	PR_FLAG_END(sb, fmtf, orig_seek, mapflags);
}
#endif /* CONFIG_LIBPOSIX_MMAP || CONFIG_LIBUKMMAP */

#if CONFIG_LIBPOSIX_FUTEX
#include <linux/futex.h>

static inline void param_futexop(struct uk_streambuf *sb, int fmtf, int op)
{
	__sz orig_seek = uk_streambuf_seek(sb);
	int flags;

	flags = op & ~(FUTEX_CMD_MASK);
	op &= FUTEX_CMD_MASK;

	switch (op) {
		PR_TYPE(sb, fmtf, FUTEX_, WAIT);
		PR_TYPE(sb, fmtf, FUTEX_, WAKE);
		PR_TYPE(sb, fmtf, FUTEX_, FD);
		PR_TYPE(sb, fmtf, FUTEX_, REQUEUE);
		PR_TYPE(sb, fmtf, FUTEX_, CMP_REQUEUE);
		PR_TYPE(sb, fmtf, FUTEX_, WAKE_OP);
		PR_TYPE(sb, fmtf, FUTEX_, LOCK_PI);
		PR_TYPE(sb, fmtf, FUTEX_, UNLOCK_PI);
		PR_TYPE(sb, fmtf, FUTEX_, TRYLOCK_PI);
		PR_TYPE(sb, fmtf, FUTEX_, WAIT_BITSET);
		PR_TYPE(sb, fmtf, FUTEX_, WAKE_BITSET);
		PR_TYPE(sb, fmtf, FUTEX_, WAIT_REQUEUE_PI);
		PR_TYPE(sb, fmtf, FUTEX_, CMP_REQUEUE_PI);
		PR_TYPE(sb, fmtf, FUTEX_, LOCK_PI2);
		PR_TYPE_DEFAULT(sb, fmtf, FUTEX_, op);
	}

	if (flags) {
		PR_FLAG(sb, fmtf, orig_seek, FUTEX_, PRIVATE_FLAG, flags);
		PR_FLAG(sb, fmtf, orig_seek, FUTEX_, CLOCK_REALTIME, flags);
		PR_FLAG_END(sb, fmtf, orig_seek, flags);
	}
}
#endif /* CONFIG_POSIX_FUTEX */

#include <time.h>

static inline void param_clockid(struct uk_streambuf *sb, int fmtf, int clockid)
{
	switch (clockid) {
		PR_TYPE(sb, fmtf, CLOCK_, REALTIME);
		PR_TYPE(sb, fmtf, CLOCK_, MONOTONIC);
		PR_TYPE(sb, fmtf, CLOCK_, PROCESS_CPUTIME_ID);
		PR_TYPE(sb, fmtf, CLOCK_, THREAD_CPUTIME_ID);
		PR_TYPE_DEFAULT(sb, fmtf, CLOCK_, clockid);
	}
}

#if CONFIG_LIBPOSIX_SOCKET
#include <sys/socket.h>

static inline void param_socketaf(struct uk_streambuf *sb, int fmtf, int domain)
{
	switch (domain) {
		PR_TYPE(sb, fmtf, AF_, UNIX); /* LOCAL, FILE */
		PR_TYPE(sb, fmtf, AF_, INET);
		PR_TYPE(sb, fmtf, AF_, INET6);
		PR_TYPE(sb, fmtf, AF_, NETLINK);
		PR_TYPE_DEFAULT(sb, fmtf, AF_, domain);
	}
}

static inline void param_sockettype(struct uk_streambuf *sb, int fmtf, int type)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	PR_FLAG(sb, fmtf, orig_seek, SOCK_, NONBLOCK, type);
	PR_FLAG(sb, fmtf, orig_seek, SOCK_, CLOEXEC, type);
	if (uk_streambuf_seek(sb) > (orig_seek))
		uk_streambuf_strcpy(sb, "|");
	switch (type) {
		PR_TYPE(sb, fmtf, SOCK_, STREAM);
		PR_TYPE(sb, fmtf, SOCK_, DGRAM);
		PR_TYPE(sb, fmtf, SOCK_, RAW);
		PR_TYPE(sb, fmtf, SOCK_, SEQPACKET);
		PR_TYPE(sb, fmtf, SOCK_, RDM);
		PR_TYPE_DEFAULT(sb, fmtf, SOCK_, type);
	}
}

static inline void param_msgflags(struct uk_streambuf *sb, int fmtf, int flags)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	PR_FLAG(sb, fmtf, orig_seek, MSG_, CONFIRM, flags);
	PR_FLAG(sb, fmtf, orig_seek, MSG_, DONTROUTE, flags);
	PR_FLAG(sb, fmtf, orig_seek, MSG_, DONTWAIT, flags);
	PR_FLAG(sb, fmtf, orig_seek, MSG_, EOR, flags);
	PR_FLAG(sb, fmtf, orig_seek, MSG_, MORE, flags);
	PR_FLAG(sb, fmtf, orig_seek, MSG_, NOSIGNAL, flags);
	PR_FLAG(sb, fmtf, orig_seek, MSG_, OOB, flags);
	PR_FLAG_END(sb, fmtf, orig_seek, flags);
}
#endif /* CONFIG_LIBPOSIX_SOCKET */

#if CONFIG_LIBPOSIX_PROCESS_CLONE
#include <uk/process.h>

static inline void param_cloneflags(struct uk_streambuf *sb, int fmtf,
				    int flags)
{
	__sz orig_seek = uk_streambuf_seek(sb);

	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWTIME,        flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, VM,             flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, FS,             flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, FILES,          flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, SIGHAND,        flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, PIDFD,          flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, PTRACE,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, VFORK,          flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, PARENT,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, THREAD,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWNS,          flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, SYSVSEM,        flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, SETTLS,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, PARENT_SETTID,  flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, CHILD_CLEARTID, flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, DETACHED,       flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, UNTRACED,       flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, CHILD_SETTID,   flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWCGROUP,      flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWUTS,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWIPC,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWUSER,        flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWPID,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, NEWNET,         flags);
	PR_FLAG(sb, fmtf, orig_seek, CLONE_, IO,             flags);
	PR_FLAG_END(sb, fmtf, orig_seek, flags);
}
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */

/* Pretty print a single parameter */
static void pr_param(struct uk_streambuf *sb, int fmtf,
		     enum param_type type, long param, int succ)
{
	unsigned int arg;
	int flags;

	UK_ASSERT(sb);
	UK_ASSERT(uk_streambuf_buf(sb));

	/* Mask out flags */
	flags = ((int) type) & PT_FLAGMASK;
	arg   = (unsigned int) type >> PT_ARGSHIFT;
	type &= PT_TYPEMASK;

	/* Indicate that this is an output parameter */
	if (flags & PT_OUT) {
		uk_streambuf_shcc(sb, fmtf, PROPERTY);
		uk_streambuf_strcpy(sb, "<out>");
		uk_streambuf_shcc(sb, fmtf, RESET);
	}

	/* Print reference as prefix for referenced parameters */
	if (flags & PT_REF) {
		if (!param) {
			/* Parameter reference is NULL, do not dereference */
			uk_streambuf_shcc(sb, fmtf, MACRO);
			uk_streambuf_strcpy(sb, "NULL");
			uk_streambuf_shcc(sb, fmtf, RESET);

			goto out; /* Don't print the actual param */
		} else {
			if (!(flags & PT_SVAL)) {
				uk_streambuf_shcc(sb, fmtf, PROPERTY);
				uk_streambuf_printf(sb, "<ref:0x%lx>",
						    (unsigned long) param);
				uk_streambuf_shcc(sb, fmtf, RESET);
			}

			/* De-reference parameter if not NULL */
			param = *((long *) param);
		}
	}

	switch (type) {
	case PT_BOOL:
		uk_streambuf_shcc(sb, fmtf, VALUE);
		if (param)
			uk_streambuf_strcpy(sb, "true");
		else
			uk_streambuf_strcpy(sb, "false");
		uk_streambuf_shcc(sb, fmtf, RESET);
		break;
	case PT_UDEC:
		uk_streambuf_shcc(sb, fmtf, VALUE);
		uk_streambuf_printf(sb, "%lu", (unsigned long) param);
		uk_streambuf_shcc(sb, fmtf, RESET);
		break;
	case PT_DEC:
		uk_streambuf_shcc(sb, fmtf, VALUE);
		uk_streambuf_printf(sb, "%ld", (long) param);
		uk_streambuf_shcc(sb, fmtf, RESET);
		break;
	case PT_HEX:
		uk_streambuf_shcc(sb, fmtf, VALUE);
		uk_streambuf_printf(sb, "0x%lx", (unsigned long) param);
		uk_streambuf_shcc(sb, fmtf, RESET);
		break;
	case PT_OCTAL:
		uk_streambuf_shcc(sb, fmtf, VALUE);
		uk_streambuf_printf(sb, "0%lo", (unsigned long) param);
		uk_streambuf_shcc(sb, fmtf, RESET);
		break;
	case PT_VADDR:
		if (!param) {
			uk_streambuf_shcc(sb, fmtf, MACRO);
			uk_streambuf_strcpy(sb, "NULL");
			uk_streambuf_shcc(sb, fmtf, RESET);
		} else {
			PR_PARAM(sb, fmtf, "va", "0x%lx",
					(unsigned long) param);
		}
		break;
	case PT_CHARP:
		if (!param) {
			uk_streambuf_shcc(sb, fmtf, MACRO);
			uk_streambuf_strcpy(sb, "NULL");
			uk_streambuf_shcc(sb, fmtf, RESET);
		} else if (!(flags & PT_OUT) || succ) {
			uk_streambuf_shcc(sb, fmtf, CHARP);
			uk_streambuf_printf(sb, "\"%s\"", (const char *) param);
			uk_streambuf_shcc(sb, fmtf, RESET);
		} else {
			PR_PARAM(sb, fmtf, "charp", "0x%lx",
				 (unsigned long) param);
		}
		break;
	case _PT_BUFP:
		if (!param) {
			uk_streambuf_shcc(sb, fmtf, MACRO);
			uk_streambuf_strcpy(sb, "NULL");
			uk_streambuf_shcc(sb, fmtf, RESET);
		} else if (!(flags & PT_OUT) || succ) {
			unsigned int left;
			char *c;

			uk_streambuf_shcc(sb, fmtf, CHARP);
			uk_streambuf_strcpy(sb, "\"");
			for (c = (char *) param,
				left = MIN((int) arg, __PR_BUFP_MAXLEN);
				left > 0;
				left--, c++) {
				if (*c >= ' ' && *c <= '~')
					uk_streambuf_printf(sb, "%c", *c);
				else
					uk_streambuf_printf(sb, "\\x%02X",
							    (int) ((__u8) *c));
			}
			uk_streambuf_strcpy(sb, "\"");
			uk_streambuf_shcc(sb, fmtf, RESET);
			if (arg > __PR_BUFP_MAXLEN)
				uk_streambuf_strcpy(sb, "...");
		} else {
			PR_PARAM(sb, fmtf, "buf", "0x%lx",
				 (unsigned long) param);
		}
		break;
	case PT_FD:
		param_fd(sb, fmtf, param);
		break;
	case PT_DIRFD:
		param_dirfd(sb, fmtf, param);
		break;
	case PT_PID:
		param_pid(sb, fmtf, param);
		break;
	case PT_OFLAGS:
		param_oflags(sb, fmtf, param);
		break;
	case PT_OKFLAGS:
		param_okflag(sb, fmtf, param);
		break;
#if CONFIG_LIBPOSIX_MMAP || CONFIG_LIBUKMMAP
	case PT_PROTFLAGS:
		param_protflags(sb, fmtf, param);
		break;
	case PT_MAPFLAGS:
		param_mapflags(sb, fmtf, param);
		break;
#endif /* CONFIG_LIBPOSIX_MMAP || CONFIG_LIBUKMMAP */
#if CONFIG_LIBPOSIX_FUTEX
	case PT_FUTEXOP:
		param_futexop(sb, fmtf, param);
		break;
#endif /* CONFIG_POSIX_FUTEX */
	case PT_CLOCKID:
		param_clockid(sb, fmtf, param);
		break;
#if CONFIG_LIBPOSIX_SOCKET
	case PT_SOCKETAF:
		param_socketaf(sb, fmtf, param);
		break;
	case PT_SOCKETTYPE:
		param_sockettype(sb, fmtf, param);
		break;
	case PT_MSGFLAGS:
		param_msgflags(sb, fmtf, param);
		break;
#endif /* CONFIG_LIBPOSIX_SOCKET */
#if CONFIG_LIBPOSIX_PROCESS_CLONE
	case PT_CLONEFLAGS:
		param_cloneflags(sb, fmtf, param);
		break;
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
	case PT_STRUCT(timespec):
		PR_STRUCT(sb, fmtf, timespec, flags, param, 0, succ,
			  PT_UDEC, tv_sec,
			  PT_UDEC, tv_nsec);
		break;
	case PT_STRUCT(stat):
		PR_STRUCT(sb, fmtf, stat, flags, param, 1, succ,
			  PT_UDEC, st_size,
			  PT_OCTAL, st_mode);
		break;
#ifdef CONFIG_LIBPOSIX_SYSINFO
	case PT_STRUCT(utsname):
		PR_STRUCT(sb, fmtf, utsname, flags, param, 1, succ,
			  PT_CHARP | PT_SVAL, sysname,
			  PT_CHARP | PT_SVAL, nodename);
		break;
#endif /* CONFIG_LIBPOSIX_SYSINFO */
	default:
		uk_streambuf_shcc(sb, fmtf, VALUE);
		uk_streambuf_printf(sb, "0x%lx", (unsigned long) param);
		uk_streambuf_shcc(sb, fmtf, RESET);
		break;
	}

out:
	return;
}

/* Pretty print a system call return code */
static void pr_retcode(struct uk_streambuf *sb, int fmtf,
		       enum param_type type, long retval)
{
	UK_ASSERT(sb);
	UK_ASSERT(uk_streambuf_buf(sb));

	if (PTRISERR((void *) retval) && (retval != 0)) {
		uk_streambuf_shcc(sb, fmtf, ERROR);
		uk_streambuf_printf(sb, "%s (%ld)", strerror((int) -retval),
				    retval);
		uk_streambuf_shcc(sb, fmtf, RESET);
	} else if (type == PT_BOOL) {
		/* Replace boolean with OK (here, errors are already catched) */
		uk_streambuf_shcc(sb, fmtf, OK);
		uk_streambuf_strcpy(sb, "OK");
		uk_streambuf_shcc(sb, fmtf, RESET);
	} else {
		/* Use same formatting as for parameters */
		pr_param(sb, fmtf, type, retval, 1);
	}
}

/*
 * Helper macros for pretty print a system call request (PR_SYSCALL,
 * VPR_SYSCALL) and to pretty print the return code of a system call
 * (PR_SYSRET).
 */
#define __PR_SYSCALL_PARAM_PRINT(i, sb, succ, type, arg)		\
	do {								\
		if ((i) != 0)						\
			uk_streambuf_strcpy((sb), ", ");		\
		pr_param((sb), fmtf, (type), (long) (arg), succ);	\
	} while (0);

#define PR_SYSCALL(sb, fmtf, syscall_num, succ, ...)			\
	do {								\
		uk_streambuf_shcc(sb, fmtf, SYSCALL);			\
		uk_streambuf_strcpy(sb, uk_syscall_name((syscall_num)));\
		uk_streambuf_shcc(sb, fmtf, RESET);			\
		uk_streambuf_strcpy(sb, "(");				\
		VARG2_EXPAND(__PR_SYSCALL_PARAM_PRINT, sb, succ,	\
			     __VA_ARGS__);				\
		uk_streambuf_strcpy(sb, ")");				\
	} while (0)

#define __VPR_SYSCALL_PARAM_DECL(i, unused0, unused1, type)		\
	long UK_CONCAT(arg, i);
#define __VPR_SYSCALL_PARAM_VALOAD(i, ap, unused, type)			\
	UK_CONCAT(arg, i) = va_arg(ap, long);
#define __VPR_SYSCALL_PARAM_PRINT(i, sb, succ, type)			\
	do {								\
		if ((i) != 0)						\
			uk_streambuf_strcpy((sb), ", ");		\
		pr_param((sb), fmtf, (type), UK_CONCAT(arg, i), succ);	\
	} while (0);

#define VPR_SYSCALL(sb, fmtf, syscall_num, ap, succ, ...)		\
	do {								\
		VARG_EXPAND(__VPR_SYSCALL_PARAM_DECL, 0, 0, __VA_ARGS__)\
		VARG_EXPAND(__VPR_SYSCALL_PARAM_VALOAD, ap, 0,		\
			    __VA_ARGS__)				\
									\
		uk_streambuf_shcc(sb, fmtf, SYSCALL);			\
		uk_streambuf_strcpy(sb, uk_syscall_name((syscall_num)));\
		uk_streambuf_shcc(sb, fmtf, RESET);			\
		uk_streambuf_strcpy(sb, "(");				\
		VARG_EXPAND(__VPR_SYSCALL_PARAM_PRINT, sb, succ,	\
			    __VA_ARGS__)				\
		uk_streambuf_strcpy(sb, ")");				\
	} while (0)

#define PR_SYSRET(sb, fmtf, type, rc)					\
	do {								\
		uk_streambuf_strcpy(sb, " = ");				\
		pr_retcode((sb), (fmtf), (type), (rc));			\
	} while (0)

/*
 * Pretty print a system call request to a stream buffer
 */
static void pr_syscall(struct uk_streambuf *sb, int fmtf,
		       long syscall_num, long rc, va_list args)
{
	switch (syscall_num) {
#ifdef HAVE_uk_syscall_brk
	case SYS_brk: {
			long addr = (long) va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, 1,
				   PT_VADDR, addr);
			PR_SYSRET(sb, fmtf, PT_VADDR, rc);
		}
		break;
#endif /* HAVE_uk_syscall_brk */

#ifdef HAVE_uk_syscall_open
	case SYS_open:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_PATH, PT_OFLAGS);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;
#endif /* HAVE_uk_syscall_open */

#ifdef HAVE_uk_syscall_openat
	case SYS_openat:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_DIRFD, PT_PATH, PT_OFLAGS);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;
#endif /* HAVE_uk_syscall_openat */

#ifdef HAVE_uk_syscall_readlink
	case SYS_readlink:
		do {
			char *path = (char *) va_arg(args, long);
			void *buf  = (void *) va_arg(args, long);
			__sz len   = (__sz)   va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc >= 0,
				   PT_PATH, path,
				   PT_BUFP(((rc >= 0) ? (__sz) rc : len))
				   | PT_OUT, buf,
				   PT_UDEC, len);
			PR_SYSRET(sb, fmtf, PT_UDEC, rc);
		} while (0);
		break;
#endif /* HAVE_uk_syscall_readlink */

#ifdef HAVE_uk_syscall_readlinkat
	case SYS_readlinkat:
		do {
			int dirfd  = (int)    va_arg(args, long);
			char *path = (char *) va_arg(args, long);
			void *buf  = (void *) va_arg(args, long);
			__sz len   = (__sz)   va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc >= 0,
				   PT_DIRFD, dirfd, PT_PATH, path,
				   PT_BUFP(((rc >= 0) ? (__sz) rc : len))
				   | PT_OUT, buf,
				   PT_UDEC, len);
			PR_SYSRET(sb, fmtf, PT_UDEC, rc);
		} while (0);
		break;
#endif /* HAVE_uk_syscall_readlinkat */

#if defined HAVE_uk_syscall_read && defined HAVE_uk_syscall_write
	case SYS_write:
		do {
			int fd    = (int)    va_arg(args, long);
			void *buf = (void *) va_arg(args, long);
			__sz len  = (__sz)   va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc >= 0, PT_FD, fd,
				   PT_BUFP(len), buf, PT_UDEC, len);
			PR_SYSRET(sb, fmtf, PT_UDEC, rc);
		} while (0);
		break;
	case SYS_read:
		do {
			int fd    = (int)    va_arg(args, long);
			void *buf = (void *) va_arg(args, long);
			__sz len  = (__sz)   va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc >= 0, PT_FD, fd,
				   PT_BUFP(((rc >= 0) ? (__sz) rc : len))
				   | PT_OUT, buf,
				   PT_UDEC, len);
			PR_SYSRET(sb, fmtf, PT_UDEC, rc);
		} while (0);
		break;
#endif /* HAVE_uk_syscall_read && HAVE_uk_syscall_write */

#ifdef HAVE_uk_syscall_pread64
	case SYS_pread64:
		do {
			int fd    = (int)    va_arg(args, long);
			void *buf = (void *) va_arg(args, long);
			__sz len  = (__sz)   va_arg(args, long);
			__sz off  = (__sz)   va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc >= 0, PT_FD, fd,
				   PT_BUFP(((rc >= 0) ? (__sz) rc : len))
				   | PT_OUT, buf,
				   PT_UDEC, len, PT_UDEC, off);
			PR_SYSRET(sb, fmtf, PT_UDEC, rc);
		} while (0);
		break;
#endif /* HAVE_uk_syscall_pread64 */

#ifdef HAVE_uk_syscall_stat
	case SYS_stat:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_PATH, PT_STRUCT(stat) | PT_OUT);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_stat */

#ifdef HAVE_uk_syscall_fstat
	case SYS_fstat:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_FD, PT_STRUCT(stat) | PT_OUT);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_fstat */

#ifdef HAVE_uk_syscall_close
	case SYS_close:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0, PT_FD);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_close */

#ifdef HAVE_uk_syscall_dup
	case SYS_dup:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc >= 0, PT_FD);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;
#endif /* HAVE_uk_syscall_dup */

#ifdef HAVE_uk_syscall_dup2
	case SYS_dup2:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc >= 0, PT_FD, PT_FD);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;
#endif /* HAVE_uk_syscall_dup2 */

#ifdef HAVE_uk_syscall_eventfd
	case SYS_eventfd:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_DEC);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;
#endif /* HAVE_uk_syscall_eventfd */

#ifdef HAVE_uk_syscall_eventfd2
	case SYS_eventfd2:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_DEC, PT_OFLAGS);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;
#endif /* HAVE_uk_syscall_eventfd2 */

#ifdef HAVE_uk_syscall_gettid
	case SYS_gettid:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, 1);
		PR_SYSRET(sb, fmtf, PT_TID, rc);
		break;
#endif /* HAVE_uk_syscall_gettid */

#ifdef HAVE_uk_syscall_getpid
	case SYS_getpid:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, 1);
		PR_SYSRET(sb, fmtf, PT_PID, rc);
		break;
#endif /* HAVE_uk_syscall_getpid */

#ifdef HAVE_uk_syscall_munmap
	case SYS_munmap:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0, PT_VADDR);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_munmap */

#ifdef HAVE_uk_syscall_mmap
	case SYS_mmap:
		VPR_SYSCALL(sb, fmtf, syscall_num, args,
			    (void *)rc != MAP_FAILED, PT_VADDR, PT_UDEC,
			    PT_PROTFLAGS, PT_MAPFLAGS, PT_FD, PT_UDEC);
		PR_SYSRET(sb, fmtf, PT_VADDR, rc);
		break;
#endif /* HAVE_uk_syscall_mmap */

#ifdef HAVE_uk_syscall_mprotect
	case SYS_mprotect:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_VADDR, PT_UDEC, PT_PROTFLAGS);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_mprotect */

#ifdef HAVE_uk_syscall_futex
	case SYS_futex:
		do {
			long addr = (long) va_arg(args, long);
			int op = (int) va_arg(args, long);
			unsigned long val = (unsigned long) va_arg(args, long);
			const struct timespec *timeout =
				(const struct timespec *) va_arg(args, long);
			long addr2 = (long) va_arg(args, long);
			unsigned long val2 = (unsigned long) va_arg(args, long);

			switch (op) {
			case FUTEX_WAIT:
			case FUTEX_WAIT_PRIVATE:
				PR_SYSCALL(sb, fmtf, syscall_num, rc == 0,
						PT_VADDR, addr,
						PT_FUTEXOP, op,
						PT_HEX, val,
						PT_STRUCT(timespec), timeout);
				PR_SYSRET(sb, fmtf, PT_STATUS, rc);
				break;

			case FUTEX_WAKE:
			case FUTEX_WAKE_PRIVATE:
				PR_SYSCALL(sb, fmtf, syscall_num, rc == 0,
						PT_VADDR, addr,
						PT_FUTEXOP, op,
						PT_HEX, val);
				PR_SYSRET(sb, fmtf, PT_STATUS, rc);
				break;

			case FUTEX_CMP_REQUEUE:
				PR_SYSCALL(sb, fmtf, syscall_num, rc == 0,
						PT_VADDR, addr,
						PT_FUTEXOP, op,
						PT_HEX, val,
						PT_VADDR, addr2,
						PT_HEX, val2);
				PR_SYSRET(sb, fmtf, PT_STATUS, rc);
				break;

			case FUTEX_FD:
			case FUTEX_REQUEUE:
			default:
				PR_SYSCALL(sb, fmtf, syscall_num, rc == 0,
						PT_VADDR, addr,
						PT_FUTEXOP, op);
				PR_SYSRET(sb, fmtf, PT_STATUS, rc);
				break;
			}
		} while (0);
		break;
#endif /* HAVE_uk_syscall_futex */

#ifdef HAVE_uk_syscall_clock_gettime
	case SYS_clock_gettime:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_CLOCKID, PT_STRUCT(timespec) | PT_OUT);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_clock_gettime */

#ifdef HAVE_uk_syscall_socket
	case SYS_socket:
		do {
			int domain = (int) va_arg(args, long);
			int type = (int) va_arg(args, long);
			int protocol = (int) va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc >= 0,
					PT_SOCKETAF, domain,
					PT_SOCKETTYPE, type,
					PT_UDEC, protocol);
		} while (0);
		PR_SYSRET(sb, fmtf, PT_FD, rc);
		break;

	case SYS_bind:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_FD, PT_VADDR, PT_UDEC);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;

	case SYS_sendto:
		do {
			int fd = (int) va_arg(args, long);
			void *buf = (void *) va_arg(args, long);
			__sz len = (__sz) va_arg(args, long);
			int flags = (int) va_arg(args, long);
			void *dst_addr = (void *) va_arg(args, long);
			long dst_len = (long) va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc = 0,
				   PT_FD, fd, PT_BUFP(len), buf,
				   PT_UDEC, len, PT_MSGFLAGS, flags,
				   PT_VADDR, dst_addr, PT_UDEC, dst_len);
		} while (0);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;

	case SYS_recvmsg:
		do {
			int fd = (int) va_arg(args, long);
			void *buf = (void *) va_arg(args, long);
			__sz maxlen = (__sz) va_arg(args, long);
			int flags = (int) va_arg(args, long);

			PR_SYSCALL(sb, fmtf, syscall_num, rc == 0,
				   PT_FD, fd,
				   PT_BUFP(rc > 0 ? (__sz) rc : maxlen), buf,
				   PT_UDEC, maxlen, PT_MSGFLAGS, flags);
		} while (0);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;

#endif /* HAVE_uk_syscall_socket */

#ifdef HAVE_uk_syscall_clone
	case SYS_clone:
#if CONFIG_ARCH_X86_64
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc >= 0,
			    PT_CLONEFLAGS,
			    PT_VADDR, /* sp */
			    PT_TID | PT_REF, /* ref to parent tid */
			    PT_TID | PT_REF, /* ref to child tid */
			    PT_VADDR /* tlsp */);
#else /* !CONFIG_ARCH_X86_64 */
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc >= 0,
			    PT_CLONEFLAGS,
			    PT_VADDR, /* sp */
			    PT_TID | PT_REF, /* ref to parent tid */
			    PT_VADDR, /* tlsp */
			    PT_TID | PT_REF /* ref to child tid */);
#endif /* !CONFIG_ARCH_X86_64 */
		PR_SYSRET(sb, fmtf, PT_TID, rc);
		break;
#endif /* HAVE_uk_syscall_clone */

#ifdef HAVE_uk_syscall_access
	case SYS_access:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_PATH, PT_OKFLAGS);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_access */

#ifdef HAVE_uk_syscall_uname
	case SYS_uname:
		VPR_SYSCALL(sb, fmtf, syscall_num, args, rc == 0,
			    PT_STRUCT(utsname) | PT_OUT);
		PR_SYSRET(sb, fmtf, PT_STATUS, rc);
		break;
#endif /* HAVE_uk_syscall_uname */

	default:
		do {
			long arg0 = va_arg(args, long);
			long arg1 = va_arg(args, long);

			uk_streambuf_shcc(sb, fmtf, SYSCALL);
			uk_streambuf_strcpy(sb, uk_syscall_name((syscall_num)));
			uk_streambuf_shcc(sb, fmtf, RESET);
			uk_streambuf_strcpy(sb, "(");
			uk_streambuf_shcc(sb, fmtf, VALUE);
			uk_streambuf_printf(sb, "0x%lx", arg0);
			uk_streambuf_shcc(sb, fmtf, RESET);
			uk_streambuf_strcpy(sb, ", ");
			uk_streambuf_shcc(sb, fmtf, VALUE);
			uk_streambuf_printf(sb, "0x%lx", arg1);
			uk_streambuf_shcc(sb, fmtf, RESET);
			uk_streambuf_strcpy(sb, ", ...)");
			PR_SYSRET(sb, fmtf, PT_HEX, rc);
		} while (0);
		break;
	}

	/* TODO: Some consoles require both a newline and a carriage return to
	 * go to the start of the next line. There should be a dedicated TTY
	 * layer to take care of this. Once that layer exists, stop printing
	 * a CRLF sequence if `UK_PRSYSCALL_FMTF_NEWLINE` is set and print a
	 * single newline character instead.
	 */

	/* Try to fix the line ending if content got truncated */
	if (uk_streambuf_istruncated(sb)) {
		/* reserve an extra byte for the newline ending if configured */
		__sz needed_len = 3
				 + ((fmtf & UK_PRSYSCALL_FMTF_NEWLINE) ? 2 : 0);
		if (uk_streambuf_buflen(sb) > needed_len + 1) {
			sb->seek = uk_streambuf_seek(sb) - needed_len;
			uk_streambuf_strcpy(sb, "...");
		}
	}

	if (fmtf & UK_PRSYSCALL_FMTF_NEWLINE)
		uk_streambuf_strcpy(sb, "\r\n");
}

int uk_snprsyscall(char *buf, __sz maxlen, int fmtf, long syscall_num,
		   long sysret, ...)
{
	struct uk_streambuf sb;
	va_list args;

	va_start(args, sysret);
	uk_streambuf_init(&sb, buf, maxlen, UK_STREAMBUF_C_TERMSHIFT);
	pr_syscall(&sb, fmtf, syscall_num, sysret, args);
	va_end(args);

	return (int) (uk_streambuf_seek(&sb));
}

int uk_vsnprsyscall(char *buf, __sz maxlen, int fmtf, long syscall_num,
		    long sysret, va_list args)
{
	struct uk_streambuf sb;

	uk_streambuf_init(&sb, buf, maxlen, UK_STREAMBUF_C_TERMSHIFT);
	pr_syscall(&sb, fmtf, syscall_num, sysret, args);

	return (int) (uk_streambuf_seek(&sb));
}

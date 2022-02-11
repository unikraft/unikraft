/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@upb.ro>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2020, University Politehnica of Bucharest. All rights reserved.
 *               2022, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
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
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/print.h>

#include <stdio.h>
#include <inttypes.h>

struct source_location {
	/** Name of the source file */
	const char *filename;
	/** Offending line in the source file */
	__u32 line;
	/** Column in the source line */
	__u32 column;
};

static const struct source_location ubsan_unknown_location = {
	"<unknown file>",
	0,
	0,
};

enum type_kind {
	UBSAN_TK_INTEGER = 0,
	UBSAN_TK_FLOAT   = 1,
	UBSAN_TK_UNKNOWN = 0xffff
};

struct type_descriptor {
	/** Kind of the type < enum type_kind > */
	__u16 kind;
	/** Info on the type (e.g., signedness, width) */
	__u16 info;
	/** Type name */
	char name[];
};

typedef __uptr value_handle_t;

static inline int ubsan_is_int(const struct type_descriptor *type)
{
	return (type->kind == UBSAN_TK_INTEGER);
}

static inline int ubsan_is_float(const struct type_descriptor *type)
{
	return (type->kind == UBSAN_TK_FLOAT);
}

static inline int ubsan_is_signed(const struct type_descriptor *type)
{
	UK_ASSERT(ubsan_is_int(type));
	return (type->info & 1);
}

static void
ubsan_pr_value(char *buf, __ssz len, const struct type_descriptor *type,
	       value_handle_t val)
{
	if (type == NULL) { /* Treat as pointer */
		snprintf(buf, len, "0x%"__PRIuptr, val);
	} else if (ubsan_is_int(type)) {
		snprintf(buf, len, ubsan_is_signed(type) ?
				"%"__PRIssz : "%"__PRIsz, val);
	} else if (ubsan_is_float(type)) {
		/* TODO: Add float support */
		buf[0] = 'F';
		buf[1] = '?';
		buf[2] = 0;
	} else {
		/* We do not know how to print the type */
		buf[0] = '?';
		buf[1] = 0;
	}
}

/* Private macros for event handler definition */
#define __no_sanitize		__attribute__((no_sanitize("undefined")))

#undef NORETURN
#undef RETURN
#undef ABORT

#define _UBSAN_ABORT		__builtin_unreachable()
#define _UBSAN_NORETURN		__builtin_unreachable()
#define _UBSAN_RETURN

#define _UBSAN_SUFFIX_ABORT	_abort
#define _UBSAN_SUFFIX_NORETURN
#define _UBSAN_SUFFIX_RETURN

#define _UBSAN_STOP_ABORT	1
#define _UBSAN_STOP_NORETURN	1
#define _UBSAN_STOP_RETURN	0

#define _UBSAN_NARGS_X(a1, a2, b1, b2, c1, c2, d1, d2, nr, ...) nr
#define _UBSAN_NARGS(...) _UBSAN_NARGS_X(__VA_ARGS__, 4, 4, 3, 3, 2, 2, 1, 1, 0)

#define _UBSAN_DVOIDARG(t, a) void *a
#define _UBSAN_DARG(t, a) t a __maybe_unused
#define _UBSAN_ARG(t, a) (t)a

#define _UBSAN_ARG_MAP1(m, t, a) m(t, a)
#define _UBSAN_ARG_MAP2(m, t, a, ...) m(t, a), _UBSAN_ARG_MAP1(m, __VA_ARGS__)
#define _UBSAN_ARG_MAP3(m, t, a, ...) m(t, a), _UBSAN_ARG_MAP2(m, __VA_ARGS__)
#define _UBSAN_ARG_MAP4(m, t, a, ...) m(t, a), _UBSAN_ARG_MAP3(m, __VA_ARGS__)
#define _UBSAN_ARG_MAPx(m, n, ...) UK_CONCAT(_UBSAN_ARG_MAP, n)(m, __VA_ARGS__)

#define _UBSAN_ARGS(...)						\
	_UBSAN_ARG_MAPx(_UBSAN_DARG, _UBSAN_NARGS(__VA_ARGS__), __VA_ARGS__)
#define _UBSAN_VOID_ARGS(...)						\
	_UBSAN_ARG_MAPx(_UBSAN_DVOIDARG, _UBSAN_NARGS(__VA_ARGS__), __VA_ARGS__)
#define _UBSAN_ARGS_CALL(...)						\
	_UBSAN_ARG_MAPx(_UBSAN_ARG, _UBSAN_NARGS(__VA_ARGS__), __VA_ARGS__)

#define _UBSAN_IMPL(event) ubsan_handle_##event##_impl
#define _UBSAN_IMPL_SIG(event, ...)					\
	static void __no_sanitize					\
	_UBSAN_IMPL(event)(int _stop __maybe_unused, _UBSAN_ARGS(__VA_ARGS__))

#define _UBSAN_HANDLER_NAME(event, type)				\
	UK_CONCAT(__ubsan_handle_##event, _UBSAN_SUFFIX_##type)

#define _UBSAN_DEFINE_HANDLER(event, type, ...)				\
	void __no_sanitize						\
	_UBSAN_HANDLER_NAME(event, type)(_UBSAN_VOID_ARGS(__VA_ARGS__))	\
	{								\
		_UBSAN_IMPL(event)(_UBSAN_STOP_##type,			\
				   _UBSAN_ARGS_CALL(__VA_ARGS__));	\
		_UBSAN_##type;						\
	}

#define _UBSAN_PR(lmethod, l, msg, ...)					\
	lmethod("Undefined behavior at %s:%d:%d: " msg "\n",		\
		l->filename, l->line, l->column, ##__VA_ARGS__)

/**
 * Define the implementation for a recoverable event. This will be called by
 * the recoverable handler and the unrecoverable handler (i.e., with the _abort
 * suffix).
 *
 * @param event The name of the UBSAN event (e.g., add_overflow)
 * @param ... A list of <type, name> pairs defining the signature of the event
 *    handler according to UBSAN
 */
#define UBSAN_RECOVERABLE(event, ...)					\
	_UBSAN_IMPL_SIG(event, __VA_ARGS__);				\
	_UBSAN_DEFINE_HANDLER(event, RETURN, __VA_ARGS__)		\
	_UBSAN_DEFINE_HANDLER(event, ABORT, __VA_ARGS__)		\
	_UBSAN_IMPL_SIG(event, __VA_ARGS__)

/**
 * Define the implementation for an unrecoverable event.
 *
 * @param event The name of the UBSAN event (e.g., add_overflow)
 * @param ... A list of <type, name> pairs defining the signature of the event
 *    handler according to UBSAN
 */
#define UBSAN_UNRECOVERABLE(event, ...)					\
	_UBSAN_IMPL_SIG(event, __VA_ARGS__);				\
	_UBSAN_DEFINE_HANDLER(event, NORETURN, __VA_ARGS__)		\
	_UBSAN_IMPL_SIG(event, __VA_ARGS__)

/**
 * Handle the event by either printing a corresponding error message or
 * crashing the system. The behavior depends on if the macro is used in
 * the definition of a recoverable or unrecoverable event handler. The
 * detection is done using the _stop argument that is inserted automatically to
 * the argument list of the handler functions.
 *
 * NOTE: If used with UBSAN_RECOVERABLE() appropriate behavior is automatically
 * selected. The handler must thus be fine with UBSAN_HANDLE() not returning.
 */
#define UBSAN_HANDLE(loc, msg, ...)					\
	do {								\
		const struct source_location *l = loc;			\
		if (!l || !l->filename)					\
			l = &ubsan_unknown_location;			\
		if (_stop)						\
			_UBSAN_PR(UK_CRASH, l, msg, ##__VA_ARGS__);	\
		else							\
			_UBSAN_PR(uk_pr_err, l, msg, ##__VA_ARGS__);	\
	} while (0)

/**
 * Undefined behavior handler definitions
 */

struct type_mismatch_data_v1 {
	struct source_location location;
	const struct type_descriptor *type;
	__u8 alignment;
	__u8 type_check_kind;
};

const char *ubsan_type_check_kind_str[] = {
	"load from", "store to", "access of"
};

UBSAN_RECOVERABLE(type_mismatch_v1, struct type_mismatch_data_v1 *, data,
		  value_handle_t, pointer)
{
	const char *kind;
	unsigned int alignment = 1 << data->alignment;

	kind = ubsan_type_check_kind_str[MIN(data->type_check_kind, 2)];

	if (!pointer)
		UBSAN_HANDLE(&data->location, "%s null pointer of type %s",
			     kind, data->type->name);
	else if (pointer & (alignment - 1))
		UBSAN_HANDLE(&data->location,
			     "%s misaligned address 0x%"__PRIuptr" for type %s,"
			     " which requires %u byte alignment",
			     kind, pointer, data->type->name, alignment);
	else
		UBSAN_HANDLE(&data->location,
			     "%s address 0x%"__PRIuptr" with insufficient "
			     "space for an object of type %s",
			     kind, pointer, data->type->name);
}

struct ubsan_overflow_data {
	struct source_location location;
	const struct type_descriptor *type;
};

#define UBSAN_HANDLE_INT_OVERFLOW(d, op, lhs, rhs)			\
	do {								\
		char slhs[32];						\
		char srhs[32];						\
									\
		ubsan_pr_value(slhs, sizeof(slhs), (d)->type, lhs);	\
		ubsan_pr_value(srhs, sizeof(srhs), (d)->type, rhs);	\
									\
		UBSAN_HANDLE(&d->location,				\
			"%s integer overflow: %s " op			\
			" %s cannot be represented in type %s",		\
			ubsan_is_signed((d)->type) ? "signed" : "unsigned",\
			slhs, srhs, (d)->type->name);			\
	} while (0)

UBSAN_RECOVERABLE(add_overflow, struct ubsan_overflow_data *, data,
		  value_handle_t, lhs, value_handle_t, rhs)
{
	UBSAN_HANDLE_INT_OVERFLOW(data, "+", lhs, rhs);
}

UBSAN_RECOVERABLE(sub_overflow, struct ubsan_overflow_data *, data,
		  value_handle_t, lhs, value_handle_t, rhs)
{
	UBSAN_HANDLE_INT_OVERFLOW(data, "-", lhs, rhs);
}

UBSAN_RECOVERABLE(mul_overflow, struct ubsan_overflow_data *, data,
		  value_handle_t, lhs, value_handle_t, rhs)
{
	UBSAN_HANDLE_INT_OVERFLOW(data, "*", lhs, rhs);
}

UBSAN_RECOVERABLE(divrem_overflow, struct ubsan_overflow_data *, data,
		  value_handle_t, lhs, value_handle_t, rhs)
{
	if (rhs == 0)
		UBSAN_HANDLE(&data->location, "division by zero");
	else
		UBSAN_HANDLE_INT_OVERFLOW(data, "/", lhs, rhs);
}

UBSAN_RECOVERABLE(negate_overflow, struct ubsan_overflow_data *, data,
		  value_handle_t, old_value)
{
	char sval[32];

	ubsan_pr_value(sval, sizeof(sval), data->type, old_value);

	UBSAN_HANDLE(&data->location,
		     "negation of %s cannot be represented in type %s",
		     sval, data->type->name);
}

struct ubsan_overflow_pointer_data {
	struct source_location location;
};

UBSAN_RECOVERABLE(pointer_overflow, struct ubsan_overflow_pointer_data *, data,
		  value_handle_t, base, value_handle_t, result)
{
	UBSAN_HANDLE(&data->location, "pointer index expression with base "
		     "0x%"__PRIuptr" overflowed to 0x%"__PRIuptr,
		     base, result);
}

struct ubsan_out_of_bounds_data {
	struct source_location location;
	const struct type_descriptor *array_type;
	const struct type_descriptor *index_type;
};

UBSAN_RECOVERABLE(out_of_bounds, struct ubsan_out_of_bounds_data *, data,
		  value_handle_t, index)
{
	char sindex[32];

	ubsan_pr_value(sindex, sizeof(sindex), data->index_type, index);

	UBSAN_HANDLE(&data->location, "index %s out of bounds for type %s",
		     sindex, data->array_type->name);
}

struct ubsan_shift_out_of_bounds_data {
	struct source_location location;
	const struct type_descriptor *lhs_type;
	const struct type_descriptor *rhs_type;
};

UBSAN_RECOVERABLE(shift_out_of_bounds,
		  struct ubsan_shift_out_of_bounds_data *, data,
		  value_handle_t, lhs, value_handle_t, rhs)
{
	char slhs[32];
	char srhs[32];

	ubsan_pr_value(slhs, sizeof(slhs), data->lhs_type, lhs);
	ubsan_pr_value(srhs, sizeof(srhs), data->rhs_type, rhs);

	UBSAN_HANDLE(&data->location,
		     "shift of %s by %s out of bounds for type %s",
		     slhs, srhs, data->lhs_type->name);
}

struct ubsan_vla_bound_data {
	struct source_location location;
	const struct type_descriptor *type;
};

UBSAN_RECOVERABLE(vla_bound_not_positive, struct ubsan_vla_bound_data *, data,
		  value_handle_t, bound)
{
	char sbound[32];

	ubsan_pr_value(sbound, sizeof(sbound), data->type, bound);

	UBSAN_HANDLE(&data->location, "variable length array bound evaluates "
		     "to non-positive value %s", sbound);
}

struct ubsan_invalid_value_data {
	struct source_location location;
	const struct type_descriptor *type;
};

UBSAN_RECOVERABLE(load_invalid_value, struct ubsan_invalid_value_data *, data,
		  value_handle_t, value)
{
	char sval[32];

	ubsan_pr_value(sval, sizeof(sval), data->type, value);

	UBSAN_HANDLE(&data->location, "load of value %s, which is not a valid "
		     "value for type %s", sval, data->type->name);
}

struct ubsan_nonnull_arg_data {
	struct source_location location;
	struct source_location attr_location;
	int arg_index;
};

UBSAN_RECOVERABLE(nonnull_arg, struct ubsan_nonnull_arg_data *, data)
{
	UBSAN_HANDLE(&data->location, "null pointer passed as argument %d, "
		     "which is declared to never be null",
		     data->arg_index);
}

UBSAN_RECOVERABLE(nullability_arg, struct ubsan_nonnull_arg_data *, data)
{
	UBSAN_HANDLE(&data->location, "null pointer passed as argument %d, "
		     "which is declared to never be null",
		     data->arg_index);
}

struct ubsan_nonnull_return_data_v1 {
	struct source_location attr_location;
};

UBSAN_RECOVERABLE(nonnull_return_v1,
		  struct ubsan_nonnull_return_data_v1 *, data,
		  struct source_location *, loc)
{
	UBSAN_HANDLE(loc, "null pointer returned from function declared to "
		     "never return null");
}

UBSAN_RECOVERABLE(nullability_return_v1,
		  struct ubsan_nonnull_return_data_v1 *, data,
		  struct source_location *, loc)
{
	UBSAN_HANDLE(loc, "null pointer returned from function declared to "
		     "never return null");
}

struct ubsan_unreachable_data {
	struct source_location location;
};

UBSAN_UNRECOVERABLE(builtin_unreachable, struct ubsan_unreachable_data *, data)
{
	UBSAN_HANDLE(&data->location,
		     "execution reached unreachable program point");
}

struct ubsan_missing_return_data {
	struct source_location location;
};

UBSAN_UNRECOVERABLE(missing_return, struct ubsan_missing_return_data *, data)
{
	UBSAN_HANDLE(&data->location,
		     "execution reached end of value-returning function "
		     "without returning a value");
}

enum ubsan_invalid_builtin_kind {
	/** __builtin_ctz has been called with 0 */
	UBSAN_IBIK_CTZ_ZERO = 0,
	/** __builtin_clz has been called with 0 */
	UBSAN_IBIK_CLZ_ZERO = 1,

	UBSAN_IBIK_MAX
};

static const char *ubsan_invalid_builtin_kind_str[2] = {
	"__builtin_ctz", /* UBSAN_IBIK_CTZ_ZERO */
	"__builtin_clz"  /* UBSAN_IBIK_CLZ_ZERO */
};

struct ubsan_invalid_builtin_data {
	struct source_location location;
	unsigned char kind;
};

UBSAN_RECOVERABLE(invalid_builtin, struct ubsan_invalid_builtin_data *, data)
{
	UK_ASSERT(data->kind < UBSAN_IBIK_MAX);

	UBSAN_HANDLE(&data->location, "calling %s with zero",
		     ubsan_invalid_builtin_kind_str[data->kind]);
}

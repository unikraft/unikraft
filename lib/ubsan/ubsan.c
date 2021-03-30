/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@upb.ro>
 *
 * Copyright (c) 2020, University Politehnica of Bucharest. All rights reserved.
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

#include <stdint.h>
#include <uk/assert.h>
#include <uk/essentials.h>

struct source_location {
	const char *filename;
	uint32_t line;
	uint32_t column;
};

struct type_descriptor {
	uint16_t kind;
	uint16_t info;
	char name[];
};

struct out_of_bounds_info {
	struct source_location location;
	struct type_descriptor left_type;
	struct type_descriptor right_type;
};

struct type_mismatch_info {
	struct source_location location;
	struct type_descriptor *type;
	unsigned long alignment;
	uint8_t type_check_kind;
};

static const struct source_location unknown_location = {

	"<unknown file>",
	0,
	0,
};

typedef uintptr_t ubsan_value_handle_t;

static void ubsan_log_location(const struct source_location *location,
			const char *violation) __noreturn;

static void ubsan_log_location(const struct source_location *location,
			const char *violation)
{
	if (!location || !location->filename)
		location = &unknown_location;

	UK_CRASH("Undefined behavior at %s:%d:%d:%s",
		location->filename, location->line,
		location->column, violation);
}

void __ubsan_handle_type_mismatch(void *data_raw,
				  void *pointer_raw)
{
	struct type_mismatch_info *data =
		(struct type_mismatch_info *) data_raw;
	ubsan_value_handle_t pointer = (ubsan_value_handle_t) pointer_raw;
	const char *violation = "type mismatch";

	if (!pointer)
		violation = "null pointer access";
	else if (data->alignment && (pointer & (data->alignment - 1)))
		violation = "unaligned access";

	ubsan_log_location(&data->location, violation);
}

struct type_mismatch_info_v1 {
	struct source_location location;
	struct type_descriptor *type;
	uintptr_t alignment;
	uint8_t type_check_kind;
};

void __ubsan_handle_type_mismatch_v1(void *data_raw,
				  void *pointer_raw)
{
	struct type_mismatch_info_v1 *data =
		(struct type_mismatch_info_v1 *) data_raw;
	ubsan_value_handle_t pointer = (ubsan_value_handle_t) pointer_raw;
	const char *violation = "type mismatch";

	if (!pointer)
		violation = "null pointer access";
	else if (data->alignment && (pointer & (data->alignment - 1)))
		violation = "unaligned access";

	ubsan_log_location(&data->location, violation);
}

struct ubsan_overflow_data {
	struct source_location location;
	struct type_descriptor *type;
};

void __ubsan_handle_mul_overflow(void *data_raw,
				 void *lhs_raw __unused,
				 void *rhs_raw __unused)
{
	struct ubsan_overflow_data *data =
			(struct ubsan_overflow_data *) data_raw;

	ubsan_log_location(&data->location, "multiplication overflow");
}

struct ubsan_overflow_pointer_data {
	struct source_location location;
};

void __ubsan_handle_pointer_overflow(void *data_raw,
				 void *base __unused,
				 void *result __unused)
{
	struct ubsan_overflow_pointer_data *data =
			(struct ubsan_overflow_pointer_data *) data_raw;

	ubsan_log_location(&data->location, "pointer overflow");
}

void __ubsan_handle_sub_overflow(void *data_raw,
				 void *lhs_raw __unused,
				 void *rhs_raw __unused)
{
	struct ubsan_overflow_data *data =
			(struct ubsan_overflow_data *) data_raw;

	ubsan_log_location(&data->location, "subtraction overflow");
}

void __ubsan_handle_add_overflow(void *data_raw,
				 void *lhs_raw __unused,
				 void *rhs_raw __unused)
{
	struct ubsan_overflow_data *data =
			(struct ubsan_overflow_data *) data_raw;

	ubsan_log_location(&data->location, "addition overflow");
}

void __ubsan_handle_negate_overflow(void *data_raw,
				    void *old_value_raw __unused)
{
	struct ubsan_overflow_data *data =
			(struct ubsan_overflow_data *) data_raw;

	ubsan_log_location(&data->location, "negation overflow");
}

void __ubsan_handle_divrem_overflow(void *data_raw,
				    void *lhs_raw __unused,
				    void *rhs_raw __unused)
{
	struct ubsan_overflow_data *data =
			(struct ubsan_overflow_data *) data_raw;

	ubsan_log_location(&data->location, "division remainder overflow");
}


struct ubsan_out_of_bounds_data {
	struct source_location location;
	struct type_descriptor *array_type;
	struct type_descriptor *index_type;
};

void __ubsan_handle_out_of_bounds(void *data_raw,
				  void *index_raw __unused)
{
	struct ubsan_out_of_bounds_data *data =
			(struct ubsan_out_of_bounds_data *) data_raw;

	ubsan_log_location(&data->location, "index out of bounds");
}

struct ubsan_shift_out_of_bounds_data {
	struct source_location location;
	struct type_descriptor *lhs_type;
	struct type_descriptor *rhs_type;
};

void __ubsan_handle_shift_out_of_bounds(void *data_raw,
					void *lhs_raw __unused,
					void *rhs_raw __unused)
{
	struct ubsan_shift_out_of_bounds_data *data =
		(struct ubsan_shift_out_of_bounds_data *) data_raw;

	/* TODO:  print cause of shift */
	ubsan_log_location(&data->location, "shift out of bounds");
}

struct ubsan_nonnull_arg_data {
	struct source_location location;
	struct source_location attr_location;
};

void __ubsan_handle_nonnull_arg(void *data_raw,  intptr_t index_raw __unused)
{
	struct ubsan_nonnull_arg_data *data =
		(struct ubsan_nonnull_arg_data *) data_raw;

	ubsan_log_location(&data->location, "null argument");
}

struct ubsan_vla_bound_data {
	struct source_location location;
	struct type_descriptor *type;
};

void __ubsan_handle_vla_bound_not_positive(void *data_raw,
					void *bound_raw __unused)
{
	struct ubsan_vla_bound_data *data =
			(struct ubsan_vla_bound_data *) data_raw;

	ubsan_log_location(&data->location, "negative variable array length");
}

struct ubsan_invalid_value_data {
	struct source_location location;
	struct type_descriptor *type;
};

void __ubsan_handle_load_invalid_value(void *data_raw,
				void *value_raw __unused)
{
	struct ubsan_invalid_value_data *data =
		(struct ubsan_invalid_value_data *) data_raw;

	ubsan_log_location(&data->location, "invalid value load");
}

struct ubsan_cfi_bad_icall_data {
	struct source_location location;
	struct type_descriptor *type;
};

void __ubsan_handle_cfi_bad_icall(void *data_raw,
				void *value_raw __unused)
{
	struct ubsan_cfi_bad_icall_data *data =
		(struct ubsan_cfi_bad_icall_data *) data_raw;

	ubsan_log_location(&data->location,
		"control flow integrity check failure during indirect call");
}

struct ubsan_nonnull_return_data {
	struct source_location location;
	struct source_location attr_location;
};

void __ubsan_handle_nonnull_return(void *data_raw)
{
	struct ubsan_nonnull_return_data *data =
		(struct ubsan_nonnull_return_data *) data_raw;

	ubsan_log_location(&data->location, "null return");
}

struct ubsan_nonnull_return_data_v1 {
	struct source_location attr_location;
};

void __ubsan_handle_nonnull_return_v1(void *data_raw,
		struct source_location *loc)
{
	struct ubsan_nonnull_return_data_v1 *data =
		(struct ubsan_nonnull_return_data_v1 *) data_raw;
	(void) data;
	ubsan_log_location(loc, "null return");
}

struct ubsan_function_type_mismatch_data {
	struct source_location location;
	struct type_descriptor *type;
};

void __ubsan_handle_function_type_mismatch(void *data_raw,
				void *value_raw __unused)
{
	struct ubsan_function_type_mismatch_data *data =
		(struct ubsan_function_type_mismatch_data *) data_raw;

	ubsan_log_location(&data->location, "function type mismatch");
}

struct ubsan_float_cast_overflow_data {
	struct source_location location;
	struct type_descriptor *from_type;
	struct type_descriptor *to_type;
};

void __ubsan_handle_float_cast_overflow(void *data_raw,
				void *from_raw __unused)
{
	struct ubsan_float_cast_overflow_data *data =
		(struct ubsan_float_cast_overflow_data *) data_raw;

	ubsan_log_location(&data->location, "float cast overflow");
}

struct ubsan_unreachable_data {
	struct source_location location;
};

void __ubsan_handle_builtin_unreachable(void *data_raw)
{
	struct ubsan_unreachable_data *data =
		(struct ubsan_unreachable_data *) data_raw;

	ubsan_log_location(&data->location, "reached unreachable");
}

void __ubsan_handle_missing_return(void *data_raw) __noreturn;

void __ubsan_handle_missing_return(void *data_raw)
{
	struct ubsan_unreachable_data *data =
		(struct ubsan_unreachable_data *) data_raw;

	ubsan_log_location(&data->location, "missing return");
}

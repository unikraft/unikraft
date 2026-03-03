/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PCPUVAR_H__
#define __UK_PCPUVAR_H__

#include <uk/essentials.h>
#include <uk/config.h>
#include <uk/pcpuvar/arch.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* Defined by linker script */
extern char _uk_pcpuvar_base[];
extern char _uk_pcpuvar_align[];
extern char _uk_pcpuvar_tmpl_start[];
extern char _uk_pcpuvar_tmpl_end[];
extern char _uk_pcpuvar_tmpl_size[];

/**
 * Attribute for defining a variable as per-CPU
 */
#define __uk_pcpuvar							\
	__section(".uk_pcpuvar")

/** Hardware CPU ID for the current CPU */
extern __uk_pcpuvar __u64 uk_pcpuvar_cpu_id;

/** Linear index of the current CPU in the per-CPU array */
extern __uk_pcpuvar __u64 uk_pcpuvar_cpu_idx;

/**
 * Get lvalue reference to a specific CPU's copy of a per-CPU variable.
 *
 * @param _idx
 *   CPU index
 * @param _sym
 *   Symbol name
 * @return
 *   Lvalue reference (can read or write)
 */
#define __uk_pcpuvar_lval(_idx, _sym)					\
	(*(__typeof__(_sym) *)						\
	 ((__u8 *)&(_sym) + (((__sz)&_uk_pcpuvar_tmpl_size) * (_idx))))

#define _uk_pcpuvar_lval(_idx, _sym)					\
	__uk_pcpuvar_lval(_idx, _sym)

#define uk_pcpuvar_lval(_idx, _sym)					\
	_uk_pcpuvar_lval(_idx, _sym)

/**
 * Read current CPU's copy of a per-CPU variable.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Value of the variable
 */
#define _uk_pcpuvar_current_get(_sym)					\
	__uk_pcpuvar_arch_current_get(_sym)
#define uk_pcpuvar_current_get(_sym)					\
	_uk_pcpuvar_current_get(_sym)

/**
 * Write to current CPU's copy of a per-CPU variable.
 *
 * @param _sym
 *   Symbol name
 * @param _val
 *   Value to write
 */
#define _uk_pcpuvar_current_set(_sym, _val)				\
	__uk_pcpuvar_arch_current_set(_sym, _val)
#define uk_pcpuvar_current_set(_sym, _val)				\
	_uk_pcpuvar_current_set(_sym, _val)

/**
 * Read a struct member from current CPU's copy.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Value of the variable
 */
#define _uk_pcpuvar_current_member_get(_sym, _member)			\
	__uk_pcpuvar_arch_current_member_get(_sym, _member)
#define uk_pcpuvar_current_member_get(_sym, _member)			\
	_uk_pcpuvar_current_member_get(_sym, _member)

/**
 * Write to a struct member of current CPU's copy.
 *
 * @param _sym
 *   Symbol name
 * @param _val
 *   Value to write
 */
#define _uk_pcpuvar_current_member_set(_sym, _member, _val)		\
	__uk_pcpuvar_arch_current_member_set(_sym, _member, _val)
#define uk_pcpuvar_current_member_set(_sym, _member, _val)		\
	_uk_pcpuvar_current_member_set(_sym, _member, _val)

/**
 * Get pointer to current CPU's copy of a per-CPU variable.
 *
 * @param _sym
 *   Symbol name
 * @return
 *   Pointer to the variable
 */
#define _uk_pcpuvar_current_ptr_get(_sym)				\
	__uk_pcpuvar_arch_current_ptr_get(_sym)
#define uk_pcpuvar_current_ptr_get(_sym)				\
	_uk_pcpuvar_current_ptr_get(_sym)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PCPUVAR_H__ */

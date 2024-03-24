/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKNOFAULT_NOFAULT_H__
#define __UKNOFAULT_NOFAULT_H__

#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forces probing of the entire range. On a fault skips the offending page. */
#define UK_NOFAULTF_CONTINUE	0x01

/* Disables on-demand paging for the duration of the operation. Note that a
 * fault will still happen when attempting to access inaccessible memory. The
 * flag does not have any effect if on-demand paging is not available in the
 * system.
 */
#define UK_NOFAULTF_NOPAGING	0x02

/**
 * Probes the specified address range for read access. If the address
 * range cannot be fully accessed, the fault is catched and the faulting offset
 * is returned. Page faults may be resolved during the operation (e.g.,
 * on-demand paging). See the flags parameter to change the behavior.
 *
 * @param vaddr
 *   Start of address range
 * @param len
 *   Number of bytes to check
 * @param flags
 *   Probe flags (see UK_NOFAULTF_*)
 *
 * @return
 *   Number of bytes read. If the number is less than specified one or more
 *   faults occurred.
 */
__sz uk_nofault_probe_r(__vaddr_t vaddr, __sz len, unsigned long flags);

/**
 * Probes the specified address range for read/write access. If the address
 * range cannot be fully accessed, the fault is catched and the faulting offset
 * is returned. Page faults may be resolved during the operation (e.g.,
 * on-demand paging). See the flags parameter to change the behavior.
 *
 * @param vaddr
 *   Start of address range
 * @param len
 *   Number of bytes to check
 * @param flags
 *   Probe flags (see UK_NOFAULTF_*)
 *
 * @return
 *   Number of bytes read/written. If the number is less than specified one or
 *   more faults occurred.
 */
__sz uk_nofault_probe_rw(__vaddr_t vaddr, __sz len, unsigned long flags);

/**
 * Copies len bytes from src to dst. The buffers must not overlap. If either
 * the source or destination buffer cannot be fully accessed, the fault is
 * catched and the faulting offset is returned. Page faults may be resolved
 * during the operation (e.g., on-demand paging). See the flags parameter to
 * change the behavior.
 *
 * @param dst
 *   Destination buffer
 * @param src
 *   Source buffer
 * @param len
 *   Number of bytes to copy
 * @param flags
 *   Probe flags (see UK_NOFAULTF_*)
 *
 * @return
 *   Number of bytes copied. If the number is less than specified one or more
 *   faults occurred.
 */
__sz uk_nofault_memcpy(char *dst, const char *src, __sz len,
		       unsigned long flags);

/**
 * Tries to read the value from a given pointer
 *
 * @param var
 *   Variable to read the value into
 * @param ptr
 *   Memory address to read from
 *
 * @return
 *   non-zero value on success, 0 otherwise
 */
#define uk_nofault_try_read(v, ptr)					\
	(uk_nofault_memcpy((char *)&(v), (const char *)(ptr), sizeof(v), 0)\
		== sizeof(v))

/**
 * Tries to write a value to the given pointer
 *
 * @param value
 *   Value that should be written
 * @param ptr
 *   Memory address to write to
 *
 * @return
 *   non-zero value on success, 0 otherwise
 */
#define uk_nofault_try_write(value, ptr)				\
	({ typeof(value) _v = (value);					\
	   (uk_nofault_memcpy((char *)(ptr), (const char *)&_v, sizeof(_v), 0)\
		== sizeof(_v)); })

/**
 * Tries to read the value from a given pointer with on-demand paging disabled
 *
 * @param var
 *   Variable to read the value into
 * @param ptr
 *   Memory address to read from
 *
 * @return
 *   non-zero value on success, 0 otherwise
 */
#define uk_nofault_try_read_nopaging(v, ptr)				\
	(uk_nofault_memcpy((char *)&(v), (const char *)(ptr), sizeof(v),\
			   UK_NOFAULTF_NOPAGING) == sizeof(v))

/**
 * Tries to write a value to the given pointer with on-demand paging disabled
 *
 * @param value
 *   Value that should be written
 * @param ptr
 *   Memory address to write to
 *
 * @return
 *   non-zero value on success, 0 otherwise
 */
#define uk_nofault_try_write_nopaging(value, ptr)			\
	({ typeof(value) _v = (value);					\
	   (uk_nofault_memcpy((char *)(ptr), (const char *)&_v, sizeof(_v),\
			      UK_NOFAULTF_NOPAGING) == sizeof(_v)); })

#ifdef __cplusplus
}
#endif

#endif /* __UKNOFAULT_NOFAULT_H__ */

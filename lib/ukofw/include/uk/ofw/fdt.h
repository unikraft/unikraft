/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Jianyong Wu <Jianyong.Wu@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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
#ifndef __UK_OFW_FDT_H__
#define __UK_OFW_FDT_H__

#include <stdbool.h>
#include <libfdt.h>

#define FDT_BAD_ADDR (uint64_t)(-1)

/**
 * fdt_getprop_u32_by_offset - retrieve u32 of a given property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @out: pointer to u32 variable (will be overwritten) or NULL
 *
 * fdt_getprop_u32_by_offset() retrieves u32 to the value of the property
 * named 'name' of the node at offset nodeoffset (this will be a
 * pointer to within the device blob itself, not a copy of the value).
 * If out is non-NULL, the u32 of the property value is returned.
 *
 * returns:
 *	0, on success
 *		out contains the u32 of a given property at nodeoffset.
 *	-FDT_ERR_NOTFOUND, node does not have named property
 *	-FDT_ERR_BADNCELLS,
 */
int fdt_getprop_u32_by_offset(const void *fdt, int nodeoffset,
		const char *name, uint32_t *out);

/**
 * fdt_find_irq_parent_offset - find the irq parent offset
 * @fdt: pointer to the device tree blob
 * @offset: offset of the node whose irq parent to find
 *
 * fdt_find_irq_parent_offset() returns the offset of the irq parent
 * of given @offset
 *
 * returns:
 *	structure block offset of the located node (>= 0), on success
 *	-FDT_ERR_NOTFOUND, no node with that phandle exists
 *	-FDT_ERR_BADPHANDLE, given phandle value was invalid (0 or -1)
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_find_irq_parent_offset(const void *fdt, int offset);

/**
 * fdt_interrupt_cells - retrieve the number of cells needed to encode an
 *                       interrupt source
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the interrupt for.
 *
 * When the node has a valid #interrupt-cells property, returns its value.
 *
 * returns:
 *     0 <= n < FDT_MAX_NCELLS, on success
 *      -FDT_ERR_BADNCELLS, if the node has a badly formatted or invalid
 *             #interrupt-cells property
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE,
 *     -FDT_ERR_BADSTRUCTURE,
 *     -FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_interrupt_cells(const void *fdt, int nodeoffset);

/*
 * read and combine the big number of reg, caller needs to make sure size
 * is correct
 */
static inline uint64_t fdt_reg_read_number(const fdt32_t *regs, uint32_t size)
{
	uint64_t number = 0;

	for (uint32_t i = 0; i < size; i++) {
		number <<= 32;
		number |= fdt32_to_cpu(*regs);
		regs++;
	}

	return number;
}

/**
 * fdt_get_address - retrieve device address of a given index
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the address for.
 * @index: index of region
 * @addr: return the region address
 * @size: return the region size
 *
 * returns:
 *     0, on success
 *      -FDT_ERR_BADNCELLS, if the node has a badly formatted or invalid
 *             address property
 *      -FDT_ERR_NOTFOUND, if the node doesn't have address property
 *      -FDT_ERR_NOSPACE, if the node doesn't have address for index
 */
int fdt_get_address(const void *fdt, int nodeoffset, uint32_t index,
			uint64_t *addr, uint64_t *size);

/**
 * fdt_node_offset_by_compatible_list - find nodes with a given
 *                                     'compatible' list value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @compatibles: a list of 'compatible' string to match, should be ended
 * with NULL string.
 * fdt_node_offset_by_compatible_list() returns the offset of the
 * first matched node after startoffset, which has a 'compatible'
 * property which lists the given compatible string; or if
 * startoffset is -1, the very first such node in the tree.
 *
 * returns:
 *     structure block offset of the located node (>= 0, >startoffset),
 *              on success
 *     -FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *             tree after startoffset
 *     -FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE,
 *     -FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_compatible_list(const void *fdt, int startoffset,
					const char * const compatibles[]);

/**
 * fdt_node_offset_idx_by_compatible_list - find nodes with a given
 *                                     'compatible' list value, and return
 *                                     index of compatible array
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @compatibles: a list of 'compatible' string to match, should be ended
 * with NULL string.
 * @idx the index of compatible array
 * fdt_node_offset_idx_by_compatible_list() returns the offset of the
 * first matched node after startoffset, which has a 'compatible'
 * property which lists the given compatible string; or if
 * startoffset is -1, the very first such node in the tree.
 *
 * returns:
 *     structure block offset of the located node (>= 0, >startoffset),
 *              on success
 *     -FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *             tree after startoffset
 *     -FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE,
 *     -FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_idx_by_compatible_list(const void *fdt, int startoffset,
				const char * const compatibles[], int *index);

/**
 * fdt_node_check_compatible_list - checks whether a node's 'compatible'
 *                                  property matches a given list of
 *                                  strings.
 *
 * @fdt:         pointer to the device tree blob
 * @offset:      node offset
 * @compatibles: a list of 'compatible' strings to match, should be
 *               terminated with NULL.
 * @return
 *	0, match
 *	1, no match
 *	-FDT_ERR see by libfdt's fdt_node_check_compatible()
 */
int fdt_node_check_compatible_list(const void *fdt, int offset,
				   const char * const compatibles[]);

/**
 * fdt_get_interrupt - retrieve device interrupt of a given index
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the address for
 * @index: the index of interrupt we want to retrieve
 * @size: interrupt cell size in fdt32_t
 * @prop: return the pointer to property
 * returns:
 *     0 on success , < 0 on failed
 *     -FDT_ERR_NOTFOUND, node does not have named property
 *     -FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE,
 *     -FDT_ERR_BADSTRUCTURE,
 *     -FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_get_interrupt(const void *fdt, int nodeoffset,
				uint32_t index, int *size, fdt32_t **prop);

/**
 * fdt_prop_read_bool - Find a property
 * @fdt: pointer to the device tree blob
 * @start_offset: start offset of the node to find the address for
 * @propname:	name of the property to be searched.
 *
 * Search for a property in a device node.
 * Returns true if the property exists false otherwise.
 */
bool fdt_prop_read_bool(const void *fdt, int start_offset,
					 const char *propname);

/**
 * fdt_translate_address_by_ranges - Translate an address from the
 * device-tree into a CPU physical address, this walks up the tree and
 * applies the various bus mappings on the way.
 * @fdt: pointer to the device tree blob
 * @node_offset: start offset of the node to find the address for
 * @regs regs in device-tree
 */
uint64_t fdt_translate_address_by_ranges(const void *fdt,
				int node_offset, const fdt32_t *regs);

/**
 * Get dtb node pointed by /chosen/stdout-path
 *
 * @fdt: pointer to device-tree blob
 * @offs[out]: pointer to node pointed by stdout-path.
 * @opt[out]: console options specified in stdout-path. Set to NULL
 *            if no console options specified.
 * @return zero on success, negatibe value on failure
 */
int fdt_chosen_stdout_path(const void *fdt, int *offs, char **opt);

int fdt_chosen_rng_seed(const void *fdt, uint32_t **seed, size_t *seed_len);

#endif /* __UK_OFW_FDT_H__ */

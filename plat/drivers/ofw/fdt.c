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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <libfdt_env.h>
#include <fdt.h>
#include <libfdt.h>

#include <ofw/fdt.h>
#include <uk/print.h>
#include <uk/assert.h>

#define FDT_MAX_ADDR_CELLS FDT_MAX_NCELLS
#define FDT_CHECK_COUNTS(na, ns)  ((na) > 0 && (na) <= FDT_MAX_ADDR_CELLS && \
					(ns) > 0)

static int fdt_find_irq_parent_offset(const void *fdt, int offset)
{
	uint32_t irq_parent;

	do {
		/* Find the interrupt-parent phandle */
		if (!fdt_getprop_u32_by_offset(fdt, offset,
				"interrupt-parent", &irq_parent))
			break;

		/* Try to find in parent node */
		offset = fdt_parent_offset(fdt, offset);
	} while (offset >= 0);

	if (offset < 0)
		return offset;

	/* Get interrupt parent node by phandle */
	return fdt_node_offset_by_phandle(fdt, irq_parent);
}

int fdt_interrupt_cells(const void *fdt, int offset)
{
	int intc_offset;
	int val;
	int ret;

	intc_offset = fdt_find_irq_parent_offset(fdt, offset);
	if (intc_offset < 0)
		return intc_offset;

	ret = fdt_getprop_u32_by_offset(fdt, intc_offset, "#interrupt-cells",
					(uint32_t *)&val);
	if (ret < 0)
		return ret;

	if ((val <= 0) || (val > FDT_MAX_NCELLS))
		return -FDT_ERR_BADNCELLS;

	return val;
}

/* Default translator (generic bus) */
static void fdt_default_count_cells(const void *fdt, int parentoffset,
					       int *addrc, int *sizec)
{
	if (addrc)
		*addrc = fdt_address_cells(fdt, parentoffset);

	if (sizec)
		*sizec = fdt_size_cells(fdt, parentoffset);
}

static int fdt_default_translate(fdt32_t *addr, uint64_t offset, int na)
{
	uint64_t a = fdt_reg_read_number(addr, na);

	memset(addr, 0, na * sizeof(fdt32_t));
	a += offset;
	if (na > 1)
		addr[na - 2] = cpu_to_fdt32(a >> 32);
	addr[na - 1] = cpu_to_fdt32(a & 0xffffffffu);

	return 0;
}

static int fdt_translate_one(const void *fdt, int parent, fdt32_t *addr,
				    int na, int pna, const char *rprop)
{
	const fdt32_t *ranges;
	int rlen;
	uint64_t offset = FDT_BAD_ADDR;

	ranges = fdt_getprop(fdt, parent, rprop, &rlen);
	if (!ranges)
		return 1;
	if (rlen == 0) {
		offset = fdt_reg_read_number(addr, na);
		uk_pr_debug("empty ranges, 1:1 translation\n");
		goto finish;
	}

	uk_pr_err("Error, only 1:1 translation is supported...\n");
	return 1;
 finish:
	uk_pr_debug("with offset: 0x%lx\n", offset);

	/* Translate it into parent bus space */
	return fdt_default_translate(addr, offset, pna);
}

/*
 * Translate an address from the device-tree into a CPU physical address,
 * this walks up the tree and applies the various bus mappings on the
 * way.
 */
static uint64_t fdt_translate_address_by_ranges(const void *fdt,
					int node_offset, const fdt32_t *regs)
{
	int parent;
	fdt32_t addr[FDT_MAX_ADDR_CELLS];
	int na, ns, pna, pns;
	uint64_t result = FDT_BAD_ADDR;

	/* Get parent */
	parent = fdt_parent_offset(fdt, node_offset);
	if (parent < 0)
		goto bail;

	/* Count address cells & copy address locally */
	fdt_default_count_cells(fdt, parent, &na, &ns);
	if (!FDT_CHECK_COUNTS(na, ns)) {
		uk_pr_err("Bad cell count for %s\n",
		       fdt_get_name(fdt, node_offset, NULL));
		goto bail;
	}
	memcpy(addr, regs, na * 4);

	/* Translate */
	for (;;) {
		/* Switch to parent bus */
		node_offset = parent;
		parent = fdt_parent_offset(fdt, node_offset);

		/* If root, we have finished */
		if (parent < 0) {
			uk_pr_debug("reached root node\n");
			result = fdt_reg_read_number(addr, na);
			break;
		}

		/* Get new parent bus and counts */
		fdt_default_count_cells(fdt, parent, &pna, &pns);
		if (!FDT_CHECK_COUNTS(pna, pns)) {
			uk_pr_err("Bad cell count for %s\n",
				fdt_get_name(fdt, node_offset, NULL));
			break;
		}

		uk_pr_debug("parent bus (na=%d, ns=%d) on %s\n",
			 pna, pns, fdt_get_name(fdt, parent, NULL));

		/* Apply bus translation */
		if (fdt_translate_one(fdt, node_offset,
					addr, na, pna, "ranges"))
			break;

		/* Complete the move up one level */
		na = pna;
		ns = pns;
	}
bail:
	return result;
}

int fdt_get_address(const void *fdt, int nodeoffset, uint32_t index,
			uint64_t *addr, uint64_t *size)
{
	int parent;
	int len, prop_addr, prop_size;
	int naddr, nsize, term_size;
	const void *regs;

	UK_ASSERT(addr && size);

	/* Get address,size cell from parent */
	parent = fdt_parent_offset(fdt, nodeoffset);
	naddr = fdt_address_cells(fdt, parent);
	if (naddr < 0 || naddr >= FDT_MAX_NCELLS)
		return naddr;

	nsize = fdt_size_cells(fdt, parent);
	if (nsize < 0 || nsize >= FDT_MAX_NCELLS)
		return nsize;

	/* Get reg content */
	regs = fdt_getprop(fdt, nodeoffset, "reg", &len);
	if (regs == NULL)
		return len;

	term_size = sizeof(fdt32_t) * (nsize + naddr);
	prop_addr = term_size * index;
	prop_size = prop_addr + sizeof(fdt32_t) * naddr;

	/* The reg content must cover the reg term[index] at least */
	if (len < (prop_addr + term_size))
		return -FDT_ERR_NOSPACE;

	*size = fdt_reg_read_number(regs + prop_size, nsize);
	/* Handle ranges property, currently only 1:1 mapping is supported */
	*addr = fdt_translate_address_by_ranges(fdt, nodeoffset,
						regs + prop_addr);
	if (*addr == FDT_BAD_ADDR)
		return -FDT_ERR_NOTFOUND;
	return 0;
}

int fdt_node_offset_by_compatible_list(const void *fdt, int startoffset,
				  const char * const compatibles[])
{
	int idx, offset;

	for (idx = 0; compatibles[idx] != NULL; idx++) {
		offset = fdt_node_offset_by_compatible(fdt, startoffset,
				  compatibles[idx]);
		if (offset >= 0)
			return offset;
	}

	return -FDT_ERR_NOTFOUND;
}

int fdt_get_interrupt(const void *fdt, int nodeoffset,
			uint32_t index, int *size, fdt32_t **prop)
{
	int nintr, len, term_size;
	const void *regs;

	UK_ASSERT(size && prop);

	nintr = fdt_interrupt_cells(fdt, nodeoffset);
	if (nintr < 0 || nintr >= FDT_MAX_NCELLS)
		return -FDT_ERR_BADNCELLS;

	/* "interrupts-extended" is not supported */
	regs = fdt_getprop(fdt, nodeoffset, "interrupts-extended", &len);
	if (regs) {
		uk_pr_warn("interrupts multiple parents is not supported\n");
		return -FDT_ERR_INTERNAL;
	}

	/*
	 * Interrupt content must cover the index specific irq information.
	 */
	regs = fdt_getprop(fdt, nodeoffset, "interrupts", &len);
	term_size = sizeof(fdt32_t) * nintr;
	if (regs == NULL || (uint32_t)len < term_size * (index + 1))
		return -FDT_ERR_NOTFOUND;

	*size = nintr;
	*prop = (fdt32_t *)(regs + term_size * index);

	return 0;
}

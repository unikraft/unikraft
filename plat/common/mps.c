/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/plat/common/mps.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/print.h>

#define BIOS_ROM_STEP		16

struct uk_mps_mpfp *mpfp;
struct uk_mps_mpc *mpc;

static int uk_mps_get_checksum(void *buf, __sz len)
{
	const __u8 *const ptr_end = (__u8 *)buf + len;
	const __u8 *ptr = (__u8 *)buf;
	__u8 checksum = 0;

	while (ptr < ptr_end)
		checksum += *ptr++;

	return checksum;
}

int uk_mps_init(void)
{
	struct ukplat_memregion_desc *mrdp;
	__paddr_t ptr, start, end;

	ukplat_memregion_foreach(&mrdp, UKPLAT_MEMRT_LBIOS, 0, 0) {
		start = mrdp->pbase;
		end = mrdp->pbase + mrdp->len;

		for (ptr = start; ptr < end; ptr += BIOS_ROM_STEP)
			if (!memcmp((void *)ptr, UK_MPS_MPFP_SIG,
			    sizeof(UK_MPS_MPFP_SIG) - 1)) {
				uk_pr_debug("MPS MPFP present at %lx\n", ptr);
				mpfp = (struct uk_mps_mpfp *)ptr;
				break;
			}
	}

	UK_ASSERT(mpfp->tbl_len == 1 &&  /* One paragraph long */
		  !uk_mps_get_checksum(mpfp, UK_MPS_PARAGRAPH_LEN));

	/* Feature info byte 1 of mpfp tells which of the default MP system
	 * configurations we are dealing with. In this case, MPC must be 0.
	 */
	if (mpfp->feat_b1)
		return 0;

	mpc = (struct uk_mps_mpc *)(__uptr)mpfp->mpc_paddr;
	/* Revision must either be 4 (MPS 1.4) or 1 (MPS 1.1) */
	UK_ASSERT(mpc->revision == 0x4 || mpc->revision == 0x1);
	UK_ASSERT(!uk_mps_get_checksum(mpc, mpc->tbl_len));

	return 0;
}

struct uk_mps_mpfp *uk_mps_get_mpfp(void)
{
	return mpfp;
}

struct uk_mps_mpc *uk_mps_get_mpc(void)
{
	return mpc;
}

void uk_mps_next_mpc_entry(void **entryp)
{
	__u8 **entry = (__u8 **)entryp;

	UK_ASSERT(entryp);

	if (!mpc)
		*entry = NULL;

	switch (**entry) {
	case UK_MPS_MPC_TYPE_CPU:
		*entry += sizeof(struct uk_mps_cpu);
		break;
	case UK_MPS_MPC_TYPE_BUS:
		*entry += sizeof(struct uk_mps_bus);
		break;
	case UK_MPS_MPC_TYPE_IOAPIC:
		*entry += sizeof(struct uk_mps_ioapic);
		break;
	case UK_MPS_MPC_TYPE_INTSRC:
		*entry += sizeof(struct uk_mps_intsrc);
		break;
	case UK_MPS_MPC_TYPE_LINTSRC:
		*entry += sizeof(struct uk_mps_lintsrc);
		break;
	case UK_MPS_XMPC_TYPE_SYSMEM:
		*entry += sizeof(struct uk_mps_sysmem);
		break;
	case UK_MPS_XMPC_TYPE_BUSLINK:
		*entry += sizeof(struct uk_mps_buslink);
		break;
	case UK_MPS_XMPC_TYPE_BUSOVR:
		*entry += sizeof(struct uk_mps_busovr);
		break;
	default:
		*entry = NULL;
	}
}

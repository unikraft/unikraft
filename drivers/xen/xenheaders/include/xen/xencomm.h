/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) IBM Corp. 2006
 */

#ifndef _XEN_XENCOMM_H_
#define _XEN_XENCOMM_H_

/* A xencomm descriptor is a scatter/gather list containing physical
 * addresses corresponding to a virtually contiguous memory area. The
 * hypervisor translates these physical addresses to machine addresses to copy
 * to and from the virtually contiguous area.
 */

#define XENCOMM_MAGIC 0x58434F4D /* 'XCOM' */
#define XENCOMM_INVALID (~0UL)

struct xencomm_desc {
    uint32_t magic;
    uint32_t nr_addrs; /* the number of entries in address[] */
    uint64_t address[0];
};

#endif /* _XEN_XENCOMM_H_ */

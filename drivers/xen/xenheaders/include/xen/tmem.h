/* SPDX-License-Identifier: MIT */
/******************************************************************************
 * tmem.h
 *
 * Guest OS interface to Xen Transcendent Memory.
 *
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __XEN_PUBLIC_TMEM_H__
#define __XEN_PUBLIC_TMEM_H__

#include "xen.h"

#if __XEN_INTERFACE_VERSION__ < 0x00041300

/* version of ABI */
#define TMEM_SPEC_VERSION          1

#define TMEM_NEW_POOL              1
#define TMEM_DESTROY_POOL          2
#define TMEM_PUT_PAGE              4
#define TMEM_GET_PAGE              5
#define TMEM_FLUSH_PAGE            6
#define TMEM_FLUSH_OBJECT          7
#if __XEN_INTERFACE_VERSION__ < 0x00040400
#define TMEM_NEW_PAGE              3
#define TMEM_READ                  8
#define TMEM_WRITE                 9
#define TMEM_XCHG                 10
#endif

/* Privileged commands now called via XEN_SYSCTL_tmem_op. */
#define TMEM_AUTH                 101 /* as XEN_SYSCTL_TMEM_OP_SET_AUTH. */
#define TMEM_RESTORE_NEW          102 /* as XEN_SYSCTL_TMEM_OP_SET_POOL. */

/* Bits for HYPERVISOR_tmem_op(TMEM_NEW_POOL) */
#define TMEM_POOL_PERSIST          1
#define TMEM_POOL_SHARED           2
#define TMEM_POOL_PRECOMPRESSED    4
#define TMEM_POOL_PAGESIZE_SHIFT   4
#define TMEM_POOL_PAGESIZE_MASK  0xf
#define TMEM_POOL_VERSION_SHIFT   24
#define TMEM_POOL_VERSION_MASK  0xff
#define TMEM_POOL_RESERVED_BITS  0x00ffff00

/* Bits for client flags (save/restore) */
#define TMEM_CLIENT_COMPRESS       1
#define TMEM_CLIENT_FROZEN         2

/* Special errno values */
#define EFROZEN                 1000
#define EEMPTY                  1001

struct xen_tmem_oid {
    uint64_t oid[3];
};
typedef struct xen_tmem_oid xen_tmem_oid_t;
DEFINE_XEN_GUEST_HANDLE(xen_tmem_oid_t);

#ifndef __ASSEMBLY__
#if __XEN_INTERFACE_VERSION__ < 0x00040400
typedef xen_pfn_t tmem_cli_mfn_t;
#endif
typedef XEN_GUEST_HANDLE(char) tmem_cli_va_t;
struct tmem_op {
    uint32_t cmd;
    int32_t pool_id;
    union {
        struct {
            uint64_t uuid[2];
            uint32_t flags;
            uint32_t arg1;
        } creat; /* for cmd == TMEM_NEW_POOL. */
        struct {
#if __XEN_INTERFACE_VERSION__ < 0x00040600
            uint64_t oid[3];
#else
            xen_tmem_oid_t oid;
#endif
            uint32_t index;
            uint32_t tmem_offset;
            uint32_t pfn_offset;
            uint32_t len;
            xen_pfn_t cmfn; /* client machine page frame */
        } gen; /* for all other cmd ("generic") */
    } u;
};
typedef struct tmem_op tmem_op_t;
DEFINE_XEN_GUEST_HANDLE(tmem_op_t);
#endif

#endif  /* __XEN_INTERFACE_VERSION__ < 0x00041300 */

#endif /* __XEN_PUBLIC_TMEM_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

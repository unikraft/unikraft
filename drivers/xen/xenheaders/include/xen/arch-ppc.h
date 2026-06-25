/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) IBM Corp. 2005, 2006
 * Copyright (C) Raptor Engineering, LLC 2023
 *
 * Authors: Hollis Blanchard <hollisb@us.ibm.com>
 *          Timothy Pearson <tpearson@raptorengineering.com>
 *          Shawn Anastasio <sanastasio@raptorengineering.com>
 */

#ifndef __XEN_PUBLIC_ARCH_PPC_H__
#define __XEN_PUBLIC_ARCH_PPC_H__

#if defined(__XEN__) || defined(__XEN_TOOLS__)
#define  int64_aligned_t  int64_t __attribute__((__aligned__(8)))
#define uint64_aligned_t uint64_t __attribute__((__aligned__(8)))
#endif

#ifndef __ASSEMBLY__
#define ___DEFINE_XEN_GUEST_HANDLE(name, type)                  \
    typedef union { type *p; unsigned long q; }                 \
        __guest_handle_ ## name;                                \
    typedef union { type *p; uint64_aligned_t q; }              \
        __guest_handle_64_ ## name

#define __DEFINE_XEN_GUEST_HANDLE(name, type) \
    ___DEFINE_XEN_GUEST_HANDLE(name, type);   \
    ___DEFINE_XEN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_XEN_GUEST_HANDLE(name)   __DEFINE_XEN_GUEST_HANDLE(name, name)
#define __XEN_GUEST_HANDLE(name)        __guest_handle_64_ ## name
#define XEN_GUEST_HANDLE(name)          __XEN_GUEST_HANDLE(name)
#define XEN_GUEST_HANDLE_PARAM(name)    __guest_handle_ ## name
#define set_xen_guest_handle_raw(hnd, val)                  \
    do {                                                    \
        __typeof__(&(hnd)) sxghr_tmp_ = &(hnd);             \
        sxghr_tmp_->q = 0;                                  \
        sxghr_tmp_->p = (val);                              \
    } while ( 0 )
#define set_xen_guest_handle(hnd, val) set_xen_guest_handle_raw(hnd, val)

#ifdef __XEN_TOOLS__
#define get_xen_guest_handle(val, hnd)  do { val = (hnd).p; } while (0)
#endif

typedef uint64_t xen_pfn_t;
#define PRI_xen_pfn PRIx64
#define PRIu_xen_pfn PRIu64

/*
 * Maximum number of virtual CPUs in legacy multi-processor guests.
 * Only one. All other VCPUS must use VCPUOP_register_vcpu_info.
 */
#define XEN_LEGACY_MAX_VCPUS 1

typedef uint64_t xen_ulong_t;
#define PRI_xen_ulong PRIx64

/*
 * User-accessible registers: most of these need to be saved/restored
 * for every nested Xen invocation.
 */
struct vcpu_guest_core_regs
{
    uint64_t gprs[32];
    uint64_t lr;
    uint64_t ctr;
    uint64_t srr0;
    uint64_t srr1;
    uint64_t pc;
    uint64_t msr;
    uint64_t fpscr;             /* XXX Is this necessary */
    uint64_t xer;
    uint64_t hid4;              /* debug only */
    uint64_t dar;               /* debug only */
    uint32_t dsisr;             /* debug only */
    uint32_t cr;
    uint32_t __pad;             /* good spot for another 32bit reg */
    uint32_t entry_vector;
};
typedef struct vcpu_guest_core_regs vcpu_guest_core_regs_t;

typedef uint64_t tsc_timestamp_t; /* RDTSC timestamp */ /* XXX timebase */

/* ONLY used to communicate with dom0! See also struct exec_domain. */
struct vcpu_guest_context {
    vcpu_guest_core_regs_t user_regs;         /* User-level CPU registers     */
    uint64_t sdr1;                     /* Pagetable base               */
    /* XXX etc */
};
typedef struct vcpu_guest_context vcpu_guest_context_t;
DEFINE_XEN_GUEST_HANDLE(vcpu_guest_context_t);

struct arch_shared_info {
    uint64_t boot_timebase;
};

struct arch_vcpu_info {
};

struct xen_arch_domainconfig {
};

typedef struct xen_pmu_arch { uint8_t dummy; } xen_pmu_arch_t;

#endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_ARCH_PPC_H__ */

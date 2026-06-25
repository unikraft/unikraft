/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
 */

#ifndef __XEN_PUBLIC_PMU_H__
#define __XEN_PUBLIC_PMU_H__

#include "xen.h"
#if defined(__i386__) || defined(__x86_64__)
#include "arch-x86/pmu.h"
#elif defined (__arm__) || defined (__aarch64__)
#include "arch-arm.h"
#elif defined (__powerpc64__)
#include "arch-ppc.h"
#elif defined(__riscv)
#include "arch-riscv.h"
#else
#error "Unsupported architecture"
#endif

#define XENPMU_VER_MAJ    0
#define XENPMU_VER_MIN    1

/*
 * ` enum neg_errnoval
 * ` HYPERVISOR_xenpmu_op(enum xenpmu_op cmd, struct xenpmu_params *args);
 *
 * @cmd  == XENPMU_* (PMU operation)
 * @args == struct xenpmu_params
 */
/* ` enum xenpmu_op { */
#define XENPMU_mode_get        0 /* Also used for getting PMU version */
#define XENPMU_mode_set        1
#define XENPMU_feature_get     2
#define XENPMU_feature_set     3
#define XENPMU_init            4
#define XENPMU_finish          5
#define XENPMU_lvtpc_set       6
#define XENPMU_flush           7 /* Write cached MSR values to HW     */
/* ` } */

/* Parameters structure for HYPERVISOR_xenpmu_op call */
struct xen_pmu_params {
    /* IN/OUT parameters */
    struct {
        uint32_t maj;
        uint32_t min;
    } version;
    uint64_t val;

    /* IN parameters */
    uint32_t vcpu;
    uint32_t pad;
};
typedef struct xen_pmu_params xen_pmu_params_t;
DEFINE_XEN_GUEST_HANDLE(xen_pmu_params_t);

/* PMU modes:
 * - XENPMU_MODE_OFF:   No PMU virtualization
 * - XENPMU_MODE_SELF:  Guests can profile themselves
 * - XENPMU_MODE_HV:    Guests can profile themselves, dom0 profiles
 *                      itself and Xen
 * - XENPMU_MODE_ALL:   Only dom0 has access to VPMU and it profiles
 *                      everyone: itself, the hypervisor and the guests.
 */
#define XENPMU_MODE_OFF           0
#define XENPMU_MODE_SELF          (1<<0)
#define XENPMU_MODE_HV            (1<<1)
#define XENPMU_MODE_ALL           (1<<2)

/*
 * PMU features:
 * - XENPMU_FEATURE_INTEL_BTS:  Intel BTS support (ignored on AMD)
 * - XENPMU_FEATURE_IPC_ONLY:   Restrict PMCs to the most minimum set possible.
 *                              Instructions, cycles, and ref cycles. Can be
 *                              used to calculate instructions-per-cycle (IPC)
 *                              (ignored on AMD).
 * - XENPMU_FEATURE_ARCH_ONLY:  Restrict PMCs to the Intel Pre-Defined
 *                              Architectural Performance Events exposed by
 *                              cpuid and listed in the Intel developer's manual
 *                              (ignored on AMD).
 */
#define XENPMU_FEATURE_INTEL_BTS  (1<<0)
#define XENPMU_FEATURE_IPC_ONLY   (1<<1)
#define XENPMU_FEATURE_ARCH_ONLY  (1<<2)

/*
 * Shared PMU data between hypervisor and PV(H) domains.
 *
 * The hypervisor fills out this structure during PMU interrupt and sends an
 * interrupt to appropriate VCPU.
 * Architecture-independent fields of xen_pmu_data are WO for the hypervisor
 * and RO for the guest but some fields in xen_pmu_arch can be writable
 * by both the hypervisor and the guest (see arch-$arch/pmu.h).
 */
struct xen_pmu_data {
    /* Interrupted VCPU */
    uint32_t vcpu_id;

    /*
     * Physical processor on which the interrupt occurred. On non-privileged
     * guests set to vcpu_id;
     */
    uint32_t pcpu_id;

    /*
     * Domain that was interrupted. On non-privileged guests set to DOMID_SELF.
     * On privileged guests can be DOMID_SELF, DOMID_XEN, or, when in
     * XENPMU_MODE_ALL mode, domain ID of another domain.
     */
    domid_t  domain_id;

    uint8_t pad[6];

    /* Architecture-specific information */
    xen_pmu_arch_t pmu;
};

#endif /* __XEN_PUBLIC_PMU_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

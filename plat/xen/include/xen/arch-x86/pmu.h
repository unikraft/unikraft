/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
 */

#ifndef __XEN_PUBLIC_ARCH_X86_PMU_H__
#define __XEN_PUBLIC_ARCH_X86_PMU_H__

/* x86-specific PMU definitions */

/* AMD PMU registers and structures */
struct xen_pmu_amd_ctxt {
    /*
     * Offsets to counter and control MSRs (relative to xen_pmu_arch.c.amd).
     * For PV(H) guests these fields are RO.
     */
    __u32 counters;
    __u32 ctrls;

    /* Counter MSRs */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    __u64 regs[];
#elif defined(__GNUC__)
    __u64 regs[0];
#endif
};
typedef struct xen_pmu_amd_ctxt xen_pmu_amd_ctxt_t;
DEFINE_XEN_GUEST_HANDLE(xen_pmu_amd_ctxt_t);

/* Intel PMU registers and structures */
struct xen_pmu_cntr_pair {
    __u64 counter;
    __u64 control;
};
typedef struct xen_pmu_cntr_pair xen_pmu_cntr_pair_t;
DEFINE_XEN_GUEST_HANDLE(xen_pmu_cntr_pair_t);

struct xen_pmu_intel_ctxt {
   /*
    * Offsets to fixed and architectural counter MSRs (relative to
    * xen_pmu_arch.c.intel).
    * For PV(H) guests these fields are RO.
    */
    __u32 fixed_counters;
    __u32 arch_counters;

    /* PMU registers */
    __u64 global_ctrl;
    __u64 global_ovf_ctrl;
    __u64 global_status;
    __u64 fixed_ctrl;
    __u64 ds_area;
    __u64 pebs_enable;
    __u64 debugctl;

    /* Fixed and architectural counter MSRs */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    __u64 regs[];
#elif defined(__GNUC__)
    __u64 regs[0];
#endif
};
typedef struct xen_pmu_intel_ctxt xen_pmu_intel_ctxt_t;
DEFINE_XEN_GUEST_HANDLE(xen_pmu_intel_ctxt_t);

/* Sampled domain's registers */
struct xen_pmu_regs {
    __u64 ip;
    __u64 sp;
    __u64 flags;
    __u16 cs;
    __u16 ss;
    __u8 cpl;
    __u8 pad[3];
};
typedef struct xen_pmu_regs xen_pmu_regs_t;
DEFINE_XEN_GUEST_HANDLE(xen_pmu_regs_t);

/* PMU flags */
#define PMU_CACHED         (1<<0) /* PMU MSRs are cached in the context */
#define PMU_SAMPLE_USER    (1<<1) /* Sample is from user or kernel mode */
#define PMU_SAMPLE_REAL    (1<<2) /* Sample is from realmode */
#define PMU_SAMPLE_PV      (1<<3) /* Sample from a PV guest */

/*
 * Architecture-specific information describing state of the processor at
 * the time of PMU interrupt.
 * Fields of this structure marked as RW for guest should only be written by
 * the guest when PMU_CACHED bit in pmu_flags is set (which is done by the
 * hypervisor during PMU interrupt). Hypervisor will read updated data in
 * XENPMU_flush hypercall and clear PMU_CACHED bit.
 */
struct xen_pmu_arch {
    union {
        /*
         * Processor's registers at the time of interrupt.
         * WO for hypervisor, RO for guests.
         */
        struct xen_pmu_regs regs;
        /* Padding for adding new registers to xen_pmu_regs in the future */
#define XENPMU_REGS_PAD_SZ  64
        __u8 pad[XENPMU_REGS_PAD_SZ];
    } r;

    /* WO for hypervisor, RO for guest */
    __u64 pmu_flags;

    /*
     * APIC LVTPC register.
     * RW for both hypervisor and guest.
     * Only APIC_LVT_MASKED bit is loaded by the hypervisor into hardware
     * during XENPMU_flush or XENPMU_lvtpc_set.
     */
    union {
        __u32 lapic_lvtpc;
        __u64 pad;
    } l;

    /*
     * Vendor-specific PMU registers.
     * RW for both hypervisor and guest (see exceptions above).
     * Guest's updates to this field are verified and then loaded by the
     * hypervisor into hardware during XENPMU_flush
     */
    union {
        struct xen_pmu_amd_ctxt amd;
        struct xen_pmu_intel_ctxt intel;

        /*
         * Padding for contexts (fixed parts only, does not include MSR banks
         * that are specified by offsets)
         */
#define XENPMU_CTXT_PAD_SZ  128
        __u8 pad[XENPMU_CTXT_PAD_SZ];
    } c;
};
typedef struct xen_pmu_arch xen_pmu_arch_t;
DEFINE_XEN_GUEST_HANDLE(xen_pmu_arch_t);

#endif /* __XEN_PUBLIC_ARCH_X86_PMU_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */


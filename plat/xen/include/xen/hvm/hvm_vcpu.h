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
 * Copyright (c) 2015, Roger Pau Monne <roger.pau@citrix.com>
 */

#ifndef __XEN_PUBLIC_HVM_HVM_VCPU_H__
#define __XEN_PUBLIC_HVM_HVM_VCPU_H__

#include "../xen.h"

struct vcpu_hvm_x86_32 {
    __u32 eax;
    __u32 ecx;
    __u32 edx;
    __u32 ebx;
    __u32 esp;
    __u32 ebp;
    __u32 esi;
    __u32 edi;
    __u32 eip;
    __u32 eflags;

    __u32 cr0;
    __u32 cr3;
    __u32 cr4;

    __u32 pad1;

    /*
     * EFER should only be used to set the NXE bit (if required)
     * when starting a vCPU in 32bit mode with paging enabled or
     * to set the LME/LMA bits in order to start the vCPU in
     * compatibility mode.
     */
    __u64 efer;

    __u32 cs_base;
    __u32 ds_base;
    __u32 ss_base;
    __u32 es_base;
    __u32 tr_base;
    __u32 cs_limit;
    __u32 ds_limit;
    __u32 ss_limit;
    __u32 es_limit;
    __u32 tr_limit;
    __u16 cs_ar;
    __u16 ds_ar;
    __u16 ss_ar;
    __u16 es_ar;
    __u16 tr_ar;

    __u16 pad2[3];
};

/*
 * The layout of the _ar fields of the segment registers is the
 * following:
 *
 * Bits   [0,3]: type (bits 40-43).
 * Bit        4: s    (descriptor type, bit 44).
 * Bit    [5,6]: dpl  (descriptor privilege level, bits 45-46).
 * Bit        7: p    (segment-present, bit 47).
 * Bit        8: avl  (available for system software, bit 52).
 * Bit        9: l    (64-bit code segment, bit 53).
 * Bit       10: db   (meaning depends on the segment, bit 54).
 * Bit       11: g    (granularity, bit 55)
 * Bits [12,15]: unused, must be blank.
 *
 * A more complete description of the meaning of this fields can be
 * obtained from the Intel SDM, Volume 3, section 3.4.5.
 */

struct vcpu_hvm_x86_64 {
    __u64 rax;
    __u64 rcx;
    __u64 rdx;
    __u64 rbx;
    __u64 rsp;
    __u64 rbp;
    __u64 rsi;
    __u64 rdi;
    __u64 rip;
    __u64 rflags;

    __u64 cr0;
    __u64 cr3;
    __u64 cr4;
    __u64 efer;

    /*
     * Using VCPU_HVM_MODE_64B implies that the vCPU is launched
     * directly in long mode, so the cached parts of the segment
     * registers get set to match that environment.
     *
     * If the user wants to launch the vCPU in compatibility mode
     * the 32-bit structure should be used instead.
     */
};

struct vcpu_hvm_context {
#define VCPU_HVM_MODE_32B 0  /* 32bit fields of the structure will be used. */
#define VCPU_HVM_MODE_64B 1  /* 64bit fields of the structure will be used. */
    __u32 mode;

    __u32 pad;

    /* CPU registers. */
    union {
        struct vcpu_hvm_x86_32 x86_32;
        struct vcpu_hvm_x86_64 x86_64;
    } cpu_regs;
};
typedef struct vcpu_hvm_context vcpu_hvm_context_t;
DEFINE_XEN_GUEST_HANDLE(vcpu_hvm_context_t);

#endif /* __XEN_PUBLIC_HVM_HVM_VCPU_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

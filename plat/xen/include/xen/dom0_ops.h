/******************************************************************************
 * dom0_ops.h
 * 
 * Process command requests from domain-0 guest OS.
 * 
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
 * Copyright (c) 2002-2003, B Dragovic
 * Copyright (c) 2002-2006, K Fraser
 */

#ifndef __XEN_PUBLIC_DOM0_OPS_H__
#define __XEN_PUBLIC_DOM0_OPS_H__

#include "xen.h"
#include "platform.h"

#if __XEN_INTERFACE_VERSION__ >= 0x00030204
#error "dom0_ops.h is a compatibility interface only"
#endif

#define DOM0_INTERFACE_VERSION XENPF_INTERFACE_VERSION

#define DOM0_SETTIME          XENPF_settime
#define dom0_settime          xenpf_settime
#define dom0_settime_t        xenpf_settime_t

#define DOM0_ADD_MEMTYPE      XENPF_add_memtype
#define dom0_add_memtype      xenpf_add_memtype
#define dom0_add_memtype_t    xenpf_add_memtype_t

#define DOM0_DEL_MEMTYPE      XENPF_del_memtype
#define dom0_del_memtype      xenpf_del_memtype
#define dom0_del_memtype_t    xenpf_del_memtype_t

#define DOM0_READ_MEMTYPE     XENPF_read_memtype
#define dom0_read_memtype     xenpf_read_memtype
#define dom0_read_memtype_t   xenpf_read_memtype_t

#define DOM0_MICROCODE        XENPF_microcode_update
#define dom0_microcode        xenpf_microcode_update
#define dom0_microcode_t      xenpf_microcode_update_t

#define DOM0_PLATFORM_QUIRK   XENPF_platform_quirk
#define dom0_platform_quirk   xenpf_platform_quirk
#define dom0_platform_quirk_t xenpf_platform_quirk_t

typedef __u64 cpumap_t;

/* Unsupported legacy operation -- defined for API compatibility. */
#define DOM0_MSR                 15
struct dom0_msr {
    /* IN variables. */
    __u32 write;
    cpumap_t cpu_mask;
    __u32 msr;
    __u32 in1;
    __u32 in2;
    /* OUT variables. */
    __u32 out1;
    __u32 out2;
};
typedef struct dom0_msr dom0_msr_t;
DEFINE_XEN_GUEST_HANDLE(dom0_msr_t);

/* Unsupported legacy operation -- defined for API compatibility. */
#define DOM0_PHYSICAL_MEMORY_MAP 40
struct dom0_memory_map_entry {
    __u64 start, end;
    __u32 flags; /* reserved */
    __u8  is_ram;
};
typedef struct dom0_memory_map_entry dom0_memory_map_entry_t;
DEFINE_XEN_GUEST_HANDLE(dom0_memory_map_entry_t);

struct dom0_op {
    __u32 cmd;
    __u32 interface_version; /* DOM0_INTERFACE_VERSION */
    union {
        struct dom0_msr               msr;
        struct dom0_settime           settime;
        struct dom0_add_memtype       add_memtype;
        struct dom0_del_memtype       del_memtype;
        struct dom0_read_memtype      read_memtype;
        struct dom0_microcode         microcode;
        struct dom0_platform_quirk    platform_quirk;
        struct dom0_memory_map_entry  physical_memory_map;
        __u8                       pad[128];
    } u;
};
typedef struct dom0_op dom0_op_t;
DEFINE_XEN_GUEST_HANDLE(dom0_op_t);

#endif /* __XEN_PUBLIC_DOM0_OPS_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

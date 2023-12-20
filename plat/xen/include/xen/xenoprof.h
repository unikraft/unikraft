/******************************************************************************
 * xenoprof.h
 * 
 * Interface for enabling system wide profiling based on hardware performance
 * counters
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
 * Copyright (C) 2005 Hewlett-Packard Co.
 * Written by Aravind Menon & Jose Renato Santos
 */

#ifndef __XEN_PUBLIC_XENOPROF_H__
#define __XEN_PUBLIC_XENOPROF_H__

#include "xen.h"

/*
 * Commands to HYPERVISOR_xenoprof_op().
 */
#define XENOPROF_init                0
#define XENOPROF_reset_active_list   1
#define XENOPROF_reset_passive_list  2
#define XENOPROF_set_active          3
#define XENOPROF_set_passive         4
#define XENOPROF_reserve_counters    5
#define XENOPROF_counter             6
#define XENOPROF_setup_events        7
#define XENOPROF_enable_virq         8
#define XENOPROF_start               9
#define XENOPROF_stop               10
#define XENOPROF_disable_virq       11
#define XENOPROF_release_counters   12
#define XENOPROF_shutdown           13
#define XENOPROF_get_buffer         14
#define XENOPROF_set_backtrace      15

/* AMD IBS support */
#define XENOPROF_get_ibs_caps       16
#define XENOPROF_ibs_counter        17
#define XENOPROF_last_op            17

#define MAX_OPROF_EVENTS    32
#define MAX_OPROF_DOMAINS   25
#define XENOPROF_CPU_TYPE_SIZE 64

/* Xenoprof performance events (not Xen events) */
struct event_log {
    __u64 eip;
    __u8 mode;
    __u8 event;
};

/* PC value that indicates a special code */
#define XENOPROF_ESCAPE_CODE (~xen_mk_ullong(0))
/* Transient events for the xenoprof->oprofile cpu buf */
#define XENOPROF_TRACE_BEGIN 1

/* Xenoprof buffer shared between Xen and domain - 1 per VCPU */
struct xenoprof_buf {
    __u32 event_head;
    __u32 event_tail;
    __u32 event_size;
    __u32 vcpu_id;
    __u64 xen_samples;
    __u64 kernel_samples;
    __u64 user_samples;
    __u64 lost_samples;
    struct event_log event_log[1];
};
#ifndef __XEN__
typedef struct xenoprof_buf xenoprof_buf_t;
DEFINE_XEN_GUEST_HANDLE(xenoprof_buf_t);
#endif

struct xenoprof_init {
    __s32  num_events;
    __s32  is_primary;
    char cpu_type[XENOPROF_CPU_TYPE_SIZE];
};
typedef struct xenoprof_init xenoprof_init_t;
DEFINE_XEN_GUEST_HANDLE(xenoprof_init_t);

struct xenoprof_get_buffer {
    __s32  max_samples;
    __s32  nbuf;
    __s32  bufsize;
    __u64 buf_gmaddr;
};
typedef struct xenoprof_get_buffer xenoprof_get_buffer_t;
DEFINE_XEN_GUEST_HANDLE(xenoprof_get_buffer_t);

struct xenoprof_counter {
    __u32 ind;
    __u64 count;
    __u32 enabled;
    __u32 event;
    __u32 hypervisor;
    __u32 kernel;
    __u32 user;
    __u64 unit_mask;
};
typedef struct xenoprof_counter xenoprof_counter_t;
DEFINE_XEN_GUEST_HANDLE(xenoprof_counter_t);

typedef struct xenoprof_passive {
    __u16 domain_id;
    __s32  max_samples;
    __s32  nbuf;
    __s32  bufsize;
    __u64 buf_gmaddr;
} xenoprof_passive_t;
DEFINE_XEN_GUEST_HANDLE(xenoprof_passive_t);

struct xenoprof_ibs_counter {
    __u64 op_enabled;
    __u64 fetch_enabled;
    __u64 max_cnt_fetch;
    __u64 max_cnt_op;
    __u64 rand_en;
    __u64 dispatched_ops;
};
typedef struct xenoprof_ibs_counter xenoprof_ibs_counter_t;
DEFINE_XEN_GUEST_HANDLE(xenoprof_ibs_counter_t);

#endif /* __XEN_PUBLIC_XENOPROF_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

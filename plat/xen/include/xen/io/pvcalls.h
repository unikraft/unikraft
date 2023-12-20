/*
 * pvcalls.h -- Xen PV Calls Protocol
 *
 * Refer to docs/misc/pvcalls.markdown for the specification
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
 * Copyright (C) 2017 Stefano Stabellini <stefano@aporeto.com>
 */

#ifndef __XEN_PUBLIC_IO_PVCALLS_H__
#define __XEN_PUBLIC_IO_PVCALLS_H__

#include "../grant_table.h"
#include "ring.h"

/*
 * See docs/misc/pvcalls.markdown in xen.git for the full specification:
 * https://xenbits.xen.org/docs/unstable/misc/pvcalls.html
 */
struct pvcalls_data_intf {
    RING_IDX in_cons, in_prod, in_error;

    __u8 pad1[52];

    RING_IDX out_cons, out_prod, out_error;

    __u8 pad2[52];

    RING_IDX ring_order;
    grant_ref_t ref[];
};
DEFINE_XEN_FLEX_RING(pvcalls);

#define PVCALLS_SOCKET         0
#define PVCALLS_CONNECT        1
#define PVCALLS_RELEASE        2
#define PVCALLS_BIND           3
#define PVCALLS_LISTEN         4
#define PVCALLS_ACCEPT         5
#define PVCALLS_POLL           6

struct xen_pvcalls_request {
    __u32 req_id; /* private to guest, echoed in response */
    __u32 cmd;    /* command to execute */
    union {
        struct xen_pvcalls_socket {
            __u64 id;
            __u32 domain;
            __u32 type;
            __u32 protocol;
        } socket;
        struct xen_pvcalls_connect {
            __u64 id;
            __u8 addr[28];
            __u32 len;
            __u32 flags;
            grant_ref_t ref;
            __u32 evtchn;
        } connect;
        struct xen_pvcalls_release {
            __u64 id;
            __u8 reuse;
        } release;
        struct xen_pvcalls_bind {
            __u64 id;
            __u8 addr[28];
            __u32 len;
        } bind;
        struct xen_pvcalls_listen {
            __u64 id;
            __u32 backlog;
        } listen;
        struct xen_pvcalls_accept {
            __u64 id;
            __u64 id_new;
            grant_ref_t ref;
            __u32 evtchn;
        } accept;
        struct xen_pvcalls_poll {
            __u64 id;
        } poll;
        /* dummy member to force sizeof(struct xen_pvcalls_request)
         * to match across archs */
        struct xen_pvcalls_dummy {
            __u8 dummy[56];
        } dummy;
    } u;
};

struct xen_pvcalls_response {
    __u32 req_id;
    __u32 cmd;
    __s32 ret;
    __u32 pad;
    union {
        struct _xen_pvcalls_socket {
            __u64 id;
        } socket;
        struct _xen_pvcalls_connect {
            __u64 id;
        } connect;
        struct _xen_pvcalls_release {
            __u64 id;
        } release;
        struct _xen_pvcalls_bind {
            __u64 id;
        } bind;
        struct _xen_pvcalls_listen {
            __u64 id;
        } listen;
        struct _xen_pvcalls_accept {
            __u64 id;
        } accept;
        struct _xen_pvcalls_poll {
            __u64 id;
        } poll;
        struct _xen_pvcalls_dummy {
            __u8 dummy[8];
        } dummy;
    } u;
};

DEFINE_RING_TYPES(xen_pvcalls, struct xen_pvcalls_request,
                  struct xen_pvcalls_response);

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

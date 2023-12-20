/*
 *  This file contains the flask_op hypercall commands and definitions.
 *
 *  Author:  George Coker, <gscoker@alpha.ncsc.mil>
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
 */

#ifndef __FLASK_OP_H__
#define __FLASK_OP_H__

#include "../event_channel.h"

#define XEN_FLASK_INTERFACE_VERSION 1

struct xen_flask_load {
    XEN_GUEST_HANDLE(char) buffer;
    __u32 size;
};

struct xen_flask_setenforce {
    __u32 enforcing;
};

struct xen_flask_sid_context {
    /* IN/OUT: sid to convert to/from string */
    __u32 sid;
    /* IN: size of the context buffer
     * OUT: actual size of the output context string
     */
    __u32 size;
    XEN_GUEST_HANDLE(char) context;
};

struct xen_flask_access {
    /* IN: access request */
    __u32 ssid;
    __u32 tsid;
    __u32 tclass;
    __u32 req;
    /* OUT: AVC data */
    __u32 allowed;
    __u32 audit_allow;
    __u32 audit_deny;
    __u32 seqno;
};

struct xen_flask_transition {
    /* IN: transition SIDs and class */
    __u32 ssid;
    __u32 tsid;
    __u32 tclass;
    /* OUT: new SID */
    __u32 newsid;
};

#if __XEN_INTERFACE_VERSION__ < 0x00040800
struct xen_flask_userlist {
    /* IN: starting SID for list */
    __u32 start_sid;
    /* IN: size of user string and output buffer
     * OUT: number of SIDs returned */
    __u32 size;
    union {
        /* IN: user to enumerate SIDs */
        XEN_GUEST_HANDLE(char) user;
        /* OUT: SID list */
        XEN_GUEST_HANDLE(__u32) sids;
    } u;
};
#endif

struct xen_flask_boolean {
    /* IN/OUT: numeric identifier for boolean [GET/SET]
     * If -1, name will be used and bool_id will be filled in. */
    __u32 bool_id;
    /* OUT: current enforcing value of boolean [GET/SET] */
    __u8 enforcing;
    /* OUT: pending value of boolean [GET/SET] */
    __u8 pending;
    /* IN: new value of boolean [SET] */
    __u8 new_value;
    /* IN: commit new value instead of only setting pending [SET] */
    __u8 commit;
    /* IN: size of boolean name buffer [GET/SET]
     * OUT: actual size of name [GET only] */
    __u32 size;
    /* IN: if bool_id is -1, used to find boolean [GET/SET]
     * OUT: textual name of boolean [GET only]
     */
    XEN_GUEST_HANDLE(char) name;
};

struct xen_flask_setavc_threshold {
    /* IN */
    __u32 threshold;
};

struct xen_flask_hash_stats {
    /* OUT */
    __u32 entries;
    __u32 buckets_used;
    __u32 buckets_total;
    __u32 max_chain_len;
};

struct xen_flask_cache_stats {
    /* IN */
    __u32 cpu;
    /* OUT */
    __u32 lookups;
    __u32 hits;
    __u32 misses;
    __u32 allocations;
    __u32 reclaims;
    __u32 frees;
};

struct xen_flask_ocontext {
    /* IN */
    __u32 ocon;
    __u32 sid;
    __u64 low, high;
};

struct xen_flask_peersid {
    /* IN */
    evtchn_port_t evtchn;
    /* OUT */
    __u32 sid;
};

struct xen_flask_relabel {
    /* IN */
    __u32 domid;
    __u32 sid;
};

struct xen_flask_devicetree_label {
    /* IN */
    __u32 sid;
    __u32 length;
    XEN_GUEST_HANDLE(char) path;
};

struct xen_flask_op {
    __u32 cmd;
#define FLASK_LOAD              1
#define FLASK_GETENFORCE        2
#define FLASK_SETENFORCE        3
#define FLASK_CONTEXT_TO_SID    4
#define FLASK_SID_TO_CONTEXT    5
#define FLASK_ACCESS            6
#define FLASK_CREATE            7
#define FLASK_RELABEL           8
#define FLASK_USER              9  /* No longer implemented */
#define FLASK_POLICYVERS        10
#define FLASK_GETBOOL           11
#define FLASK_SETBOOL           12
#define FLASK_COMMITBOOLS       13
#define FLASK_MLS               14
#define FLASK_DISABLE           15
#define FLASK_GETAVC_THRESHOLD  16
#define FLASK_SETAVC_THRESHOLD  17
#define FLASK_AVC_HASHSTATS     18
#define FLASK_AVC_CACHESTATS    19
#define FLASK_MEMBER            20
#define FLASK_ADD_OCONTEXT      21
#define FLASK_DEL_OCONTEXT      22
#define FLASK_GET_PEER_SID      23
#define FLASK_RELABEL_DOMAIN    24
#define FLASK_DEVICETREE_LABEL  25
    __u32 interface_version; /* XEN_FLASK_INTERFACE_VERSION */
    union {
        struct xen_flask_load load;
        struct xen_flask_setenforce enforce;
        /* FLASK_CONTEXT_TO_SID and FLASK_SID_TO_CONTEXT */
        struct xen_flask_sid_context sid_context;
        struct xen_flask_access access;
        /* FLASK_CREATE, FLASK_RELABEL, FLASK_MEMBER */
        struct xen_flask_transition transition;
#if __XEN_INTERFACE_VERSION__ < 0x00040800
        struct xen_flask_userlist userlist;
#endif
        /* FLASK_GETBOOL, FLASK_SETBOOL */
        struct xen_flask_boolean boolean;
        struct xen_flask_setavc_threshold setavc_threshold;
        struct xen_flask_hash_stats hash_stats;
        struct xen_flask_cache_stats cache_stats;
        /* FLASK_ADD_OCONTEXT, FLASK_DEL_OCONTEXT */
        struct xen_flask_ocontext ocontext;
        struct xen_flask_peersid peersid;
        struct xen_flask_relabel relabel;
        struct xen_flask_devicetree_label devicetree_label;
    } u;
};
typedef struct xen_flask_op xen_flask_op_t;
DEFINE_XEN_GUEST_HANDLE(xen_flask_op_t);

#endif

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight host-call (hcall) implementation.
 *
 * Guest-to-host function calls use the Hyperlight FlatBuffer protocol
 * over the G2H virtqueue (transport.c).  This file provides a minimal C
 * FlatBuffer encoder/decoder for the FunctionCall and
 * FunctionCallResult schemas: one encoder for every host call, the
 * boot-time queries included, taking int, uint, long, ulong, string and
 * byte parameters.
 *
 * Byte values (VecBytes) travel outside the FlatBuffer: it records only
 * their length, and the bytes follow it in the message, in parameter
 * order.
 *
 * Wire format reference: hyperlight-common 0.18.0 flatbuffers.
 */

#include <string.h>
#include <uk/alloc.h>
#include <uk/arch/limits.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/plat/spinlock.h>
#include <uk/print.h>

#include <hyperlight/hcall.h>
#include <hyperlight/transport.h>

/*
 * Serialise host calls.  The transport has one request and one reply
 * buffer.  If the scheduler preempts a thread in the middle of a call
 * (e.g. on a timer tick), another thread could start its own on the same
 * buffers, corrupting the one in flight.
 *
 * We use irqsave/irqrestore to disable interrupts (prevents preemption
 * on our single vCPU) plus a spinlock (compiler barrier; also correct
 * if SMP is ever enabled).
 */
static __spinlock g_hcall_lock = UKARCH_SPINLOCK_INITIALIZER();

void hl_hcall_init(void)
{
	/* Nothing works without host calls: not even the command line. */
	if (hl_transport_init() < 0)
		UK_CRASH("hcall: cannot attach to the host's virtqueues\n");
	/* A G2H pool too small for the framing leaves no host call a byte
	 * to carry: the console, hostfs and hostsock would have nothing.
	 */
	if (!hl_hcall_max_payload())
		UK_CRASH("hcall: the G2H pool is too small; give it at least 3 pages\n");
}

int hl_hcall_ready(void)
{
	return hl_transport_ready();
}

/*
 * What a call needs besides its payload.  Outbound, the FunctionCall
 * framing: a 36-byte root, a 4 + 4n parameter vector and some 46 bytes
 * per parameter (HL_MAX_PARAMS of them at most), plus the function name
 * and any other string parameter -- a hostfs path is at most 1024 bytes.
 * Inbound, the FunctionCallResult framing, under 64 bytes.  4 KiB covers
 * all of it with room to spare.
 */
#define HL_HCALL_FRAMING 4096

/* hl_hcall_init() refuses a pool that leaves this at 0. */
__u64 hl_hcall_max_payload(void)
{
	__u64 cap = hl_transport_payload_cap();

	return cap > HL_HCALL_FRAMING ? cap - HL_HCALL_FRAMING : 0;
}

/*
 * The copies in and out of a host call run under g_hcall_lock, with
 * interrupts off, where a demand-paging fault cannot be served; and
 * parameters and results are often a program's own memory, or fresh
 * heap, not mapped until first touched.  So every page is read once
 * before the lock is taken.  A read maps a demand-paged page (with a
 * frame of its own, once: these are mostly long-lived buffers, sized
 * for a whole call, and a page already mapped is not touched again in
 * any way that costs); a copy-on-write fault on a mapped page, should
 * the copy out take one, is served by the platform's own handler
 * (cow.c), which needs no lock.
 */
static void hl_touch(const void *buf, __sz len)
{
	__uptr p, end = (__uptr)buf + len;

	if (!len)
		return;
	for (p = (__uptr)buf & ~(__uptr)(__PAGE_SIZE - 1); p < end;
	     p += __PAGE_SIZE)
		(void)*(volatile const __u8 *)MAX(p, (__uptr)buf);
}

static void hl_touch_params(const struct hl_param *params, int nparams)
{
	int i;

	for (i = 0; i < nparams; i++) {
		if (params[i].type == HL_PV_HLSTRING)
			hl_touch(params[i].str.ptr, params[i].str.len);
		else if (params[i].type == HL_PV_HLVECBYTES)
			hl_touch(params[i].vec.ptr, params[i].vec.len);
	}
}

int hl_utf8_valid(const __u8 *s, __u64 len)
{
	const __u8 *end = s + len;

	while (s < end) {
		__u8 c = *s++;
		__u32 cp, min;
		int n;

		if (c < 0x80)
			continue;
		if ((c & 0xe0) == 0xc0) {
			n = 1; cp = c & 0x1f; min = 0x80;
		} else if ((c & 0xf0) == 0xe0) {
			n = 2; cp = c & 0x0f; min = 0x800;
		} else if ((c & 0xf8) == 0xf0) {
			n = 3; cp = c & 0x07; min = 0x10000;
		} else {
			return 0;
		}
		if (end - s < n)
			return 0;
		while (n--) {
			if ((*s & 0xc0) != 0x80)
				return 0;
			cp = (cp << 6) | (*s++ & 0x3f);
		}
		if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
			return 0;
	}
	return 1;
}

/* ── Minimal FlatBuffer helpers ──────────────────────────────────── */

/*
 * Readers for a FlatBuffer the host wrote: every read is checked
 * against the reply's length, and one out of bounds reads as 0 (an
 * absent field, a null offset), so a malformed reply decodes as no
 * value rather than a read past it.
 */
static inline __u32 fb_u32(const __u8 *buf, __u64 len, __u64 off)
{
	if (off > len || len - off < 4)
		return 0;
	return buf[off]
	     | ((__u32)buf[off + 1] << 8)
	     | ((__u32)buf[off + 2] << 16)
	     | ((__u32)buf[off + 3] << 24);
}

static inline __u16 fb_u16(const __u8 *buf, __u64 len, __u64 off)
{
	if (off > len || len - off < 2)
		return 0;
	return buf[off] | ((__u16)buf[off + 1] << 8);
}

/*
 * Read field offset from vtable.  `vt_off` is the VT_* constant
 * (byte offset into vtable).  Returns 0 if the field is absent.
 */
static inline __u16 fb_field(const __u8 *buf, __u64 len, __u64 tbl,
			     __u16 vt_off)
{
	/* The table's soffset, signed, points back to its vtable. */
	__u64 vt = tbl - (__u64)(__s64)(__s32)fb_u32(buf, len, tbl);
	__u16 vt_size = fb_u16(buf, len, vt);

	if (vt_off >= vt_size)
		return 0;
	return fb_u16(buf, len, vt + vt_off);
}

/*
 * Follow a uoffset field (string / sub-table) to its target, 0 if the
 * field is absent or the target outside the reply.
 */
static inline __u64 fb_follow(const __u8 *buf, __u64 len, __u64 tbl,
			      __u16 vt_off)
{
	__u16 foff = fb_field(buf, len, tbl, vt_off);
	__u64 field_pos = tbl + foff, target;

	if (foff == 0)
		return 0;
	target = field_pos + fb_u32(buf, len, field_pos);
	return target < len ? target : 0;
}

/*
 * Read a u8 scalar from a table, with a default for absent fields.
 */
static inline __u8 fb_u8_default(const __u8 *buf, __u64 len, __u64 tbl,
				 __u16 vt_off, __u8 defval)
{
	__u16 foff = fb_field(buf, len, tbl, vt_off);

	if (foff == 0 || tbl + foff >= len)
		return defval;
	return buf[tbl + foff];
}

/* ── FunctionCallResult decoders ─────────────────────────────────── */

/*
 * VT offsets for the FunctionCallResult table.
 */
#define VT_FCR_RESULT_TYPE	4
#define VT_FCR_RESULT		6

/*
 * VT offsets for the ReturnValueBox table.
 */
#define VT_RVB_VALUE_TYPE	4
#define VT_RVB_VALUE		6

/*
 * VT offset of the value field of every hl* wrapper table (hlint,
 * hlulong, hlstring, ...).
 */
#define VT_VALUE		4

/* hlexternalbytes fields */
#define VT_HLEB_LENGTH		4
#define VT_HLEB_CHUNKED		6

/*
 * The value table of a size-prefixed FunctionCallResult that returned a
 * value of type @rv_type (HL_RV_*), or 0 if it returned something else:
 * a GuestError (no such host function, say), another type, or a reply
 * that is not a well-formed FlatBuffer of its length.
 */
static __u64 fb_result_value(const __u8 *buf, __u64 buf_len, __u8 rv_type)
{
	__u64 root, rvb;

	if (buf_len < 8 || 4 + (__u64)fb_u32(buf, buf_len, 0) > buf_len)
		return 0;

	/* Size-prefixed: content starts at byte 4, root_offset at byte 4 */
	root = 4 + fb_u32(buf, buf_len, 4);
	if (fb_u8_default(buf, buf_len, root, VT_FCR_RESULT_TYPE, 0) !=
	    HL_FCRT_RETURN_VALUE)
		return 0;

	rvb = fb_follow(buf, buf_len, root, VT_FCR_RESULT);
	if (!rvb || fb_u8_default(buf, buf_len, rvb, VT_RVB_VALUE_TYPE, 0) != rv_type)
		return 0;
	return fb_follow(buf, buf_len, rvb, VT_RVB_VALUE);
}

/* Whether the FunctionCallResult @buf is a GuestError: the host answered,
 * with an error (a host without the function, say).
 */
static int fb_result_is_error(const __u8 *buf, __u64 buf_len)
{
	if (buf_len < 8 || 4 + (__u64)fb_u32(buf, buf_len, 0) > buf_len)
		return 0;
	return fb_u8_default(buf, buf_len, 4 + fb_u32(buf, buf_len, 4),
			     VT_FCR_RESULT_TYPE, 0) == HL_FCRT_GUEST_ERROR;
}

/*
 * A scalar field of table @tbl, @size bytes, into @out: 0 when the field
 * is absent, which is how a FlatBuffer stores the default.
 */
static int fb_scalar(const __u8 *buf, __u64 buf_len, __u64 tbl,
		     __u16 vt_off, void *out, __sz size)
{
	__u16 foff = fb_field(buf, buf_len, tbl, vt_off);

	if (!foff) {
		memset(out, 0, size);
		return 0;
	}
	if (tbl + foff + size > buf_len)
		return -1;
	memcpy(out, buf + tbl + foff, size);
	return 0;
}

/*
 * A string result: a pointer into @buf, and its length (no NUL).
 */
static int fb_decode_result_string(const __u8 *buf, __u64 buf_len,
				   const char **out_str, __u64 *out_len)
{
	__u64 hls = fb_result_value(buf, buf_len, HL_RV_HLSTRING);
	__u64 str = hls ? fb_follow(buf, buf_len, hls, VT_VALUE) : 0;
	__u32 slen;

	if (!str || str + 4 > buf_len)
		return -1;
	slen = fb_u32(buf, buf_len, str);
	if (str + 4 + slen > buf_len)
		return -1;

	*out_str = (const char *)(buf + str + 4);
	*out_len = slen;
	return 0;
}

static int fb_decode_result_ulong(const __u8 *buf, __u64 buf_len,
				  __u64 *out_val)
{
	__u64 hlu = fb_result_value(buf, buf_len, HL_RV_HLULONG);

	return hlu ? fb_scalar(buf, buf_len, hlu, VT_VALUE, out_val,
			       sizeof(*out_val)) : -1;
}

static int fb_decode_result_int(const __u8 *buf, __u64 buf_len,
				__s32 *out_val)
{
	__u64 hli = fb_result_value(buf, buf_len, HL_RV_HLINT);

	return hli ? fb_scalar(buf, buf_len, hli, VT_VALUE, out_val,
			       sizeof(*out_val)) : -1;
}

/*
 * A Vec<u8> result: the FlatBuffer holds its length, and the bytes
 * follow the FlatBuffer in the reply.
 */
static int fb_decode_result_vecbytes(const __u8 *buf, __u64 buf_len,
				     const __u8 **out_data, __u64 *out_len)
{
	__u64 eb = fb_result_value(buf, buf_len, HL_RV_HLEXTERNALBYTES);
	__u64 fb_len, len;

	if (!eb || fb_u8_default(buf, buf_len, eb, VT_HLEB_CHUNKED, 0) ||
	    fb_scalar(buf, buf_len, eb, VT_HLEB_LENGTH, &len, sizeof(len)) < 0)
		return -1;

	/* fb_result_value() checked the FlatBuffer fits */
	fb_len = 4 + (__u64)fb_u32(buf, buf_len, 0);
	if (len > buf_len - fb_len)
		return -1;

	*out_data = buf + fb_len;
	*out_len = len;
	return 0;
}

/* ── Public API ──────────────────────────────────────────────────── */

/*
 * A parameterless host function returning u64: the boot-time queries.
 * 0 if the call fails or the host has no such function.
 */
static __u64 hl_query_ulong(const char *name)
{
	__u64 val;

	return hl_hcall_ulong(name, NULL, 0, &val) < 0 ? 0 : val;
}

/*
 * A parameterless host function returning a string, copied into @out
 * and NUL-terminated.  Returns its length -- @cap or more, with nothing
 * stored, when it does not fit (snprintf()) -- or -1 on failure.  Any
 * length fits an int: hl_transport_init() refuses a G2H half of 2 GiB
 * or more, so a reply is shorter.
 */
static int hl_query_string(const char *name, char *out, __sz cap)
{
	__sz len = (__sz)-1;

	if (hl_hcall_string(name, NULL, 0, out, cap, &len) == 0 ||
	    len != (__sz)-1)
		return (int)len;
	return -1;
}

int hl_call_get_cmdline(char *out_buf, __sz buf_sz)
{
	int len = hl_query_string("GetCmdLine", out_buf, buf_sz);

	return len >= 0 && (__sz)len < buf_sz ? len : -1;
}

int hl_call_get_env_vars(char *out_buf, __sz buf_sz)
{
	return hl_query_string("GetEnvVars", out_buf, buf_sz);
}

/*
 * Buffered stdin -- the host returns a chunk of whatever size it likes
 * (no params), so we cache it and hand out bytes on demand.  Bytes, not a
 * string: what is piped in need not be text, and a chunk can end inside a
 * character.  The chunk arrives in a host call reply, so a buffer that
 * size holds any of them; allocated on the first read.
 */
static __u8 *g_stdin_buf;
static __sz g_stdin_cap;
static __sz g_stdin_pos;
static __sz g_stdin_len;
static int  g_stdin_eof;

/*
 * Fetch the next chunk from the host's stdin buffer.
 * Returns the number of bytes fetched, or 0 on EOF/error.
 */
static int hl_stdin_fetch(void)
{
	__sz len;

	if (!hl_hcall_ready() || g_stdin_eof)
		return 0;

	if (!g_stdin_buf) {
		g_stdin_cap = hl_transport_payload_cap();
		g_stdin_buf = uk_malloc(uk_alloc_get_default(), g_stdin_cap);
		if (unlikely(!g_stdin_buf))
			return 0;
	}

	if (hl_hcall_vecbytes("ReadStdin", NULL, 0, g_stdin_buf, g_stdin_cap,
			      &len) < 0)
		return 0;
	if (len == 0) {
		g_stdin_eof = 1;
		return 0;
	}

	g_stdin_pos = 0;
	g_stdin_len = len;
	return len;
}

int hl_call_read_stdin(char *out_buf, __sz buf_sz)
{
	/* Serve from the guest-side buffer first */
	if (g_stdin_pos < g_stdin_len) {
		__sz avail = g_stdin_len - g_stdin_pos;
		__sz n = avail < buf_sz ? avail : buf_sz;

		memcpy(out_buf, g_stdin_buf + g_stdin_pos, n);
		g_stdin_pos += n;
		return (int)n;
	}

	/* Buffer empty — fetch from host */
	if (hl_stdin_fetch() <= 0)
		return 0;

	/* Serve what we just fetched */
	__sz n = g_stdin_len < buf_sz ? g_stdin_len : buf_sz;

	memcpy(out_buf, g_stdin_buf, n);
	g_stdin_pos = n;
	return (int)n;
}

__u64 hl_call_get_paging_budget(void)
{
	return hl_query_ulong("GetPagingBudget");
}

__u64 hl_call_get_initrd_base(void)
{
	return hl_query_ulong("GetInitrdBase");
}

__u64 hl_call_get_initrd_size(void)
{
	return hl_query_ulong("GetInitrdSize");
}

__u64 hl_call_get_exn_stack_top(void)
{
	return hl_query_ulong("GetExnStackTop");
}

__u64 hl_call_get_wall_clock_ns(void)
{
	return hl_query_ulong("GetWallClockNs");
}

/* ── Generic FlatBuffer encoder ──────────────────────────────────── */

/*
 * Encode a size-prefixed FunctionCall FlatBuffer with arbitrary typed
 * parameters, followed by the bytes of its VecBytes parameters: every
 * host call goes through it.  Supports hlint, hluint, hllong, hlulong,
 * hlstring and byte vectors.  Returns the payload length (the
 * FlatBuffer and the bytes), 0 if it does not fit @buf_sz.
 */

#define VW_SCALAR_VT_SZ  6
#define VW_INT_TBL_SZ    8	/* [soffset=4, value=4] */
#define VW_ULONG_TBL_SZ  12	/* [soffset=4, value=8] */
#define VW_REF_TBL_SZ    8	/* [soffset=4, uoffset=4] */
#define PM_VT_SZ	  8
#define PM_TBL_SZ	  12

#define GA2(x) (((x) + 1) & ~(__u64)1)
#define GA4(x) (((x) + 3) & ~(__u64)3)
/* Smallest value >= x congruent to 4 mod 8.  Ensures u64 field at
 * (result + 4) is 8-byte aligned, as the FlatBuffer verifier requires. */
#define GA8_OFF4(x) ((((x) + 3) & ~(__u64)7) | 4)

struct playout {
	__u64 pvt, ptbl, vvt, vtbl, vdata;
	__u16 vvtsz, vtblsz;
};

static void ew16(__u8 *b, __u64 p, __u16 v)
{
	b[p] = v; b[p + 1] = v >> 8;
}

static void ew32(__u8 *b, __u64 p, __u32 v)
{
	b[p] = v; b[p+1] = v >> 8; b[p+2] = v >> 16; b[p+3] = v >> 24;
}

static void ew64(__u8 *b, __u64 p, __u64 v)
{
	ew32(b, p, (__u32)v);
	ew32(b, p + 4, (__u32)(v >> 32));
}

static __u64 fb_encode_generic(
	__u8 *buf, __u64 buf_sz,
	const char *name,
	__u8 call_type, __u8 ret_type,
	const struct hl_param *params, int np)
{
	__u64 nlen = strlen(name);
	__u64 pos;
	int i;
	struct playout pl[HL_MAX_PARAMS];

	if (np > HL_MAX_PARAMS)
		return 0;

	/* ── Pass 1: compute positions ───────────────────────── */
	/* header: size(4)+root_off(4)+root_vt(12)+root_tbl(16) = 36 */
	pos = 36;

	__u64 pvec = 0;

	if (np > 0) {
		pvec = GA4(pos);
		pos = pvec + 4 + (__u64)np * 4;
	}

	for (i = 0; i < np; i++) {
		pl[i].pvt  = GA2(pos);
		pl[i].ptbl = GA4(pl[i].pvt + PM_VT_SZ);

		switch (params[i].type) {
		case HL_PV_HLINT: case HL_PV_HLUINT:
			pl[i].vvtsz  = VW_SCALAR_VT_SZ;
			pl[i].vtblsz = VW_INT_TBL_SZ;
			break;
		case HL_PV_HLLONG: case HL_PV_HLULONG:
		case HL_PV_HLVECBYTES:	/* hlexternalbytes: its length */
			pl[i].vvtsz  = VW_SCALAR_VT_SZ;
			pl[i].vtblsz = VW_ULONG_TBL_SZ;
			break;
		case HL_PV_HLSTRING:
			pl[i].vvtsz  = VW_SCALAR_VT_SZ;
			pl[i].vtblsz = VW_REF_TBL_SZ;
			break;
		default:
			return 0;
		}

		pl[i].vvt  = GA2(pl[i].ptbl + PM_TBL_SZ);
		/* u64 field at vtbl+4 must be 8-byte aligned */
		if (params[i].type == HL_PV_HLLONG ||
		    params[i].type == HL_PV_HLULONG ||
		    params[i].type == HL_PV_HLVECBYTES)
			pl[i].vtbl = GA8_OFF4(pl[i].vvt + pl[i].vvtsz);
		else
			pl[i].vtbl = GA4(pl[i].vvt + pl[i].vvtsz);
		pl[i].vdata = 0;
		pos = pl[i].vtbl + pl[i].vtblsz;
	}

	/* Strings; byte values go after the FlatBuffer instead */
	__u64 ext = 0;

	for (i = 0; i < np; i++) {
		if (params[i].type == HL_PV_HLSTRING) {
			pl[i].vdata = GA4(pos);
			pos = pl[i].vdata + 4
			    + GA4(params[i].str.len + 1);
		} else if (params[i].type == HL_PV_HLVECBYTES) {
			ext += params[i].vec.len;
		}
	}

	__u64 fnpos = GA4(pos);

	pos = fnpos + 4 + GA4(nlen + 1);

	__u64 total = pos;

	if (total + ext > buf_sz)
		return 0;

	/* ── Pass 2: emit bytes ──────────────────────────────── */
	memset(buf, 0, total);

	ew32(buf, 0, (__u32)(total - 4));
	ew32(buf, 4, 16);			/* root offset */

	/* Root vtable at 8 */
	ew16(buf, 8, 12);  ew16(buf, 10, 16);
	ew16(buf, 12, 4);  ew16(buf, 14, np > 0 ? 8 : 0);
	ew16(buf, 16, 12); ew16(buf, 18, 13);

	/* Root table at 20 */
	ew32(buf, 20, 12);			/* soffset → vt@8 */
	ew32(buf, 24, (__u32)(fnpos - 24));
	if (np > 0)
		ew32(buf, 28, (__u32)(pvec - 28));
	buf[32] = call_type;
	buf[33] = ret_type;

	/* Params vector */
	if (np > 0) {
		ew32(buf, pvec, (__u32)np);
		for (i = 0; i < np; i++) {
			__u64 ep = pvec + 4 + (__u64)i * 4;

			ew32(buf, ep,
			     (__u32)(pl[i].ptbl - ep));
		}
	}

	/* Each parameter */
	for (i = 0; i < np; i++) {
		ew16(buf, pl[i].pvt,     PM_VT_SZ);
		ew16(buf, pl[i].pvt + 2, PM_TBL_SZ);
		ew16(buf, pl[i].pvt + 4, 4);
		ew16(buf, pl[i].pvt + 6, 8);

		ew32(buf, pl[i].ptbl,
		     (__u32)(pl[i].ptbl - pl[i].pvt));
		buf[pl[i].ptbl + 4] = params[i].type == HL_PV_HLVECBYTES ?
				      HL_PV_HLEXTERNALBYTES : params[i].type;
		ew32(buf, pl[i].ptbl + 8,
		     (__u32)(pl[i].vtbl - (pl[i].ptbl + 8)));

		ew16(buf, pl[i].vvt,     pl[i].vvtsz);
		ew16(buf, pl[i].vvt + 2, pl[i].vtblsz);
		ew16(buf, pl[i].vvt + 4, 4);

		ew32(buf, pl[i].vtbl,
		     (__u32)(pl[i].vtbl - pl[i].vvt));

		switch (params[i].type) {
		case HL_PV_HLINT:
			ew32(buf, pl[i].vtbl + 4,
			     (__u32)params[i].i32_val);
			break;
		case HL_PV_HLUINT:
			ew32(buf, pl[i].vtbl + 4, params[i].u32_val);
			break;
		case HL_PV_HLLONG:
			ew64(buf, pl[i].vtbl + 4,
			     (__u64)params[i].i64_val);
			break;
		case HL_PV_HLULONG:
			ew64(buf, pl[i].vtbl + 4, params[i].u64_val);
			break;
		case HL_PV_HLSTRING:
			ew32(buf, pl[i].vtbl + 4,
			     (__u32)(pl[i].vdata - (pl[i].vtbl + 4)));
			ew32(buf, pl[i].vdata, params[i].str.len);
			memcpy(buf + pl[i].vdata + 4,
			       params[i].str.ptr, params[i].str.len);
			break;
		case HL_PV_HLVECBYTES:
			/* hlexternalbytes.length; chunked = false is the
			 * default and left out.
			 */
			ew64(buf, pl[i].vtbl + 4, params[i].vec.len);
			break;
		}
	}

	/* Function name string */
	ew32(buf, fnpos, (__u32)nlen);
	memcpy(buf + fnpos + 4, name, nlen);

	/* The byte values themselves, in parameter order */
	for (i = 0; i < np; i++) {
		if (params[i].type != HL_PV_HLVECBYTES || !params[i].vec.len)
			continue;
		memcpy(buf + total, params[i].vec.ptr, params[i].vec.len);
		total += params[i].vec.len;
	}

	return total;
}

/* ── Generic host call public API ────────────────────────────────── */

/*
 * Encode a host call straight into the transport's request buffer.
 * The caller holds g_hcall_lock.  Returns the payload length, 0 if it
 * does not fit or no host call can be made now.
 */
static __u64 hl_encode_request(const char *func_name, __u8 ret_type,
			       const struct hl_param *params, int nparams)
{
	__u8 *req = hl_transport_request_buf();

	if (!req)
		return 0;
	return fb_encode_generic(req, hl_transport_payload_cap(), func_name,
				 HL_FCT_HOST, ret_type, params, nparams);
}

/* Where a result goes, and how much of it there was. */
struct hl_out {
	void *buf;
	__sz cap;
	__sz len;
};

/*
 * One host call: encode it, send it, and @take the result out of the
 * reply, all under g_hcall_lock (the reply buffer is the next call's
 * too).  The parameters and @out's buffer are touched first: see
 * hl_touch().  A string parameter that is not UTF-8 fails the call here:
 * the host would refuse the whole message, and with it the guest
 * function call in flight.  Returns 0 if @take had the result,
 * HL_HCALL_REFUSED if the host answered with an error,
 * HL_HCALL_BAD_RESULT if it ran the call but its reply, or @take's
 * result from it, is unusable, and -1 if the call did not reach it.
 */
static int hl_call(const char *func_name, __u8 ret_type,
		   const struct hl_param *params, int nparams,
		   int (*take)(const __u8 *rd, __u64 rl, struct hl_out *out),
		   struct hl_out *out)
{
	const __u8 *rd;
	__u64 rl, fc_len;
	unsigned long irqf;
	int rc = -1, t, i;

	if (!hl_hcall_ready())
		return -1;

	hl_touch_params(params, nparams);
	hl_touch(out->buf, out->cap);

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);
	/* Checked under the lock, which no thread runs through on this
	 * vCPU: what is checked is what is sent.
	 */
	for (i = 0; i < nparams; i++)
		if (params[i].type == HL_PV_HLSTRING &&
		    !hl_utf8_valid((const __u8 *)params[i].str.ptr,
				   params[i].str.len))
			goto out;
	fc_len = hl_encode_request(func_name, ret_type, params, nparams);
	t = fc_len ? hl_transport_call(fc_len, &rd, &rl) :
		     HL_TRANSPORT_NOT_SENT;
	if (t == 0)
		rc = fb_result_is_error(rd, rl) ? HL_HCALL_REFUSED :
		     take(rd, rl, out) < 0 ? HL_HCALL_BAD_RESULT : 0;
	else if (t == HL_TRANSPORT_BAD_REPLY)
		/* The host saw the request: it ran the call. */
		rc = HL_HCALL_BAD_RESULT;
out:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return rc;
}

int hl_hcall_respond(__u32 cid, const __u8 *payload, __u64 len)
{
	unsigned long irqf;
	int rc;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);
	rc = hl_transport_respond(cid, payload, len);
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return rc;
}

static int hl_take_int(const __u8 *rd, __u64 rl, struct hl_out *out)
{
	return fb_decode_result_int(rd, rl, out->buf);
}

static int hl_take_ulong(const __u8 *rd, __u64 rl, struct hl_out *out)
{
	return fb_decode_result_ulong(rd, rl, out->buf);
}

static int hl_take_string(const __u8 *rd, __u64 rl, struct hl_out *out)
{
	const char *s;
	__u64 sl;

	if (fb_decode_result_string(rd, rl, &s, &sl) < 0)
		return -1;
	/* Too small: say what it takes. */
	out->len = sl;
	if (sl >= out->cap)
		return -1;
	memcpy(out->buf, s, sl);
	((char *)out->buf)[sl] = '\0';
	return 0;
}

static int hl_take_vecbytes(const __u8 *rd, __u64 rl, struct hl_out *out)
{
	const __u8 *vd;
	__u64 vl;

	/* A result that does not fit is refused, not cut short. */
	if (fb_decode_result_vecbytes(rd, rl, &vd, &vl) < 0 || vl > out->cap)
		return -1;
	if (vl)
		memcpy(out->buf, vd, vl);
	out->len = vl;
	return 0;
}

int hl_hcall_int(const char *func_name,
		 const struct hl_param *params, int nparams,
		 __s32 *out)
{
	struct hl_out o = { .buf = out, .cap = sizeof(*out) };

	return hl_call(func_name, HL_RT_INT, params, nparams, hl_take_int,
		       &o);
}

int hl_hcall_ulong(const char *func_name,
		   const struct hl_param *params, int nparams,
		   __u64 *out)
{
	struct hl_out o = { .buf = out, .cap = sizeof(*out) };

	return hl_call(func_name, HL_RT_ULONG, params, nparams,
		       hl_take_ulong, &o);
}

int hl_hcall_string(const char *func_name,
		    const struct hl_param *params, int nparams,
		    char *out_buf, __sz buf_sz, __sz *out_len)
{
	struct hl_out o = { .buf = out_buf, .cap = buf_sz,
			    .len = (__sz)-1 };
	int rc = hl_call(func_name, HL_RT_STRING, params, nparams,
			 hl_take_string, &o);

	if (out_len && o.len != (__sz)-1)
		*out_len = o.len;
	return rc;
}

int hl_hcall_vecbytes(const char *func_name,
		      const struct hl_param *params, int nparams,
		      __u8 *out_buf, __sz buf_sz, __sz *out_len)
{
	struct hl_out o = { .buf = out_buf, .cap = buf_sz };
	int rc = hl_call(func_name, HL_RT_VECBYTES, params, nparams,
			 hl_take_vecbytes, &o);

	*out_len = rc == 0 ? o.len : 0;
	return rc;
}

/*
 * GetRandomBytes(len) -> Vec<u8>, for the CSPRNG's seed.  The random
 * driver registers during early boot, before there is a heap; nothing
 * here needs one.
 */
int hl_call_get_random_bytes(__u8 *out_buf, __sz len)
{
	struct hl_param p = { .type = HL_PV_HLULONG, .u64_val = len };
	__sz got;

	if (hl_hcall_vecbytes("GetRandomBytes", &p, 1, out_buf, len, &got) < 0)
		return -1;
	return got == len ? 0 : -1;
}

/*
 * Console output, as bytes, through the host function @fn: what a
 * program writes need not be UTF-8, and a write can end inside a
 * character, so it is not sent as a string (whose FlatBuffer the host
 * would refuse).  Returns the bytes the host took: all of them, unless a
 * call failed or the host wrote fewer than it was given; or if none,
 * the first call's failure (HL_HCALL_REFUSED or -1).
 */
static __ssz host_write(const char *fn, const char *buf, __sz len)
{
	__u64 max = hl_hcall_max_payload();
	struct hl_param p = { .type = HL_PV_HLVECBYTES };
	__s32 written;
	__sz sent = 0;
	int rc;

	if (!max)
		return -1;
	while (sent < len) {
		__sz n = MIN(len - sent, max), back = 0;

		/* Cut at the nearest start of a character within 3 bytes, so
		 * that another thread's output, which may come in between two
		 * chunks, never lands inside one; bytes that are not UTF-8 are
		 * cut anywhere.  (n is a whole host call's worth: > 3.)
		 */
		if (n < len - sent) {
			while (back < 3 &&
			       (buf[sent + n - back] & 0xc0) == 0x80)
				back++;
			if ((buf[sent + n - back] & 0xc0) != 0x80)
				n -= back;
		}

		/* Counted once the host ran the call, by the count it says it
		 * wrote; in full if its reply is unusable, so as not to be
		 * sent again by the fallback.  An error result (a host without
		 * @fn, say), a call that never reached it, a failed write or a
		 * short count leaves the rest to the caller's fallback.
		 */
		p.vec.ptr = (const __u8 *)buf + sent;
		p.vec.len = n;
		rc = hl_hcall_int(fn, &p, 1, &written);
		if (rc == HL_HCALL_BAD_RESULT) {
			sent += n;
			continue;
		}
		if (rc < 0)
			return sent ? (__ssz)sent : rc;
		if (written < 0)
			return sent ? (__ssz)sent : -1;
		sent += MIN((__sz)written, n);
		if ((__sz)written < n)
			break;
	}
	return (__ssz)sent;
}

__ssz hl_call_host_write(const char *buf, __sz len)
{
	return host_write("HostWrite", buf, len);
}

__ssz hl_call_host_write_err(const char *buf, __sz len)
{
	return host_write("HostWriteErr", buf, len);
}

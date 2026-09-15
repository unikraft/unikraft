/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight host-call (hcall) implementation.
 *
 * Guest-to-host function calls use the Hyperlight FlatBuffer protocol
 * over the PEB's shared I/O stacks.  This file provides a minimal C
 * FlatBuffer encoder/decoder for the FunctionCall and
 * FunctionCallResult schemas — just enough for the two host functions
 * we call: GetCmdLine() → String and HostPrint(String) → Int.
 *
 * TODO: Add a generic dispatch mechanism for arbitrary host function
 * calls (e.g. serialise parameters as JSON over a vec<u8> payload)
 * when more host functions are needed.
 *
 * Wire format reference: hyperlight-common 0.16.0 flatbuffers.
 */

#include <string.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/plat/spinlock.h>
#include <uk/print.h>

#include <hyperlight-x86/hcall.h>
#include <hyperlight-x86/outb.h>

/*
 * Cached PEB I/O stack pointers.
 */
static volatile __u8 *g_input_stack;
static __u64 g_input_stack_size;
static volatile __u8 *g_output_stack;
static __u64 g_output_stack_size;
static int g_hcall_ready;

/*
 * Serialise host calls.  The PEB I/O stacks are global, single-instance
 * buffers with no built-in synchronisation.  If the scheduler preempts
 * a thread between push and pop (e.g. on a timer tick), another thread
 * could start its own push → outb → pop sequence on the same stacks,
 * corrupting the in-flight call.
 *
 * We use irqsave/irqrestore to disable interrupts (prevents preemption
 * on our single vCPU) plus a spinlock (compiler barrier; also correct
 * if SMP is ever enabled).
 *
 * TODO: This may become unnecessary once Hyperlight switches to
 * virtqueue-based host-guest communication (hyperlight-dev/hyperlight
 * PR #1717), which replaces the PEB I/O stacks entirely.
 */
static __spinlock g_hcall_lock = UKARCH_SPINLOCK_INITIALIZER();

void hl_hcall_init(const struct hyperlight_peb *peb)
{
	g_input_stack = (volatile __u8 *)peb->input_stack.ptr;
	g_input_stack_size = peb->input_stack.size;
	g_output_stack = (volatile __u8 *)peb->output_stack.ptr;
	g_output_stack_size = peb->output_stack.size;
	g_hcall_ready = 1;
}

int hl_hcall_ready(void)
{
	return g_hcall_ready;
}

/*
 * What a call needs on a stack besides its payload.  Outbound, the
 * FunctionCall framing: a 36-byte root, a 4 + 4n parameter vector and
 * some 46 bytes per parameter (HL_MAX_PARAMS of them at most), plus the
 * function name and any other string parameter -- a hostfs path is at
 * most 1024 bytes.  Inbound, the FunctionCallResult framing, under 64
 * bytes.  Either way plus the 16 bytes of stack bookkeeping.  4 KiB
 * covers all of it with room to spare.
 */
#define HL_HCALL_FRAMING 4096

__u64 hl_hcall_max_payload(void)
{
	__u64 stack = MIN(g_input_stack_size, g_output_stack_size);

	return stack > HL_HCALL_FRAMING ? stack - HL_HCALL_FRAMING : 0;
}

/* ── Shared-memory stack protocol ────────────────────────────────── */

/*
 * The shared I/O stacks use a stack-of-blobs layout:
 *
 *   offset 0:   [stack_ptr: u64]      relative offset to next free byte
 *   offset 8:   [data0 bytes...]
 *   offset ?:   [back_ptr0: u64]      previous stack_ptr value
 *   offset ?:   [data1 bytes...]
 *   offset ?:   [back_ptr1: u64]
 *   ...
 *
 * Push appends data + 8-byte back_ptr, then updates stack_ptr.
 * Pop reads back_ptr at (stack_ptr - 8), rewinds stack_ptr to it.
 */

int hl_stack_push(volatile __u8 *stack, __u64 stack_size,
		  const __u8 *data, __u64 data_len)
{
	volatile __u64 *sp = (volatile __u64 *)stack;
	__u64 old_sp = *sp;
	__u64 needed = data_len + 8; /* data + back_ptr */

	if (old_sp + needed > stack_size)
		return -1;

	memcpy((void *)(stack + old_sp), data, data_len);
	*(volatile __u64 *)(stack + old_sp + data_len) = old_sp;
	*sp = old_sp + data_len + 8;
	return 0;
}

/*
 * Pop the top element off the input stack.  Returns pointer to the
 * data (inside the stack buffer) and its length.  The data remains
 * valid until the next push/pop.
 */
int hl_stack_pop(volatile __u8 *stack,
		 const __u8 **out_data, __u64 *out_len)
{
	volatile __u64 *sp = (volatile __u64 *)stack;
	__u64 cur_sp = *sp;

	if (cur_sp < 16)
		return -1;

	__u64 back_ptr = *(volatile __u64 *)(stack + cur_sp - 8);

	if (back_ptr < 8 || back_ptr >= cur_sp - 8)
		return -1;

	*out_data = (const __u8 *)(stack + back_ptr);
	*out_len = cur_sp - 8 - back_ptr;
	*sp = back_ptr;
	return 0;
}

/* ── Minimal FlatBuffer helpers ──────────────────────────────────── */

static inline __u32 fb_u32(const __u8 *buf, __u64 off)
{
	return buf[off]
	     | ((__u32)buf[off + 1] << 8)
	     | ((__u32)buf[off + 2] << 16)
	     | ((__u32)buf[off + 3] << 24);
}

static inline __u16 fb_u16(const __u8 *buf, __u64 off)
{
	return buf[off] | ((__u16)buf[off + 1] << 8);
}

static inline __s32 fb_i32(const __u8 *buf, __u64 off)
{
	return (__s32)fb_u32(buf, off);
}

/*
 * Resolve vtable position for a table at `tbl`.
 */
static inline __u64 fb_vtable(const __u8 *buf, __u64 tbl)
{
	return tbl - fb_i32(buf, tbl);
}

/*
 * Read field offset from vtable.  `vt_off` is the VT_* constant
 * (byte offset into vtable).  Returns 0 if the field is absent.
 */
static inline __u16 fb_field(const __u8 *buf, __u64 tbl, __u16 vt_off)
{
	__u64 vt = fb_vtable(buf, tbl);
	__u16 vt_size = fb_u16(buf, vt);

	if (vt_off >= vt_size)
		return 0;
	return fb_u16(buf, vt + vt_off);
}

/*
 * Follow a uoffset field (string / sub-table) to its target.
 */
static inline __u64 fb_follow(const __u8 *buf, __u64 tbl, __u16 vt_off)
{
	__u16 foff = fb_field(buf, tbl, vt_off);

	if (foff == 0)
		return 0;

	__u64 field_pos = tbl + foff;

	return field_pos + fb_u32(buf, field_pos);
}

/*
 * Read a u8 scalar from a table, with a default for absent fields.
 */
static inline __u8 fb_u8_default(const __u8 *buf, __u64 tbl,
				 __u16 vt_off, __u8 defval)
{
	__u16 foff = fb_field(buf, tbl, vt_off);

	if (foff == 0)
		return defval;
	return buf[tbl + foff];
}

/* ── FunctionCall encoder (no parameters) ────────────────────────── */

/*
 * Encode a size-prefixed FunctionCall FlatBuffer with no parameters.
 *
 * Layout (verified against hyperlight-common 0.16.0):
 *   [u32 size_prefix]
 *   [u32 root_offset = 16]
 *   [vtable: 12 bytes]
 *   [table:  12 bytes]
 *   [string: 4 + name_len + 1 + pad]
 *
 * Returns total buffer length, or 0 if buf_sz is too small.
 */
static __u64 fb_encode_function_call(
	__u8 *buf, __u64 buf_sz,
	const char *name,
	__u8 call_type,		/* HL_FCT_GUEST or HL_FCT_HOST */
	__u8 return_type)	/* HL_RT_* */
{
	__u64 name_len = strlen(name);
	/* String: 4-byte length prefix + chars + NUL, padded to 4 bytes */
	__u64 str_padded = (name_len + 1 + 3) & ~3ULL;
	__u64 content = 4 + 12 + 12 + 4 + str_padded;
	/* content = root_offset(4) + vtable(12) + table(12) + string(4+padded) */
	/* But size_prefix counts from byte 4, so it equals content */
	__u64 total = 4 + content;

	if (total > buf_sz)
		return 0;

	memset(buf, 0, total);

	/* Size prefix (bytes after this field) */
	*(__u32 *)(buf + 0) = (__u32)content;

	/* Root offset (from byte 4 → table at byte 20) */
	*(__u32 *)(buf + 4) = 16;

	/* Vtable at byte 8 (12 bytes = 6 u16 entries) */
	__u8 *vt = buf + 8;

	*(__u16 *)(vt + 0) = 12;  /* vtable_size */
	*(__u16 *)(vt + 2) = 12;  /* table_inline_size */
	*(__u16 *)(vt + 4) = 8;   /* VT_FUNCTION_NAME (field offset in table) */
	*(__u16 *)(vt + 6) = 0;   /* VT_PARAMETERS (absent) */
	*(__u16 *)(vt + 8) = 6;   /* VT_FUNCTION_CALL_TYPE */
	*(__u16 *)(vt + 10) = 7;  /* VT_EXPECTED_RETURN_TYPE */

	/* Table at byte 20 (12 bytes inline) */
	__u8 *tbl = buf + 20;

	*(__s32 *)(tbl + 0) = 12;	       /* soffset to vtable */
	/* tbl[4..5] = padding (0) */
	tbl[6] = call_type;		       /* function_call_type */
	tbl[7] = return_type;		       /* expected_return_type */
	*(__u32 *)(tbl + 8) = 4;	       /* uoffset to string (from here) */

	/* String at byte 32 */
	__u8 *str = buf + 32;

	*(__u32 *)(str + 0) = (__u32)name_len; /* string length */
	memcpy(str + 4, name, name_len);       /* string data (NUL already 0) */

	return total;
}

/* ── FunctionCall encoder (one string parameter) ─────────────────── */

/* Align to 4-byte boundary */
#define ALIGN4(x) (((x) + 3) & ~3ULL)

/*
 * Encode a size-prefixed FunctionCall FlatBuffer with one string
 * parameter.  Used by HostPrint(msg) and similar single-string calls.
 *
 * The fixed layout was reverse-engineered from hyperlight-common 0.16.0
 * and verified byte-for-byte against the Rust FlatBuffer builder output.
 *
 * Layout:
 *   [u32 size_prefix]           -- byte 0
 *   [u32 root_offset = 16]      -- byte 4
 *   [pad: 2 bytes]              -- byte 8
 *   [root vtable: 10 bytes]     -- byte 10
 *   [root table: 16 bytes]      -- byte 20
 *   [params vector: 8 bytes]    -- byte 36
 *   [param vtable: 8 bytes]     -- byte 44
 *   [param table: 14 bytes]     -- byte 52
 *   [hlstring vtable: 6 bytes]  -- byte 66
 *   [hlstring table: 8 bytes]   -- byte 72
 *   [message string: variable]  -- byte 80
 *   [function name: variable]   -- after message
 *
 * Returns total buffer length, or 0 if buf_sz is too small.
 */
static __u64 fb_encode_function_call_1str(
	__u8 *buf, __u64 buf_sz,
	const char *name, __u64 name_len,
	const char *msg, __u64 msg_len,
	__u8 call_type,		/* HL_FCT_HOST */
	__u8 return_type)	/* HL_RT_INT for HostPrint */
{
	__u64 msg_padded = ALIGN4(msg_len + 1);
	__u64 name_padded = ALIGN4(name_len + 1);
	/* Fixed structure: 80 bytes header + variable strings */
	__u64 total = 80 + 4 + msg_padded + 4 + name_padded;
	__u64 content = total - 4; /* size_prefix excluded */

	if (total > buf_sz)
		return 0;

	memset(buf, 0, total);

	/* Size prefix */
	*(__u32 *)(buf + 0) = (__u32)content;

	/* Root offset (from byte 4 → root table at byte 20) */
	*(__u32 *)(buf + 4) = 16;

	/* byte 8-9: padding (already 0) */

	/* Root vtable at byte 10 (10 bytes = 5 u16 entries)
	 * VT_EXPECTED_RETURN_TYPE at vt_off 10 is omitted because
	 * its value (0 = Int) is the default — matching the SDK.
	 */
	*(__u16 *)(buf + 10) = 10;  /* vtable_size */
	*(__u16 *)(buf + 12) = 16;  /* table_inline_size */
	*(__u16 *)(buf + 14) = 8;   /* VT_FUNCTION_NAME offset in table */
	*(__u16 *)(buf + 16) = 12;  /* VT_PARAMETERS offset in table */
	*(__u16 *)(buf + 18) = 7;   /* VT_FUNCTION_CALL_TYPE offset in table */

	/* Root table at byte 20 (16 bytes inline) */
	*(__s32 *)(buf + 20) = 10;  /* soffset → vtable at 20-10=10 */
	/* byte 24-26: padding */
	buf[27] = call_type;	    /* function_call_type at offset 7 */
	/* func_name uoffset at offset 8 → string starts at 80+4+msg_padded */
	__u64 name_str_start = 80 + 4 + msg_padded;

	*(__u32 *)(buf + 28) = (__u32)(name_str_start - 28);
	*(__u32 *)(buf + 32) = 4;   /* params uoffset at offset 12 → byte 36 */

	/* Parameters vector at byte 36 */
	*(__u32 *)(buf + 36) = 1;   /* vector length = 1 */
	*(__u32 *)(buf + 40) = 12;  /* uoffset to Parameter[0] → byte 52 */

	/* Parameter vtable at byte 44 (8 bytes = 4 u16 entries) */
	*(__u16 *)(buf + 44) = 8;   /* vtable_size */
	*(__u16 *)(buf + 46) = 14;  /* table_inline_size */
	*(__u16 *)(buf + 48) = 7;   /* VT_VALUE_TYPE offset in table */
	*(__u16 *)(buf + 50) = 8;   /* VT_VALUE offset in table */

	/* Parameter table at byte 52 (14 bytes inline) */
	*(__s32 *)(buf + 52) = 8;   /* soffset → vtable at 52-8=44 */
	/* byte 56-58: padding */
	buf[59] = HL_RV_HLSTRING;   /* value_type = 7 (hlstring) */
	*(__u32 *)(buf + 60) = 12;  /* uoffset → hlstring table at byte 72 */
	/* byte 64-65: padding to inline size 14 */

	/* hlstring vtable at byte 66 (6 bytes = 3 u16 entries) */
	*(__u16 *)(buf + 66) = 6;   /* vtable_size */
	*(__u16 *)(buf + 68) = 8;   /* table_inline_size */
	*(__u16 *)(buf + 70) = 4;   /* VT_VALUE offset in table */

	/* hlstring table at byte 72 (8 bytes inline) */
	*(__s32 *)(buf + 72) = 6;   /* soffset → vtable at 72-6=66 */
	*(__u32 *)(buf + 76) = 4;   /* uoffset → message string at byte 80 */

	/* Message string at byte 80 */
	*(__u32 *)(buf + 80) = (__u32)msg_len;
	memcpy(buf + 84, msg, msg_len);
	/* NUL + padding already 0 */

	/* Function name string */
	*(__u32 *)(buf + name_str_start) = (__u32)name_len;
	memcpy(buf + name_str_start + 4, name, name_len);

	return total;
}

/* ── FunctionCallResult decoder ──────────────────────────────────── */

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
 * VT offset for hlstring's value field.
 */
#define VT_HLS_VALUE		4

/*
 * Decode a size-prefixed FunctionCallResult FlatBuffer and extract
 * a string return value.
 *
 * @param buf	    FlatBuffer data (with 4-byte size prefix)
 * @param buf_len   Length of buf
 * @param out_str   Pointer to the string data (inside buf)
 * @param out_len   Length of the string (excluding NUL)
 * @return          0 on success, -1 on error
 */
static int fb_decode_result_string(const __u8 *buf, __u64 buf_len,
				   const char **out_str, __u64 *out_len)
{
	if (buf_len < 8)
		return -1;

	/* Size-prefixed: content starts at byte 4, root_offset at byte 4 */
	__u64 root = 4 + fb_u32(buf, 4);

	/* FunctionCallResult.result_type must be ReturnValueBox (1) */
	__u8 result_type = fb_u8_default(buf, root, VT_FCR_RESULT_TYPE, 0);

	if (result_type != HL_FCRT_RETURN_VALUE)
		return -1;

	/* Follow result → ReturnValueBox table */
	__u64 rvb = fb_follow(buf, root, VT_FCR_RESULT);

	if (rvb == 0)
		return -1;

	/* ReturnValueBox.value_type must be hlstring (7) */
	__u8 value_type = fb_u8_default(buf, rvb, VT_RVB_VALUE_TYPE, 0);

	if (value_type != HL_RV_HLSTRING)
		return -1;

	/* Follow value → hlstring table */
	__u64 hls = fb_follow(buf, rvb, VT_RVB_VALUE);

	if (hls == 0)
		return -1;

	/* Follow hlstring.value → string */
	__u64 str = fb_follow(buf, hls, VT_HLS_VALUE);

	if (str == 0)
		return -1;

	/* String: u32 length, then chars */
	__u32 slen = fb_u32(buf, str);

	if (str + 4 + slen > buf_len)
		return -1;

	*out_str = (const char *)(buf + str + 4);
	*out_len = slen;
	return 0;
}

/*
 * Decode a size-prefixed FunctionCallResult FlatBuffer and extract
 * a u64 (ulong) return value.
 *
 * @param buf       FlatBuffer data (with 4-byte size prefix)
 * @param buf_len   Length of buf
 * @param out_val   Pointer to receive the u64 value
 * @return          0 on success, -1 on error / function not found
 */
static int fb_decode_result_ulong(const __u8 *buf, __u64 buf_len,
				  __u64 *out_val)
{
	if (buf_len < 8)
		return -1;

	__u64 root = 4 + fb_u32(buf, 4);

	/* FunctionCallResult.result_type must be ReturnValueBox (1) */
	__u8 result_type = fb_u8_default(buf, root, VT_FCR_RESULT_TYPE, 0);

	if (result_type != HL_FCRT_RETURN_VALUE)
		return -1;

	/* Follow result → ReturnValueBox table */
	__u64 rvb = fb_follow(buf, root, VT_FCR_RESULT);

	if (rvb == 0)
		return -1;

	/* ReturnValueBox.value_type must be hlulong (4) */
	__u8 value_type = fb_u8_default(buf, rvb, VT_RVB_VALUE_TYPE, 0);

	if (value_type != HL_RV_HLULONG)
		return -1;

	/* Follow value → hlulong table */
	__u64 hlu = fb_follow(buf, rvb, VT_RVB_VALUE);

	if (hlu == 0)
		return -1;

	/* hlulong.value: u64 at VT offset 4 */
	__u16 foff = fb_field(buf, hlu, VT_HLS_VALUE);

	if (foff == 0)
		return -1;

	memcpy(out_val, buf + hlu + foff, sizeof(__u64));
	return 0;
}

/* ── Public API ──────────────────────────────────────────────────── */

int hl_call_get_cmdline(char *out_buf, __sz buf_sz)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	const char *str;
	__u64 str_len;
	unsigned long irqf;
	int rc = -1;

	if (!g_output_stack || !g_input_stack) {
		uk_pr_err("hcall: not initialised\n");
		return -1;
	}

	/* Encode the FunctionCall (no shared state — outside lock) */
	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetCmdLine",
					 HL_FCT_HOST, HL_RT_STRING);
	if (fc_len == 0) {
		uk_pr_err("hcall: encode failed\n");
		return -1;
	}

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	/* Push onto output stack */
	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0)
		goto out;

	/* Trigger host dispatch — port 101 = CallFunction */
	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	/* Pop result from input stack */
	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0)
		goto out;

	/* Decode the string result */
	if (fb_decode_result_string(result_data, result_len,
				    &str, &str_len) < 0)
		goto out;

	/* Copy to caller's buffer */
	if (str_len >= buf_sz)
		goto out;

	memcpy(out_buf, str, str_len);
	out_buf[str_len] = '\0';
	rc = (int)str_len;

out:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return rc;
}

int hl_call_get_env_vars(char *out_buf, __sz buf_sz)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	const char *str;
	__u64 str_len;
	unsigned long irqf;
	int rc = -1;

	if (!g_output_stack || !g_input_stack) {
		uk_pr_err("hcall: not initialised\n");
		return -1;
	}

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetEnvVars",
					 HL_FCT_HOST, HL_RT_STRING);
	if (fc_len == 0) {
		uk_pr_err("hcall: encode failed\n");
		return -1;
	}

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0)
		goto out_env;

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0)
		goto out_env;

	if (fb_decode_result_string(result_data, result_len,
				    &str, &str_len) < 0)
		goto out_env;

	/* Too small: report what it takes, store nothing (snprintf). */
	rc = (int)str_len;
	if (str_len >= buf_sz)
		goto out_env;

	/*
	 * Copy the raw bytes -- the string contains embedded NULs as
	 * separators between KEY=VALUE pairs -- and terminate them
	 * ourselves: whether the FlatBuffer string carries a trailing NUL
	 * is not part of the contract.
	 */
	memcpy(out_buf, str, str_len);
	out_buf[str_len] = '\0';

out_env:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return rc;
}

/*
 * Buffered stdin -- the host returns a chunk of whatever size it likes
 * (no params), so we cache it and hand out bytes on demand.  The chunk
 * arrives on the input stack, so a buffer that size holds any of them;
 * allocated on the first read.
 */
static char *g_stdin_buf;
static __sz g_stdin_pos;
static __sz g_stdin_len;
static int  g_stdin_eof;

/*
 * Fetch the next chunk from the host's stdin buffer.
 * Returns the number of bytes fetched, or 0 on EOF/error.
 */
static int hl_stdin_fetch(void)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	const char *str;
	__u64 str_len;
	unsigned long irqf;

	if (!g_hcall_ready || g_stdin_eof)
		return 0;

	if (!g_stdin_buf) {
		g_stdin_buf = uk_malloc(uk_alloc_get_default(),
					g_input_stack_size);
		if (unlikely(!g_stdin_buf))
			return 0;
	}

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "ReadStdin",
					 HL_FCT_HOST, HL_RT_STRING);
	if (fc_len == 0)
		return 0;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0)
		goto fail;

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0)
		goto fail;

	if (fb_decode_result_string(result_data, result_len,
				    &str, &str_len) < 0)
		goto fail;

	/* It came off the input stack: it fits a buffer that size. */
	if (unlikely(str_len > g_input_stack_size))
		goto fail;

	memcpy(g_stdin_buf, str, str_len);

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);

	if (str_len == 0) {
		g_stdin_eof = 1;
		return 0;
	}

	g_stdin_pos = 0;
	g_stdin_len = str_len;
	return (int)str_len;

fail:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return 0;
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
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	__u64 budget;
	unsigned long irqf;

	if (!g_hcall_ready)
		return 0;

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetPagingBudget",
					 HL_FCT_HOST, HL_RT_ULONG);
	if (fc_len == 0)
		return 0;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	if (fb_decode_result_ulong(result_data, result_len, &budget) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return budget;
}

__u64 hl_call_get_initrd_base(void)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	__u64 base;
	unsigned long irqf;

	if (!g_hcall_ready)
		return 0;

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetInitrdBase",
					 HL_FCT_HOST, HL_RT_ULONG);
	if (fc_len == 0)
		return 0;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	if (fb_decode_result_ulong(result_data, result_len, &base) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return base;
}

__u64 hl_call_get_initrd_size(void)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	__u64 size;
	unsigned long irqf;

	if (!g_hcall_ready)
		return 0;

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetInitrdSize",
					 HL_FCT_HOST, HL_RT_ULONG);
	if (fc_len == 0)
		return 0;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	if (fb_decode_result_ulong(result_data, result_len, &size) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return size;
}

int hl_call_host_print(const char *msg, __sz len)
{
	/*
	 * Stack-allocate a buffer large enough for the FlatBuffer.
	 * Fixed overhead is 80 + 4 (name) + 4 (msg) = 88 bytes,
	 * plus the two strings aligned to 4 bytes.
	 * "HostPrint" is 9 chars → 12 aligned. Cap message at ~4K.
	 */
	__u8 fc_buf[4096 + 128];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	unsigned long irqf;

	if (!g_hcall_ready)
		return -1;

	if (len > 4096)
		len = 4096;

	/* Encode outside the lock — no shared state touched */
	fc_len = fb_encode_function_call_1str(
		fc_buf, sizeof(fc_buf),
		"HostPrint", 9,
		msg, len,
		HL_FCT_HOST, HL_RT_INT);
	if (fc_len == 0)
		return -1;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return -1;
	}

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	/* Pop and discard the return value (i32 result code) */
	hl_stack_pop(g_input_stack, &result_data, &result_len);

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return 0;
}

__u64 hl_call_get_exn_stack_top(void)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	__u64 val;
	unsigned long irqf;

	if (!g_hcall_ready)
		return 0;

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetExnStackTop",
					 HL_FCT_HOST, HL_RT_ULONG);
	if (fc_len == 0)
		return 0;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	if (fb_decode_result_ulong(result_data, result_len, &val) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return val;
}

__u64 hl_call_get_wall_clock_ns(void)
{
	__u8 fc_buf[256];
	__u64 fc_len;
	const __u8 *result_data;
	__u64 result_len;
	__u64 val;
	unsigned long irqf;

	if (!g_hcall_ready)
		return 0;

	fc_len = fb_encode_function_call(fc_buf, sizeof(fc_buf),
					 "GetWallClockNs",
					 HL_FCT_HOST, HL_RT_ULONG);
	if (fc_len == 0)
		return 0;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  fc_buf, fc_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);

	if (hl_stack_pop(g_input_stack, &result_data, &result_len) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	if (fb_decode_result_ulong(result_data, result_len, &val) < 0) {
		ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
		return 0;
	}

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return val;
}

/* ── Generic FlatBuffer encoder ──────────────────────────────────── */

/*
 * Encode a size-prefixed FunctionCall FlatBuffer with arbitrary typed
 * parameters.  Supports hlint, hlulong, hlstring, and hlvecbytes —
 * enough for all hostfs/hostsock host functions.
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
			pl[i].vvtsz  = VW_SCALAR_VT_SZ;
			pl[i].vtblsz = VW_ULONG_TBL_SZ;
			break;
		case HL_PV_HLSTRING: case HL_PV_HLVECBYTES:
			pl[i].vvtsz  = VW_SCALAR_VT_SZ;
			pl[i].vtblsz = VW_REF_TBL_SZ;
			break;
		default:
			return 0;
		}

		pl[i].vvt  = GA2(pl[i].ptbl + PM_TBL_SZ);
		/* u64 field at vtbl+4 must be 8-byte aligned */
		if (params[i].type == HL_PV_HLLONG ||
		    params[i].type == HL_PV_HLULONG)
			pl[i].vtbl = GA8_OFF4(pl[i].vvt + pl[i].vvtsz);
		else
			pl[i].vtbl = GA4(pl[i].vvt + pl[i].vvtsz);
		pl[i].vdata = 0;
		pos = pl[i].vtbl + pl[i].vtblsz;
	}

	/* Variable-length data (strings, vectors) */
	for (i = 0; i < np; i++) {
		if (params[i].type == HL_PV_HLSTRING) {
			pl[i].vdata = GA4(pos);
			pos = pl[i].vdata + 4
			    + GA4(params[i].str.len + 1);
		} else if (params[i].type == HL_PV_HLVECBYTES) {
			pl[i].vdata = GA4(pos);
			__u64 dlen = params[i].vec.len;

			pos = pl[i].vdata + 4 + GA4(dlen ? dlen : 1);
		}
	}

	__u64 fnpos = GA4(pos);

	pos = fnpos + 4 + GA4(nlen + 1);

	__u64 total = pos;

	if (total > buf_sz)
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
		buf[pl[i].ptbl + 4] = params[i].type;
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
			ew32(buf, pl[i].vtbl + 4,
			     (__u32)(pl[i].vdata - (pl[i].vtbl + 4)));
			ew32(buf, pl[i].vdata, params[i].vec.len);
			if (params[i].vec.len > 0)
				memcpy(buf + pl[i].vdata + 4,
				       params[i].vec.ptr,
				       params[i].vec.len);
			break;
		}
	}

	/* Function name string */
	ew32(buf, fnpos, (__u32)nlen);
	memcpy(buf + fnpos + 4, name, nlen);

	return total;
}

/* ── Additional FunctionCallResult decoders ──────────────────────── */

static int fb_decode_result_int(const __u8 *buf, __u64 buf_len,
				__s32 *out_val)
{
	if (buf_len < 8)
		return -1;

	__u64 root = 4 + fb_u32(buf, 4);

	if (fb_u8_default(buf, root, VT_FCR_RESULT_TYPE, 0)
	    != HL_FCRT_RETURN_VALUE)
		return -1;

	__u64 rvb = fb_follow(buf, root, VT_FCR_RESULT);

	if (!rvb)
		return -1;

	if (fb_u8_default(buf, rvb, VT_RVB_VALUE_TYPE, 0) != HL_RV_HLINT)
		return -1;

	__u64 hli = fb_follow(buf, rvb, VT_RVB_VALUE);

	if (!hli)
		return -1;

	__u16 foff = fb_field(buf, hli, VT_HLS_VALUE);

	if (!foff) {
		/* Field absent → value is the FlatBuffer default (0). */
		*out_val = 0;
	} else {
		*out_val = fb_i32(buf, hli + foff);
	}
	return 0;
}

#define VT_SPB_VALUE 6

static int fb_decode_result_vecbytes(const __u8 *buf, __u64 buf_len,
				     const __u8 **out_data, __u64 *out_len)
{
	if (buf_len < 8)
		return -1;

	__u64 root = 4 + fb_u32(buf, 4);

	if (fb_u8_default(buf, root, VT_FCR_RESULT_TYPE, 0)
	    != HL_FCRT_RETURN_VALUE)
		return -1;

	__u64 rvb = fb_follow(buf, root, VT_FCR_RESULT);

	if (!rvb)
		return -1;

	if (fb_u8_default(buf, rvb, VT_RVB_VALUE_TYPE, 0)
	    != HL_RV_HLSIZEPREFIXED)
		return -1;

	__u64 spb = fb_follow(buf, rvb, VT_RVB_VALUE);

	if (!spb)
		return -1;

	__u64 vec = fb_follow(buf, spb, VT_SPB_VALUE);

	if (!vec) {
		*out_data = NULL;
		*out_len = 0;
		return 0;
	}

	__u32 vlen = fb_u32(buf, vec);

	if (vec + 4 + vlen > buf_len)
		return -1;

	*out_data = buf + vec + 4;
	*out_len = vlen;
	return 0;
}

/* ── Generic host call public API ────────────────────────────────── */

/*
 * Encoder buffer for the generic calls, sized to the PEB output stack: a
 * call that fits the stack fits here, whatever size the host chose.
 *
 * Allocated on first use rather than statically, and outside the lock:
 * the allocator may log through the console, which is itself a host
 * call.  Nothing uses the generic API before the allocator is up -- the
 * boot-time queries above encode into small stack buffers with the
 * fixed-layout encoders -- and the cooperative scheduler serialises the
 * first use, as uk_malloc() does not yield.
 */
static __u8 *g_fc_buf;

static int hl_fc_buf_get(void)
{
	if (likely(g_fc_buf))
		return 0;
	g_fc_buf = uk_malloc(uk_alloc_get_default(), g_output_stack_size);
	if (unlikely(!g_fc_buf)) {
		uk_pr_err("hcall: no memory for a %llu-byte encoder buffer\n",
			  (unsigned long long)g_output_stack_size);
		return -1;
	}
	return 0;
}

int hl_hcall_int(const char *func_name,
		 const struct hl_param *params, int nparams,
		 __s32 *out)
{
	__u64 fc_len;
	const __u8 *rd;
	__u64 rl;
	unsigned long irqf;

	if (!g_hcall_ready || hl_fc_buf_get() < 0)
		return -1;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	fc_len = fb_encode_generic(g_fc_buf, g_output_stack_size,
				   func_name, HL_FCT_HOST, HL_RT_INT,
				   params, nparams);
	if (!fc_len)
		goto err;

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  g_fc_buf, fc_len) < 0)
		goto err;
	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);
	if (hl_stack_pop(g_input_stack, &rd, &rl) < 0)
		goto err;
	if (fb_decode_result_int(rd, rl, out) < 0)
		goto err;

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return 0;
err:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return -1;
}

int hl_hcall_ulong(const char *func_name,
		   const struct hl_param *params, int nparams,
		   __u64 *out)
{
	__u64 fc_len;
	const __u8 *rd;
	__u64 rl;
	unsigned long irqf;

	if (!g_hcall_ready || hl_fc_buf_get() < 0)
		return -1;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	fc_len = fb_encode_generic(g_fc_buf, g_output_stack_size,
				   func_name, HL_FCT_HOST, HL_RT_ULONG,
				   params, nparams);
	if (!fc_len)
		goto err;

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  g_fc_buf, fc_len) < 0)
		goto err;
	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);
	if (hl_stack_pop(g_input_stack, &rd, &rl) < 0)
		goto err;
	if (fb_decode_result_ulong(rd, rl, out) < 0)
		goto err;

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return 0;
err:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return -1;
}

int hl_hcall_string(const char *func_name,
		    const struct hl_param *params, int nparams,
		    char *out_buf, __sz buf_sz, __sz *out_len)
{
	__u64 fc_len;
	const __u8 *rd;
	__u64 rl;
	const char *s;
	__u64 sl;
	unsigned long irqf;

	if (!g_hcall_ready || hl_fc_buf_get() < 0)
		return -1;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	fc_len = fb_encode_generic(g_fc_buf, g_output_stack_size,
				   func_name, HL_FCT_HOST, HL_RT_STRING,
				   params, nparams);
	if (!fc_len)
		goto err;

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  g_fc_buf, fc_len) < 0)
		goto err;
	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);
	if (hl_stack_pop(g_input_stack, &rd, &rl) < 0)
		goto err;
	if (fb_decode_result_string(rd, rl, &s, &sl) < 0)
		goto err;
	if (sl >= buf_sz)
		goto err;
	memcpy(out_buf, s, sl);
	out_buf[sl] = '\0';
	if (out_len)
		*out_len = sl;

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return 0;
err:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return -1;
}

int hl_hcall_vecbytes(const char *func_name,
		      const struct hl_param *params, int nparams,
		      __u8 *out_buf, __sz buf_sz, __sz *out_len)
{
	__u64 fc_len;
	const __u8 *rd;
	__u64 rl;
	const __u8 *vd;
	__u64 vl;
	unsigned long irqf;

	if (!g_hcall_ready || hl_fc_buf_get() < 0)
		return -1;

	ukplat_spin_lock_irqsave(&g_hcall_lock, irqf);

	fc_len = fb_encode_generic(g_fc_buf, g_output_stack_size,
				   func_name, HL_FCT_HOST, HL_RT_VECBYTES,
				   params, nparams);
	if (!fc_len)
		goto err;

	if (hl_stack_push(g_output_stack, g_output_stack_size,
			  g_fc_buf, fc_len) < 0)
		goto err;
	hyperlight_out32(HYPERLIGHT_PORT_CALL_FUNCTION, 0);
	if (hl_stack_pop(g_input_stack, &rd, &rl) < 0)
		goto err;
	if (fb_decode_result_vecbytes(rd, rl, &vd, &vl) < 0)
		goto err;
	if (vl > buf_sz)
		vl = buf_sz;
	if (vl > 0 && vd)
		memcpy(out_buf, vd, vl);
	*out_len = vl;

	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return 0;
err:
	ukplat_spin_unlock_irqrestore(&g_hcall_lock, irqf);
	return -1;
}

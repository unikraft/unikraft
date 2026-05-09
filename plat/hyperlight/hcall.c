/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * /dev/hcall - Hyperlight host function call device
 *
 * Provides a simple interface for user-space to call host functions:
 *   write() - Send JSON request, triggers host call, stores result
 *   read()  - Retrieve JSON result from last call
 *
 * Internally handles FlatBuffer encoding/decoding and shared memory
 * communication with the Hyperlight host via the __dispatch protocol.
 *
 * Protocol:
 *   1. User writes JSON: {"name":"tool_name","args":{...}}
 *   2. Driver encodes as FlatBuffer FunctionCall for __dispatch(VecBytes)
 *   3. Pushes to PEB output_stack, triggers outb(101) VM exit
 *   4. Host decodes, routes to tool handler, encodes result
 *   5. Driver pops result from PEB input_stack
 *   6. Decodes FlatBuffer FunctionCallResult to extract VecBytes
 *   7. User reads JSON result
 */

#include <string.h>
#include <uk/arch/types.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <vfscore/uio.h>
#include <devfs/device.h>

#include <hyperlight-x86/peb.h>
#include <hyperlight-x86/setup.h>
#include <hyperlight-x86/outb.h>

/* Maximum payload size for host calls */
#define HCALL_MAX_PAYLOAD 65536

/* ========================================================================
 * FlatBuffer Encoder
 * ========================================================================
 *
 * Encodes exactly one FlatBuffer shape:
 *   FunctionCall {
 *     function_name: "__dispatch",
 *     parameters: [Parameter { value: hlvecbytes { value: <payload> } }],
 *     function_call_type: Host(2),
 *     expected_return_type: hlsizeprefixedbuffer(9)
 *   }
 *
 * The FlatBuffer is size-prefixed. The fixed template (bytes 4-95) is
 * constant for all payloads. Only the size prefix (bytes 0-3) and
 * payload vector (bytes 96+) vary.
 *
 * Total size: 100 + ALIGN4(payload_len)
 */

/*
 * Fixed template: bytes 4-95 of the encoded FlatBuffer (92 bytes).
 * Contains: root_offset, FunctionCall vtable+table, "__dispatch" string,
 * parameters vector, Parameter vtable+table, hlvecbytes vtable+table.
 * All internal offsets are pre-computed and constant.
 */
static const __u8 hcall_fb_template[92] = {
	/* root_offset = 16 (from byte 4 to FunctionCall table at byte 20) */
	0x10, 0x00, 0x00, 0x00,

	/* FunctionCall vtable (12 bytes at offset 8) */
	0x0C, 0x00,             /* vtable_size = 12 */
	0x10, 0x00,             /* table_inline_size = 16 */
	0x04, 0x00,             /* VT[4] function_name at table+4 */
	0x08, 0x00,             /* VT[6] parameters at table+8 */
	0x0C, 0x00,             /* VT[8] function_call_type at table+12 */
	0x0D, 0x00,             /* VT[10] expected_return_type at table+13 */

	/* FunctionCall table (16 bytes at offset 20) */
	0x0C, 0x00, 0x00, 0x00, /* soffset to vtable = 20-8 = 12 */
	0x0C, 0x00, 0x00, 0x00, /* fn_name offset = 36-24 = 12 */
	0x18, 0x00, 0x00, 0x00, /* params offset = 52-28 = 24 */
	0x02,                   /* function_call_type = Host(2) */
	0x09,                   /* expected_return_type = hlsizeprefixedbuffer(9) */
	0x00, 0x00,             /* padding */

	/* function_name string "__dispatch" (16 bytes at offset 36) */
	0x0A, 0x00, 0x00, 0x00, /* length = 10 */
	0x5F, 0x5F, 0x64, 0x69, /* "__di" */
	0x73, 0x70, 0x61, 0x74, /* "spat" */
	0x63, 0x68,             /* "ch" */
	0x00,                   /* NUL terminator */
	0x00,                   /* padding to 4-byte alignment */

	/* parameters vector (8 bytes at offset 52) */
	0x01, 0x00, 0x00, 0x00, /* length = 1 */
	0x0C, 0x00, 0x00, 0x00, /* offset to Parameter table = 68-56 = 12 */

	/* Parameter vtable (8 bytes at offset 60) */
	0x08, 0x00,             /* vtable_size = 8 */
	0x0C, 0x00,             /* table_inline_size = 12 */
	0x04, 0x00,             /* VT[4] value_type at table+4 */
	0x08, 0x00,             /* VT[6] value at table+8 */

	/* Parameter table (12 bytes at offset 68) */
	0x08, 0x00, 0x00, 0x00, /* soffset to vtable = 68-60 = 8 */
	0x09, 0x00, 0x00, 0x00, /* value_type=hlvecbytes(9) + 3 bytes padding */
	0x0C, 0x00, 0x00, 0x00, /* value offset = 88-76 = 12 */

	/* hlvecbytes vtable (6 bytes + 2 padding at offset 80) */
	0x06, 0x00,             /* vtable_size = 6 */
	0x08, 0x00,             /* table_inline_size = 8 */
	0x04, 0x00,             /* VT[4] value at table+4 */
	0x00, 0x00,             /* padding to 4-byte alignment */

	/* hlvecbytes table (8 bytes at offset 88) */
	0x08, 0x00, 0x00, 0x00, /* soffset to vtable = 88-80 = 8 */
	0x04, 0x00, 0x00, 0x00, /* value offset = 96-92 = 4 */
};

#define FB_TEMPLATE_OFFSET 4  /* template starts at byte 4 in output */
#define FB_HEADER_SIZE     100 /* fixed overhead before payload data */

/**
 * Encode a __dispatch host function call as a size-prefixed FlatBuffer.
 *
 * @param buf Output buffer (must be >= FB_HEADER_SIZE + ALIGN4(payload_len))
 * @param buf_sz Size of output buffer
 * @param payload JSON payload bytes
 * @param payload_len Length of payload
 * @return Total encoded size, or 0 on error
 */
static __sz hcall_encode(__u8 *buf, __sz buf_sz,
			 const __u8 *payload, __sz payload_len)
{
	__sz aligned_len = (payload_len + 3) & ~(__sz)3;
	__sz total_size = FB_HEADER_SIZE + aligned_len;

	if (total_size > buf_sz)
		return 0;

	/* Byte 0-3: size prefix (total - 4) */
	__u32 sp = (__u32)(total_size - 4);

	buf[0] = sp & 0xFF;
	buf[1] = (sp >> 8) & 0xFF;
	buf[2] = (sp >> 16) & 0xFF;
	buf[3] = (sp >> 24) & 0xFF;

	/* Bytes 4-95: fixed template */
	memcpy(buf + FB_TEMPLATE_OFFSET, hcall_fb_template,
	       sizeof(hcall_fb_template));

	/* Byte 96-99: payload vector length */
	__u32 plen = (__u32)payload_len;

	buf[96] = plen & 0xFF;
	buf[97] = (plen >> 8) & 0xFF;
	buf[98] = (plen >> 16) & 0xFF;
	buf[99] = (plen >> 24) & 0xFF;

	/* Byte 100+: payload data + padding */
	if (payload_len > 0)
		memcpy(buf + FB_HEADER_SIZE, payload, payload_len);
	if (aligned_len > payload_len)
		memset(buf + FB_HEADER_SIZE + payload_len, 0,
		       aligned_len - payload_len);

	return total_size;
}

/* ========================================================================
 * FlatBuffer Decoder
 * ========================================================================
 * Decodes FunctionCallResult to extract the VecBytes return payload.
 *
 * Expected structure:
 *   FunctionCallResult {
 *     result_type: ReturnValueBox(1),
 *     result: ReturnValueBox {
 *       value_type: hlsizeprefixedbuffer(10),
 *       value: hlsizeprefixedbuffer { size: i32, value: [ubyte] }
 *     }
 *   }
 */

static inline __u32 fb_u32(const __u8 *buf, __sz off)
{
	return buf[off] | ((__u32)buf[off + 1] << 8) |
	       ((__u32)buf[off + 2] << 16) | ((__u32)buf[off + 3] << 24);
}

static inline __u16 fb_u16(const __u8 *buf, __sz off)
{
	return buf[off] | ((__u16)buf[off + 1] << 8);
}

static inline __s32 fb_i32(const __u8 *buf, __sz off)
{
	return (__s32)fb_u32(buf, off);
}

/* Get vtable position for a table at tbl */
static inline __sz fb_vtable(const __u8 *buf, __sz tbl)
{
	__s32 soff = fb_i32(buf, tbl);

	return tbl - soff;
}

/* Get field offset from vtable (returns 0 if field not present) */
static inline __u16 fb_field(const __u8 *buf, __sz tbl, __u16 vt_off)
{
	__sz vt = fb_vtable(buf, tbl);
	__u16 vt_size = fb_u16(buf, vt);

	if (vt_off >= vt_size)
		return 0;
	return fb_u16(buf, vt + vt_off);
}

/* Follow a UOffset field to its target */
static inline __sz fb_follow(const __u8 *buf, __sz tbl, __u16 vt_off)
{
	__u16 foff = fb_field(buf, tbl, vt_off);

	if (foff == 0)
		return 0;

	__sz field_pos = tbl + foff;

	return field_pos + fb_u32(buf, field_pos);
}

/* Read u8 scalar field with default */
static inline __u8 fb_u8f(const __u8 *buf, __sz tbl,
			   __u16 vt_off, __u8 defval)
{
	__u16 foff = fb_field(buf, tbl, vt_off);

	if (foff == 0)
		return defval;
	return buf[tbl + foff];
}

/* Union discriminant values for decoding */
#define FCR_RESULT_TYPE_RVBOX     1   /* FunctionCallResultType::ReturnValueBox */
#define RV_TYPE_SIZEPREFIXEDBUF  10   /* ReturnValue::hlsizeprefixedbuffer */

/**
 * Decode a FunctionCallResult FlatBuffer to extract the VecBytes payload.
 *
 * @param buf Size-prefixed FlatBuffer data
 * @param buf_len Length of buffer
 * @param out_data Output: pointer to result bytes (within buf)
 * @param out_len Output: length of result bytes
 * @return 0 on success, negative on error
 */
static int hcall_decode(const __u8 *buf, __sz buf_len,
			const __u8 **out_data, __sz *out_len)
{
	if (buf_len < 8)
		return -1;

	/* Root table (size-prefixed: skip 4-byte prefix) */
	__u32 root_off = fb_u32(buf, 4);
	__sz fcr = 4 + root_off;

	/* FunctionCallResult.result_type (VT=4) == ReturnValueBox(1)? */
	__u8 result_type = fb_u8f(buf, fcr, 4, 0);

	if (result_type != FCR_RESULT_TYPE_RVBOX)
		return -2;

	/* Follow result (VT=6) -> ReturnValueBox */
	__sz rvb = fb_follow(buf, fcr, 6);

	if (rvb == 0)
		return -3;

	/* ReturnValueBox.value_type (VT=4) == hlsizeprefixedbuffer(10)? */
	__u8 value_type = fb_u8f(buf, rvb, 4, 0);

	if (value_type != RV_TYPE_SIZEPREFIXEDBUF)
		return -4;

	/* Follow value (VT=6) -> hlsizeprefixedbuffer */
	__sz spb = fb_follow(buf, rvb, 6);

	if (spb == 0)
		return -5;

	/* Follow value vector (VT=6) -> byte vector */
	__sz vec = fb_follow(buf, spb, 6);

	if (vec == 0)
		return -6;

	__u32 vec_len = fb_u32(buf, vec);

	*out_data = buf + vec + 4;
	*out_len = vec_len;

	return 0;
}

/* ========================================================================
 * Shared Memory Stack Protocol
 * ========================================================================
 * Implements push (to output_stack) and pop (from input_stack)
 * matching Hyperlight's io.rs protocol.
 *
 * Stack layout:
 *   [stack_ptr:u64] [data] [back_ptr:u64] [data] [back_ptr:u64] ...
 *
 * stack_ptr at offset 0 points to next free byte (initially 8).
 * Each push appends: data bytes + back_ptr (8 bytes = old stack_ptr).
 * Each pop reads back_ptr to find data start, resets stack_ptr.
 */

static inline __u64 read_u64_le(const __u8 *p)
{
	return p[0] | ((__u64)p[1] << 8) | ((__u64)p[2] << 16) |
	       ((__u64)p[3] << 24) | ((__u64)p[4] << 32) |
	       ((__u64)p[5] << 40) | ((__u64)p[6] << 48) |
	       ((__u64)p[7] << 56);
}

static inline void write_u64_le(__u8 *p, __u64 v)
{
	p[0] = v & 0xFF;
	p[1] = (v >> 8) & 0xFF;
	p[2] = (v >> 16) & 0xFF;
	p[3] = (v >> 24) & 0xFF;
	p[4] = (v >> 32) & 0xFF;
	p[5] = (v >> 40) & 0xFF;
	p[6] = (v >> 48) & 0xFF;
	p[7] = (v >> 56) & 0xFF;
}

/**
 * Push data onto a shared memory stack (output_stack).
 */
static int hcall_push(__u8 *stack, __u64 stack_size,
		      const __u8 *data, __sz data_len)
{
	__u64 sp = read_u64_le(stack);

	if (sp < 8 || sp > stack_size)
		return -1;

	/* Need space for data + 8-byte back pointer */
	if (sp + data_len + 8 > stack_size)
		return -1;

	/* Write data */
	memcpy(stack + sp, data, data_len);

	/* Write back pointer (old sp) after data */
	write_u64_le(stack + sp + data_len, sp);

	/* Update stack pointer */
	write_u64_le(stack, sp + data_len + 8);

	return 0;
}

/**
 * Pop data from a shared memory stack (input_stack).
 * Returns pointer into the stack buffer (valid until next pop/push).
 */
static int hcall_pop(__u8 *stack, __u64 stack_size,
		     const __u8 **out_data, __sz *out_len)
{
	__u64 sp = read_u64_le(stack);

	if (sp < 16 || sp > stack_size)
		return -1;

	/* Read back pointer (8 bytes before current sp) */
	__u64 back_ptr = read_u64_le(stack + sp - 8);

	if (back_ptr < 8 || back_ptr >= sp)
		return -1;

	*out_data = stack + back_ptr;
	*out_len = sp - 8 - back_ptr;

	/* Reset stack pointer to free the popped data.
	 * Note: we do NOT zero the freed region here because out_data
	 * points into it and the caller still needs to read the data.
	 */
	write_u64_le(stack, back_ptr);

	return 0;
}

/* ========================================================================
 * /dev/hcall Device Driver
 * ======================================================================== */

/* Static buffers */
static __u8 hcall_req_buf[HCALL_MAX_PAYLOAD];
static __sz hcall_req_len;

static __u8 hcall_result_buf[HCALL_MAX_PAYLOAD];
static __sz hcall_result_len;
static __sz hcall_result_pos;

static __u8 hcall_encode_buf[HCALL_MAX_PAYLOAD + 256];

/**
 * Execute the __dispatch host function call.
 */
static int hcall_dispatch(void)
{
	struct hyperlight_peb *peb = hyperlight_get_peb();
	__u8 *output_stack;
	__u64 output_size;
	__u8 *input_stack;
	__u64 input_size;
	__sz fb_len;
	const __u8 *result_fb;
	__sz result_fb_len;
	const __u8 *payload_data;
	__sz payload_len;
	int rc;

	if (!peb)
		return -1;

	output_stack = (__u8 *)peb->output_stack.ptr;
	output_size = peb->output_stack.size;
	input_stack = (__u8 *)peb->input_stack.ptr;
	input_size = peb->input_stack.size;

	if (!output_stack || !input_stack ||
	    output_size == 0 || input_size == 0)
		return -2;

	/* 1. Encode FlatBuffer */
	fb_len = hcall_encode(hcall_encode_buf, sizeof(hcall_encode_buf),
			      hcall_req_buf, hcall_req_len);
	if (fb_len == 0)
		return -3;

	/* 2. Push to output_stack */
	rc = hcall_push(output_stack, output_size,
			hcall_encode_buf, fb_len);
	if (rc < 0)
		return -4;

	/* 3. Trigger host function call (VM exit) */
	hyperlight_out32(HYPERLIGHT_OUTB_CALL_FUNCTION, 0);

	/* 4. Pop result from input_stack */
	rc = hcall_pop(input_stack, input_size,
		       &result_fb, &result_fb_len);
	if (rc < 0)
		return -5;

	/* 5. Decode FlatBuffer result */
	rc = hcall_decode(result_fb, result_fb_len,
			  &payload_data, &payload_len);
	if (rc < 0)
		return -6;

	/* 6. Store result for read() */
	if (payload_len > HCALL_MAX_PAYLOAD)
		payload_len = HCALL_MAX_PAYLOAD;
	memcpy(hcall_result_buf, payload_data, payload_len);
	hcall_result_len = payload_len;
	hcall_result_pos = 0;

	return 0;
}

/**
 * Write handler: receive JSON request and dispatch to host.
 */
static int dev_hcall_write(struct device *dev __unused,
			   struct uio *uio, int flags __unused)
{
	__sz len = uio->uio_iov->iov_len;
	int rc;

	if (len > HCALL_MAX_PAYLOAD)
		return ENOMEM;

	/* Copy request data */
	memcpy(hcall_req_buf, uio->uio_iov->iov_base, len);
	hcall_req_len = len;

	/* Dispatch to host */
	rc = hcall_dispatch();
	if (rc < 0) {
		/* Encode error step into JSON so user-space can diagnose:
		 * -1: null PEB
		 * -2: null/zero stacks
		 * -3: FlatBuffer encode failed
		 * -4: push to output_stack failed
		 * -5: pop from input_stack failed
		 * -6: FlatBuffer decode failed
		 */
		char err[64];
		int n = 0;
		int abs_rc = rc < 0 ? -rc : rc;

		/* sprintf not available, format manually */
		memcpy(err, "{\"error\":\"hcall step ", 21);
		n = 21;
		if (abs_rc >= 10)
			err[n++] = '0' + (abs_rc / 10);
		err[n++] = '0' + (abs_rc % 10);
		memcpy(err + n, " failed\"}", 9);
		n += 9;

		memcpy(hcall_result_buf, err, n);
		hcall_result_len = n;
		hcall_result_pos = 0;
	}

	uio->uio_resid = 0;
	return 0;
}

/**
 * Read handler: return result from last dispatch call.
 */
static int dev_hcall_read(struct device *dev __unused,
			  struct uio *uio, int flags __unused)
{
	__sz avail = hcall_result_len - hcall_result_pos;
	__sz len = uio->uio_iov->iov_len;

	if (avail == 0) {
		/* No more data - signal EOF */
		return 0;
	}

	if (len > avail)
		len = avail;

	memcpy(uio->uio_iov->iov_base,
	       hcall_result_buf + hcall_result_pos, len);
	hcall_result_pos += len;
	uio->uio_resid = uio->uio_iov->iov_len - len;

	return 0;
}

static struct devops hcall_devops = {
	.open  = dev_noop_open,
	.close = dev_noop_close,
	.read  = dev_hcall_read,
	.write = dev_hcall_write,
	.ioctl = dev_noop_ioctl,
};

static struct driver drv_hcall = {
	.devops = &hcall_devops,
	.devsz  = 0,
	.name   = "hcall",
};

static int devfs_register_hcall(struct uk_init_ctx *ictx __unused)
{
	int rc;

	rc = device_create(&drv_hcall, "hcall", D_CHR, NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register /dev/hcall: %d\n", rc);
		return -rc;
	}

	uk_pr_info("Registered /dev/hcall\n");
	return 0;
}

devfs_initcall(devfs_register_hcall);

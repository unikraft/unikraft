/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors. */

#ifndef __HYPERLIGHT_X86_FB_H__
#define __HYPERLIGHT_X86_FB_H__

/*
 * Tiny, header-only FlatBuffer reader for the fixed Hyperlight
 * FunctionCall / FunctionCallResult shapes.
 *
 * FlatBuffers table/vtable layout (little-endian):
 *   table_base:     [soffset_to_vtable:i32][table_inline_bytes...]
 *   vtable (above): [vtable_size:u16][table_inline_size:u16]
 *                   [field_offset_0:u16][field_offset_1:u16]...
 *   soffset_to_vtable = (signed) table_base - vtable_base, so
 *     vtable_base = table_base - soffset_to_vtable.
 *
 * Size-prefixed buffers have a 4-byte little-endian u32 length prefix
 * followed by a u32 root_offset (byte 4 of buffer) that points from
 * its own position to the root table.
 */

#include <uk/arch/types.h>

static inline __u32 hl_fb_u32(const __u8 *buf, __sz off)
{
	return buf[off] | ((__u32)buf[off + 1] << 8) |
	       ((__u32)buf[off + 2] << 16) | ((__u32)buf[off + 3] << 24);
}

static inline __u16 hl_fb_u16(const __u8 *buf, __sz off)
{
	return buf[off] | ((__u16)buf[off + 1] << 8);
}

static inline __s32 hl_fb_i32(const __u8 *buf, __sz off)
{
	return (__s32)hl_fb_u32(buf, off);
}

/* Given a table position, resolve the vtable it points to. */
static inline __sz hl_fb_vtable(const __u8 *buf, __sz tbl)
{
	__s32 soff = hl_fb_i32(buf, tbl);

	return tbl - soff;
}

/* Read the field offset slot at `vt_off` in the vtable of table `tbl`.
 * Returns 0 if the field is absent (table shorter than vt_off).
 */
static inline __u16 hl_fb_field(const __u8 *buf, __sz tbl, __u16 vt_off)
{
	__sz vt = hl_fb_vtable(buf, tbl);
	__u16 vt_size = hl_fb_u16(buf, vt);

	if (vt_off >= vt_size)
		return 0;
	return hl_fb_u16(buf, vt + vt_off);
}

/* Follow a uoffset field (string / vector / sub-table) to its target.
 * Returns 0 if the field is absent.
 */
static inline __sz hl_fb_follow(const __u8 *buf, __sz tbl, __u16 vt_off)
{
	__u16 foff = hl_fb_field(buf, tbl, vt_off);

	if (foff == 0)
		return 0;

	__sz field_pos = tbl + foff;

	return field_pos + hl_fb_u32(buf, field_pos);
}

/* Read a u8 scalar inlined in the table, with a default when absent. */
static inline __u8 hl_fb_u8f(const __u8 *buf, __sz tbl,
			     __u16 vt_off, __u8 defval)
{
	__u16 foff = hl_fb_field(buf, tbl, vt_off);

	if (foff == 0)
		return defval;
	return buf[tbl + foff];
}

/* ParameterValue union discriminants (must stay in sync with
 * src/schema/function_types.fbs).
 */
#define HL_PV_NONE       0
#define HL_PV_HLINT      1
#define HL_PV_HLUINT     2
#define HL_PV_HLLONG     3
#define HL_PV_HLULONG    4
#define HL_PV_HLFLOAT    5
#define HL_PV_HLDOUBLE   6
#define HL_PV_HLSTRING   7
#define HL_PV_HLBOOL     8
#define HL_PV_HLVECBYTES 9

#endif /* __HYPERLIGHT_X86_FB_H__ */

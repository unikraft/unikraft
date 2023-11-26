/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __LXBOOT_H__
#define __LXBOOT_H__

#define LXBOOT_HDR_HEADER_MAGIC			0x53726448
#define LXBOOT_HDR_HEADER_OFFSET		(497 + 17)

#ifndef __ASSEMBLY__
#include <uk/essentials.h>

/* Linux Boot Protocol */

struct lxboot_setup_header {
	__u8 setup_sects;
	__u16 root_flags;
	__u32 syssize;
	__u16 ram_size;
	__u16 vid_mode;
	__u16 root_dev;
	__u16 boot_flag;
	__u16 jump;
	__u32 header;
	__u16 version;
	__u32 realmode_swtch;
	__u16 start_sys_seg;
	__u16 kernel_version;
	__u8 type_of_loader;
	__u8 loadflags;
	__u16 setup_move_size;
	__u32 code32_start;
	__u32 ramdisk_image;
	__u32 ramdisk_size;
	__u32 bootsect_kludge;
	__u16 heap_end_ptr;
	__u8 ext_loader_ver;
	__u8 ext_loader_type;
	__u32 cmd_line_ptr;
	__u32 initrd_addr_max;
	__u32 kernel_alignment;
	__u8 relocatable_kernel;
	__u8 min_alignment;
	__u16 xloadflags;
	__u32 cmd_line_size;
	__u32 hardware_subarch;
	__u64 hardware_subarch_data;
	__u32 payload_offset;
	__u32 payload_length;
	__u64 setup_data;
	__u64 pref_address;
	__u32 init_size;
	__u32 handover_offset;
} __packed;

struct lxboot_screen_info {
	__u8 orig_x;
	__u8 orig_y;
	__u16 ext_mem_k;
	__u16 orig_video_page;
	__u8 orig_video_mode;
	__u8 orig_video_cols;
	__u8 flags;
	__u8 unused2;
	__u16 orig_video_ega_bx;
	__u16 unused3;
	__u8 orig_video_lines;
	__u8 orig_video_isVGA;
	__u16 orig_video_points;
	__u16 lfb_width;
	__u16 lfb_height;
	__u16 lfb_depth;
	__u32 lfb_base;
	__u32 lfb_size;
	__u16 cl_magic;
	__u16 cl_offset;
	__u16 lfb_linelength;
	__u8 red_size;
	__u8 red_pos;
	__u8 green_size;
	__u8 green_pos;
	__u8 blue_size;
	__u8 blue_pos;
	__u8 rsvd_size;
	__u8 rsvd_pos;
	__u16 vesapm_seg;
	__u16 vesapm_off;
	__u16 pages;
	__u16 vesa_attributes;
	__u32 capabilities;
	__u32 ext_lfb_base;
	__u8 reserved[2];
} __packed;

struct lxboot_apm_bios_info {
	__u16 version;
	__u16 cseg;
	__u32 offset;
	__u16 cseg_16;
	__u16 dseg;
	__u16 flags;
	__u16 cseg_len;
	__u16 cseg_16_len;
	__u16 dseg_len;
} __packed;

struct lxboot_ist_info {
	__u32 signature;
	__u32 command;
	__u32 event;
	__u32 perf_level;
} __packed;

struct lxboot_sys_desc_table {
	__u16 length;
	__u8 table[14];
} __packed;

struct lxboot_olpc_ofw_header {
	__u32 ofw_magic;
	__u32 ofw_version;
	__u32 cif_handler;
	__u32 irq_desc_table;
} __packed;

struct lxboot_e820_entry {
	__u64 addr;
	__u64 size;
#define LXBOOT_E820_TYPE_RAM		1
#define LXBOOT_E820_TYPE_RESERVED	2
#define LXBOOT_E820_TYPE_ACPI		3
#define LXBOOT_E820_TYPE_NVS		4
#define LXBOOT_E820_TYPE_UNUSABLE	5
#define LXBOOT_E820_TYPE_PMEM		7
	__u32 type;
} __packed;

struct lxboot_edid_info {
	unsigned char dummy[128];
} __packed;

struct lxboot_efi_info {
	__u32 efi_loader_signature;
	__u32 efi_systab;
	__u32 efi_memdesc_size;
	__u32 efi_memdesc_version;
	__u32 efi_memmap;
	__u32 efi_memmap_size;
	__u32 efi_systab_hi;
	__u32 efi_memmap_hi;
} __packed;

struct lxboot_params {
	struct lxboot_screen_info screen_info;
	struct lxboot_apm_bios_info apm_bios_info;
	__u8 _pad2[4];
	__u64 tboot_addr;
	struct lxboot_ist_info ist_info;
	__u64 acpi_rsdp_addr;
	__u8 _pad3[8];
	__u8 hd0_info[16];
	__u8 hd1_info[16];
	struct lxboot_sys_desc_table sys_desc_table;
	struct lxboot_olpc_ofw_header olpc_ofw_header;
	__u32 ext_ramdisk_image;
	__u32 ext_ramdisk_size;
	__u32 ext_cmd_line_ptr;
	__u8 _pad4[116];
	struct lxboot_edid_info edid_info;
	struct lxboot_efi_info efi_info;
	__u32 alt_mem_k;
	__u32 scratch;
	__u8 e820_entries;
	__u8 eddbuf_entries;
	__u8 edd_mbr_sig_buf_entries;
	__u8 kbd_status;
	__u8 secure_boot;
	__u8 _pad5[2];
	__u8 sentinel;
	__u8 _pad6[1];
	struct lxboot_setup_header hdr;
	__u8 _pad7[40];
	__u32 ebb_mbr_sig_buffer[16];
#define LXBOOT_E820_MAX_ENTRIES 128
	struct lxboot_e820_entry e820_table[LXBOOT_E820_MAX_ENTRIES];
	/* Leave out definitions for BIOS Enhanced Disk Drive (EDD) for now */
	__u8 _pad8[4096 - 3280];
} __packed;

UK_CTASSERT(__offsetof(struct lxboot_params, hdr) +
	    __offsetof(struct lxboot_setup_header, header) ==
	    LXBOOT_HDR_HEADER_OFFSET);

#endif /* !__ASSEMBLY__ */
#endif /* ! __LXBOOT_H__ */

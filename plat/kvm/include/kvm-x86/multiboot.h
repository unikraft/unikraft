/* SPDX-License-Identifier: MIT */
/* Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 * DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

/* How many bytes from the start of the file we search for the header. */
#define MULTIBOOT_SEARCH			8192
#define MULTIBOOT_HEADER_ALIGN			4

/* The magic field should contain this. */
#define MULTIBOOT_HEADER_MAGIC			0x1BADB002

/* This should be in %eax. */
#define MULTIBOOT_BOOTLOADER_MAGIC		0x2BADB002

/* Align all boot modules on i386 page (4KB) boundaries. */
#define MULTIBOOT_PAGE_ALIGN			0x00000001

/* Must pass memory information to OS. */
#define MULTIBOOT_MEMORY_INFO			0x00000002

/* Must pass video information to OS. */
#define MULTIBOOT_VIDEO_MODE			0x00000004

/* This flag indicates the use of the address fields in the header. */
#define MULTIBOOT_AOUT_KLUDGE			0x00010000

#ifndef __ASSEMBLY__
#include <uk/essentials.h>
#include <uk/arch/types.h>

typedef __u8  multiboot___u8;
typedef __u16 multiboot___u16;
typedef __u32 multiboot___u32;
typedef __u64 multiboot___u64;

struct multiboot_header {
	/* Must be MULTIBOOT_MAGIC - see above */
	multiboot___u32 magic;

	/* Feature flags. */
	multiboot___u32 flags;

	/* The above fields plus this one must equal 0 mod 2^32 */
	multiboot___u32 checksum;

	/* These are only valid if MULTIBOOT_AOUT_KLUDGE is set */
	multiboot___u32 header_addr;
	multiboot___u32 load_addr;
	multiboot___u32 load_end_addr;
	multiboot___u32 bss_end_addr;
	multiboot___u32 entry_addr;

	/* These are only valid if MULTIBOOT_VIDEO_MODE is set */
	multiboot___u32 mode_type;
	multiboot___u32 width;
	multiboot___u32 height;
	multiboot___u32 depth;
};

/* The symbol table for a.out */
struct multiboot_aout_symbol_table {
	multiboot___u32 tabsize;
	multiboot___u32 strsize;
	multiboot___u32 addr;
	multiboot___u32 reserved;
};
typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

/* The section header table for ELF */
struct multiboot_elf_section_header_table {
	multiboot___u32 num;
	multiboot___u32 size;
	multiboot___u32 addr;
	multiboot___u32 shndx;
};
typedef struct multiboot_elf_section_header_table
		multiboot_elf_section_header_table_t;

struct multiboot_info {
	/* Multiboot info version number */
#define MULTIBOOT_INFO_MEMORY			0x00000001
#define MULTIBOOT_INFO_BOOTDEV			0x00000002
#define MULTIBOOT_INFO_CMDLINE			0x00000004
#define MULTIBOOT_INFO_MODS			0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS		0x00000010
#define MULTIBOOT_INFO_ELF_SHDR			0X00000020
#define MULTIBOOT_INFO_MEM_MAP			0x00000040
#define MULTIBOOT_INFO_DRIVE_INFO		0x00000080
#define MULTIBOOT_INFO_CONFIG_TABLE		0x00000100
#define MULTIBOOT_INFO_BOOT_LOADER_NAME		0x00000200
#define MULTIBOOT_INFO_APM_TABLE		0x00000400
#define MULTIBOOT_INFO_VBE_INFO			0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO		0x00001000
	multiboot___u32 flags;

	/* Available memory from BIOS */
	multiboot___u32 mem_lower;
	multiboot___u32 mem_upper;

	/* "root" partition */
	multiboot___u32 boot_device;

	/* Kernel command line */
	multiboot___u32 cmdline;

	/* Boot-Module list */
	multiboot___u32 mods_count;
	multiboot___u32 mods_addr;

	union {
		multiboot_aout_symbol_table_t aout_sym;
		multiboot_elf_section_header_table_t elf_sec;
	} u;

	/* Memory Mapping buffer */
	multiboot___u32 mmap_length;
	multiboot___u32 mmap_addr;

	/* Drive Info buffer */
	multiboot___u32 drives_length;
	multiboot___u32 drives_addr;

	/* ROM configuration table */
	multiboot___u32 config_table;

	/* Boot Loader Name */
	multiboot___u32 boot_loader_name;

	/* APM table */
	multiboot___u32 apm_table;

	/* Video */
	multiboot___u32 vbe_control_info;
	multiboot___u32 vbe_mode_info;
	multiboot___u16 vbe_mode;
	multiboot___u16 vbe_interface_seg;
	multiboot___u16 vbe_interface_off;
	multiboot___u16 vbe_interface_len;

	multiboot___u64 framebuffer_addr;
	multiboot___u32 framebuffer_pitch;
	multiboot___u32 framebuffer_width;
	multiboot___u32 framebuffer_height;
	multiboot___u8 framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED		0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB			1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT		2
	multiboot___u8 framebuffer_type;
	union {
		struct {
			multiboot___u32 framebuffer_palette_addr;
			multiboot___u16 framebuffer_palette_num_colors;
		};
		struct {
			multiboot___u8 framebuffer_red_field_position;
			multiboot___u8 framebuffer_red_mask_size;
			multiboot___u8 framebuffer_green_field_position;
			multiboot___u8 framebuffer_green_mask_size;
			multiboot___u8 framebuffer_blue_field_position;
			multiboot___u8 framebuffer_blue_mask_size;
		};
	};
};
typedef struct multiboot_info multiboot_info_t;

struct multiboot_color {
	multiboot___u8 red;
	multiboot___u8 green;
	multiboot___u8 blue;
};

struct multiboot_mmap_entry {
	multiboot___u32 size;
	multiboot___u64 addr;
	multiboot___u64 len;
#define MULTIBOOT_MEMORY_AVAILABLE			1
#define MULTIBOOT_MEMORY_RESERVED			2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE		3
#define MULTIBOOT_MEMORY_NVS				4
#define MULTIBOOT_MEMORY_BADRAM				5
	multiboot___u32 type;
} __packed;
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

struct multiboot_mod_list {
	/* Memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
	multiboot___u32 mod_start;
	multiboot___u32 mod_end;

	/* Module command line */
	multiboot___u32 cmdline;

	/* Padding to take it to 16 bytes (must be zero) */
	multiboot___u32 pad;
};
typedef struct multiboot_mod_list multiboot_module_t;

/* APM BIOS info. */
struct multiboot_apm_info {
	multiboot___u16 version;
	multiboot___u16 cseg;
	multiboot___u32 offset;
	multiboot___u16 cseg_16;
	multiboot___u16 dseg;
	multiboot___u16 flags;
	multiboot___u16 cseg_len;
	multiboot___u16 cseg_16_len;
	multiboot___u16 dseg_len;
};

#endif /* !__ASSEMBLY__ */
#endif /* ! __MULTIBOOT_H__ */

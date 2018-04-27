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
/* multiboot.h - Multiboot header file.  */

#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER 1

#include <uk/essentials.h>

typedef unsigned char multiboot_uint8_t;
typedef unsigned short multiboot_uint16_t;
typedef unsigned int multiboot_uint32_t;
typedef unsigned long long multiboot_uint64_t;

struct multiboot_header {
	/* Must be MULTIBOOT_MAGIC - see above.  */
	multiboot_uint32_t magic;

	/* Feature flags.  */
	multiboot_uint32_t flags;

	/* The above fields plus this one must equal 0 mod 2^32. */
	multiboot_uint32_t checksum;

	/* These are only valid if MULTIBOOT_AOUT_KLUDGE is set.  */
	multiboot_uint32_t header_addr;
	multiboot_uint32_t load_addr;
	multiboot_uint32_t load_end_addr;
	multiboot_uint32_t bss_end_addr;
	multiboot_uint32_t entry_addr;

	/* These are only valid if MULTIBOOT_VIDEO_MODE is set.  */
	multiboot_uint32_t mode_type;
	multiboot_uint32_t width;
	multiboot_uint32_t height;
	multiboot_uint32_t depth;
};

/* The symbol table for a.out.  */
struct multiboot_aout_symbol_table {
	multiboot_uint32_t tabsize;
	multiboot_uint32_t strsize;
	multiboot_uint32_t addr;
	multiboot_uint32_t reserved;
};
typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

/* The section header table for ELF.  */
struct multiboot_elf_section_header_table {
	multiboot_uint32_t num;
	multiboot_uint32_t size;
	multiboot_uint32_t addr;
	multiboot_uint32_t shndx;
};
typedef struct multiboot_elf_section_header_table
		multiboot_elf_section_header_table_t;

struct multiboot_info {
	/* Multiboot info version number */
	multiboot_uint32_t flags;

	/* Available memory from BIOS */
	multiboot_uint32_t mem_lower;
	multiboot_uint32_t mem_upper;

	/* "root" partition */
	multiboot_uint32_t boot_device;

	/* Kernel command line */
	multiboot_uint32_t cmdline;

	/* Boot-Module list */
	multiboot_uint32_t mods_count;
	multiboot_uint32_t mods_addr;

	union {
		multiboot_aout_symbol_table_t aout_sym;
		multiboot_elf_section_header_table_t elf_sec;
	} u;

	/* Memory Mapping buffer */
	multiboot_uint32_t mmap_length;
	multiboot_uint32_t mmap_addr;

	/* Drive Info buffer */
	multiboot_uint32_t drives_length;
	multiboot_uint32_t drives_addr;

	/* ROM configuration table */
	multiboot_uint32_t config_table;

	/* Boot Loader Name */
	multiboot_uint32_t boot_loader_name;

	/* APM table */
	multiboot_uint32_t apm_table;

	/* Video */
	multiboot_uint32_t vbe_control_info;
	multiboot_uint32_t vbe_mode_info;
	multiboot_uint16_t vbe_mode;
	multiboot_uint16_t vbe_interface_seg;
	multiboot_uint16_t vbe_interface_off;
	multiboot_uint16_t vbe_interface_len;

	multiboot_uint64_t framebuffer_addr;
	multiboot_uint32_t framebuffer_pitch;
	multiboot_uint32_t framebuffer_width;
	multiboot_uint32_t framebuffer_height;
	multiboot_uint8_t framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED  0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB      1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2
	multiboot_uint8_t framebuffer_type;
	union {
		struct {
			multiboot_uint32_t framebuffer_palette_addr;
			multiboot_uint16_t framebuffer_palette_num_colors;
		};
		struct {
			multiboot_uint8_t framebuffer_red_field_position;
			multiboot_uint8_t framebuffer_red_mask_size;
			multiboot_uint8_t framebuffer_green_field_position;
			multiboot_uint8_t framebuffer_green_mask_size;
			multiboot_uint8_t framebuffer_blue_field_position;
			multiboot_uint8_t framebuffer_blue_mask_size;
		};
	};
};
typedef struct multiboot_info multiboot_info_t;

struct multiboot_color {
	multiboot_uint8_t red;
	multiboot_uint8_t green;
	multiboot_uint8_t blue;
};

struct multiboot_mmap_entry {
	multiboot_uint32_t size;
	multiboot_uint64_t addr;
	multiboot_uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5
	multiboot_uint32_t type;
} __packed;
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

struct multiboot_mod_list {
	/* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive
	 */
	multiboot_uint32_t mod_start;
	multiboot_uint32_t mod_end;

	/* Module command line */
	multiboot_uint32_t cmdline;

	/* padding to take it to 16 bytes (must be zero) */
	multiboot_uint32_t pad;
};
typedef struct multiboot_mod_list multiboot_module_t;

/* APM BIOS info.  */
struct multiboot_apm_info {
	multiboot_uint16_t version;
	multiboot_uint16_t cseg;
	multiboot_uint32_t offset;
	multiboot_uint16_t cseg_16;
	multiboot_uint16_t dseg;
	multiboot_uint16_t flags;
	multiboot_uint16_t cseg_len;
	multiboot_uint16_t cseg_16_len;
	multiboot_uint16_t dseg_len;
};

#endif /* ! MULTIBOOT_HEADER */

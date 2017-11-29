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

#ifndef MULTIBOOT_DEFS_HEADER
#define MULTIBOOT_DEFS_HEADER 1

/* The magic field should contain this.  */
#define MULTIBOOT_HEADER_MAGIC			0x1BADB002

/* This should be in %eax.  */
#define MULTIBOOT_BOOTLOADER_MAGIC		0x2BADB002

/* Align all boot modules on i386 page (4KB) boundaries.  */
#define MULTIBOOT_PAGE_ALIGN			0x00000001

/* Must pass memory information to OS.  */
#define MULTIBOOT_MEMORY_INFO			0x00000002

/* This flag indicates the use of the address fields in the header.  */
#define MULTIBOOT_AOUT_KLUDGE			0x00010000

/* is the command-line defined? */
#define MULTIBOOT_INFO_CMDLINE			0x00000004

#endif /* ! MULTIBOOT_DEFS_HEADER */

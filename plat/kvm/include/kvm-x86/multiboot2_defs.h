/*   Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 *  DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*  How many bytes from the start of the file we search for the header. */
#define MULTIBOOT_SEARCH 32768
#define MULTIBOOT_HEADER_ALIGN 8

/*  The magic field should contain this. */
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6

/*  This should be in %eax. */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

/*  Alignment of multiboot modules. */
#define MULTIBOOT_MOD_ALIGN 0x00001000

/*  Alignment of the multiboot info structure. */
#define MULTIBOOT_INFO_ALIGN 0x00000008

/*  Flags set in the ’flags’ member of the multiboot header. */
#define MULTIBOOT_TAG_ALIGN                 0x8
#define MULTIBOOT_TAG_TYPE_END              0x0
#define MULTIBOOT_TAG_TYPE_CMDLINE          0x1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 0x2
#define MULTIBOOT_TAG_TYPE_MODULE           0x3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO    0x4
#define MULTIBOOT_TAG_TYPE_BOOTDEV          0x5
#define MULTIBOOT_TAG_TYPE_MMAP             0x6
#define MULTIBOOT_TAG_TYPE_VBE              0x7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER      0x8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS     0x9
#define MULTIBOOT_TAG_TYPE_APM              0xA
#define MULTIBOOT_TAG_TYPE_EFI32            0xB
#define MULTIBOOT_TAG_TYPE_EFI64            0xC
#define MULTIBOOT_TAG_TYPE_SMBIOS           0xD
#define MULTIBOOT_TAG_TYPE_ACPI_OLD         0xE
#define MULTIBOOT_TAG_TYPE_ACPI_NEW         0xF
#define MULTIBOOT_TAG_TYPE_NETWORK          0x10
#define MULTIBOOT_TAG_TYPE_EFI_MMAP         0x11
#define MULTIBOOT_TAG_TYPE_EFI_BS           0x12
#define MULTIBOOT_TAG_TYPE_EFI32_IH         0x13
#define MULTIBOOT_TAG_TYPE_EFI64_IH         0x14
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR   0x15

#define MULTIBOOT_HEADER_TAG_END 0
#define MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST 1
#define MULTIBOOT_HEADER_TAG_ADDRESS 2
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS 3
#define MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS 4
#define MULTIBOOT_HEADER_TAG_FRAMEBUFFER 5
#define MULTIBOOT_HEADER_TAG_MODULE_ALIGN 6
#define MULTIBOOT_HEADER_TAG_EFI_BS 7
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32 8
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64 9
#define MULTIBOOT_HEADER_TAG_RELOCATABLE 10

#define MULTIBOOT_ARCHITECTURE_I386 0
#define MULTIBOOT_ARCHITECTURE_MIPS32 4
#define MULTIBOOT_HEADER_TAG_OPTIONAL 1

#define MULTIBOOT_LOAD_PREFERENCE_NONE 0
#define MULTIBOOT_LOAD_PREFERENCE_LOW 1
#define MULTIBOOT_LOAD_PREFERENCE_HIGH 2

#define MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED 1
#define MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED 2

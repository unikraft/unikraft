/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  This file is a modified version of the elf.h header included
  in the libc6-dev package. Unneeded definitions have been removed.

=============================================================================*/

/* This file defines standard ELF types, structures, and macros.
   Copyright (C) 1995-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef CPPMICROSERVICES_ELF_H
#define CPPMICROSERVICES_ELF_H 1

/* Standard ELF types.  */

#include <cstdint>

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t  Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct
{
  unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
  Elf32_Half e_type;   /* Object file type */
  Elf32_Half e_machine;  /* Architecture */
  Elf32_Word e_version;  /* Object file version */
  Elf32_Addr e_entry;  /* Entry point virtual address */
  Elf32_Off e_phoff;  /* Program header table file offset */
  Elf32_Off e_shoff;  /* Section header table file offset */
  Elf32_Word e_flags;  /* Processor-specific flags */
  Elf32_Half e_ehsize;  /* ELF header size in bytes */
  Elf32_Half e_phentsize;  /* Program header table entry size */
  Elf32_Half e_phnum;  /* Program header table entry count */
  Elf32_Half e_shentsize;  /* Section header table entry size */
  Elf32_Half e_shnum;  /* Section header table entry count */
  Elf32_Half e_shstrndx;  /* Section header string table index */
} Elf32_Ehdr;

typedef struct
{
  unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
  Elf64_Half e_type;   /* Object file type */
  Elf64_Half e_machine;  /* Architecture */
  Elf64_Word e_version;  /* Object file version */
  Elf64_Addr e_entry;  /* Entry point virtual address */
  Elf64_Off e_phoff;  /* Program header table file offset */
  Elf64_Off e_shoff;  /* Section header table file offset */
  Elf64_Word e_flags;  /* Processor-specific flags */
  Elf64_Half e_ehsize;  /* ELF header size in bytes */
  Elf64_Half e_phentsize;  /* Program header table entry size */
  Elf64_Half e_phnum;  /* Program header table entry count */
  Elf64_Half e_shentsize;  /* Section header table entry size */
  Elf64_Half e_shnum;  /* Section header table entry count */
  Elf64_Half e_shstrndx;  /* Section header string table index */
} Elf64_Ehdr;

/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

#define EI_MAG0  0  /* File identification byte 0 index */
#define ELFMAG0  0x7f  /* Magic number byte 0 */

#define EI_MAG1  1  /* File identification byte 1 index */
#define ELFMAG1  'E'  /* Magic number byte 1 */

#define EI_MAG2  2  /* File identification byte 2 index */
#define ELFMAG2  'L'  /* Magic number byte 2 */

#define EI_MAG3  3  /* File identification byte 3 index */
#define ELFMAG3  'F'  /* Magic number byte 3 */

/* Conglomeration of the identification bytes, for easy testing as a word.  */
#define ELFMAG  "\177ELF"
#define SELFMAG  4

#define EI_CLASS 4  /* File class byte index */
#define ELFCLASSNONE 0  /* Invalid class */
#define ELFCLASS32 1  /* 32-bit objects */
#define ELFCLASS64 2  /* 64-bit objects */
#define ELFCLASSNUM 3

#define EI_DATA  5  /* Data encoding byte index */
#define ELFDATANONE 0  /* Invalid data encoding */
#define ELFDATA2LSB 1  /* 2's complement, little endian */
#define ELFDATA2MSB 2  /* 2's complement, big endian */
#define ELFDATANUM 3

#define EI_VERSION 6  /* File version byte index */
          /* Value must be EV_CURRENT */

#define EI_OSABI 7  /* OS ABI identification */
#define ELFOSABI_NONE  0 /* UNIX System V ABI */
#define ELFOSABI_SYSV  0 /* Alias.  */
#define ELFOSABI_HPUX  1 /* HP-UX */
#define ELFOSABI_NETBSD  2 /* NetBSD.  */
#define ELFOSABI_GNU  3 /* Object uses GNU ELF extensions.  */
#define ELFOSABI_LINUX  ELFOSABI_GNU /* Compatibility alias.  */
#define ELFOSABI_SOLARIS 6 /* Sun Solaris.  */
#define ELFOSABI_AIX  7 /* IBM AIX.  */
#define ELFOSABI_IRIX  8 /* SGI Irix.  */
#define ELFOSABI_FREEBSD 9 /* FreeBSD.  */
#define ELFOSABI_TRU64  10 /* Compaq TRU64 UNIX.  */
#define ELFOSABI_MODESTO 11 /* Novell Modesto.  */
#define ELFOSABI_OPENBSD 12 /* OpenBSD.  */
#define ELFOSABI_ARM_AEABI 64 /* ARM EABI */
#define ELFOSABI_ARM  97 /* ARM */
#define ELFOSABI_STANDALONE 255 /* Standalone (embedded) application */

#define EI_ABIVERSION 8  /* ABI version */

#define EI_PAD  9  /* Byte index of padding bytes */

/* Legal values for e_type (object file type).  */

#define ET_NONE  0  /* No file type */
#define ET_REL  1  /* Relocatable file */
#define ET_EXEC  2  /* Executable file */
#define ET_DYN  3  /* Shared object file */
#define ET_CORE  4  /* Core file */
#define ET_NUM  5  /* Number of defined types */
#define ET_LOOS  0xfe00  /* OS-specific range start */
#define ET_HIOS  0xfeff  /* OS-specific range end */
#define ET_LOPROC 0xff00  /* Processor-specific range start */
#define ET_HIPROC 0xffff  /* Processor-specific range end */

/* Legal values for e_version (version).  */

#define EV_NONE  0  /* Invalid ELF version */
#define EV_CURRENT 1  /* Current version */
#define EV_NUM  2

/* Section header.  */

typedef struct
{
  Elf32_Word sh_name;  /* Section name (string tbl index) */
  Elf32_Word sh_type;  /* Section type */
  Elf32_Word sh_flags;  /* Section flags */
  Elf32_Addr sh_addr;  /* Section virtual addr at execution */
  Elf32_Off sh_offset;  /* Section file offset */
  Elf32_Word sh_size;  /* Section size in bytes */
  Elf32_Word sh_link;  /* Link to another section */
  Elf32_Word sh_info;  /* Additional section information */
  Elf32_Word sh_addralign;  /* Section alignment */
  Elf32_Word sh_entsize;  /* Entry size if section holds table */
} Elf32_Shdr;

typedef struct
{
  Elf64_Word sh_name;  /* Section name (string tbl index) */
  Elf64_Word sh_type;  /* Section type */
  Elf64_Xword sh_flags;  /* Section flags */
  Elf64_Addr sh_addr;  /* Section virtual addr at execution */
  Elf64_Off sh_offset;  /* Section file offset */
  Elf64_Xword sh_size;  /* Section size in bytes */
  Elf64_Word sh_link;  /* Link to another section */
  Elf64_Word sh_info;  /* Additional section information */
  Elf64_Xword sh_addralign;  /* Section alignment */
  Elf64_Xword sh_entsize;  /* Entry size if section holds table */
} Elf64_Shdr;

/* Special section indices.  */

#define SHN_UNDEF 0  /* Undefined section */
#define SHN_LORESERVE 0xff00  /* Start of reserved indices */
#define SHN_LOPROC 0xff00  /* Start of processor-specific */
#define SHN_BEFORE 0xff00  /* Order section before all others
             (Solaris).  */
#define SHN_AFTER 0xff01  /* Order section after all others
             (Solaris).  */
#define SHN_HIPROC 0xff1f  /* End of processor-specific */
#define SHN_LOOS 0xff20  /* Start of OS-specific */
#define SHN_HIOS 0xff3f  /* End of OS-specific */
#define SHN_ABS  0xfff1  /* Associated symbol is absolute */
#define SHN_COMMON 0xfff2  /* Associated symbol is common */
#define SHN_XINDEX 0xffff  /* Index is in extra table.  */
#define SHN_HIRESERVE 0xffff  /* End of reserved indices */

/* Legal values for sh_type (section type).  */

#define SHT_NULL   0  /* Section header table entry unused */
#define SHT_PROGBITS   1  /* Program data */
#define SHT_SYMTAB   2  /* Symbol table */
#define SHT_STRTAB   3  /* String table */
#define SHT_RELA   4  /* Relocation entries with addends */
#define SHT_HASH   5  /* Symbol hash table */
#define SHT_DYNAMIC   6  /* Dynamic linking information */
#define SHT_NOTE   7  /* Notes */
#define SHT_NOBITS   8  /* Program space with no data (bss) */
#define SHT_REL    9  /* Relocation entries, no addends */
#define SHT_SHLIB   10  /* Reserved */
#define SHT_DYNSYM   11  /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY   14  /* Array of constructors */
#define SHT_FINI_ARRAY   15  /* Array of destructors */
#define SHT_PREINIT_ARRAY 16  /* Array of pre-constructors */
#define SHT_GROUP   17  /* Section group */
#define SHT_SYMTAB_SHNDX  18  /* Extended section indeces */
#define SHT_NUM    19  /* Number of defined types.  */
#define SHT_LOOS   0x60000000 /* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5 /* Object attributes.  */
#define SHT_GNU_HASH   0x6ffffff6 /* GNU-style hash table.  */
#define SHT_GNU_LIBLIST   0x6ffffff7 /* Prelink library list */
#define SHT_CHECKSUM   0x6ffffff8 /* Checksum for DSO content.  */
#define SHT_LOSUNW   0x6ffffffa /* Sun-specific low bound.  */
#define SHT_SUNW_move   0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef   0x6ffffffd /* Version definition section.  */
#define SHT_GNU_verneed   0x6ffffffe /* Version needs section.  */
#define SHT_GNU_versym   0x6fffffff /* Version symbol table.  */
#define SHT_HISUNW   0x6fffffff /* Sun-specific high bound.  */
#define SHT_HIOS   0x6fffffff /* End OS-specific type */
#define SHT_LOPROC   0x70000000 /* Start of processor-specific */
#define SHT_HIPROC   0x7fffffff /* End of processor-specific */
#define SHT_LOUSER   0x80000000 /* Start of application-specific */
#define SHT_HIUSER   0x8fffffff /* End of application-specific */

/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE      (1 << 0) /* Writable */
#define SHF_ALLOC      (1 << 1) /* Occupies memory during execution */
#define SHF_EXECINSTR      (1 << 2) /* Executable */
#define SHF_MERGE      (1 << 4) /* Might be merged */
#define SHF_STRINGS      (1 << 5) /* Contains nul-terminated strings */
#define SHF_INFO_LINK      (1 << 6) /* `sh_info' contains SHT index */
#define SHF_LINK_ORDER      (1 << 7) /* Preserve order after combining */
#define SHF_OS_NONCONFORMING (1 << 8) /* Non-standard OS specific handling
             required */
#define SHF_GROUP      (1 << 9) /* Section is member of a group.  */
#define SHF_TLS       (1 << 10) /* Section hold thread-local data.  */
#define SHF_MASKOS      0x0ff00000 /* OS-specific.  */
#define SHF_MASKPROC      0xf0000000 /* Processor-specific */
#define SHF_ORDERED      (1 << 30) /* Special ordering requirement
             (Solaris).  */
#define SHF_EXCLUDE      (1 << 31) /* Section is excluded unless
             referenced or allocated (Solaris).*/

/* Section group handling.  */
#define GRP_COMDAT 0x1  /* Mark group as COMDAT.  */

/* Symbol table entry.  */

typedef struct
{
  Elf32_Word st_name;  /* Symbol name (string tbl index) */
  Elf32_Addr st_value;  /* Symbol value */
  Elf32_Word st_size;  /* Symbol size */
  unsigned char st_info;  /* Symbol type and binding */
  unsigned char st_other;  /* Symbol visibility */
  Elf32_Section st_shndx;  /* Section index */
} Elf32_Sym;

typedef struct
{
  Elf64_Word st_name;  /* Symbol name (string tbl index) */
  unsigned char st_info;  /* Symbol type and binding */
  unsigned char st_other;  /* Symbol visibility */
  Elf64_Section st_shndx;  /* Section index */
  Elf64_Addr st_value;  /* Symbol value */
  Elf64_Xword st_size;  /* Symbol size */
} Elf64_Sym;

/* The syminfo section if available contains additional information about
   every dynamic symbol.  */

typedef struct
{
  Elf32_Half si_boundto;  /* Direct bindings, symbol bound to */
  Elf32_Half si_flags;   /* Per symbol flags */
} Elf32_Syminfo;

typedef struct
{
  Elf64_Half si_boundto;  /* Direct bindings, symbol bound to */
  Elf64_Half si_flags;   /* Per symbol flags */
} Elf64_Syminfo;

/* Possible values for si_boundto.  */
#define SYMINFO_BT_SELF  0xffff /* Symbol bound to self */
#define SYMINFO_BT_PARENT 0xfffe /* Symbol bound to parent */
#define SYMINFO_BT_LOWRESERVE 0xff00 /* Beginning of reserved entries */

/* Possible bitmasks for si_flags.  */
#define SYMINFO_FLG_DIRECT 0x0001 /* Direct bound symbol */
#define SYMINFO_FLG_PASSTHRU 0x0002 /* Pass-thru symbol for translator */
#define SYMINFO_FLG_COPY 0x0004 /* Symbol is a copy-reloc */
#define SYMINFO_FLG_LAZYLOAD 0x0008 /* Symbol bound to object to be lazy
             loaded */
/* Syminfo version values.  */
#define SYMINFO_NONE  0
#define SYMINFO_CURRENT  1
#define SYMINFO_NUM  2


/* How to extract and insert information held in the st_info field.  */

#define ELF32_ST_BIND(val)  (((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)  ((val) & 0xf)
#define ELF32_ST_INFO(bind, type) (((bind) << 4) + ((type) & 0xf))

/* Both Elf32_Sym and Elf64_Sym use the same one-byte st_info field.  */
#define ELF64_ST_BIND(val)  ELF32_ST_BIND (val)
#define ELF64_ST_TYPE(val)  ELF32_ST_TYPE (val)
#define ELF64_ST_INFO(bind, type) ELF32_ST_INFO ((bind), (type))

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */

#define STB_LOCAL 0  /* Local symbol */
#define STB_GLOBAL 1  /* Global symbol */
#define STB_WEAK 2  /* Weak symbol */
#define STB_NUM  3  /* Number of defined types.  */
#define STB_LOOS 10  /* Start of OS-specific */
#define STB_GNU_UNIQUE 10  /* Unique symbol.  */
#define STB_HIOS 12  /* End of OS-specific */
#define STB_LOPROC 13  /* Start of processor-specific */
#define STB_HIPROC 15  /* End of processor-specific */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#define STT_NOTYPE 0  /* Symbol type is unspecified */
#define STT_OBJECT 1  /* Symbol is a data object */
#define STT_FUNC 2  /* Symbol is a code object */
#define STT_SECTION 3  /* Symbol associated with a section */
#define STT_FILE 4  /* Symbol's name is file name */
#define STT_COMMON 5  /* Symbol is a common data object */
#define STT_TLS  6  /* Symbol is thread-local data object*/
#define STT_NUM  7  /* Number of defined types.  */
#define STT_LOOS 10  /* Start of OS-specific */
#define STT_GNU_IFUNC 10  /* Symbol is indirect code object */
#define STT_HIOS 12  /* End of OS-specific */
#define STT_LOPROC 13  /* Start of processor-specific */
#define STT_HIPROC 15  /* End of processor-specific */


/* Symbol table indices are found in the hash buckets and chain table
   of a symbol hash table section.  This special index value indicates
   the end of a chain, meaning no further symbols are found in that bucket.  */

#define STN_UNDEF 0  /* End of a chain.  */

/* Program segment header.  */

typedef struct
{
  Elf32_Word p_type;   /* Segment type */
  Elf32_Off p_offset;  /* Segment file offset */
  Elf32_Addr p_vaddr;  /* Segment virtual address */
  Elf32_Addr p_paddr;  /* Segment physical address */
  Elf32_Word p_filesz;  /* Segment size in file */
  Elf32_Word p_memsz;  /* Segment size in memory */
  Elf32_Word p_flags;  /* Segment flags */
  Elf32_Word p_align;  /* Segment alignment */
} Elf32_Phdr;

typedef struct
{
  Elf64_Word p_type;   /* Segment type */
  Elf64_Word p_flags;  /* Segment flags */
  Elf64_Off p_offset;  /* Segment file offset */
  Elf64_Addr p_vaddr;  /* Segment virtual address */
  Elf64_Addr p_paddr;  /* Segment physical address */
  Elf64_Xword p_filesz;  /* Segment size in file */
  Elf64_Xword p_memsz;  /* Segment size in memory */
  Elf64_Xword p_align;  /* Segment alignment */
} Elf64_Phdr;

/* Dynamic section entry.  */

typedef struct
{
  Elf32_Sword d_tag;   /* Dynamic entry type */
  union
    {
      Elf32_Word d_val;   /* Integer value */
      Elf32_Addr d_ptr;   /* Address value */
    } d_un;
} Elf32_Dyn;

typedef struct
{
  Elf64_Sxword d_tag;   /* Dynamic entry type */
  union
    {
      Elf64_Xword d_val;  /* Integer value */
      Elf64_Addr d_ptr;   /* Address value */
    } d_un;
} Elf64_Dyn;

/* Legal values for d_tag (dynamic entry type).  */

#define DT_NULL  0  /* Marks end of dynamic section */
#define DT_NEEDED 1  /* Name of needed library */
#define DT_PLTRELSZ 2  /* Size in bytes of PLT relocs */
#define DT_PLTGOT 3  /* Processor defined value */
#define DT_HASH  4  /* Address of symbol hash table */
#define DT_STRTAB 5  /* Address of string table */
#define DT_SYMTAB 6  /* Address of symbol table */
#define DT_RELA  7  /* Address of Rela relocs */
#define DT_RELASZ 8  /* Total size of Rela relocs */
#define DT_RELAENT 9  /* Size of one Rela reloc */
#define DT_STRSZ 10  /* Size of string table */
#define DT_SYMENT 11  /* Size of one symbol table entry */
#define DT_INIT  12  /* Address of init function */
#define DT_FINI  13  /* Address of termination function */
#define DT_SONAME 14  /* Name of shared object */
#define DT_RPATH 15  /* Library search path (deprecated) */
#define DT_SYMBOLIC 16  /* Start symbol search here */
#define DT_REL  17  /* Address of Rel relocs */
#define DT_RELSZ 18  /* Total size of Rel relocs */
#define DT_RELENT 19  /* Size of one Rel reloc */
#define DT_PLTREL 20  /* Type of reloc in PLT */
#define DT_DEBUG 21  /* For debugging; unspecified */
#define DT_TEXTREL 22  /* Reloc might modify .text */
#define DT_JMPREL 23  /* Address of PLT relocs */
#define DT_BIND_NOW 24  /* Process relocations of object */
#define DT_INIT_ARRAY 25  /* Array with addresses of init fct */
#define DT_FINI_ARRAY 26  /* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ 27  /* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ 28  /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH 29  /* Library search path */
#define DT_FLAGS 30  /* Flags for the object being loaded */
#define DT_ENCODING 32  /* Start of encoded range */
#define DT_PREINIT_ARRAY 32  /* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ 33  /* size in bytes of DT_PREINIT_ARRAY */
#define DT_NUM  34  /* Number used */
#define DT_LOOS  0x6000000d /* Start of OS-specific */
#define DT_HIOS  0x6ffff000 /* End of OS-specific */
#define DT_LOPROC 0x70000000 /* Start of processor-specific */
#define DT_HIPROC 0x7fffffff /* End of processor-specific */
#define DT_PROCNUM DT_MIPS_NUM /* Most used by any processor */

#endif /* elf.h */

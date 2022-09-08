/*
 * Portions Copyright (c) 1999-2010 Apple Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 */
#ifndef CPPMICROSERVICES_MACHO_LOADER_H
#define CPPMICROSERVICES_MACHO_LOADER_H

/*
 * This file describes the format of mach object files.
 */
#include <cstdint>

typedef int32_t cpu_type_t;
typedef int32_t cpu_subtype_t;
typedef int32_t vm_prot_t;

/*
 * Capability bits used in the definition of cpu_type.
 */
#define  CPU_ARCH_MASK  0xff000000    /* mask for architecture bits */
#define CPU_ARCH_ABI64  0x01000000    /* 64 bit ABI */

/*
 *  Machine types known by all.
 */

#define CPU_TYPE_ANY       (-1)

#define CPU_TYPE_VAX        (1)
/* skip                     (2) */
/* skip                     (3) */
/* skip                     (4) */
/* skip                     (5) */
#define  CPU_TYPE_MC680x0   (6)
#define CPU_TYPE_X86        (7)
#define CPU_TYPE_I386       CPU_TYPE_X86    /* compatibility */
#define  CPU_TYPE_X86_64   (CPU_TYPE_X86 | CPU_ARCH_ABI64)

/* skip CPU_TYPE_MIPS       (8) */
/* skip       (9)  */
#define CPU_TYPE_MC98000   (10)
#define CPU_TYPE_HPPA      (11)
#define CPU_TYPE_ARM       (12)
#define CPU_TYPE_MC88000   (13)
#define CPU_TYPE_SPARC     (14)
#define CPU_TYPE_I860      (15)
/* skip  CPU_TYPE_ALPHA    (16) */
/* skip                    (17) */
#define CPU_TYPE_POWERPC   (18)
#define CPU_TYPE_POWERPC64 (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)

#ifndef _MACH_O_FAT_H_
#define _MACH_O_FAT_H_
/*
 * This header file describes the structures of the file format for "fat"
 * architecture specific file (wrapper design).  At the begining of the file
 * there is one fat_header structure followed by a number of fat_arch
 * structures.  For each architecture in the file, specified by a pair of
 * cputype and cpusubtype, the fat_header describes the file offset, file
 * size and alignment in the file of the architecture specific member.
 * The padded bytes in the file to place each member on it's specific alignment
 * are defined to be read as zeros and can be left as "holes" if the file system
 * can support them as long as they read as zeros.
 *
 * All structures defined here are always written and read to/from disk
 * in big-endian order.
 */

#define FAT_MAGIC  0xcafebabe
#define FAT_CIGAM  0xbebafeca  /* NXSwapLong(FAT_MAGIC) */

struct fat_header {
  uint32_t  magic;    /* FAT_MAGIC */
  uint32_t  nfat_arch;  /* number of structs that follow */
};

struct fat_arch {
  cpu_type_t  cputype;  /* cpu specifier (int) */
  cpu_subtype_t  cpusubtype;  /* machine specifier (int) */
  uint32_t  offset;    /* file offset to this object file */
  uint32_t  size;    /* size of this object file */
  uint32_t  align;    /* alignment as a power of 2 */
};

#endif /* _MACH_O_FAT_H_ */

/*
 * The 32-bit mach header appears at the very beginning of the object file for
 * 32-bit architectures.
 */
struct mach_header {
  uint32_t  magic;    /* mach magic number identifier */
  cpu_type_t  cputype;  /* cpu specifier */
  cpu_subtype_t  cpusubtype;  /* machine specifier */
  uint32_t  filetype;  /* type of file */
  uint32_t  ncmds;    /* number of load commands */
  uint32_t  sizeofcmds;  /* the size of all the load commands */
  uint32_t  flags;    /* flags */
};

/* Constant for the magic field of the mach_header (32-bit architectures) */
#define MH_MAGIC  0xfeedface  /* the mach magic number */
#define MH_CIGAM  0xcefaedfe  /* NXSwapInt(MH_MAGIC) */

/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64 {
  uint32_t  magic;    /* mach magic number identifier */
  cpu_type_t  cputype;  /* cpu specifier */
  cpu_subtype_t  cpusubtype;  /* machine specifier */
  uint32_t  filetype;  /* type of file */
  uint32_t  ncmds;    /* number of load commands */
  uint32_t  sizeofcmds;  /* the size of all the load commands */
  uint32_t  flags;    /* flags */
  uint32_t  reserved;  /* reserved */
};

/* Constant for the magic field of the mach_header_64 (64-bit architectures) */
#define MH_MAGIC_64 0xfeedfacf /* the 64-bit mach magic number */
#define MH_CIGAM_64 0xcffaedfe /* NXSwapInt(MH_MAGIC_64) */

/*
 * The layout of the file depends on the filetype.  For all but the MH_OBJECT
 * file type the segments are padded out and aligned on a segment alignment
 * boundary for efficient demand pageing.  The MH_EXECUTE, MH_FVMLIB, MH_DYLIB,
 * MH_DYLINKER and MH_BUNDLE file types also have the headers included as part
 * of their first segment.
 *
 * The file type MH_OBJECT is a compact format intended as output of the
 * assembler and input (and possibly output) of the link editor (the .o
 * format).  All sections are in one unnamed segment with no segment padding.
 * This format is used as an executable format when the file is so small the
 * segment padding greatly increases its size.
 *
 * The file type MH_PRELOAD is an executable format intended for things that
 * are not executed under the kernel (proms, stand alones, kernels, etc).  The
 * format can be executed under the kernel but may demand paged it and not
 * preload it before execution.
 *
 * A core file is in MH_CORE format and can be any in an arbritray legal
 * Mach-O file.
 *
 * Constants for the filetype field of the mach_header
 */
#define  MH_OBJECT  0x1    /* relocatable object file */
#define  MH_EXECUTE  0x2    /* demand paged executable file */
#define  MH_FVMLIB  0x3    /* fixed VM shared library file */
#define  MH_CORE    0x4    /* core file */
#define  MH_PRELOAD  0x5    /* preloaded executable file */
#define  MH_DYLIB  0x6    /* dynamically bound shared library */
#define  MH_DYLINKER  0x7    /* dynamic link editor */
#define  MH_BUNDLE  0x8    /* dynamically bound bundle file */
#define  MH_DYLIB_STUB  0x9    /* shared library stub for static */
          /*  linking only, no section contents */
#define  MH_DSYM    0xa    /* companion file with only debug */
          /*  sections */
#define  MH_KEXT_BUNDLE  0xb    /* x86_64 kexts */

/* Constants for the flags field of the mach_header */
#define  MH_NOUNDEFS  0x1    /* the object file has no undefined
             references */
#define  MH_INCRLINK  0x2    /* the object file is the output of an
             incremental link against a base file
             and can't be link edited again */
#define MH_DYLDLINK  0x4    /* the object file is input for the
             dynamic linker and can't be staticly
             link edited again */
#define MH_BINDATLOAD  0x8    /* the object file's undefined
             references are bound by the dynamic
             linker when loaded. */
#define MH_PREBOUND  0x10    /* the file has its dynamic undefined
             references prebound. */
#define MH_SPLIT_SEGS  0x20    /* the file has its read-only and
             read-write segments split */
#define MH_LAZY_INIT  0x40    /* the shared library init routine is
             to be run lazily via catching memory
             faults to its writeable segments
             (obsolete) */
#define MH_TWOLEVEL  0x80    /* the image is using two-level name
             space bindings */
#define MH_FORCE_FLAT  0x100    /* the executable is forcing all images
             to use flat name space bindings */
#define MH_NOMULTIDEFS  0x200    /* this umbrella guarantees no multiple
             defintions of symbols in its
             sub-images so the two-level namespace
             hints can always be used. */
#define MH_NOFIXPREBINDING 0x400  /* do not have dyld notify the
             prebinding agent about this
             executable */
#define MH_PREBINDABLE  0x800           /* the binary is not prebound but can
             have its prebinding redone. only used
                                           when MH_PREBOUND is not set. */
#define MH_ALLMODSBOUND 0x1000    /* indicates that this binary binds to
                                           all two-level namespace modules of
             its dependent libraries. only used
             when MH_PREBINDABLE and MH_TWOLEVEL
             are both set. */
#define MH_SUBSECTIONS_VIA_SYMBOLS 0x2000/* safe to divide up the sections into
              sub-sections via symbols for dead
              code stripping */
#define MH_CANONICAL    0x4000    /* the binary has been canonicalized
             via the unprebind operation */
#define MH_WEAK_DEFINES  0x8000    /* the final linked image contains
             external weak symbols */
#define MH_BINDS_TO_WEAK 0x10000  /* the final linked image uses
             weak symbols */

#define MH_ALLOW_STACK_EXECUTION 0x20000/* When this bit is set, all stacks
             in the task will be given stack
             execution privilege.  Only used in
             MH_EXECUTE filetypes. */
#define MH_ROOT_SAFE 0x40000           /* When this bit is set, the binary
            declares it is safe for use in
            processes with uid zero */

#define MH_SETUID_SAFE 0x80000         /* When this bit is set, the binary
            declares it is safe for use in
            processes when issetugid() is true */

#define MH_NO_REEXPORTED_DYLIBS 0x100000 /* When this bit is set on a dylib,
            the static linker does not need to
            examine dependent dylibs to see
            if any are re-exported */
#define  MH_PIE 0x200000      /* When this bit is set, the OS will
             load the main executable at a
             random address.  Only used in
             MH_EXECUTE filetypes. */
#define  MH_DEAD_STRIPPABLE_DYLIB 0x400000 /* Only for use on dylibs.  When
               linking against a dylib that
               has this bit set, the static linker
               will automatically not create a
               LC_LOAD_DYLIB load command to the
               dylib if no symbols are being
               referenced from the dylib. */
#define MH_HAS_TLV_DESCRIPTORS 0x800000 /* Contains a section of type
              S_THREAD_LOCAL_VARIABLES */

#define MH_NO_HEAP_EXECUTION 0x1000000  /* When this bit is set, the OS will
             run the main executable with
             a non-executable heap even on
             platforms (e.g. i386) that don't
             require it. Only used in MH_EXECUTE
             filetypes. */

/*
 * The load commands directly follow the mach_header.  The total size of all
 * of the commands is given by the sizeofcmds field in the mach_header.  All
 * load commands must have as their first two fields cmd and cmdsize.  The cmd
 * field is filled in with a constant for that command type.  Each command type
 * has a structure specifically for it.  The cmdsize field is the size in bytes
 * of the particular load command structure plus anything that follows it that
 * is a part of the load command (i.e. section structures, strings, etc.).  To
 * advance to the next load command the cmdsize can be added to the offset or
 * pointer of the current load command.  The cmdsize for 32-bit architectures
 * MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
 * of 8 bytes (these are forever the maximum alignment of any load commands).
 * The padded bytes must be zero.  All tables in the object file must also
 * follow these rules so the file can be memory mapped.  Otherwise the pointers
 * to these tables will not work well or at all on some machines.  With all
 * padding zeroed like objects will compare byte for byte.
 */
struct load_command {
  uint32_t cmd;    /* type of load command */
  uint32_t cmdsize;  /* total size of command in bytes */
};

/*
 * After MacOS X 10.1 when a new load command is added that is required to be
 * understood by the dynamic linker for the image to execute properly the
 * LC_REQ_DYLD bit will be or'ed into the load command constant.  If the dynamic
 * linker sees such a load command it it does not understand will issue a
 * "unknown load command required for execution" error and refuse to use the
 * image.  Other load commands without this bit that are not understood will
 * simply be ignored.
 */
#define LC_REQ_DYLD 0x80000000

/* Constants for the cmd field of all load commands, the type */
#define  LC_SEGMENT  0x1  /* segment of this file to be mapped */
#define  LC_SYMTAB  0x2  /* link-edit stab symbol table info */
#define  LC_SYMSEG  0x3  /* link-edit gdb symbol table info (obsolete) */
#define  LC_THREAD  0x4  /* thread */
#define  LC_UNIXTHREAD  0x5  /* unix thread (includes a stack) */
#define  LC_LOADFVMLIB  0x6  /* load a specified fixed VM shared library */
#define  LC_IDFVMLIB  0x7  /* fixed VM shared library identification */
#define  LC_IDENT  0x8  /* object identification info (obsolete) */
#define LC_FVMFILE  0x9  /* fixed VM file inclusion (internal use) */
#define LC_PREPAGE      0xa     /* prepage command (internal use) */
#define  LC_DYSYMTAB  0xb  /* dynamic link-edit symbol table info */
#define  LC_LOAD_DYLIB  0xc  /* load a dynamically linked shared library */
#define  LC_ID_DYLIB  0xd  /* dynamically linked shared lib ident */
#define LC_LOAD_DYLINKER 0xe  /* load a dynamic linker */
#define LC_ID_DYLINKER  0xf  /* dynamic linker identification */
#define  LC_PREBOUND_DYLIB 0x10  /* modules prebound for a dynamically */
        /*  linked shared library */
#define  LC_ROUTINES  0x11  /* image routines */
#define  LC_SUB_FRAMEWORK 0x12  /* sub framework */
#define  LC_SUB_UMBRELLA 0x13  /* sub umbrella */
#define  LC_SUB_CLIENT  0x14  /* sub client */
#define  LC_SUB_LIBRARY  0x15  /* sub library */
#define  LC_TWOLEVEL_HINTS 0x16  /* two-level namespace lookup hints */
#define  LC_PREBIND_CKSUM  0x17  /* prebind checksum */

/*
 * load a dynamically linked shared library that is allowed to be missing
 * (all symbols are weak imported).
 */
#define  LC_LOAD_WEAK_DYLIB (0x18 | LC_REQ_DYLD)

#define  LC_SEGMENT_64  0x19  /* 64-bit segment of this file to be
           mapped */
#define  LC_ROUTINES_64  0x1a  /* 64-bit image routines */
#define LC_UUID    0x1b  /* the uuid */
#define LC_RPATH       (0x1c | LC_REQ_DYLD)    /* runpath additions */
#define LC_CODE_SIGNATURE 0x1d  /* local of code signature */
#define LC_SEGMENT_SPLIT_INFO 0x1e /* local of info to split segments */
#define LC_REEXPORT_DYLIB (0x1f | LC_REQ_DYLD) /* load and re-export dylib */
#define  LC_LAZY_LOAD_DYLIB 0x20  /* delay load of dylib until first use */
#define  LC_ENCRYPTION_INFO 0x21  /* encrypted segment information */
#define  LC_DYLD_INFO   0x22  /* compressed dyld information */
#define  LC_DYLD_INFO_ONLY (0x22|LC_REQ_DYLD)  /* compressed dyld information only */
#define  LC_LOAD_UPWARD_DYLIB (0x23 | LC_REQ_DYLD) /* load upward dylib */
#define LC_VERSION_MIN_MACOSX 0x24   /* build for MacOSX min OS version */
#define LC_VERSION_MIN_IPHONEOS 0x25 /* build for iPhoneOS min OS version */
#define LC_FUNCTION_STARTS 0x26 /* compressed table of function start addresses */
#define LC_DYLD_ENVIRONMENT 0x27 /* string for dyld to treat
            like environment variable */

/*
 * A variable length string in a load command is represented by an lc_str
 * union.  The strings are stored just after the load command structure and
 * the offset is from the start of the load command structure.  The size
 * of the string is reflected in the cmdsize field of the load command.
 * Once again any padded bytes to bring the cmdsize field to a multiple
 * of 4 bytes must be zero.
 */
union lc_str {
  uint32_t  offset;  /* offset to the string */
#ifndef __LP64__
  char    *ptr;  /* pointer to the string */
#endif
};

/*
 * Dynamicly linked shared libraries are identified by two things.  The
 * pathname (the name of the library as found for execution), and the
 * compatibility version number.  The pathname must match and the compatibility
 * number in the user of the library must be greater than or equal to the
 * library being used.  The time stamp is used to record the time a library was
 * built and copied into user so it can be use to determined if the library used
 * at runtime is exactly the same as used to built the program.
 */
struct dylib {
    union lc_str  name;      /* library's path name */
    uint32_t timestamp;      /* library's build time stamp */
    uint32_t current_version;    /* library's current version number */
    uint32_t compatibility_version;  /* library's compatibility vers number*/
};

/*
 * A dynamically linked shared library (filetype == MH_DYLIB in the mach header)
 * contains a dylib_command (cmd == LC_ID_DYLIB) to identify the library.
 * An object that uses a dynamically linked shared library also contains a
 * dylib_command (cmd == LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, or
 * LC_REEXPORT_DYLIB) for each library it uses.
 */
struct dylib_command {
  uint32_t  cmd;    /* LC_ID_DYLIB, LC_LOAD_{,WEAK_}DYLIB,
             LC_REEXPORT_DYLIB */
  uint32_t  cmdsize;  /* includes pathname string */
  struct dylib  dylib;    /* the library identification */
};

/*
 * The symtab_command contains the offsets and sizes of the link-edit 4.3BSD
 * "stab" style symbol table information as described in the header files
 * <nlist.h> and <stab.h>.
 */
struct symtab_command {
  uint32_t  cmd;    /* LC_SYMTAB */
  uint32_t  cmdsize;  /* sizeof(struct symtab_command) */
  uint32_t  symoff;    /* symbol table offset */
  uint32_t  nsyms;    /* number of symbol table entries */
  uint32_t  stroff;    /* string table offset */
  uint32_t  strsize;  /* string table size in bytes */
};

/*
 * This is the second set of the symbolic information which is used to support
 * the data structures for the dynamically link editor.
 *
 * The original set of symbolic information in the symtab_command which contains
 * the symbol and string tables must also be present when this load command is
 * present.  When this load command is present the symbol table is organized
 * into three groups of symbols:
 *  local symbols (static and debugging symbols) - grouped by module
 *  defined external symbols - grouped by module (sorted by name if not lib)
 *  undefined external symbols (sorted by name if MH_BINDATLOAD is not set,
 *                 and in order the were seen by the static
 *            linker if MH_BINDATLOAD is set)
 * In this load command there are offsets and counts to each of the three groups
 * of symbols.
 *
 * This load command contains a the offsets and sizes of the following new
 * symbolic information tables:
 *  table of contents
 *  module table
 *  reference symbol table
 *  indirect symbol table
 * The first three tables above (the table of contents, module table and
 * reference symbol table) are only present if the file is a dynamically linked
 * shared library.  For executable and object modules, which are files
 * containing only one module, the information that would be in these three
 * tables is determined as follows:
 *   table of contents - the defined external symbols are sorted by name
 *  module table - the file contains only one module so everything in the
 *           file is part of the module.
 *  reference symbol table - is the defined and undefined external symbols
 *
 * For dynamically linked shared library files this load command also contains
 * offsets and sizes to the pool of relocation entries for all sections
 * separated into two groups:
 *  external relocation entries
 *  local relocation entries
 * For executable and object modules the relocation entries continue to hang
 * off the section structures.
 */
struct dysymtab_command {
    uint32_t cmd;  /* LC_DYSYMTAB */
    uint32_t cmdsize;  /* sizeof(struct dysymtab_command) */

    /*
     * The symbols indicated by symoff and nsyms of the LC_SYMTAB load command
     * are grouped into the following three groups:
     *    local symbols (further grouped by the module they are from)
     *    defined external symbols (further grouped by the module they are from)
     *    undefined symbols
     *
     * The local symbols are used only for debugging.  The dynamic binding
     * process may have to use them to indicate to the debugger the local
     * symbols for a module that is being bound.
     *
     * The last two groups are used by the dynamic binding process to do the
     * binding (indirectly through the module table and the reference symbol
     * table when this is a dynamically linked shared library file).
     */
    uint32_t ilocalsym;  /* index to local symbols */
    uint32_t nlocalsym;  /* number of local symbols */

    uint32_t iextdefsym;/* index to externally defined symbols */
    uint32_t nextdefsym;/* number of externally defined symbols */

    uint32_t iundefsym;  /* index to undefined symbols */
    uint32_t nundefsym;  /* number of undefined symbols */

    /*
     * For the for the dynamic binding process to find which module a symbol
     * is defined in the table of contents is used (analogous to the ranlib
     * structure in an archive) which maps defined external symbols to modules
     * they are defined in.  This exists only in a dynamically linked shared
     * library file.  For executable and object modules the defined external
     * symbols are sorted by name and is use as the table of contents.
     */
    uint32_t tocoff;  /* file offset to table of contents */
    uint32_t ntoc;  /* number of entries in table of contents */

    /*
     * To support dynamic binding of "modules" (whole object files) the symbol
     * table must reflect the modules that the file was created from.  This is
     * done by having a module table that has indexes and counts into the merged
     * tables for each module.  The module structure that these two entries
     * refer to is described below.  This exists only in a dynamically linked
     * shared library file.  For executable and object modules the file only
     * contains one module so everything in the file belongs to the module.
     */
    uint32_t modtaboff;  /* file offset to module table */
    uint32_t nmodtab;  /* number of module table entries */

    /*
     * To support dynamic module binding the module structure for each module
     * indicates the external references (defined and undefined) each module
     * makes.  For each module there is an offset and a count into the
     * reference symbol table for the symbols that the module references.
     * This exists only in a dynamically linked shared library file.  For
     * executable and object modules the defined external symbols and the
     * undefined external symbols indicates the external references.
     */
    uint32_t extrefsymoff;  /* offset to referenced symbol table */
    uint32_t nextrefsyms;  /* number of referenced symbol table entries */

    /*
     * The sections that contain "symbol pointers" and "routine stubs" have
     * indexes and (implied counts based on the size of the section and fixed
     * size of the entry) into the "indirect symbol" table for each pointer
     * and stub.  For every section of these two types the index into the
     * indirect symbol table is stored in the section header in the field
     * reserved1.  An indirect symbol table entry is simply a 32bit index into
     * the symbol table to the symbol that the pointer or stub is referring to.
     * The indirect symbol table is ordered to match the entries in the section.
     */
    uint32_t indirectsymoff; /* file offset to the indirect symbol table */
    uint32_t nindirectsyms;  /* number of indirect symbol table entries */

    /*
     * To support relocating an individual module in a library file quickly the
     * external relocation entries for each module in the library need to be
     * accessed efficiently.  Since the relocation entries can't be accessed
     * through the section headers for a library file they are separated into
     * groups of local and external entries further grouped by module.  In this
     * case the presents of this load command who's extreloff, nextrel,
     * locreloff and nlocrel fields are non-zero indicates that the relocation
     * entries of non-merged sections are not referenced through the section
     * structures (and the reloff and nreloc fields in the section headers are
     * set to zero).
     *
     * Since the relocation entries are not accessed through the section headers
     * this requires the r_address field to be something other than a section
     * offset to identify the item to be relocated.  In this case r_address is
     * set to the offset from the vmaddr of the first LC_SEGMENT command.
     * For MH_SPLIT_SEGS images r_address is set to the the offset from the
     * vmaddr of the first read-write LC_SEGMENT command.
     *
     * The relocation entries are grouped by module and the module table
     * entries have indexes and counts into them for the group of external
     * relocation entries for that the module.
     *
     * For sections that are merged across modules there must not be any
     * remaining external relocation entries for them (for merged sections
     * remaining relocation entries must be local).
     */
    uint32_t extreloff;  /* offset to external relocation entries */
    uint32_t nextrel;  /* number of external relocation entries */

    /*
     * All the local relocation entries are grouped together (they are not
     * grouped by their module since they are only used if the object is moved
     * from it staticly link edited address).
     */
    uint32_t locreloff;  /* offset to local relocation entries */
    uint32_t nlocrel;  /* number of local relocation entries */

};

/*
 * The rpath_command contains a path which at runtime should be added to
 * the current run path used to find @rpath prefixed dylibs.
 */
struct rpath_command {
    uint32_t   cmd;    /* LC_RPATH */
    uint32_t   cmdsize;  /* includes string */
    union lc_str path;    /* path to add to run path */
};


#ifndef _MACHO_NLIST_H_
#define _MACHO_NLIST_H_
/*  $NetBSD: nlist.h,v 1.5 1994/10/26 00:56:11 cgd Exp $  */

/*-
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)nlist.h  8.2 (Berkeley) 1/21/94
 */

/*
 * Format of a symbol table entry of a Mach-O file for 32-bit architectures.
 * Modified from the BSD format.  The modifications from the original format
 * were changing n_other (an unused field) to n_sect and the addition of the
 * N_SECT type.  These modifications are required to support symbols in a larger
 * number of sections not just the three sections (text, data and bss) in a BSD
 * file.
 */
struct nlist {
  union {
#ifndef __LP64__
    char *n_name;  /* for use when in-core */
#endif
    int32_t n_strx;  /* index into the string table */
  } n_un;
  uint8_t n_type;    /* type flag, see below */
  uint8_t n_sect;    /* section number or NO_SECT */
  int16_t n_desc;    /* see <mach-o/stab.h> */
  uint32_t n_value;  /* value of this symbol (or stab offset) */
};

/*
 * This is the symbol table entry structure for 64-bit architectures.
 */
struct nlist_64 {
    union {
        uint32_t  n_strx; /* index into the string table */
    } n_un;
    uint8_t n_type;        /* type flag, see below */
    uint8_t n_sect;        /* section number or NO_SECT */
    uint16_t n_desc;       /* see <mach-o/stab.h> */
    uint64_t n_value;      /* value of this symbol (or stab offset) */
};

/*
 * Symbols with a index into the string table of zero (n_un.n_strx == 0) are
 * defined to have a null, "", name.  Therefore all string indexes to non null
 * names must not have a zero string index.  This is bit historical information
 * that has never been well documented.
 */

/*
 * The n_type field really contains four fields:
 *  unsigned char N_STAB:3,
 *          N_PEXT:1,
 *          N_TYPE:3,
 *          N_EXT:1;
 * which are used via the following masks.
 */
#define  N_STAB  0xe0  /* if any of these bits set, a symbolic debugging entry */
#define  N_PEXT  0x10  /* private external symbol bit */
#define  N_TYPE  0x0e  /* mask for the type bits */
#define  N_EXT  0x01  /* external symbol bit, set for external symbols */

/*
 * Only symbolic debugging entries have some of the N_STAB bits set and if any
 * of these bits are set then it is a symbolic debugging entry (a stab).  In
 * which case then the values of the n_type field (the entire field) are given
 * in <mach-o/stab.h>
 */

/*
 * Values for N_TYPE bits of the n_type field.
 */
#define  N_UNDF  0x0    /* undefined, n_sect == NO_SECT */
#define  N_ABS  0x2    /* absolute, n_sect == NO_SECT */
#define  N_SECT  0xe    /* defined in section number n_sect */
#define  N_PBUD  0xc    /* prebound undefined (defined in a dylib) */
#define N_INDR  0xa    /* indirect */

/*
 * If the type is N_INDR then the symbol is defined to be the same as another
 * symbol.  In this case the n_value field is an index into the string table
 * of the other symbol's name.  When the other symbol is defined then they both
 * take on the defined type and value.
 */

/*
 * If the type is N_SECT then the n_sect field contains an ordinal of the
 * section the symbol is defined in.  The sections are numbered from 1 and
 * refer to sections in order they appear in the load commands for the file
 * they are in.  This means the same ordinal may very well refer to different
 * sections in different files.
 *
 * The n_value field for all symbol table entries (including N_STAB's) gets
 * updated by the link editor based on the value of it's n_sect field and where
 * the section n_sect references gets relocated.  If the value of the n_sect
 * field is NO_SECT then it's n_value field is not changed by the link editor.
 */
#define  NO_SECT    0  /* symbol is not in any section */
#define MAX_SECT  255  /* 1 thru 255 inclusive */

#endif /* _MACHO_LIST_H_ */

#endif /* CPPMICROSERVICES_MACHO_LOADER_H */

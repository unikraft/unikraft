/* SPDX-License-Identifier: MIT */
/*
 * hvm/save.h
 *
 * Structure definitions for HVM state that is held by Xen and must
 * be saved along with the domain's memory and device-model state.
 *
 * Copyright (c) 2007 XenSource Ltd.
 */

#ifndef __XEN_PUBLIC_HVM_SAVE_H__
#define __XEN_PUBLIC_HVM_SAVE_H__

/*
 * Structures in this header *must* have the same layout in 32bit
 * and 64bit environments: this means that all fields must be explicitly
 * sized types and aligned to their sizes, and the structs must be
 * a multiple of eight bytes long.
 *
 * Only the state necessary for saving and restoring (i.e. fields
 * that are analogous to actual hardware state) should go in this file.
 * Internal mechanisms should be kept in Xen-private headers.
 */

#if !defined(__GNUC__) || defined(__STRICT_ANSI__)
#error "Anonymous structs/unions are a GNU extension."
#endif

/*
 * Each entry is preceded by a descriptor giving its type and length
 */
struct hvm_save_descriptor {
    uint16_t typecode;          /* Used to demux the various types below */
    uint16_t instance;          /* Further demux within a type */
    uint32_t length;            /* In bytes, *not* including this descriptor */
};


/*
 * Each entry has a datatype associated with it: for example, the CPU state
 * is saved as a HVM_SAVE_TYPE(CPU), which has HVM_SAVE_LENGTH(CPU),
 * and is identified by a descriptor with typecode HVM_SAVE_CODE(CPU).
 * DECLARE_HVM_SAVE_TYPE binds these things together with some type-system
 * ugliness.
 */

#ifdef __XEN__
# define DECLARE_HVM_SAVE_TYPE_COMPAT(_x, _code, _type, _ctype, _fix)     \
    static inline int __HVM_SAVE_FIX_COMPAT_##_x(void *h, uint32_t size)  \
        { return _fix(h, size); }                                         \
    struct __HVM_SAVE_TYPE_##_x { _type t; char c[_code]; char cpt[2];};  \
    struct __HVM_SAVE_TYPE_COMPAT_##_x { _ctype t; }

# include <xen/bug.h> /* BUG() */
# define DECLARE_HVM_SAVE_TYPE(_x, _code, _type)                         \
    static inline int __HVM_SAVE_FIX_COMPAT_##_x(void *h, uint32_t size) \
        { BUG(); return -1; }                                            \
    struct __HVM_SAVE_TYPE_##_x { _type t; char c[_code]; char cpt[1];}; \
    struct __HVM_SAVE_TYPE_COMPAT_##_x { _type t; }
#else
# define DECLARE_HVM_SAVE_TYPE_COMPAT(_x, _code, _type, _ctype, _fix)     \
    struct __HVM_SAVE_TYPE_##_x { _type t; char c[_code]; char cpt[2];}

# define DECLARE_HVM_SAVE_TYPE(_x, _code, _type)                         \
    struct __HVM_SAVE_TYPE_##_x { _type t; char c[_code]; char cpt[1];}
#endif

#define HVM_SAVE_TYPE(_x) __typeof__(((struct __HVM_SAVE_TYPE_##_x *)NULL)->t)
#define HVM_SAVE_LENGTH(_x) (sizeof (HVM_SAVE_TYPE(_x)))
#define HVM_SAVE_CODE(_x) (sizeof(((struct __HVM_SAVE_TYPE_##_x *)NULL)->c))

#ifdef __XEN__
# define HVM_SAVE_TYPE_COMPAT(_x) __typeof__(((struct __HVM_SAVE_TYPE_COMPAT_##_x *)NULL)->t)
# define HVM_SAVE_LENGTH_COMPAT(_x) (sizeof (HVM_SAVE_TYPE_COMPAT(_x)))

# define HVM_SAVE_HAS_COMPAT(_x) (sizeof(((struct __HVM_SAVE_TYPE_##_x *)NULL)->cpt) - 1)
# define HVM_SAVE_FIX_COMPAT(_x, _dst, _size) __HVM_SAVE_FIX_COMPAT_##_x(_dst, _size)
#endif

/*
 * The series of save records is teminated by a zero-type, zero-length
 * descriptor.
 */

struct hvm_save_end {};
DECLARE_HVM_SAVE_TYPE(END, 0, struct hvm_save_end);

#if defined(__i386__) || defined(__x86_64__)
#include "../arch-x86/hvm/save.h"
#elif defined(__arm__) || defined(__aarch64__)
#include "../arch-arm/hvm/save.h"
#elif defined(__powerpc64__) || defined(__riscv)
/* no specific header to include */
#else
#error "unsupported architecture"
#endif

#endif /* __XEN_PUBLIC_HVM_SAVE_H__ */

/******************************************************************************
 * platform.h
 * 
 * Hardware platform operations. Intended for use by domain-0 kernel.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2002-2006, K Fraser
 */

#ifndef __XEN_PUBLIC_PLATFORM_H__
#define __XEN_PUBLIC_PLATFORM_H__

#include "xen.h"

#define XENPF_INTERFACE_VERSION 0x03000001

/*
 * Set clock such that it would read <secs,nsecs> after 00:00:00 UTC,
 * 1 January, 1970 if the current system time was <system_time>.
 */
#define XENPF_settime32           17
struct xenpf_settime32 {
    /* IN variables. */
    __u32 secs;
    __u32 nsecs;
    __u64 system_time;
};
#define XENPF_settime64           62
struct xenpf_settime64 {
    /* IN variables. */
    __u64 secs;
    __u32 nsecs;
    __u32 mbz;
    __u64 system_time;
};
#if __XEN_INTERFACE_VERSION__ < 0x00040600
#define XENPF_settime XENPF_settime32
#define xenpf_settime xenpf_settime32
#else
#define XENPF_settime XENPF_settime64
#define xenpf_settime xenpf_settime64
#endif
typedef struct xenpf_settime xenpf_settime_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_settime_t);

/*
 * Request memory range (@mfn, @mfn+@nr_mfns-1) to have type @type.
 * On x86, @type is an architecture-defined MTRR memory type.
 * On success, returns the MTRR that was used (@reg) and a handle that can
 * be passed to XENPF_DEL_MEMTYPE to accurately tear down the new setting.
 * (x86-specific).
 */
#define XENPF_add_memtype         31
struct xenpf_add_memtype {
    /* IN variables. */
    xen_pfn_t mfn;
    __u64 nr_mfns;
    __u32 type;
    /* OUT variables. */
    __u32 handle;
    __u32 reg;
};
typedef struct xenpf_add_memtype xenpf_add_memtype_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_add_memtype_t);

/*
 * Tear down an existing memory-range type. If @handle is remembered then it
 * should be passed in to accurately tear down the correct setting (in case
 * of overlapping memory regions with differing types). If it is not known
 * then @handle should be set to zero. In all cases @reg must be set.
 * (x86-specific).
 */
#define XENPF_del_memtype         32
struct xenpf_del_memtype {
    /* IN variables. */
    __u32 handle;
    __u32 reg;
};
typedef struct xenpf_del_memtype xenpf_del_memtype_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_del_memtype_t);

/* Read current type of an MTRR (x86-specific). */
#define XENPF_read_memtype        33
struct xenpf_read_memtype {
    /* IN variables. */
    __u32 reg;
    /* OUT variables. */
    xen_pfn_t mfn;
    __u64 nr_mfns;
    __u32 type;
};
typedef struct xenpf_read_memtype xenpf_read_memtype_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_read_memtype_t);

#define XENPF_microcode_update    35
struct xenpf_microcode_update {
    /* IN variables. */
    XEN_GUEST_HANDLE(const_void) data;/* Pointer to microcode data */
    __u32 length;                  /* Length of microcode data. */
};
typedef struct xenpf_microcode_update xenpf_microcode_update_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_microcode_update_t);

#define XENPF_platform_quirk      39
#define QUIRK_NOIRQBALANCING      1 /* Do not restrict IO-APIC RTE targets */
#define QUIRK_IOAPIC_BAD_REGSEL   2 /* IO-APIC REGSEL forgets its value    */
#define QUIRK_IOAPIC_GOOD_REGSEL  3 /* IO-APIC REGSEL behaves properly     */
struct xenpf_platform_quirk {
    /* IN variables. */
    __u32 quirk_id;
};
typedef struct xenpf_platform_quirk xenpf_platform_quirk_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_platform_quirk_t);

#define XENPF_efi_runtime_call    49
#define XEN_EFI_get_time                      1
#define XEN_EFI_set_time                      2
#define XEN_EFI_get_wakeup_time               3
#define XEN_EFI_set_wakeup_time               4
#define XEN_EFI_get_next_high_monotonic_count 5
#define XEN_EFI_get_variable                  6
#define XEN_EFI_set_variable                  7
#define XEN_EFI_get_next_variable_name        8
#define XEN_EFI_query_variable_info           9
#define XEN_EFI_query_capsule_capabilities   10
#define XEN_EFI_update_capsule               11

struct xenpf_efi_time {
    __u16 year;
    __u8 month;
    __u8 day;
    __u8 hour;
    __u8 min;
    __u8 sec;
    __u32 ns;
    __s16 tz;
    __u8 daylight;
};

struct xenpf_efi_guid {
    __u32 data1;
    __u16 data2;
    __u16 data3;
    __u8 data4[8];
};

struct xenpf_efi_runtime_call {
    __u32 function;
    /*
     * This field is generally used for per sub-function flags (defined
     * below), except for the XEN_EFI_get_next_high_monotonic_count case,
     * where it holds the single returned value.
     */
    __u32 misc;
    xen_ulong_t status;
    union {
#define XEN_EFI_GET_TIME_SET_CLEARS_NS 0x00000001
        struct {
            struct xenpf_efi_time time;
            __u32 resolution;
            __u32 accuracy;
        } get_time;

        struct xenpf_efi_time set_time;

#define XEN_EFI_GET_WAKEUP_TIME_ENABLED 0x00000001
#define XEN_EFI_GET_WAKEUP_TIME_PENDING 0x00000002
        struct xenpf_efi_time get_wakeup_time;

#define XEN_EFI_SET_WAKEUP_TIME_ENABLE      0x00000001
#define XEN_EFI_SET_WAKEUP_TIME_ENABLE_ONLY 0x00000002
        struct xenpf_efi_time set_wakeup_time;

#define XEN_EFI_VARIABLE_NON_VOLATILE       0x00000001
#define XEN_EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define XEN_EFI_VARIABLE_RUNTIME_ACCESS     0x00000004
        struct {
            XEN_GUEST_HANDLE(void) name;  /* UCS-2/UTF-16 string */
            xen_ulong_t size;
            XEN_GUEST_HANDLE(void) data;
            struct xenpf_efi_guid vendor_guid;
        } get_variable, set_variable;

        struct {
            xen_ulong_t size;
            XEN_GUEST_HANDLE(void) name;  /* UCS-2/UTF-16 string */
            struct xenpf_efi_guid vendor_guid;
        } get_next_variable_name;

#define XEN_EFI_VARINFO_BOOT_SNAPSHOT       0x00000001
        struct {
            __u32 attr;
            __u64 max_store_size;
            __u64 remain_store_size;
            __u64 max_size;
        } query_variable_info;

        struct {
            XEN_GUEST_HANDLE(void) capsule_header_array;
            xen_ulong_t capsule_count;
            __u64 max_capsule_size;
            __u32 reset_type;
        } query_capsule_capabilities;

        struct {
            XEN_GUEST_HANDLE(void) capsule_header_array;
            xen_ulong_t capsule_count;
            __u64 sg_list; /* machine address */
        } update_capsule;
    } u;
};
typedef struct xenpf_efi_runtime_call xenpf_efi_runtime_call_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_efi_runtime_call_t);

#define XENPF_firmware_info       50
#define XEN_FW_DISK_INFO          1 /* from int 13 AH=08/41/48 */
#define XEN_FW_DISK_MBR_SIGNATURE 2 /* from MBR offset 0x1b8 */
#define XEN_FW_VBEDDC_INFO        3 /* from int 10 AX=4f15 */
#define XEN_FW_EFI_INFO           4 /* from EFI */
#define  XEN_FW_EFI_VERSION        0
#define  XEN_FW_EFI_CONFIG_TABLE   1
#define  XEN_FW_EFI_VENDOR         2
#define  XEN_FW_EFI_MEM_INFO       3
#define  XEN_FW_EFI_RT_VERSION     4
#define  XEN_FW_EFI_PCI_ROM        5
#define  XEN_FW_EFI_APPLE_PROPERTIES 6
#define XEN_FW_KBD_SHIFT_FLAGS    5
struct xenpf_firmware_info {
    /* IN variables. */
    __u32 type;
    __u32 index;
    /* OUT variables. */
    union {
        struct {
            /* Int13, Fn48: Check Extensions Present. */
            __u8 device;                   /* %dl: bios device number */
            __u8 version;                  /* %ah: major version      */
            __u16 interface_support;       /* %cx: support bitmap     */
            /* Int13, Fn08: Legacy Get Device Parameters. */
            __u16 legacy_max_cylinder;     /* %cl[7:6]:%ch: max cyl # */
            __u8 legacy_max_head;          /* %dh: max head #         */
            __u8 legacy_sectors_per_track; /* %cl[5:0]: max sector #  */
            /* Int13, Fn41: Get Device Parameters (as filled into %ds:%esi). */
            /* NB. First __u16 of buffer must be set to buffer size.      */
            XEN_GUEST_HANDLE(void) edd_params;
        } disk_info; /* XEN_FW_DISK_INFO */
        struct {
            __u8 device;                   /* bios device number  */
            __u32 mbr_signature;           /* offset 0x1b8 in mbr */
        } disk_mbr_signature; /* XEN_FW_DISK_MBR_SIGNATURE */
        struct {
            /* Int10, AX=4F15: Get EDID info. */
            __u8 capabilities;
            __u8 edid_transfer_time;
            /* must refer to 128-byte buffer */
            XEN_GUEST_HANDLE(__u8) edid;
        } vbeddc_info; /* XEN_FW_VBEDDC_INFO */
        union xenpf_efi_info {
            __u32 version;
            struct {
                __u64 addr;                /* EFI_CONFIGURATION_TABLE */
                __u32 nent;
            } cfg;
            struct {
                __u32 revision;
                __u32 bufsz;               /* input, in bytes */
                XEN_GUEST_HANDLE(void) name;  /* UCS-2/UTF-16 string */
            } vendor;
            struct {
                __u64 addr;
                __u64 size;
                __u64 attr;
                __u32 type;
            } mem;
            struct {
                /* IN variables */
                __u16 segment;
                __u8 bus;
                __u8 devfn;
                __u16 vendor;
                __u16 devid;
                /* OUT variables */
                __u64 address;
                xen_ulong_t size;
            } pci_rom;
            struct {
                /* OUT variables */
                __u64 address;
                xen_ulong_t size;
            } apple_properties;
        } efi_info; /* XEN_FW_EFI_INFO */

        /* Int16, Fn02: Get keyboard shift flags. */
        __u8 kbd_shift_flags; /* XEN_FW_KBD_SHIFT_FLAGS */
    } u;
};
typedef struct xenpf_firmware_info xenpf_firmware_info_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_firmware_info_t);

#define XENPF_enter_acpi_sleep    51
struct xenpf_enter_acpi_sleep {
    /* IN variables */
#if __XEN_INTERFACE_VERSION__ < 0x00040300
    __u16 pm1a_cnt_val;      /* PM1a control value. */
    __u16 pm1b_cnt_val;      /* PM1b control value. */
#else
    __u16 val_a;             /* PM1a control / sleep type A. */
    __u16 val_b;             /* PM1b control / sleep type B. */
#endif
    __u32 sleep_state;       /* Which state to enter (Sn). */
#define XENPF_ACPI_SLEEP_EXTENDED 0x00000001
    __u32 flags;             /* XENPF_ACPI_SLEEP_*. */
};
typedef struct xenpf_enter_acpi_sleep xenpf_enter_acpi_sleep_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_enter_acpi_sleep_t);

#define XENPF_change_freq         52
struct xenpf_change_freq {
    /* IN variables */
    __u32 flags; /* Must be zero. */
    __u32 cpu;   /* Physical cpu. */
    __u64 freq;  /* New frequency (Hz). */
};
typedef struct xenpf_change_freq xenpf_change_freq_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_change_freq_t);

/*
 * Get idle times (nanoseconds since boot) for physical CPUs specified in the
 * @cpumap_bitmap with range [0..@cpumap_nr_cpus-1]. The @idletime array is
 * indexed by CPU number; only entries with the corresponding @cpumap_bitmap
 * bit set are written to. On return, @cpumap_bitmap is modified so that any
 * non-existent CPUs are cleared. Such CPUs have their @idletime array entry
 * cleared.
 */
#define XENPF_getidletime         53
struct xenpf_getidletime {
    /* IN/OUT variables */
    /* IN: CPUs to interrogate; OUT: subset of IN which are present */
    XEN_GUEST_HANDLE(__u8) cpumap_bitmap;
    /* IN variables */
    /* Size of cpumap bitmap. */
    __u32 cpumap_nr_cpus;
    /* Must be indexable for every cpu in cpumap_bitmap. */
    XEN_GUEST_HANDLE(__u64) idletime;
    /* OUT variables */
    /* System time when the idletime snapshots were taken. */
    __u64 now;
};
typedef struct xenpf_getidletime xenpf_getidletime_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_getidletime_t);

#define XENPF_set_processor_pminfo      54

/* ability bits */
#define XEN_PROCESSOR_PM_CX	1
#define XEN_PROCESSOR_PM_PX	2
#define XEN_PROCESSOR_PM_TX	4

/* cmd type */
#define XEN_PM_CX   0
#define XEN_PM_PX   1
#define XEN_PM_TX   2
#define XEN_PM_PDC  3

/* Px sub info type */
#define XEN_PX_PCT   1
#define XEN_PX_PSS   2
#define XEN_PX_PPC   4
#define XEN_PX_PSD   8

struct xen_power_register {
    __u32     space_id;
    __u32     bit_width;
    __u32     bit_offset;
    __u32     access_size;
    __u64     address;
};

struct xen_processor_csd {
    __u32    domain;      /* domain number of one dependent group */
    __u32    coord_type;  /* coordination type */
    __u32    num;         /* number of processors in same domain */
};
typedef struct xen_processor_csd xen_processor_csd_t;
DEFINE_XEN_GUEST_HANDLE(xen_processor_csd_t);

struct xen_processor_cx {
    struct xen_power_register  reg; /* GAS for Cx trigger register */
    __u8     type;     /* cstate value, c0: 0, c1: 1, ... */
    __u32    latency;  /* worst latency (ms) to enter/exit this cstate */
    __u32    power;    /* average power consumption(mW) */
    __u32    dpcnt;    /* number of dependency entries */
    XEN_GUEST_HANDLE(xen_processor_csd_t) dp; /* NULL if no dependency */
};
typedef struct xen_processor_cx xen_processor_cx_t;
DEFINE_XEN_GUEST_HANDLE(xen_processor_cx_t);

struct xen_processor_flags {
    __u32 bm_control:1;
    __u32 bm_check:1;
    __u32 has_cst:1;
    __u32 power_setup_done:1;
    __u32 bm_rld_set:1;
};

struct xen_processor_power {
    __u32 count;  /* number of C state entries in array below */
    struct xen_processor_flags flags;  /* global flags of this processor */
    XEN_GUEST_HANDLE(xen_processor_cx_t) states; /* supported c states */
};

struct xen_pct_register {
    __u8  descriptor;
    __u16 length;
    __u8  space_id;
    __u8  bit_width;
    __u8  bit_offset;
    __u8  reserved;
    __u64 address;
};

struct xen_processor_px {
    __u64 core_frequency; /* megahertz */
    __u64 power;      /* milliWatts */
    __u64 transition_latency; /* microseconds */
    __u64 bus_master_latency; /* microseconds */
    __u64 control;        /* control value */
    __u64 status;     /* success indicator */
};
typedef struct xen_processor_px xen_processor_px_t;
DEFINE_XEN_GUEST_HANDLE(xen_processor_px_t);

struct xen_psd_package {
    __u64 num_entries;
    __u64 revision;
    __u64 domain;
    __u64 coord_type;
    __u64 num_processors;
};

struct xen_processor_performance {
    __u32 flags;     /* flag for Px sub info type */
    __u32 platform_limit;  /* Platform limitation on freq usage */
    struct xen_pct_register control_register;
    struct xen_pct_register status_register;
    __u32 state_count;     /* total available performance states */
    XEN_GUEST_HANDLE(xen_processor_px_t) states;
    struct xen_psd_package domain_info;
    __u32 shared_type;     /* coordination type of this processor */
};
typedef struct xen_processor_performance xen_processor_performance_t;
DEFINE_XEN_GUEST_HANDLE(xen_processor_performance_t);

struct xenpf_set_processor_pminfo {
    /* IN variables */
    __u32 id;    /* ACPI CPU ID */
    __u32 type;  /* {XEN_PM_CX, XEN_PM_PX} */
    union {
        struct xen_processor_power          power;/* Cx: _CST/_CSD */
        struct xen_processor_performance    perf; /* Px: _PPC/_PCT/_PSS/_PSD */
        XEN_GUEST_HANDLE(__u32)            pdc;  /* _PDC */
    } u;
};
typedef struct xenpf_set_processor_pminfo xenpf_set_processor_pminfo_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_set_processor_pminfo_t);

#define XENPF_get_cpuinfo 55
struct xenpf_pcpuinfo {
    /* IN */
    __u32 xen_cpuid;
    /* OUT */
    /* The maxium cpu_id that is present */
    __u32 max_present;
#define XEN_PCPU_FLAGS_ONLINE   1
    /* Correponding xen_cpuid is not present*/
#define XEN_PCPU_FLAGS_INVALID  2
    __u32 flags;
    __u32 apic_id;
    __u32 acpi_id;
};
typedef struct xenpf_pcpuinfo xenpf_pcpuinfo_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_pcpuinfo_t);

#define XENPF_get_cpu_version 48
struct xenpf_pcpu_version {
    /* IN */
    __u32 xen_cpuid;
    /* OUT */
    /* The maxium cpu_id that is present */
    __u32 max_present;
    char vendor_id[12];
    __u32 family;
    __u32 model;
    __u32 stepping;
};
typedef struct xenpf_pcpu_version xenpf_pcpu_version_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_pcpu_version_t);

#define XENPF_cpu_online    56
#define XENPF_cpu_offline   57
struct xenpf_cpu_ol
{
    __u32 cpuid;
};
typedef struct xenpf_cpu_ol xenpf_cpu_ol_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_cpu_ol_t);

#define XENPF_cpu_hotadd    58
struct xenpf_cpu_hotadd
{
	__u32 apic_id;
	__u32 acpi_id;
	__u32 pxm;
};

#define XENPF_mem_hotadd    59
struct xenpf_mem_hotadd
{
    __u64 spfn;
    __u64 epfn;
    __u32 pxm;
    __u32 flags;
};

#define XENPF_core_parking  60

#define XEN_CORE_PARKING_SET 1
#define XEN_CORE_PARKING_GET 2
struct xenpf_core_parking {
    /* IN variables */
    __u32 type;
    /* IN variables:  set cpu nums expected to be idled */
    /* OUT variables: get cpu nums actually be idled */
    __u32 idle_nums;
};
typedef struct xenpf_core_parking xenpf_core_parking_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_core_parking_t);

/*
 * Access generic platform resources(e.g., accessing MSR, port I/O, etc)
 * in unified way. Batch resource operations in one call are supported and
 * they are always non-preemptible and executed in their original order.
 * The batch itself returns a negative integer for general errors, or a
 * non-negative integer for the number of successful operations. For the latter
 * case, the @ret in the failed entry (if any) indicates the exact error.
 */
#define XENPF_resource_op   61

#define XEN_RESOURCE_OP_MSR_READ  0
#define XEN_RESOURCE_OP_MSR_WRITE 1

/*
 * Specially handled MSRs:
 * - MSR_IA32_TSC
 * READ: Returns the scaled system time(ns) instead of raw timestamp. In
 *       multiple entry case, if other MSR read is followed by a MSR_IA32_TSC
 *       read, then both reads are guaranteed to be performed atomically (with
 *       IRQ disabled). The return time indicates the point of reading that MSR.
 * WRITE: Not supported.
 */

struct xenpf_resource_entry {
    union {
        __u32 cmd;   /* IN: XEN_RESOURCE_OP_* */
        __s32  ret;   /* OUT: return value for failed entry */
    } u;
    __u32 rsvd;      /* IN: padding and must be zero */
    __u64 idx;       /* IN: resource address to access */
    __u64 val;       /* IN/OUT: resource value to set/get */
};
typedef struct xenpf_resource_entry xenpf_resource_entry_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_resource_entry_t);

struct xenpf_resource_op {
    __u32 nr_entries;    /* number of resource entry */
    __u32 cpu;           /* which cpu to run */
    XEN_GUEST_HANDLE(xenpf_resource_entry_t) entries;
};
typedef struct xenpf_resource_op xenpf_resource_op_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_resource_op_t);

#define XENPF_get_symbol   63
struct xenpf_symdata {
    /* IN/OUT variables */
    __u32 namelen; /* IN:  size of name buffer                       */
                      /* OUT: strlen(name) of hypervisor symbol (may be */
                      /*      larger than what's been copied to guest)  */
    __u32 symnum;  /* IN:  Symbol to read                            */
                      /* OUT: Next available symbol. If same as IN then */
                      /*      we reached the end                        */

    /* OUT variables */
    XEN_GUEST_HANDLE(char) name;
    __u64 address;
    char type;
};
typedef struct xenpf_symdata xenpf_symdata_t;
DEFINE_XEN_GUEST_HANDLE(xenpf_symdata_t);

/*
 * ` enum neg_errnoval
 * ` HYPERVISOR_platform_op(const struct xen_platform_op*);
 */
struct xen_platform_op {
    __u32 cmd;
    __u32 interface_version; /* XENPF_INTERFACE_VERSION */
    union {
        struct xenpf_settime           settime;
        struct xenpf_settime32         settime32;
        struct xenpf_settime64         settime64;
        struct xenpf_add_memtype       add_memtype;
        struct xenpf_del_memtype       del_memtype;
        struct xenpf_read_memtype      read_memtype;
        struct xenpf_microcode_update  microcode;
        struct xenpf_platform_quirk    platform_quirk;
        struct xenpf_efi_runtime_call  efi_runtime_call;
        struct xenpf_firmware_info     firmware_info;
        struct xenpf_enter_acpi_sleep  enter_acpi_sleep;
        struct xenpf_change_freq       change_freq;
        struct xenpf_getidletime       getidletime;
        struct xenpf_set_processor_pminfo set_pminfo;
        struct xenpf_pcpuinfo          pcpu_info;
        struct xenpf_pcpu_version      pcpu_version;
        struct xenpf_cpu_ol            cpu_ol;
        struct xenpf_cpu_hotadd        cpu_add;
        struct xenpf_mem_hotadd        mem_add;
        struct xenpf_core_parking      core_parking;
        struct xenpf_resource_op       resource_op;
        struct xenpf_symdata           symdata;
        __u8                        pad[128];
    } u;
};
typedef struct xen_platform_op xen_platform_op_t;
DEFINE_XEN_GUEST_HANDLE(xen_platform_op_t);

#endif /* __XEN_PUBLIC_PLATFORM_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

#ifndef BOOTPARAMS_HEADER
#define BOOTPARAMS_HEADER

#include <uk/essentials.h>

struct setup_header {
        __u8 _pad1[39];
        __u32 ramdisk_image;
        __u32 ramdisk_size;
        __u8 _pad2[4];
        __u16 heap_end_ptr;
        __u8 _pad3[2];
        __u32 cmd_line_ptr;
        __u8 _pad4[12];
        __u32 cmdline_size;
        __u8 _pad5[44];
} __attribute__((packed));

struct boot_e820_entry {
        __u64 addr;
        __u64 size;
        __u32 type;
} __attribute__((packed));

#define E820_MAX_ENTRIES 128

struct boot_params {
        __u8 _pad1[192];
        __u32 ext_ramdisk_image;
        __u32 ext_ramdisk_size;
        __u32 ext_cmd_line_ptr;
        __u8 _pad2[284];
        __u8 e820_entries;
        __u8 _pad3[8];
        struct setup_header hdr;
        __u8 _pad4[104];
        struct boot_e820_entry e820_table[E820_MAX_ENTRIES];
        __u8 _pad5[2560-(sizeof(struct boot_e820_entry) * E820_MAX_ENTRIES)];
        __u8 _pad6[816];
} __attribute__((packed));

#endif /* ! BOOTPARAMS_HEADER */
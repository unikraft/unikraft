# Lxboot Support in Unikraft

<img width="100px" src="https://summerofcode.withgoogle.com/assets/media/gsoc-generic-badge.svg" align="right" />

## Project Overview

The Linux boot protocol plays an important role in the initialization of the Linux operating system, emphasizing the importance of system optimization and scalability.
This system ensures a seamless transfer of control from the bootloader to the Linux kernel by providing essential information including memory map, command line parameters, and device specifications.
By adhering to a uniform boot protocol, Linux achieves a level of versatility across the hardware spectrum of architectures and bootloader functions.
Essentially, this protocol is a commitment to a standardized approach that is not only stable but also facilitates bootloader development, improving compatibility and reliability.
In particular, the Linux boot protocol greatly contributes to the overall robustness and reliability of the bootstrapping process on Linux-based systems.
Almost, if not all, widely available bootloaders that exist on the market are able to speak this protocol and Unikraft, being unable to be detected by these bootloaders as a Linux Boot Protocol compliant system, is hindering the ability to boot on a very large amount of platforms.

This is how the Lxboot header looks:

| Offset/Size | Protocol    | Name                | Meaning                                                |
|-------------|-------------|---------------------|--------------------------------------------------------|
| 01F1/1      | ALL(1)      | setup_sects         | The size of the setup in sectors                       |
| 01F2/2      | ALL         | root_flags          | If set, the root is mounted readonly                   |
| 01F4/4      | 2.04+(2)    | syssize             | The size of the 32-bit code in 16-byte paras           |
| 01F8/2      | ALL         | ram_size            | DO NOT USE - for bootsect.S use only                   |
| 01FA/2      | ALL         | vid_mode            | Video mode control                                     |
| 01FC/2      | ALL         | root_dev            | Default root device number                             |
| 01FE/2      | ALL         | boot_flag           | 0xAA55 magic number                                    |
| 0200/2      | 2.00+       | jump                | Jump instruction                                       |
| 0202/4      | 2.00+       | header              | Magic signature “HdrS”                                 |
| 0206/2      | 2.00+       | version             | Boot protocol version supported                        |
| 0208/4      | 2.00+       | realmode_swtch      | Boot loader hook (see below)                           |
| 020C/2      | 2.00+       | start_sys_seg       | The load-low segment (0x1000) (obsolete)               |
| 020E/2      | 2.00+       | kernel_version      | Pointer to kernel version string                       |
| 0210/1      | 2.00+       | type_of_loader      | Boot loader identifier                                 |
| 0211/1      | 2.00+       | loadflags           | Boot protocol option flags                             |
| 0212/2      | 2.00+       | setup_move_size     | Move to high memory size (used with hooks)             |
| 0214/4      | 2.00+       | code32_start        | Boot loader hook (see below)                           |
| 0218/4      | 2.00+       | ramdisk_image       | initrd load address (set by boot loader)               |
| 021C/4      | 2.00+       | ramdisk_size        | initrd size (set by boot loader)                       |
| 0220/4      | 2.00+       | bootsect_kludge     | DO NOT USE - for bootsect.S use only                   |
| 0224/2      | 2.01+       | heap_end_ptr        | Free memory after setup end                            |
| 0226/1      | 2.02+(3)    | ext_loader_ver      | Extended boot loader version                           |
| 0227/1      | 2.02+(3)    | ext_loader_type     | Extended boot loader ID                                |
| 0228/4      | 2.02+       | cmd_line_ptr        | 32-bit pointer to the kernel command line              |
| 022C/4      | 2.03+       | initrd_addr_max     | Highest legal initrd address                           |
| 0230/4      | 2.05+       | kernel_alignment    | Physical addr alignment required for kernel            |
| 0234/1      | 2.05+       | relocatable_kernel  | Whether kernel is relocatable or not                   |
| 0235/1      | 2.10+       | min_alignment       | Minimum alignment, as a power of two                   |
| 0236/2      | 2.12+       | xloadflags          | Boot protocol option flags                             |
| 0238/4      | 2.06+       | cmdline_size        | Maximum size of the kernel command line                |
| 023C/4      | 2.07+       | hardware_subarch    | Hardware subarchitecture                               |
| 0240/8      | 2.07+       | hardware_subarch_data| Subarchitecture-specific data                          |
| 0248/4      | 2.08+       | payload_offset      | Offset of kernel payload                               |
| 024C/4      | 2.08+       | payload_length      | Length of kernel payload                               |
| 0250/8      | 2.09+       | setup_data          | 64-bit physical pointer to linked list of struct setup_data|
| 0258/8      | 2.10+       | pref_address        | Preferred loading address                              |
| 0260/4      | 2.10+       | init_size           | Linear memory required during initialization           |
| 0264/4      | 2.11+       | handover_offset     | Offset of handover entry point                         |

## GSoC Contributor

Name: Mihnea Firoiu

Email: <mihneafiroiu0@gmail.com>

GitHub profile: [Mihnea0Firoiu](https://github.com/Mihnea0Firoiu)

## Mentors

* [Răzvan Deaconescu](https://github.com/razvand)
* [Sergiu Moga](https://github.com/mogasergiu)
* [Ștefan Jumărea](https://github.com/StefanJum)

## Contributions

Here you can see the [pull request](https://github.com/unikraft/unikraft/pull/1466) that represents everything I did during this project.

### `mklinux_x86_64.py`

The `mklinux_x86_64.py` is the biggest and most significant part of my project.
Here you can find everything about from the way the image was prepended with the Lxboot header, to the creation of the 16bit assembly trampoline.

The final image should look like this:

```
    Padding to lxboot header
    Lxboot header
    Padding to alignment
    Binary
```

The padding before the Lxboot header is needed because it starts at offset `0x1f1`.
TODO: Why offset `0x1f1`
Inside the second padding area, the 16bit assembly trampoline is found, that makes the jump to the 32bit section, finishing the booting process.

### `plat/kvm/Linker.uk`, `plat/kvm/Config.uk` and `plat/common/Makefile.rules`

These were modified to facilitate a seamless integration with the already available booting protocols.

These ... (TODO: fill the phrase)

## Blog Posts

* [Blog post #1](https://github.com/unikraft/docs/pull/429)

* [Blog post #2](https://github.com/unikraft/docs/pull/440)

* [Blog post #3](https://github.com/unikraft/docs/pull/449)

## Current Status

At this point, the Lxboot protocol reaches the 64bit section.
A `Hello, world!` is not yet possible.
TODO: Why is not yet possible, what is required.

## Future Work

The [memory map](https://wiki.osdev.org/Detecting_Memory_(x86)#Getting_an_E820_Memory_Map) needs to be implemented and the [`boot_params` struct](https://github.com/torvalds/linux/blob/master/arch/x86/include/uapi/asm/bootparam.h#L116) needs to be built.
TODO: Why doe the need to be implemented and built? What will you get after this?

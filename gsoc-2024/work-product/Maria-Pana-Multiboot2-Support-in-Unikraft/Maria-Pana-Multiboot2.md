# Multiboot2 support in Unikraft

<img width="100px" src="https://summerofcode.withgoogle.com/assets/media/gsoc-generic-badge.svg" align="right" />

## Project Overview

The aim of this project is to enhance Unikraft's compatibility and versatility by integrating support for the [Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html) protocol.
By adopting [Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html), [Unikraft](https://github.com/unikraft/unikraft) will gain the ability to boot directly in 64-bit mode, access modern firmware interfaces, and leverage the protocol's improved communication mechanisms.

When booting an x86 system, the CPU starts in 16-bit mode and the memory is limited to 1MB (in Real Mode).
The CPU starts executing at a specific memory address known as a reset vector situated at 0xFFFF0 (for Intel 8086 processors) or 0xFFFFFFF0 (for Intel 80386 and later).
The initial instructions found here are part of the BIOS, stored in flash memory on the motherboard.
The BIOS's first goal is to run self test and initialization routines on hardware.
During this phase, the BIOS sets up standardized interfaces (e.g. legacy BIOS interrupts or UEFI routines) that software that succeeds it (bootloader or operating system) will use to understand the computer's resources without needing special drivers for each different motherboard model.

However, having the kernel directly deal with the interfaces installed by the BIOS unnecessarily increases complexity and maintenance effort.
This is where the bootloader comes in.
After its initial routines, BIOS looks for bootable devices, such as hard disk drives (HDDs).
If it finds one that has a valid (P)MBR or GPT partitioning scheme with a boot partition, the control is passed to the bootloader.
This will then load the OS kernel into memory either by using the interfaces offered by the BIOS or its own drivers, and transfer control to it.

Boot protocols define the specific methods and standards used for this transfer of control.
There are multiple of them, some already supported by Unikraft.
Nonetheless, there is still room to add other boot protocols, such as [Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html), which is the focus of this project.

[Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html), an enhanced version of [Multiboot](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html), solves some of the limitations of the original protocol, such as the lack of support for 64-bit UEFI systems.
It also provides a more accurate and detailed communication during the bootstrapping process and offers a standardized tag system for easier configuration.
For instance, the [Command Line Tag](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Boot-command-line) retrieves boot parameters essential for configuring the kernel, while the [Memory Map Tag](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Memory-map) provides a memory map crucial for managing physical memory.
Additionally, the ELF Sections Tag delivers information about ELF sections, aiding in the proper handling of executable formats.
By leveraging these tags and the others Multiboot2 offers, we can ensure that the kernel has access to all the necessary information right from the boot process, which simplifies kernel initialization and enhances compatibility and flexibility.

## GSoC Contributor

Name: Maria Pana

Email: <maria.pana4@gmail.com>

Github profile: [mariapana](https://github.com/mariapana)

## Mentors

* [Razvan Deaconescu](https://github.com/razvand)
* [Sergiu Moga](https://github.com/mogasergiu)
* [Cristian Vijelie](https://github.com/cristian-vijelie)
* [Michalis Pappas](https://github.com/michpappas)

## Contributions

My contributions to the project, condensed into this [pull request](https://github.com/unikraft/unikraft/pull/1463), are focused on integrating Multiboot2 support into Unikraft.

Once successful, the interaction between the bootloader (GRUB) and the kernel (Unikraft) goes as follows:

The bootloader locates the Multiboot2 header within the ELF file.
After verifying the magic number and architecture compatibility, it parses the header and tags.
Next, the information requested by the tags is placed into the kernel memory and the bootloader passes control to the kernel.

The kernel, in turn, accesses the Multiboot2 header using the information provided by the bootloader.
It iterates through the tags to extract the necessary data, such as memory regions, boot command line, and other parameters.
This information is used to initialize its own data structures, set up memory management, and configure the system for execution.

To achieve this, I mirrored the existing file organization established for Multiboot and extended it to accommodate the new protocol.
Three core files handle the protocol itself, while the rest of the codebase is updated around them to ensure proper integration and functionality.
Let's take a closer look:

### `multiboot.S`

This assembly file plays a pivotal role in preparing a standardized operating environment.
It verifies the system is booted through a compliant Multiboot/Multiboot2 bootloader and manages essential memory relocations.
In this context, it was required to update the file to support Multiboot2 as well.
This way, based on the chosen configurations, the program uses either `multiboot.h` and `MULTIBOOT_BOOTLOADER_MAGIC` for Multiboot or their Multiboot2 counterparts.

### `multiboot2.py`

The Python script generates and adds the Multiboot2 and updated ELF headers to the original ELF file.
This ensures that the bootloader information is strategically positioned at the start of the resulting ELF, facilitating correct system initialization.
Since the Multiboot protocol is limited to 32-bit systems, the `multiboot.py` script also required transforming 64-bit ELFs into 32-bit ones.
This is no longer the case when it comes to Multiboot2, only needing to incorporate the Multiboot2 and updated ELF headers into the file, without any other alterations.

The Multiboot2 header follows a specific format, starting with a fixed-size header followed by a series of variable-length tags.
The fixed-size header includes:

- The magic number `0xE85250D6` indicating a Multiboot2-compliant image
- The target architecture  (e.g. `i386`, `x86_64`)
- The header length, including all tags
- The checksum of the header, used for error detection

The header continues with the header tags, which provide detailed information about the system's memory layout, boot command line, and other essential parameters.
Commonly, each tag has a type, size and payload.
There is a multitude of tags to choose from, depending on the specific requirements of the system, adding to the flexibility of Multiboot2.

In the context of this project, it is worth noting that simply adding the Multiboot2 header to the ELF file is not sufficient.
On top of that, both `multiboot.py` and `multiboot2.py` scripts add an extra ELF header with updated offsets, "sandwiching" the Multiboot2 header between it and the original ELF.
To better visualize this, we can refer to the following diagram:

```text
          ┌────────────────────┐
  ┌───────│ Updated EHDR       |─────────────┐
  |       ├────────────────────┤             |
  |       | MB2HDR             |             |
  |       ├────────────────────┤             |
  └──────>| Updated PHDR       |             |
          ├────────────────────┤             |  
  ┌───────| Original EHDR      |────────┐    |
  |       ├────────────────────┤        |    |
  └──────>| Original PHDR      |        |    |
          ├────────────────────┤        |    |
          | Original SHDR      |<───────┘    |
          ├────────────────────┤             |
          | ...                |             |
          ├────────────────────┤             |
          | Updated SHDR       |<────────────┘
          └────────────────────┘
```

The offset to the ELF64 Program/Sections Headers are at the end of the Multiboot2 Header and the end of the file respectively, so that we do not mess up the Multiboot2 header, which must exist in the first 8192 bytes.
Although the specification only enforces it to be contained completely within the first 32768 bytes, GRUB, for whatever reason, wants the Program Headers to also be placed in the first 8192 bytes (see commit 9a5c1ad).

### `multiboot2.c`

This C program is responsible for processing boot information from a Multiboot2-compliant bootloader (the Multiboot2 `mbi` from GRUB in our case).
It meticulously manages memory regions, integrates boot parameters, and prepares the system for kernel execution by consolidating memory configurations and allocating essential resources.

Alongside these core files, I made adjustments to adjacent files to seamlessly link everything together and ensure successful booting under Multiboot2 (e.g. duplicating the `mkukimg` script's menuentry to use `multiboot2` instead of `multiboot` etc.).
Additionally, I updated the `Linker.uk` script to replace uses of the `ELF64_TO_32` option with checks for Multiboot, ensuring smooth and similar processing/maintanence for both protocols.

## Blog Posts

For more details of the process and the challenges faced, I documented my progress in a series of blog posts which can be found here:

* [Blog post #1](https://github.com/unikraft/docs/pull/428)

* [Blog post #2](https://github.com/unikraft/docs/pull/439)

* [Blog post #3](https://github.com/unikraft/docs/pull/447)

* [Blog post #4](https://github.com/unikraft/docs/pull/453)

## Current Status

To test and validate the Multiboot2 support, I used the [`helloworld`](https://github.com/unikraft/catalog/tree/main/native/helloworld-c) and [`nginx`](https://github.com/unikraft/catalog/tree/main/library/nginx) applications to check if the tags are correctly parsed and have proper functionality.
Unfortunately, QEMU doesn't natively support Multiboot2.
Consequently, options such as `-kernel` and `-append` used in Unikraft's usual running script are not applicable.
Instead, I tested on the `.iso` file generated by [`mkukimg`](https://github.com/unikraft/unikraft/blob/staging/support/scripts/mkukimg) while passing the command line arguments through the script's `-c` option.
For a clearer understanding, you can check this sample [testing script](https://gist.github.com/mariapana/90ab2e61715e812def0c7582c9482626).

With Multiboot2 support being successfully integrated into Unikraft, the next step is to review and merge the pull request.

## Future Work

One of the greatest advantages of Multiboot2 is its flexibility and extensibility through tags.
Therefore, I want to explore the possibility of adding more tags to the implementation, such as the Relocation Tag.
Also on the TODO list is to extend the support to UEFI systems, besides the current GRUB support.

---
title: Google Summer of Code 2024 Ideas List
---

## Unikraft Project Ideas

Thank you for your interest in participating in [Google Summer of Code 2024 (GSoC24)](https://summerofcode.withgoogle.com/programs/2024)!

Unikernels are a novel Operating System (OS) model providing unprecedented optimization for software services.
The technology offers a clean slate OS design which improves the efficiency of cloud services, IoT and embedded system applications by removing unnecessary software layers by specializing the OS image.
One unikernel framework which provides minimal runtime footprint and fast (millisecond-level) boot times is Unikraft, and aims as a means to reduce operating costs for all services that utilize it as a runtime mechanism.

Unikraft is a Unikernel Development Kit and consists of an extensive build system in addition to core and external library ecosystem which facilitate the underlying functionality of a unikernel.

## Mentors of the projects

Mentors will be assigned when the project is initiated.  Please feel free to reach out beforehand to discuss the project.

| Mentor | Email |
|--------|-------|
| [Razvan Deaconescu](https://github.com/razvand) | razvan.deaconescu@upb.ro |
| [Alexander Jung](https://github.com/nderjung) | alex@unikraft.io |
| [Simon Kuenzer](https://github.com/skuenzer) | simon@unikraft.io |
| [Cezar Crăciunoiu](https://github.com/craciunoiuc) | cezar@unikraft.io |
| [Michalis Pappas](https://github.com/michpappas) | michalis@unikraft.io |
| [Ștefan Jumărea](https://github.com/StefanJum) | stefanjumarea02@gmail.com |
| [Maria Sfîrăială](https://github.com/mariasfiraiala) | maria.sfiraiala@gmail.com |
| [Răzvan Vîrtan](https://github.com/razvanvirtan) | virtanrazvan@gmail.com |
| [Radu Nichita](https://github.com/RaduNichita) | radunichita99@gmail.com |
| [Sergiu Moga](https://github.com/mogasergiu) | sergiu.moga@protonmail.com |

Below are a list of open projects for Unikraft which can be developed as part of GSoC24.

Also check the [Unikraft OSS Roadmap](https://hackmd.io/@unikraft/HyNdSyAki).

---

### Expanding the Unikraft Software Support Ecosystem

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 2 |
| **Constraints/requirements** | Basic OS concepts, familiarity with POSIX and system calls, build systems and tool stacks. |

#### Description

One of the weak points of most unikernel projects has always been application support, often requiring that applications be ported to the target unikernel OS.
With Unikraft we have been making a strong push towards POSIX compatibility so that applications can run unmodified on Unikraft.
We have been doing this in two different ways:

1. by adding support for the Musl libc library such that applications can be compiled against it, using their native build systems, and then linked into Unikraft
1. through [binary-compatibility mode](), where unmodified ELFs are directly executed in Unikraft and the resulting syscalls trapped and redirected to the Unikraft core, via the [`app-elfloader`](https://github.com/unikraft/app-elfloader).

This has lead to the creation of the [application `catalog` repository](https://github.com/unikraft/catalog) where running applications and examples are brought together.

This project focuses on expanding Unikraft's software support ecosystem by [adding new applications](https://unikraft.org/docs/contributing/adding-to-the-app-catalog) to the [application `catalog` repository](https://github.com/unikraft/catalog), primarily in binary-compatibility mode.
While doing this, you will also:

1. implement and extend system calls
1. add extensive testing for the application or framework that is to be included in the catalog
1. add benchmarking scripts to measure the performance and resource consumption of the application running with Unikraft
1. conduct synthetic tests using tools such as [the Linux Test Project](https://linux-test-project.github.io/)

The success of this project will directly impact Unikraft adoption.
The project length can be varied depending on which of these items are covered by the project.

#### Reading & Related Material

* https://www.musl-libc.org/
* https://unikraft.org/guides/using-the-app-catalog
* https://github.com/unikraft/catalog
* https://unikraft.org/docs/contributing/adding-to-the-app-catalog

---

### Software Quality Assurance of Unikraft Codebase

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 2 |
| **Constraints/requirements** | C programming skills, Linux command-line experience, build tools |

#### Description

During its 6 years of existence, Unikraft, now at version 0.16.1, has grown in features, application support and codebase.
As it matures, a high quality of the code and robust behavior are a must to provide a stable solution for its user base.

The aim of this project is to assist in the software quality assurance of the Unikraft codebase, by tackling one of the following ideas:

1. The use of the [`uktest` framework](https://github.com/unikraft/unikraft/tree/staging/lib/uktest) to create unit tests for [internal libraries](https://github.com/unikraft/unikraft/tree/staging/lib/) and [external libraries](https://github.com/search?q=topic%3Alibrary+org%3Aunikraft+fork%3Atrue&type=repositories).
   Not many libraries have unit tests, those that do are mostly exceptions.
   This will directly impact the stability of the code base and allow quick validation of new features that may break existing functionality.

1. Inclusion of static and dynamic analysis tools that highlight potential spots of faulty or undefined behavior.

1. The use of compiler builtins and compiler flags providing constraints on the code to increase its resilience to faulty behavior.

1. Augmenting the CI/CD system used by Unikraft (based on [GitHub Actions](https://github.com/features/actions)) to feature automatic testing, validation and vetting of contributions to Unikraft repositories: core, libraries, applications.
   Potential items are:
   1. handling running of unikernels instead of simple builds
   1. static analysis of images to be delivered as reports to GitHub pull requests
   1. regression checks on performance (delivered as % change from the current upstream version)

Any other project that is targeted towards increasing the robustness of Unikraft source code is welcome.
These will both increase the viability of Unikraft as a stable solution and increase the quality of future contributions, by enforcing good practices on submitted code.

#### Reading & Related Material

* [Writing Tests in Unikraft](https://unikraft.org/docs/develop/writing-tests/)
* https://www.guru99.com/unit-testing-guide.html
* https://docs.kernel.org/dev-tools/kunit/index.html
* https://github.com/features/actions
* https://unikraft.org/docs/contributing/review-process/

### Supporting User-provided, Long-lived Environmental Variables for Unikraft Builds

| | |
|-|-|
| **Difficulty** | 2/5 |
| **Project Size** | Small (90 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Good Go skills, familiarity with build tools, good OS knowledge |

#### Description

Unikraft is a highly modular library operating system designed for the cloud.
Its high degree of modularization allows for extreme customization and specialization.
As such, its tooling should not interfere with the user's desire to support such customization.
Towards increasing the unikernel's developer's ability to customize the build whilst simultaneously systemitizing the process of retrieving, organizing and generally facilitating the build of a unikernel based on Unikraft and its many components, the supported tooling, [kraft](https://github.com/unikraft/kraftkit), should allow for the injection of the user's environment and or additional toolchain requirements.

This project is designed to better facilitate the dynamic injection of user provided variables into Unikraft's build system through the addition of a dynamically configured toolchain towards greater customization of the unikernel build through the use of its command-line companion client tool, `kraft`.
This manifests itself as an injection into KraftKit's core configuration system and must propagate across the codebase appropriately.

Distinct results of this addition would enable, but are not limited to: alternating the GNU Compiler Collection (GCC) version, providing alternative compile-time flags, and more.

#### Reading & Related Material

* https://github.com/unikraft/kraftkit/issues/673

### Supporting macOS networking (medium-large, 175-350hrs)

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Good Go skills, familiarity with virtualization, macOS and networking, good OS knowledge |

#### Description

[KraftKit](https://github.com/unikraft/kraftkit), the supporting codebase for the modular library operating system Unikraft designed for cloud native applications, provides users with the ability to build, package and run unikernels.
As a swiss-army-knife of unikernel development, it eases both the construction and deployment of unikernels.
To this end, supporting diverse user environments and their ability to run unikernels locally supports the ultimate goal of the project.  One such environment which requires more attention is macOS.

Towards better facilitating the execution of unikernel virtual machine images on macOS, this project aims to introduce new packages which interface directly with macOS environments by interfacing natively with the local networking environment such that the execution of unikernels is accessible through a more direct communication mechanisms of the host.
Until now, the project only supports Linux bridge networking with accommodation (albeit "stubs") in the codebase for Darwin.

#### Reading & Related Material

* https://github.com/unikraft/kraftkit/issues/841

---

### Unikraft as **the** Secure Configurable Unikernel

| | |
|-|-|
| **Difficulty** | 4/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 2 |
| **Constraints/requirements** | Good C skills, familiarity with security and hardening features, basic OS knowledge. Basic assembly (x86 / ARM) knowledge is a plus. |

#### Description

Unikraft has matured to a featureful unikernel.
Key to its adoption in real world deployments is an extensive set of security features.

In order to achieve this, Unikraft counts on a combination of unikernels' inherent security properties (minimal attack surface, strong cross-application isolation, safe languages), generic security features matching Linux (ASLR, stack protection, Write-xor-Execute, etc.), and state-of-the-art, fine-grained intra-unikernel compartmentalization.
These are listed in its [Security Benefits](https://unikraft.org/docs/features/security/) page.

Apart from existing ones, Unikraft must employ all other relevant security features out there.
Top priority planned or ongoing security features include ASLR, shadow stacks and `FORTIFY_SOURCE`.
Other targets include Intel Control-flow Enforcement Technology (CET), or ARM Speculation Barrier (SB).

Another idea is the use of static or dynamic analysis techniques for software penetration testing of the Unikraft code base and for its security assessment.

The goal is to make Unikraft **the** secure configurable unikernel of choice for researchers, professionals and commercial use cases.
By combining its inherent design for specialization with an excellent set of security features, Unikraft will be an easy choice for anyone looking for a fast, secure and efficient unikernel solution.

#### Reading & Related Material

* https://unikraft.org/docs/features/security/
* https://dl.acm.org/doi/abs/10.1145/3503222.3507759
* https://github.com/a13xp0p0v/linux-kernel-defence-map

---

### Synchronization Support in Internal Libraries

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, familiarity with typical OS synchronization primitives, understanding of hardware (caching, reordering, atomic instructions). |

#### Description

As part of a [GSoC'22 project](https://github.com/unikraft/unikraft/pull/476) Unikraft now features new SMP-safe synchronization primitives such as mutexes, semaphores, reader-writer locks, and condition variables.
This will enable Unikraft to protect critical sections and lay the foundation for full SMP support of core Unkraft components.

These primitives have not yet been integrated in the Unikraft core libraries.
These libraries need to be made SMP / thread-safe, by redesigning their data structures and making appropriate calls to synchronization primitives.
The goal is to integrate synchronization primitives to ensure SMP safety while preserving a good level of performance.

While locking is effective it is not always the most efficient solution.
Sometimes, for example, when manipulating data structures it can be faster to operate on the data structures directly using atomic instructions.
These lockless data structures can benefit SMP performance.
One potential item is therefore to extend Unikraft's existing data structures with lockless variants and research alternatives to simple locking such as read-copy-update (RCU).

#### Reading & Related Material

* [Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/) (Chapter on Concurrency)
* https://dl.acm.org/doi/abs/10.1145/2517349.2522714
* https://wiki.osdev.org/Synchronization_Primitives
* https://www.kernel.org/doc/html/latest/RCU/index.html

---

### Multiboot2 Support in Unikraft

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, familiarity with OS topics, knowledge of booting process |

#### Description

Unikraft is a unikernel capable to boot in various environments and through quite a few boot protocols.
From supporting the first version of Multiboot, being able to boot through the Linux Boot Protocol on ARM64 and having its own UEFI stub enabling the Operating System to load without a Bootloader, Unikraft has still a lot to learn with respect to the booting environments it knows.

One such booting protocol that would be very useful is Multiboot2.
This protocol represents a significant improvement over Multiboot version 1, providing improved flexibility in the realm of bootloaders.
A notable improvement is the extended information provided by Multiboot2, which provides detailed and accurate communication between the bootloader and the operating system kernel during the bootstrapping process.
In addition, Multiboot2 comes with a standardized tag system, enabling easier data and system configuration parsing between the bootloader and the kernel and allowing greater interoperability and better integration of boot modules.
The introduction of optional commandline further enhances the flexibility of the boot process.
Together, these enhancements contribute to a more robust and scalable bootstrapping mechanism, making Multiboot2 a better choice than Multiboot version 1.

Concretely speaking, it would allow us to boot directly in 64-bit mode, while having access to pointers to the UEFI tables as well as the ACPI tables, unlike Multiboot 1, offering us access to more modern firmware interfaces.
The task at hand involves integrating the Multiboot2 definitions as well as its corresponding bootstrapping code into Unikraft.
Auxiliary scripts may be implemented to allow the automatic creation of Multiboot2-based Unikraft bootable images.

#### Reading & Related Material

* https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
* https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html

---

### UEFI Graphics Output Protocol Support in Unikraft

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, familiarity with OS topics, knowledge of booting process |

#### Description

When it comes to modern baremetal environments as well as second generation Virtual Machines, Unikraft's ability to print debugging logs to the screen is very limited. This can represent an unnecessary blocker for technical tracks such as Hyper-V, VMWare support or just being able to debug Unikraft on UEFI-based baremetal machines. While we may be able to debug in such situations using the serial port console, it is not as easily available for the usual consumer machines, leaving us with the main option that is used in the modern times: the widely available and standardized UEFI Graphics Output Protocol (GOP) interface.

UEFI GOP remains a staple in today's computer systems, providing a customizable and versatile interface for viewing images generated during and after the boot process as well as being able to read and write raw pixels from and to the physical display. Its usefulness to the operating system is paramount in UEFI-based systems as it allows drawing fonts directly to the screen, thus enabling the printing of logs. Unlike the old, nowadays not so much available, VGA standard, its replacement, UEFI GOP, supports multiple rendering resolutions and color depths, ensuring close compatibility with hardware systems. This protocol enables operating systems to successfully boot and quickly manage graphics hardware during the boot process for a smoother and more accurate visual user experience.

Supporting this modern interface would directly allow Unikraft to be easier to be debugged on UEFI-based systems and, furthermore, allow us to open a complex track, namely the one for increasing support on x86_64 baremetal machines. The proposed work involves writing a font parser and writing a UEFI GOP driver that would receive the necessary framebuffer information from the bootloader and implement the necessary functionality to interact with the display in order to print logs under the form of characters drawn through raw pixels on the screen.

#### Reading & Related Material

* https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#graphics-output-protocol

---

### Linux x86 Boot Protocol Support (medium-large, 175-350hrs)

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, familiarity with OS topics, knowledge of booting process |

#### Description

While Unikraft may have the bare minimum implementation required to receive and process the information passed from Firecracker under the form of the Linux Boot Protocol structures, it is not able to boot through this protocol from more strict and proper environments such as QEMU, GRUB, U-Boot or any of the many open source bootloaders that speak this protocol.
The main reason this does not happen is because the Unikraft bootable images are missing the Linux header.
This is a very large header with many fields, highly configurable and yet fairly difficult to debug since it involves a large amount of binary interfacing, proving to be a valuable debugging experience.

The Linux boot protocol plays an important role in the initialization of the Linux operating system, emphasizing the importance of system optimization and scalability.
This system ensures a seamless transfer of control from the bootloader to the Linux kernel by providing essential information including memory map, command line parameters, and device specifications.
By adhering to a uniform boot protocol, Linux achieves a level of versatility across the hardware spectrum of architectures and bootloader functions.
Essentially, this protocol is a commitment to a standardized approach that is not only stable but also facilitates bootloader development, improving compatibility and reliability.
In particular, the Linux boot protocol greatly contributes to the overall robustness and reliability of the bootstrapping process on Linux-based systems.
Almost, if not all, widely available bootloaders that exist on the market are able to speak this protocol and Unikraft, being unable to be detected by these bootloaders as a Linux Boot Protocol compliant system, is hindering the ability of Unikraft to boot on a very large amount of platforms.

To be able to achieve such booting flexibility, the student will be required to write an external program that will be part of the build system and will be used to parse the usual Unikraft configurable metadata and convert it into its Linux Boot Protocol equivalents, the resulted data then is to be prepended to the bootable image as a header.
Following this procedure, the bootable image media creation script we have begun writing will need to be adapted to create such a booting image and then tested on various platforms and bootloaders so as to catch potential underlying quirks of the different loaders.

#### Reading & Related Material

* https://www.kernel.org/doc/html/v5.6/x86/boot.html

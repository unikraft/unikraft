---
title: Google Summer of Code 2023 Ideas List
---

## Unikraft Project Ideas

Thank you for your interest in participating in [Google Summer of Code 2023 (GSoC23)](https://summerofcode.withgoogle.com/programs/2023)!

Unikernels are a novel Operating System (OS) model providing unprecedented optimization for software services.
The technology offers a clean slate OS design which improves the efficiency of cloud services, IoT and embedded system applications by removing unnecessary software layers by specializing the OS image.
One unikernel framework which provides minimal runtime footprint and fast (millisecond-level) boot times is Unikraft, and aims as a means to reduce operating costs for all services that utilize it as a runtime mechanism.

Unikraft is a Unikernel Development Kit and consists of an extensive build system in addition to core and external library ecosystem which facilitate the underlying functionality of a unikernel.

## Mentors of the projects

Mentors will be assigned when the project is initiated.  Please feel free to reach out beforehand to discuss the project.

| Mentor | Email |
|--------|-------|
| [Razvan Deaconescu](https://github.com/razvand) | razvan.deaconescu@upb.ro |
| [Marc Rittinghaus](https://github.com/marcrittinghaus) | marc@unikraft.io |
| [Alexander Jung](https://github.com/nderjung) | alex@unikraft.io |
| [Simon Kuenzer](https://github.com/skuenzer) | simon@unikraft.io |
| [Cezar Crăciunoiu](https://github.com/craciunoiuc) | cezar@unikraft.io |
| [Michalis Pappas](https://github.com/michpappas) | michalis@unikraft.io |
| [Andra Paraschiv](https://github.com/andraprs) | andra@unikraft.io |
| [Ștefan Jumărea](https://github.com/StefanJum) | stefanjumarea02@gmail.com |
| [Maria Sfîrăială](https://github.com/mariasfiraiala) | maria.sfiraiala@gmail.com |
| [Răzvan Vîrtan](https://github.com/razvanvirtan) | virtanrazvan@gmail.com |
| [Radu Nichita](https://github.com/RaduNichita) | radunichita99@gmail.com |
| [Sergiu Moga](https://github.com/mogasergiu) | sergiu.moga@protonmail.com |

Below are a list of open projects for Unikraft which can be developed as part of GSoC23.

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
1. through binary compatibility mode, where unmodified ELFs are directly executed in Unikraft and the resulting syscalls trapped and redirected to the Unikraft core, via the [`app-elfloader`](https://github.com/unikraft/app-elfloader).

This project focuses on expanding Unikraft's software support ecosystem by:

1. looking at the state of Musl integration and analyzing to what extent unmodified application can be successfully linked and run with Unikraft
1. conducting an equivalent analysis for binary compatibility mode
1. implementation or extension of syscalls

The success of this project will directly impact Unikraft adoption.
The project length can be varied depending on which of these items are covered by the project.

#### Reading & Related Material

* https://www.musl-libc.org/
* https://unikraft.org/docs/features/posix-compatibility/

---

### Software Quality Assurance of Unikraft Codebase

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 2 |
| **Constraints/requirements** | C programming skills, Linux command-line experience, build tools |

#### Description

During its 5 years of existence, Unikraft, now at version 0.12, has grown in features, application support and codebase.
As it matures, a high quality of the code and robust behavior are a must to provide a stable solution for its user base.

The aim of this project is to assist in the software quality assurance of the Unikraft codebase, by tackling one of the following ideas:

1. The use of the [`uktest` framework](https://github.com/unikraft/unikraft/tree/staging/lib/uktest) to create unit tests for [internal libraries](https://github.com/unikraft/unikraft/tree/staging/lib/) and [external libraries](https://github.com/search?q=topic%3Alibrary+org%3Aunikraft+fork%3Atrue&type=repositories).
   Not many libraries have unit tests, those that do are mostly exceptions.
   This will directly impact the stability of the code base and allow quick validation of new features that may break existing functionality.

1. Inclusion of static and dynamic analysis tools that highlight potential spots of faulty or undefined behavior.

1. The use of compiler builtins and compiler flags providing constraints on the code to increase its resilience to faulty behavior.

1. Augmenting the [CI/CD system used by Unikraft](builds.unikraft.io/) (based on [Concourse](https://concourse-ci.org/)) to feature automatic testing, validation and vetting of contributions to Unikraft repositories: core, libraries, applications.
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
* https://concourse-ci.org/
* https://builds.unikraft.io/
* https://unikraft.org/docs/contributing/review-process/
* https://fosdem.org/2022/schedule/event/massive_unikernel_matrices_with_unikraft_concourse_and_more/

---

### Build-once, Run Anything: Leveraging Unikraft to Run Any Linux Application

| | |
|-|-|
| **Difficulty** | 2/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Go experience, Linux command-line experience, build tools |

#### Description

Towards easing the use of Unikraft and leveling-up the user experience of Unikraft, this project aims to enhance [Unikraft's CLI companion tool KraftKit](https://kraftkit.sh) to use unikernels via the use of the [Unikraft ELF loader](https://github.com/unikraft/app-elfloader).

Kraftkit is the main entry point into the Unikraft world and has been written from scratch in Go and released in December 2022 to replace the now-deprecated version built in Python (known as [`pykraft`](https://github.com/unikraft/pykraft)).
This re-implementation has already eased many previous pains, including enabling faster integration into the wider Cloud Native ecosystem, which Unikraft is targeted to the ELF loader
unikernel.

The approach will leverage the [ELF loader unikernel](https://github.com/unikraft/app-elfloader) which is based on the idea of binary and POSIX-compatibility which runs unmodified Linux applications atop a unikernel.
The approach is made possible by loading the Linux application as initramfs.

In this project you will take KraftKit and extend it to build and leverage unikernels based on the ELF loader application and to provide a tight integration with the ELF loader.
The goal is to allow the `kraft` command-line tool to quickly access pre-built ELF loader unikernels so that users do not need to build a new unikernel for any provided Linux application and to to significantly simplify the user experience, making unikernels extremely easy to use, and thus boost bottoms-up adoption.

#### Reading & Related Material

* https://github.com/unikraft/kraftkit
* https://github.com/unikraft/app-elfloader

---

### Enhancing the VSCode Developer Experience

| | |
|-|-|
| **Difficulty** | 2/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | TypeScript & Go experience, Linux command-line experience, build tools |

#### Description

The [VSCode Extension for Unikraft](https://github.com/unikraft/ide-vscode) enables developers to quickly and painlessly build unikernels from the VSCode IDE.
Amongst other features, it allows developers to list and find Unikernel libraries as well as run basic commands via [`pykraft`](https://github.com/unikraft/pykraft).

In this project, you will upgrade the VSCode extension to use [KraftKit](https://kraftkit.sh), the newly released CLI companion tool for Unikraft rewritten in Go.
The project requires modifications to the project's main binary, `kraft`, to enable JSON output of various commands so that the integration with VSCode can be done through a machine-readable interface.

Additionally, the project allows for enhancing the experience, including adding support for additional steps in Unikraft's build cycle;
packaging unikernels in different formats; providing a linting mechanism so that projects which are developed for Unikraft can be checked for compatibility.

#### Reading & Related Material

* https://github.com/unikraft/ide-vscode
* https://github.com/unikraft/kraftkit

---

### Packaging Pre-built Micro-libraries for Faster and More Secure Builds

| | |
|-|-|
| **Difficulty** | 2/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Go experience, Linux command-line experience, build tools |

#### Description

Building a Unikraft unikernel is made possible by it's highly modular architecture which enables the use of third-party libraries which offer kernel-level and user-level interfaces for an application.
To make accessing these libraries easier, the companion CLI tool [KraftKit](https://kraftkit.sh) helps managing these libraries, as well as the Unikraft core, their versions and metadata information.

In this project, you will extend KraftKit to provide additional mechanisms to enable access to pre-built libraries to enable faster builds.
Currently, libraries are accessed from source, either from a remote tarball or Git repository.
To speed up builds, pre-built libraries can be made and delivered before the main build process to speed up the overall build of the unikernel.

#### Reading & Related Material

* https://github.com/unikraft/kraftkit

---

### re:Arch Unikraft

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, good software engineering skills |

#### Description

Unikraft provides specialization and strong modularity by strictly following the "everything is a library" concept.
Support for hypervisors, bare metal nodes, and drivers is broken down into individual libraries and only those libraries necessary to run an application on a given target environment (e.g., hypervisor platform) are selected.

Over time, we noticed that certain code organizations for drivers and platform support are semi-optimal and not clearly defined, creating uncertainty for contributors and maintenance headaches.
In this project you get the opportunity to deeply engage in a restructuring process, understanding the low-level components of the Unikraft's code base, and contribute to the new the code structure guideline.
You will interact directly with core maintainers and work with them on re-structuring and optimizing the code base. In doing so, you will enable much more streamlined upstreaming of additional platforms (e.g., Hyper-V, VMware), CPU architectures (e.g., RISC-V) and drivers.

#### Reading & Related Material

* https://unikraft.org/docs/concepts/architecture/
* https://unikraft.org/docs/concepts/build-process/

---

### Embedded Devices & Driver Shim Layer

| | |
|-|-|
| **Difficulty** | 4/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, OS/driver concepts, familiarity with ARM64 architecture. Experience with ARM64 boards is a plus. |

#### Description

Thanks to its modularity, efficiency and support for ARM64, Unikraft is well suited for running on embedded/small devices (e.g., Raspberry Pis, Pine64s, etc.). We have done preliminary work to have Unikraft boot on some of these devices, but more work is needed for it to be able to run mainstream applications, frameworks and cool demos.
The main part of the work would consist of building a driver shim layer that would allow Unikraft to transparently re-use unmodified drivers from an existing project (e.g., FreeRTOS) in order to support basic functionality such as networking and sensors.
Time permitting, the project could be extended to include support for other devices as well, and to measure the efficiency of running Unikraft on them versus using mainstream OSes (e.g., Linux).

#### Reading & Related Material

* https://ieeexplore.ieee.org/document/9244044
* https://www.freertos.org/

---

### Driver Probing Framework

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, OS/driver concepts, device tree. |

#### Description

Unlike devices connected to enumeration-capable busses like PCI and USB, non-discoverable devices require that the operating system is manually provided with information on their presence, configuration, and their relation to other devices. Some commonly used methods to provide this information are ACPI, FDT (device-tree), or even the kernel's command line.

This task involves introducing a generic driver probing framework to Unikraft.
At a minimum, the framework must provide a way for drivers to associate themselves to compatible devices, and have these drivers probe devices based on the information provided by the underlying framework (eg FDT).
More complex tasks involve handling device dependencies, deferred probing, and device prioritization. Common cases for such dependencies are clocks and crypto devices.

The main focus will be on the device-tree, with additional options being available depending on the progress.

#### Reading & Related Material

* https://www.devicetree.org/specifications/
* https://lwn.net/Articles/662820/
* https://lwn.net/Articles/450460/

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

Apart from security features, Unikraft could benefit from VM-based isolation features such as ARM Confidential Compute Architecture (CCA), Intel Trust Domain Extensions (TDE) or AMD Secure Encrypted Virtualization (SEV).

Another idea is the use of static or dynamic analysis techniques for software penetration testing of the Unikraft code base and for its security assessment.

The goal is to make Unikraft **the** secure configurable unikernel of choice for researchers, professionals and commercial use cases.
By combining its inherent design for specialization with an excellent set of security features, Unikraft will be an easy choice for anyone looking for a fast, secure and efficient unikernel solution.

#### Reading & Related Material

* https://unikraft.org/docs/features/security/
* https://dl.acm.org/doi/abs/10.1145/3503222.3507759
* https://github.com/a13xp0p0v/linux-kernel-defence-map

---

### MSI/MSI-X Interrupt Driver Support (x86, KVM)

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, OS/driver concepts, familiarity with x86 architecture. Experience with interrupt controllers, the PCI standard, and virtio is a plus. |

#### Description

Unikraft's interrupt system on KVM/x86 is based on the legacy 8259 programmable interrupt controller (PIC) which is limited to 15 interrupt lines and programmed using slow port I/O.
Because the interrupt vector space is so small, when having many devices, interrupt lines need to be shared.
The operating system thus has to query multiple drivers to figure out which one is responsible for handling an interrupt on a shared line and which device caused the interruption.
This entails reading many device registers, leading to high interrupt latency.
However, in order to achieve high I/O performance, one key is to keep the software overhead of interrupt handlers minimal.
Modern devices (including virtio) reduce software processing time further by setting up multiple interrupt lines, like one for each I/O queue they provide.
With having just 15 interrupt lines, line sharing happens even with only a few devices attached.
Advanced programmable interrupt controllers (APICs) utilizing the MSI and MSI-X PCI standards overcome this problem by providing a much larger interrupt vector space (currently up to 2048).
Interrupt line sharing is thus not required anymore.

In this project, you will:

1. develop a driver for interfacing a modern x86 I/O APIC and extend Unikraft's libpci to support MSI/MSI-X
2. enable the existing virtio drivers to make use of these technologies

#### Reading & Related Material

* [Intel 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
* https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html
* https://wiki.osdev.org/Interrupts
* https://wiki.osdev.org/APIC
* https://wiki.osdev.org/PCI#Message_Signaled_Interrupts
* https://habr.com/en/post/501912/

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

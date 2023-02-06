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
| [Michalis Pappas](https://github.com/michpappas) | mpappas@fastmail.fm |
| [Andra Paraschiv](https://github.com/andraprs) | andra@unikraft.io |
| [Ștefan Jumărea](https://github.com/StefanJum) | stefanjumarea02@gmail.com |
| [Maria Sfîrăială](https://github.com/mariasfiraiala) | maria.sfiraiala@gmail.com |
| [Răzvan Vîrtan](https://github.com/razvanvirtan) | virtanrazvan@gmail.com |
| [Radu Nichita](https://github.com/RaduNichita) | radunichita99@gmail.com |
| [Sergiu Moga](https://github.com/mogasergiu) | sergiu.moga@protonmail.com |

Below are a list of open projects for Unikraft which can be developed as part of GSoC23.

Also check the [Unikraft OSS Roadmap](https://hackmd.io/@unikraft/HyNdSyAki).

---

### Lightweight and Scalable VMMs

| | |
|-|-|
| **Difficulty** | 4/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Programming skills in C. Familiarity with virtio drivers, virtualization, and basic networking concepts is a plus. |

#### Description

VMMs (Virtual Machine Monitors) are software components that interact with the hypervisor to manage the virtual machines running on a host (e.g., QEMU, Firecracker, etc.).
While Unikraft is able to build extremely efficient virtual machines that can boot in a few milliseconds and suspend/resume in the 10s of milliseconds, achieving such numbers partly depends on the efficiency of the VMM, and especially how it scales with an increasing number of Unikraft instances (think thousands on the same server!).

The work entails adding support to Unikraft for modern, scalable VMMs such as and Intel Cloud Hypervisor, including the ability to boot Unikraft, suspend/resume its instances and provide networking support when using those VMMs.
In addition, the project would require measuring the performance of such VMMs when running an increasing number of Unikraft instances (e.g., what are their boot times, suspend/resume times, throughput, VMM memory consumption, etc.).
The length of the project (175 vs. 350 hours) can be negotiated based on the number of VMMs supported and which drivers.

#### Reading & Related Material

* https://dl.acm.org/doi/10.1145/3447786.3456248 (in particular Figure 10)
* https://github.com/cloud-hypervisor/cloud-hypervisor/

---

### POSIX & Application Support

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Basic OS concepts, familiarity with POSIX and system calls, build systems and tool stacks. |

#### Description

One of the weak points of most unikernel projects has always been application support, often requiring that applications be ported to the target unikernel OS.
With Unikraft we have been making a strong push towards POSIX compatibility so that applications can run unmodified on Unikraft.
We have been doing this in two different ways: (1) by adding support for the Musl libc library such that applications can be compiled against it, using their native build systems, and then linked into Unikraft and (2) through binary compatibility mode, where unmodified ELFs are directly executed in Unikraft and the resulting syscalls trapped and redirected to the Unikraft core.

This project may consist of, but is not limited to, any of the following items: improving Unikraft’s application support by (1) looking at the state of Musl integration and analyzing to what extent unmodified application can be successfully linked and run with Unikraft; (2) conducting an equivalent analysis for binary compatibility mode and/or (3) implementation or extension of syscalls.
The success of this project would provide an enormous boost, we think, to Unikraft adoption.
The project length can be varied depending on which of these items are covered by the project.

#### Reading & Related Material

* https://www.musl-libc.org/
* https://unikraft.org/docs/features/posix-compatibility/

---

### Unikraft & DevOps

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Strong Linux command-line and tooling experience, Golang, Makefile & C programming skills. |

#### Description

With Unikraft, the core pillars of its development are found from its inherent performance, security benefits as well as its integrity.
These properties can be facilitated with CI/CD systems which are designed for modern systems and provide the ability to run checks, tests, and audits for systems before they are deployed.
Thanks to its modular architecture and multi-platform capabilities, running tests on Unikraft results in many permutations of the same application – which implies a large and complex CI/CD system.

The current CI/CD system, based on Concourse, tests many applications against many target arch/plat combinations but only facilitates the construction of final unikernel images, while neglecting the execution/runtime part.
This project follows additional implementation and improvements to this system, for example: handling running of unikernels instead of simple builds; static analysis of images to be delivered as reports to GitHub pull requests; regression checks on performance (delivered as % change from the current upstream version); building new tools and instrumentation for presenting, easing, accessing and monitoring the results of the CI/CD for unikernel images (e.g. delivering performance graphs or tables of tests results); code coverage; etc.

The success of this project would provide a significant boost, we think, to Unikraft adoption.
The project length can be varied depending on which of these items are covered by the project.

#### Reading & Related Material

* https://concourse-ci.org/
* https://builds.unikraft.io/
* https://unikraft.org/docs/contributing/review-process/
* https://fosdem.org/2022/schedule/event/massive_unikernel_matrices_with_unikraft_concourse_and_more/

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

### Security Features

| | |
|-|-|
| **Difficulty** | 4/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Good C skills, familiarity with security and hardening features, basic OS knowledge. Basic x86 assembly knowledge is a plus. |

#### Description

Unikraft should provide the highest level of security to its users.
In order to achieve this, the Unikraft project counts on a combination of unikernels' inherent security properties (minimal attack surface, strong cross-application isolation, safe languages), generic security features matching Linux (ASLR, stack protection, Write-xor-Execute, etc.), and state-of-the-art, fine-grained intra-unikernel compartmentalization.

As of Unikraft v0.8.0 (Enceladus), the Unikraft main tree features support for a limited range of security features present in Linux such as Stack Smashing Protection (SP), or Undefined Behavior Sanitization (UBSAN).
Top priority planned or ongoing security features include ASLR, shadow stacks and `FORTIFY_SOURCE`.
Other targets include Intel Control-flow Enforcement Technology (CET), or ARM Speculation Barrier (SB).

In this security-focused project, you will have the opportunity to contribute towards the implementation and testing efforts of planned security features.
In parallel to this, Unikraft features out-of-tree support for intra-unikernel isolation with Intel MPK and EPT.

Another possible idea is the use of static or dynamic analysis techniques for software penetration testing of the Unikraft code base and for its security assessment.

Project length variable depending on selected features.

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

In this project, you will (1) develop a driver for interfacing a modern x86 I/O APIC and extend Unikraft’s libpci to support MSI/MSI-X, and (2) enable the existing virtio drivers to make use of these technologies.

#### Reading & Related Material

* [Intel 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
* https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html
* https://wiki.osdev.org/Interrupts
* https://wiki.osdev.org/APIC
* https://wiki.osdev.org/PCI#Message_Signaled_Interrupts
* https://habr.com/en/post/501912/

---

### Unit Tests in Internal and External Libraries via `uktest`

| | |
|-|-|
| **Difficulty** | 2/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, experience with unit testing is a plus. |

#### Description

Since release 0.6, Unikraft provides the [`uktest` framework](https://github.com/unikraft/unikraft/tree/staging/lib/uktest) as an internal library to create unit tests for core and external libraries.
`uktest` includes the definition of a test (or suite of tests) and the mechanism by which one can use to register the suite to be run.

Not many libraries have unit tests, those that do are mostly exceptions.

The goal of the project is to add unit test via the `uktest` framework in as many [internal libraries](https://github.com/unikraft/unikraft/tree/staging/lib/) and [external libraries](https://github.com/search?q=topic%3Alibrary+org%3Aunikraft+fork%3Atrue&type=repositories) as possible.
This will directly impact the stability of the code base and allow quick validation of new features that may break existing functionality.
Moreover, unit tests will be fed in the Unikraft CI/CD system.

For internal libraries, tests are to be designed from zero, by analyzing the library API.
For external libraries, the typical approach is to port existing unit tests to the `uktest` framework;
this is because, in general, external ported libraries have a predefined set of unit tests.

#### Reading & Related Material

* [Writing Tests in Unikraft](https://unikraft.org/docs/develop/writing-tests/)
* https://www.guru99.com/unit-testing-guide.html
* https://docs.kernel.org/dev-tools/kunit/index.html

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

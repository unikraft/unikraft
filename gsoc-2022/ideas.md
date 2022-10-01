---
title: Google Summer of Code 2022 Ideas List
---

<img width="100px" src="https://summerofcode.withgoogle.com/assets/media/gsoc-2022-badge.svg" align="right" />

## Unikraft Project Ideas

Thank you for your interest in participating in [Google Summer of Code 2022 (GSoC22)](https://summerofcode.withgoogle.com/programs/2022)!


Unikernels are a novel Operating System (OS) model providing unprecedented optimization for software services.
The technology offers a clean slate OS design which improves the efficiency of cloud services, IoT and embedded system applications by removing unnecessary software layers by specializing the OS image.
One unikernel framework which provides minimal runtime footprint and fast (~35μs) boottimes is Unikraft, and aims as a means to reduce operating costs for all services that utilise it as a runtime mechanism.

Unikraft is a Unikernel Development Kit and consists of an extensive build system in addition to core and external library ecosystem which facilitate the underlying functionality of a unikernel.

## Mentors of the projects

Mentors will be assigned when the project is initiated.  Please feel free to reach out beforehand to discuss the project.

| Mentor | Email |
|--------|-------|
| [Razvan Deaconescu](https://github.com/razvand) | razvan.deaconescu@cs.pub.ro |
| [Alexander Jung](https://github.com/nderjung) | a.jung@lancs.ac.uk |
| [Simon Kuenzer](https://github.com/skuenzer) | simon.kuenzer@neclab.eu |
| [Felipe Huici](https://github.com/felipehuici) | felipe.huici@neclab.eu |

Below are a list of open projects for Unikraft which can be developed as part of GSoC22.  


---

### Lightweight and Scalable VMMs 

| | |
|-|-|
| **Difficulty** | 4/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Programming skills in C and Rust. Familiarity with virtio drivers, virtualization, and basic networking concepts is a plus. |

#### Description

VMMs (Virtual Machine Monitors) are the software that interacts with the hypervisor to manage the virtual machines running on a host (e.g., QEMU, Firecracker, etc.).
While Unikraft is able to build extremely efficient virtual machines that can boot in a few milliseconds and suspend/resume in the 10s of milliseconds, achieving such numbers partly depends on the efficiency of the VMM, and especially how it scales with an increasing number of Unikraft instances (think thousands on the same server!). 

The work entails adding support to Unikraft for modern, scalable VMMs such as Amazon’s Firecracker and Intel’s Cloud Hypervisor, including the ability to boot Unikraft, suspend/resume its instances and provide networking support when using those VMMs.
In addition, the project would require measuring the performance of such VMMs when running an increasing number of Unikraft instances (e.g., what are their boot times, suspend/resume times, throughput, VMM memory consumption, etc.).
The length of the project (175 vs. 350 hours) can be negotiated based on the number of VMMs supported and which drivers.

#### Reading & Related Material

 * https://dl.acm.org/doi/10.1145/3447786.3456248 (in particular Figure 10)
 * https://firecracker-microvm.github.io/
 * https://github.com/cloud-hypervisor/cloud-hypervisor/


---

### POSIX & Application Support 

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Basic OS concepts, familiarity with POSIX and system calls, build systems and toolstacks. |

#### Description

One of the weak points of most unikernel projects has always been weak application support, often requiring that applications be ported to the target unikernel OS.
With Unikraft we have been making a strong push towards POSIX compatibility so that applications can run unmodified on Unikraft.
We have been doing this in two different ways: (1) by adding support to the musl libc library such that applications can be compiled against it, using their native build systems, and then linked into Unikraft and (2) through binary compatibility mode, where unmodified ELFs are directly executed in Unikraft and the resulting syscalls trapped and redirected to Unikraft code.

This project may consist of, but is not limited to, any of the following items: improving Unikraft’s application support by (1) looking at the state of musl integration and analyzing to what extent unmodified application can be successfully linked and run with Unikraft; (2) conducting an equivalent analysis for binary compatibility mode and/or (3) implementation or extension of syscalls.
The success of this project would provide an enormous boost, we think, to Unikraft adoption. The project length can be varied depending on which of these items are covered by the project.


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
Thanks to its modular architecture and multi-architecture and multi-platform capabilities, running tests on Unikraft results in many permutations of the same application – resulting in a large and complex CI/CD system.

The current CI/CD system, based on Concourse, tests many applications against many target arch/plat combinations but only facilitates the construction and but not the execution/runtime of final unikernel images.
This project follows additional implementation and improvements to this system, for example: handling running of unikernels instead of simple builds; static analysis of images to be delivered as reports to GitHub pull requests; regression checks on performance (delivered as % change from the current upstream version); building new tools and instrumentation for presenting, easing, accessing and monitoring the results of the CI/CD for unikernel images (e.g. delivering performance graphs or tables of tests results); code coverage; etc.

The success of this project would provide an enormous boost, we think, to Unikraft adoption. The project length can be varied depending on which of these items are covered by the project.

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

Thanks to its modularity, efficiency and support for ARM64, Unikraft is well suited for running on embedded/small devices (e.g., Raspberry Pis, Pine64s, etc). We have done preliminary work to have Unikraft boot on some of these devices, but more work is needed for it to be able to run mainstream applications, frameworks and cool demos.
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
| **Constraints/requirements** | Good C skills, familiarity with security and hardening features, basic OS knowledge, basic x86 assembly knowledge is a plus. |

#### Description

Unikraft should provide the highest level of security to its users.
In order to achieve this, the Unikraft project counts on a combination of unikernels' inherent security properties (minimal attack surface, strong cross-application isolation, safe languages), generic security features matching Linux (ASLR, stack protection, Write-or-Execute, etc.), and state-of-the-art,
fine-grained intra-unikernel compartmentalization.

As of Unikraft v0.8.0 (Enceladus), the Unikraft main tree features support for a limited range of security features present in Linux such as Stack Smashing Protection (SP), or Undefined Behavior Sanitization (UBSAN).
Top priority planned or ongoing security features include ASLR, shadow stacks, or `FORTIFY_SOURCE`.
Other targets include Intel Control-flow Enforcement Technology (CET), or ARM Speculation Barrier (SB).

In this security-focused project, you will have the opportunity to contribute towards the implementation and testing efforts of planned security features. In parallel to this, Unikraft features out-of-tree support for intra-unikernel isolation with Intel MPK and EPT.
Interested candidates could also work towards the implementation of additional isolation mechanisms such as Intel SGX or ARM TrustZone.
Project length variable depending on selected features.

#### Reading & Related Material

 * https://unikraft.org/docs/features/security/ 
 * https://dl.acm.org/doi/abs/10.1145/3503222.3507759 
 * https://github.com/a13xp0p0v/linux-kernel-defence-map 


---

### SMP Synchronization

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, familiarity with typical OS synchronization primitives, understanding of hardware (caching, reordering, atomic instructions). |

#### Description

At the moment, Unikraft can leverage a single processor only. While there can be multiple threads (i.e., multiprogramming), no two activities actually run at the same time (i.e., multiprocessing).
With version v0.9.0 Hyperion, Unikraft introduces support for symmetric multiprocessing (SMP), thereby allowing it to execute code on all processors and cores of a system concurrently. This creates new challenges for synchronizing access to shared resources.
On a single-processor machine synchronization primitives like mutexes and semaphores can consider interrupts as the only source of parallel execution.
It is thus sufficient to protect a critical section of code by temporarily disabling interrupts. With SMP, however, this is no longer sufficient as other processors may access shared resources at any time.

In the first part of the project, you will make Unikraft fit for SMP by developing new SMP-safe synchronization primitives such as mutexes, semaphores, reader-writer locks, and condition variables.
This will enable Unikraft to protect critical sections and lay the foundation for full SMP support of core Unkraft components.

While locking is effective it is not always the most efficient solution.
Sometimes, for example, when manipulating data structures it can be faster to operate on the data structures directly using atomic instructions.
These lockless data structures can benefit SMP performance.
In the second part of the project, you will therefore extend Unikraft's existing data structures with lockless variants and research alternatives to simple locking such as read-copy-update (RCU).


#### Reading & Related Material

 * [Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/) (Chapter on Concurrency)
 * https://dl.acm.org/doi/abs/10.1145/2517349.2522714
 * https://wiki.osdev.org/Synchronization_Primitives
 * https://www.kernel.org/doc/html/latest/RCU/index.html


---

### MSI/MSI-X Interrupt Driver Support (x86, KVM)

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, OS/driver concepts, familiarity with x86 architecture. Experience with interrupt controllers, the PCI standard, and virtio is a plus. |

#### Description

Unikraft’s interrupt system on KVM/x86 is based on the legacy 8259 programmable interrupt controller (PIC) which is limited to 15 interrupt lines and programmed using slow port I/O.
Because the interrupt vector space is so small, when having many devices, interrupt lines need to be shared.
The operating system thus has to query multiple drivers to figure out which one is responsible for handling an interrupt on a shared line and which device caused the interruption. This entails reading many device registers, leading to high interrupt latency.
However, in order to achieve high I/O performance, one key is to keep the software overhead of interrupt handlers minimal.
Modern devices (including virtio) even reduce software processing time further by setting up multiple interrupt lines, like one for each I/O queue they provide.
With having just 15 interrupt lines, line sharing happens even with only a few devices attached.
Advanced programmable interrupt controllers (APICs) utilizing the MSI and MSI-X PCI standards overcome this problem by providing a much larger interrupt vector space (currently up to 2048). Interrupt line sharing is thus not required anymore.

In this project, you will (1) develop a driver for interfacing a modern x86 I/O APIC and extend Unikraft’s libpci to support MSI/MSI-X, and (2) enable the existing virtio drivers to make use of these technologies.

#### Reading & Related Material

 * [Intel 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
 * https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html
 * https://wiki.osdev.org/Interrupts
 * https://wiki.osdev.org/APIC
 * https://wiki.osdev.org/PCI#Message_Signaled_Interrupts
 * https://habr.com/en/post/501912/


---

### Call-Tree Analysis Tool for confirming ISR property

| | |
|-|-|
| **Difficulty** | 3/5 |
| **Project Size** | Variable (175 or 350 hours) |
| **Maximum instances** | 1 |
| **Constraints/requirements** | C programming skills, familiarity with the GCC/Clang compiler toolchain, ELF object files, and Makefiles, basic knowledge of OS/drivers and CPU architectures. Programming skills of python and bash is a plus. |

#### Description

In Unikraft, we compile monolithic unikernel images.
Since application logic and drivers share the same binary, we must take special care when combining code that can use all CPU features and code that is limited to a basic set of registers.
For example, an interrupt handler must not touch any extended CPU units (e.g., floating point, vector registers) because they are not saved before an interrupt handler is executed and restored on its return, as this would be too costly.
If they would anyway do so, the result would be a corrupted application state leading to undefined behavior or even a system crash.

Unikraft provides interrupt-safe routines (ISR) for basic primitives and a minimal interrupt-safe libc variant (`lib/isrlib`) that should be used in interrupt-safe functions instead.
Such functions are compiled with different compiler arguments that avoid the usage of extended CPU features.

The result of this project is an analysis tool that can be integrated into Unikraft’s build system.
Its purpose is to confirm that none of the interrupt-safe routines is ever calling a regular function that is not an ISR function.
To assist developers, the tool should report which function violates this property at the end of or during a compilation.
The project would start with a survey of already existing tools, libraries, or compiler support that could serve as a basis for a static analysis.
In a second step, this static analysis would be implemented on the selected basis and integrated into the Unikraft build system.

#### Reading & Related Material

 * http://docs.unikraft.org/developers-app.html#makefile-uk (especially isr variant)
 * https://github.com/unikraft/unikraft/blob/staging/arch/x86/x86_64/Makefile.uk (ISR_ARCHFLAGS)
 * https://github.com/unikraft/unikraft/tree/staging/lib/isrlib
 * https://github.com/chaudron/cally (as example for a potential base)

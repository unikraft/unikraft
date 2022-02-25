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

### Add support for external schedulers

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/187 |
| **Difficulty** | 3/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), GNU Make] and experience with Linux tooling.  Very strong knowledge of scheduling and OS primitives. |

#### Description

Unikraft uses modularity to enable specialization, splitting OS functionality into fine-grained components that only communicate across well-defined API boundaries.
The ability to easily swap components in and out, and to plug applications in at different levels presents application developers with a wide range of optimization possibilities.

This project tracks the introduction of a new macro-based interface to allow for plugging in an alternative scheduler as part of the internal microlibrary [`uksched`](https://github.com/unikraft/unikraft/tree/staging/lib/uksched).
At the moment there is no registration mechanism within Unikraft to allow for _external schedulers_.
External schedulers would require initialization [here within Unikraft](https://github.com/unikraft/unikraft/blob/ab4e9db54d98b6667990a57ce5502162557cd97c/lib/uksched/sched.c#L50-L64).
The interface would allow for introducing the registration of the scheduler as part of the linking of Unikraft's build process as part of a new ELF section as well as pointing to the initialization function and passing [standard scheduling interfaces](https://github.com/unikraft/unikraft/blob/ab4e9db54d98b6667990a57ce5502162557cd97c/lib/uksched/include/uk/sched.h#L83-L93).

The project can then test this by introducing a new external microlibrary into the ecosystem which registers itself against this interface.
One example of an external scheduler includes [atomthreads](https://github.com/kelvinlawson/atomthreads).

#### Reading & Related Material

* [Overview of Unikraft's Architecture](https://unikraft.org/docs/concepts/architecture/)
* [Overview of Unikraft's build process](https://unikraft.org/docs/concepts/build-process/)


---

### Librarize Persistent Memory Development Kit (PMDK)

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/73 |
| **Difficulty** | 5/5 |
| **Project Size** | 350 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Assembly, GNU Make] and experience with Linux tooling.  Very strong knowledge of OS primitives and memory and disk systems. |

#### Description

The term persistent memory is used to describe technologies which allow programs to access data as memory, directly byte-addressable, while the contents are non-volatile, preserved across power cycles.
It has aspects that are like memory, and aspects that are like storage, but it doesn't typically replace either memory or storage.
Instead, persistent memory is a third tier, used in conjunction with memory and storage. 
Systems containing persistent memory can outperform legacy configurations, providing faster start-up times, faster access to large in-memory datasets, and often improved total cost of ownership.

This project aims to introduce a set of new micro-libraries to Unikraft for interfacing with Persistent Memory by use of mission-critical applications wishing to store data during, for instance, power failures.

From Persistent Memory Development Kit (PMDK), the following libraries would be provided to Unikraft applications:

- [libpmem2](https://pmem.io/pmdk/libpmem2/):  provides low level persistent memory support, is a new version of libpmem.

- [libpmemobj](https://pmem.io/pmdk/libpmemobj/):  provides a transactional object store, providing memory allocation, transactions, and general facilities for persistent memory programming.

- [libpmemblk](https://pmem.io/pmdk/libpmemblk/):  supports arrays of pmem-resident blocks, all the same size, that are atomically updated.

- [libpmemlog](https://pmem.io/pmdk/libpmemlog/):  provides a pmem-resident log file.

- [libpmempool](https://pmem.io/pmdk/libpmempool/):  provides support for off-line pool management and diagnostics.

#### Reading & Related Material

* https://pmem.io/
* https://github.com/pmem/pmdk/
* https://cateee.net/lkddb/web-lkddb/RAS.html
* https://github.com/pmem/valgrind
* https://unikraft.org/docs/develop/porting/


---

### Solo5 arm64 (`aarch64`) support

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/63 |
| **Difficulty** | 4/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Assembly, GNU Make] and experience with Linux tooling.  Very strong knowledge of OS primitives and working with hypervisors. |

#### Description

By tailoring the OS, libraries and tools to the particular needs of your specifically targeted application, a Unikraft unikernel image is both highly performant and has a reduced attack surface.
Out of the box, Unikraft supports multiple platforms (e.g., Xen and KVM) and CPU architectures, meaning that if you are able to build an application in Unikraft, you get support for these platforms and architectures for "free".

Solo5 originally started as a project by Dan Williams at IBM Research to port MirageOS to run on the Linux/KVM hypervisor.
Since then, it has grown into a more general sandboxed execution environment, suitable for running applications built using various unikernels (a.k.a. library operating systems), targeting different sandboxing technologies on diverse host operating systems and hypervisors.
Solo5 has been shown to be highly performant since its execution environment is tailored to that of Unikernels.

This project tracks the introduction of another platform/architecture combination, namely running Unikraft via Solo5 on arm64 chipset.

#### Reading & Related Material

* https://github.com/solo5/solo5


---

### Xen arm64 (`aarch64`) support

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/62 |
| **Difficulty** | 4/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Assembly, GNU Make] and experience with Linux tooling.  Very strong knowledge of networking and networking protocols. |

#### Description

By tailoring the OS, libraries and tools to the particular needs of your specifically targeted application, a Unikraft unikernel image is both highly performant and has a reduced attack surface.
Out of the box, Unikraft supports multiple platforms (e.g., Xen and KVM) and CPU architectures, meaning that if you are able to build an application in Unikraft, you get support for these platforms and architectures for "free".

The Xen Project hypervisor is an open-source type-1 or baremetal hypervisor, which makes it possible to run many instances of an operating system or indeed different operating systems in parallel on a single machine (or host).
The Xen Project hypervisor is the only type-1 hypervisor that is available as open source. 
Xen has a small footprint and interface (is around 1MB in size) because it uses a microkernel design, with a small memory footprint and limited interface to the guest, it is more robust and secure than other hypervisors.
This makes it a great (and secure) choice for running unikernels.

This project tracks the introduction of another platform/architecture combination, namely running Unikraft via Xen on arm64 chipset.
Support already exists for running Unikraft on Xen but only for Intel x86_64.

#### Reading & Related Material

* https://wiki.xenproject.org/wiki/Main_Page
* https://github.com/unikraft/unikraft/tree/staging/plat/xen


---

### RISC-V/OpenRISC Support

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/60 |
| **Difficulty** | 4/5 |
| **Project Size** | 350 hours |
| **Maximum instances** | 2 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Assembly, RISC-V, GNU Make] and experience with Linux tooling.  Very strong assembly and familiarity with RISC-V architecture and ISA is required. |

#### Description

By tailoring the OS, libraries and tools to the particular needs of your specifically targeted application, a Unikraft unikernel image is both highly performant and has a reduced attack surface.
Out of the box, Unikraft supports multiple platforms (e.g., Xen and KVM) and CPU architectures, meaning that if you are able to build an application in Unikraft, you get support for these platforms and architectures for "free".

RISC-V is an emerging open standard instruction set architecture (ISA) that began in 2010 and is based on established reduced instruction set computer (RISC) principles.
Unlike most other ISA designs, RISC-V is provided under open source licenses that do not 
require fees to use.

This project tracks the introduction of another architecture, namely running Unikraft via on RISC-V chipset.
Some initial work has been done on this project, with an initial investigation and documentation available to help guide the project.

#### Reading & Related Material

* https://github.com/openrisc/newlib


---

### Implement IO multiplex

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/51 |
| **Difficulty** | 4/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc),, GNU Make] and experience with Linux tooling.  Strong knowledge of OS primitives and exposure to driver design. |

#### Description

Unikraft uses modularity to enable specialization, splitting OS functionality into fine-grained components that only communicate across well-defined API boundaries.
The ability to easily swap components in and out, and to plug applications in at different levels presents application developers with a wide range of optimization possibilities.

`epoll` and `select` represent two POSIX methods for IO multipex.
Currently, `select` is provided by the external library [LwIP](https://github.com/unikraft/lib-lwip) and does not reflect use of alternative file descriptors.

These two methods should be provided as a driver mechanism which allows external libraries to register interest in a custom implementation.

#### Reading & Related Material

- https://github.com/unikraft/unikraft/pull/65 - Similar interface to be provided via `posix-multiplex`


---

### Track changes of compile flags

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/47 |
| **Difficulty** | 2/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Python, GNU Make] and experience with Linux tooling.  Knowledge of build systems and using gcc/Make. |

#### Description

Unikraft is a Unikernel Development Kit and consists of an extensive build system in addition to core and external library ecosystem which facilitate the underlying functionality of a unikernel.

The build system is currently not taking changes of compile flags or build commands into account for detecting which objects need to be recompiled.
We have already `fixdep` taken from Linux so we should be able to adopt the build rules.

This project namely starts as resolving an error resulting in incorrect builds but can lead on to additional tasks, including making builds more lightweight by using pre-built archives via the command-line utility and companion to Unikraft known as [kraft](https://github.com/unikraft/kraft).

#### Reading & Related Material

* https://github.com/torvalds/linux/blob/master/scripts/basic/fixdep.c


---

### Unikraft on TEEs (TrustZone, SGX, etc.)

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/33 |
| **Difficulty** | 5/5 |
| **Project Size** | 350 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), GNU make] and experience with Linux tooling. |

#### Description

Unikernels are a novel Operating System (OS) model providing unprecedented optimization for software services.
The technology offers a clean slate OS design which improves the efficiency of cloud services, IoT and embedded system applications by removing unnecessary software layers by specializing the OS image.
Their execution is managed by a Virtual Machine Monitor (VMM) and their runtime is already considered secure for their clear memory and processor segmentation between other running VMs. 
However, for tasks which rely on encryption or the sharing of memory regions, this security can be compromised.

Many of today's processors implement Trusted Execution Environments (TEE) to protect shared memory regions between user-space applications.  However, with increased sharing of these privileged memory regions in platform environments with high density Unikernel application runtimes in kernel-mode, the need for adopting traditional user-space mechanics to protect against adverse reads of protected data becomes ever greater.

This project focuses on implementing the use of TEE on Intel-based processors (known as SGX) into Unikraft.

#### Reading & Related Material

* https://github.com/sslab-gatech/sgx-tutorial-ccs17
* Tsai, Chia-Che, et al. "[Cooperation and Security Isolation of Library OSes for Multi-Process Applications](http://www.cs.unc.edu/~porter/pubs/tsai14graphene.pdf)" Proceedings of the Ninth European Conference on Computer Systems. 2014.
* Sabt, Mohamed, Mohammed Achemlal, and Abdelmadjid Bouabdallah. "[Trusted execution environment: what it is, and what it is not](https://hal.archives-ouvertes.fr/hal-01246364/document)." 2015 IEEE Trustcom/BigDataSE/ISPA. Vol. 1. IEEE, 2015.


---

### Unikernel QEMU/Stub Domains

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/20 |
| **Difficulty** | 3/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Python, GNU Make] and experience with Linux tooling.  Strong knowledge of hypervisors and OS primitives. |

#### Description

By tailoring the OS, libraries and tools to the particular needs of your specifically targeted application, a Unikraft unikernel image is both highly performant and has a reduced attack surface.
Out of the box, Unikraft supports multiple platforms (e.g., Xen and KVM) and CPU architectures, meaning that if you are able to build an application in Unikraft, you get support for these platforms and architectures for "free".

The Xen Project hypervisor is an open-source type-1 or baremetal hypervisor, which makes it possible to run many instances of an operating system or indeed different operating systems in parallel on a single machine (or host).
The Xen Project hypervisor is the only type-1 hypervisor that is available as open source. 
Xen has a small footprint and interface (is around 1MB in size) because it uses a microkernel design, with a small memory footprint and limited interface to the guest, it is more robust and secure than other hypervisors.
This makes it a great (and secure) choice for running unikernels.

The Xen architecture has the concept of "stub domains", where, in principle, dom0 functionality can be dissagregated onto multiple, separate VMs that together mimic the overall functionality of dom0.
This improves reliability, performance/scalability and flexibility.

This project consists of generating different stub domains based on Unikraft by porting the XenStore and QEMU to Unikraft.

#### Reading & Related Material

* https://wiki.xenproject.org/wiki/Device_Model_Stub_Domains


---

### Additional filesystem

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/10 |
| **Difficulty** | 4/5 |
| **Project Size** | 350 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Python, GNU Make] and experience with Linux tooling.  Very strong knowledge of filesystems and OS primitives. |

#### Description

Unikraft uses modularity to enable specialization, splitting OS functionality into fine-grained components that only communicate across well-defined API boundaries.
The ability to easily swap components in and out, and to plug applications in at different levels presents application developers with a wide range of optimization possibilities.

Unikraft has an abstract interface for introducing new filesystem backends, allowing compile-time decision of using one way to interface with files over another.

This project tracks adding support for additional filesystems, for example: `ext4`, `nfs`, etc as external libraries.
The external library can be ported to Unikraft and then interface with `vfscore`.

#### Reading & Related Material

* https://github.com/unikraft/unikraft/tree/staging/lib/vfscore
* https://unikraft.org/docs/develop/porting/


---

### Library for generic unikernel control and monitoring (e.g., REST API)

| | |
|-|-|
| **Project Github Issue** | https://github.com/unikraft/unikraft/issues/5 |
| **Difficulty** | 3/5 |
| **Project Size** | 175 hours |
| **Maximum instances** | 1 |
| **Constraints/requirements** | Very strong programming skills [C (gcc), Python, GNU Make] and experience with Linux tooling.  Very strong knowledge of networking and networking protocols. |

#### Description

Unikernels are increasingly gaining traction in real-world deployments, especially for NFV and micro-services, where their low footprint and high performance are especially beneficial.
However, they still suffer from a lack of tools to support developers.

Unikraft is in the process of upstreaming an internal and abstract interface for managing and monitoring anything within the runtime of the unikernel.
The library, known as `ukstore`, has the role of this new library is to provide a standard way of passing information getters/setters from one place to another.

The function pointers are stored in entries and entries are stored in folders.
These are structured in two different ways, statically and dynamically.
The static ones are used for information that does not change at runtime.
The dynamic ones can be added and deleted at runtime and they are structured differently (section array of folders vs. list of folders).

This project tracks adding a new internal microlibrary into Unikraft which interfaces with `ukstore` and allows for making HTTP GET requests to retrieve and `POST`/`PUT` requests to update some of the data within `ukstore`.
This allows for remote "command-and-control" functionality of a running unikernel.

#### Reading & Related Material

* https://github.com/unikraft/unikraft/pull/202
* https://fosdem.org/2022/schedule/event/skuenzer/


Introduction
============

The high level goal of Unikraft is to be able to build unikernels and
specialized OSes targeted at specific applications, without requiring
the time-consuming, expert work that building such systems requires
today. Unikraft supports multiple platforms (e.g., Xen and KVM) and CPU
architectures, meaning that if you are able to build an application in
Unikraft, you get support for these platforms and architectures for
"free". Unikraft consists of three basic components:

* **Library Pools** are Unikraft modules, each of which provides a
  basic piece of functionality. Libraries can be arbitrarily small
  (e.g., a small library providing a proof-of-concept scheduler) or as
  large as standard libraries like libc.

* **Configuration Menu**. Inspired by Linux's Kconfig system, this menu
  allows users to pick and choose which libraries to include in the
  build process, as well as to configure options for each of them,
  where available. Like Kconfig, the menu keeps track of dependencies
  and automatically selects them where applicable.

* **Build Tool**. Based on make, it takes care of compiling and
  linking all the relevant libraries, and of producing images for the
  different platforms selected via the configuration menu.

***********************
Unikraft Libraries
***********************
Unikraft libraries are grouped into two large groups: core (or
internal) libraries, and external libraries. Core libraries generally
provide functionality typically found in operating systems. Such
libraries are grouped into categories such as memory allocators,
schedulers, filesystems, network stacks and drivers, among
others. Core libraries are part of the main Unikraft repo which also
contains the build tool and configuration menu.

External libraries consist of existing code not specifically meant for
Unikraft. This includes standard libraries such as libc or openssl, but
also run-times like Python.

Whether core or external, from a user's perspective these are
indistinguishable: they simply show up as libraries in the menu.

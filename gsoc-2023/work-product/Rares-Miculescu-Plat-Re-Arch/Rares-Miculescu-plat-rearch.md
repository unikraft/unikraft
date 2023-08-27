# re:Arch Unikraft

<img width="100px" src="https://summerofcode.withgoogle.com/assets/media/gsoc-2023-badge.svg" align="right" />

## Summary

Unikraft is an automated system for building specialized POSIX-compliant OSes, as unikernels.
These unikernels are configured for the needs of specific applications.
Being based on the concept that "everything is a library", Unikraft requires architectural changes that can improve the program’s modularity of its code base, together with improved developer and maintainer experience.
This project consists in the reorganzing of platform and architecture code, through multiple tasks and subtasks.
For example, the code is organised in multiple sections.
Three out of five directories contain libraries.
The sections describe the purpose of the libraries in detail.
Based on conventions and ease of work, some libraries were moved to other sections (drivers) or new ones were created, with code that was not part of a library (e.g. `ukintctlr`).

## GSoC contributor

Name: Rares Miculescu

Email: <miculescur@gmail.com>

Github profile: [rares-miculescu](https://github.com/rares-miculescu)

## Mentors

* [Razvan Deaconescu](https://github.com/razvand)
* [Simon Kuenzer](https://github.com/skuenzer)
* [Sergiu Moga](https://github.com/mogasergiu)
* [Michalis Pappas](https://github.com/michpappas)
* [Marc Rittinghaus](https://github.com/marcrittinghaus)

## Contributions

My contributions consist in working on migrating drivers, compiler definitions, registers and data types.
At the momment, Unikraft uses `stdint.h` as library for data types.
Because we need to be independent from any external libraries, it was decided to define our own data types and correct all the files.

There are several drivers which provide hardware-centric low-level implementation, that are scattered around the filesystem.
These drivers should be all in the same place. 

The work can be found here:

* [plat: Move register definitions into arch](https://github.com/unikraft/unikraft/pull/937)

* [doc: Replaced libc types with unikraft defined](https://github.com/unikraft/unikraft/pull/954)

* [include/uk: Move compiler definitions from essentials.h to compiler.h](https://github.com/unikraft/unikraft/pull/960)

* [lib: Add ofw from plat/drivers to lib](https://github.com/unikraft/unikraft/pull/966)

* [plat: Migrate gic to drivers/ukintctlr](https://github.com/unikraft/unikraft/pull/971)

* [drivers: Moving virtio from plat/drivers/ to drivers/](https://github.com/unikraft/unikraft/pull/967)

* [plat: Migrate rtc pl031 to drivers/ukrtc/](https://github.com/unikraft/unikraft/pull/972)

## Blog posts

You can find my blog posts here:

* [Blog post #1](https://github.com/unikraft/docs/pull/268)

* [Blog post #2](https://github.com/unikraft/docs/pull/289)

* [Blog post #3](https://github.com/unikraft/docs/pull/302)

* [Blog post #4](https://github.com/unikraft/docs/pull/307)

## Current status

Besides creating a pull request with the definition of data types, the work is done.
The pull requests need to be reviewed by the mentors.

## Future work

The project is far from finished, so I will keep working on other tasks that need to be done, alongside other members of Unikraft.
One task that we were discussing on proceeding after the merge of the pull requests, is the future `ukboot` part of plat re-arch.
I plan to assist in other platform / architecture-related work as well, such as the addition of new platforms (VMware / Hyper-V) or new architectures (RISC-V).

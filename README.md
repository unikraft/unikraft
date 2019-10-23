Unikraft - "Unikernel Crafting"
==============================

Unikraft is an automated system for building specialized OSes and
unikernels tailored to the needs of specific applications. It is based
around the concept of small, modular libraries, each providing a part
of the functionality commonly found in an operating system (e.g.,
memory allocation, scheduling, filesystem support, network stack,
etc.).

In addition, Unikraft has the concept of external libraries. These are
what we commonly understand as standard libraries such as libc or
openssl, and help to enhance the functionality of Unikraft and the
range of applications it is able to support.

Unikraft supports multiple target platforms (e.g., Xen, KVM and Linux
userspace for development purposes), so that it is possible to build
multiple images, one for each platform, for a single application
*without* requiring the application developer to do any additional,
platform-specific work.

The configuration and build process are driven by a menu system
inspired by Linux's kConfig system, making it easy to choose different
libraries and configure them. This simplifies the process of trying
out different configurations in order to extract the best possible
performance out of a particular application.

In all, Unikraft is able to build specialized OSes and unikernels
targeted at specific applications without requiring the
time-consuming, expert work that is required today to build such
images.

For more information information about Unikraft, including user and
developer guides, please refer to the `docs/guides` directory or point
your browser to the Unikraft
[documentation](http://docs.unikraft.org/)

Contributing
-----------------

If you're interested in contributing please take a look at the list of
[open projects](https://github.com/unikraft/unikraft/issues). If one
of these interests you please drop us a line via the mailing list or
directly at unikraft@listserv.neclab.eu .

Further Resources
-----------------
* [Unikraft Core Team](https://www.xenproject.org/developers/teams/unikraft.html)
* [Mailing List Archive](https://lists.xenproject.org/archives/html/minios-devel)
* [Mailing List Subscription](mailto:minios-devel-request@lists.xenproject.org)
* [Patchwork](https://patchwork.unikraft.org/project/unikraft/list/)

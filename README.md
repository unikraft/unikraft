# Unikraft - Unleash the Power of Unikernels

Unikraft is an automated system for building specialized OSes and
unikernels tailored to the needs of specific applications. It is based
around the concept of small, modular libraries, each providing a part
of the functionality commonly found in an operating system (e.g.,
memory allocation, scheduling, filesystem support, network stack,
etc.).

Unikraft supports multiple target platforms (e.g., Xen, KVM and Linux
userspace for development purposes), so that it is possible to build
multiple images, one for each platform, for a single application
*without* requiring the application developer to do any additional,
platform-specific work. In all, Unikraft is able to build specialized
OSes and unikernels targeted at specific applications without
requiring the time-consuming, expert work that is required today to
build such images.

## Getting Started
The easiest way to get started with Unikraft is to follow the
[instructions](http://www.unikraft.org/getting-started) on our website's getting started page.

## Contributing
If you're interested in contributing please take a look at the list of [open projects](https://github.com/unikraft/unikraft/issues?q=is%3Aissue+is%3Aopen+label%3Aproject). If one of these interests you please drop us a line via the [mailing list](https://lists.xenproject.org/cgi-bin/mailman/listinfo/minios-devel) or directly at unikraft@listserv.neclab.eu .

## Further Resources
For more information information about Unikraft, including user and
developer guides, please refer to the `docs/guides` directory or point
your browser to the Unikraft
[documentation](http://docs.unikraft.org/). Further resources can be
found on the project's [website](http://www.unikraft.org/) .

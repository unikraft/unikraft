# [![Unikraft](http://unikraft.org/assets/imgs/unikraft-logo-small.png)][unikraft-website]

[![](https://img.shields.io/badge/version-v0.16.0%20(Telesto)-%23EC591A)][unikraft-latest]
[![](https://img.shields.io/static/v1?label=license&message=BSD-3&color=%23385177)][unikraft-license]
[![](https://img.shields.io/discord/762976922531528725.svg?label=discord&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)][unikraft-discord]
[![](https://img.shields.io/github/contributors/unikraft/unikraft)](https://github.com/unikraft/unikraft/graphs/contributors)
[![](https://app.codacy.com/project/badge/Grade/454f62251d96413fac8024b28df2ce5b)](https://www.codacy.com/gh/unikraft/unikraft/dashboard)

***Unleash the Power of Unikernels!***

![](http://unikraft.org/assets/imgs/monkey-business.gif)

<img align="right" height="250" src="http://unikraft.org/assets/imgs/how-unikraft-works.svg" alt="How Unikraft works">

Unikraft is an automated system for building specialized POSIX-compliant OSes known as [unikernels][unikernel-wikipedia]; these images are tailored to the needs of specific applications.  Unikraft is based around the concept of small, modular libraries, each providing a part of the functionality commonly found in an operating system (e.g., memory allocation, scheduling, filesystem support, network stack, etc.).

Unikraft supports multiple target platforms (e.g., Xen, KVM, and Linux userspace) so that it is possible to build multiple images, one for each platform, for a single application *without* requiring the application developer to do any additional, platform-specific work. In all, Unikraft is able to build specialized OSes and unikernels targeted at specific applications without requiring the time-consuming, expert work that is required today to build such images.

## Typical Use Cases

Unikraft is a new system for ultra-light virtualization of your services in the cloud or at the edge, as well as extremely efficient software stacks to run bare metal on embedded devices. Smaller, quicker, and way more efficient than conventional systems:

⚡ **Cold boot virtual machines in a fraction of a second**
   While Linux-based systems might take tens of seconds to boot, Unikraft will be up in a blink.

📈 **Deploy significantly more instances per physical machine**
   Don’t waste CPU cycles on unneeded functionality – focus on your users' needs.

📉 **Drastic reductions in memory consumption**
   With all your applications and data strongly separated into ultra light-weight virtual machines, scaling becomes a breeze.

🛡️ **Ready for mission critical deployments**
   Focus your trust on a minimal set of required components, significantly reduce your service's attack surface, and minimize certification costs.

🏎 **Outstanding performance**
   Specializing the OS to meet your application's needs is the key to achieving superior performance, making you ready to drive your infrastructure to the peak.

## Supported Architectures and Platforms

Unikraft supports the construction of multiple architectures, platforms, and images. The following tables give an overview of the current support.

### 💡 Architecture Support

| Architecture         | Status                                         |
|----------------------|------------------------------------------------|
| x86                  | [`x86_64`][arch-x86_64]                        |
| Arm                  | [`armv7`][arch-arm], [`aarch64`][arch-arm64]   |
| RISC-V               | ⚙️ [Issue #60][i60]                            |

### 💻 Platform Support

| Platform                       | `x86_64`                                                                                                                                                                                                                                   | `arm32`             | `arm64`                                                                                                                                                                                                                                  |
|--------------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:-------------------:|:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| [Linux Userspace][plat-linuxu] | [![](https://builds.unikraft.io/api/v1/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-x86_64-linuxu/badge)](https://builds.unikraft.io/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-x86_64-linuxu) | ✅                  | [![](https://builds.unikraft.io/api/v1/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-arm64-linuxu/badge)](https://builds.unikraft.io/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-arm64-linuxu) |
| [Linux KVM][plat-kvm]          | [![](https://builds.unikraft.io/api/v1/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-x86_64-kvm/badge)](https://builds.unikraft.io/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-x86_64-kvm)       | -                   | [![](https://builds.unikraft.io/api/v1/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-arm64-kvm/badge)](https://builds.unikraft.io/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-arm64-kvm)       |
| [Xen Hypervisor][plat-xen]     | [![](https://builds.unikraft.io/api/v1/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-x86_64-xen/badge)](https://builds.unikraft.io/teams/unikraft/pipelines/unikraft-staging/jobs/compile-helloworld-x86_64-xen)       | ⚙️ [Issue #34][i34] | ⚙️ [Issue #62][i62]                                                                                                                                                                                                                      |
| [Solo5][plat-solo5]            | ✅                                                                                                                                                                                                                                         | -                   | ⚙️ [Issue #63][i63]                                                                                                                                                                                                                      |
| VMWare                         | ⚙️ [Issue #3][i3]                                                                                                                                                                                                                          | -                   | -                                                                                                                                                                                                                                        |
| Hyper-V                        | ⚙️ [Issue #61][i61]                                                                                                                                                                                                                        | -                   | -                                                                                                                                                                                                                                        |


### ☁️ IaaS Providers

| Cloud Provider          | Images                                           |
|-------------------------|:-------------------------------------------------|
| Amazon Web Services     | [AMI][plat-aws], [Firecracker][plat-firecracker] |
| Google Compute Platform | [GCP Image][plat-gcp]                            |
| Digital Ocean           | [Droplet][plat-digitalocean]                     |

## Getting Started

The fastest way to get started configuring, building and deploying Unikraft unikernels is to use our companion tool, [**kraft**][kraft].

With kraft installed, you can download Unikraft components, configure your unikernel to your needs, build it and run it -- there's no need to be an expert!

## Contributing

Contributions are welcome!  Please see our [Contributing Guide][unikraft-contributing] for more details.
A good starting point is the list of [open projects][github-projects].
If one of these interests you or you are interested in finding out more information, please drop us a line via the [mailing list][dev-discuss-list] or directly at <dev-discuss@unikraft.org>.

## Additional resources

* [Quick-start guide][unikraft-gettingstarted]
* [What is a unikernel?][unikraft-concepts]
* [Unikraft's inherent security benefits][unikraft-security]
* [Performance of Unikraft][unikraft-performance]
* [POSIX-compatibility with Unikraft][unikraft-posix-compatibility]
* [Energy efficiency with Unikraft][Unikraft-green]
* [Unikraft Community][unikraft-community]
* [Unikraft Documentation][unikraft-docs]

## License

Unikraft is licensed under a BSD-3-Clause.  For more information, please refer to [`COPYING.md`][unikraft-license].


[unikraft-website]: https://unikraft.org
[unikraft-docs]: https://unikraft.org/docs
[unikraft-community]: https://unikraft.org/community
[unikraft-contributing]: https://unikraft.org/docs/contributing/
[unikraft-ci]: http://ci.unikraft.org
[unikraft-license]: https://github.com/unikraft/unikraft/blob/staging/COPYING.md
[unikraft-latest]: https://github.com/unikraft/unikraft/tree/RELEASE-0.16.0
[unikraft-gettingstarted]: http://unikraft.org/docs/getting-started
[unikraft-concepts]: https://unikraft.org/docs/concepts/
[unikraft-posix-compatibility]: https://unikraft.org/docs/features/posix-compatibility
[unikraft-performance]: https://unikraft.org/docs/features/performance/
[unikraft-security]: https://unikraft.org/docs/features/security/
[unikraft-green]: https://unikraft.org/docs/features/green/
[unikraft-discord]: https://bit.ly/UnikraftDiscord
[kraft]: https://github.com/unikraft/kraft/
[github-issues]: https://github.com/unikraft/unikraft/issues
[github-projects]: https://github.com/unikraft/unikraft/labels/kind/project
[dockerhub-kraft]: https://hub.docker.com/r/unikraft/kraft
[dev-discuss-list]: https://groups.google.com/a/unikraft.org/g/user-discuss
[unikernel-wikipedia]: https://en.wikipedia.org/wiki/Unikernel
[arch-x86_64]: https://github.com/unikraft/unikraft/tree/staging/arch/x86/x86_64
[arch-arm]: https://github.com/unikraft/unikraft/tree/staging/arch/arm/arm
[arch-arm64]: https://github.com/unikraft/unikraft/tree/staging/arch/arm/arm64
[plat-linuxu]: https://github.com/unikraft/unikraft/tree/staging/plat/linuxu
[plat-kvm]: https://github.com/unikraft/unikraft/tree/staging/plat/kvm
[plat-xen]: https://github.com/unikraft/unikraft/tree/staging/plat/xen
[plat-solo5]: https://github.com/unikraft/plat-solo5
[plat-raspi]: https://github.com/unikraft/plat-raspi
[plat-gcp]: https://github.com/unikraft/plat-gcp
[plat-aws]: https://github.com/unikraft/plat-aws
[plat-digitalocean]: https://github.com/unikraft/plat-digitalocean
[plat-firecracker]: https://github.com/unikraft/plat-firecracker
[i3]: https://github.com/unikraft/unikraft/issues/3
[i34]: https://github.com/unikraft/unikraft/issues/34
[i60]: https://github.com/unikraft/unikraft/issues/60
[i61]: https://github.com/unikraft/unikraft/issues/61
[i62]: https://github.com/unikraft/unikraft/issues/62
[i63]: https://github.com/unikraft/unikraft/issues/63


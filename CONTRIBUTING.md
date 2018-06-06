Contributing to Unikraft
=======================

First of all, welcome to Unikraft! We are happy that you are interested
in contributing to this project. Unikraft is mailing-list driven,
meaning that you should submit your patches to
<minios-devel@lists.xen.org> and CC the corresponding maintainer(s)
(have a look at `MAINTAINERS.md`).

We basically follow the patch submission procedure from the [Xen
project](https://wiki.xenproject.org/wiki/Submitting_Xen_Project_Patches).
Unikraft uses the `staging` and `master` branch principle,
too. Releases are marked with tags. We highly recommend you use
[`git`](http://git-scm.com/), `git format-patch`, and `git send-email`
because these tools ensure the correct formatting of your
patches. E-Mail clients often do some sophisticated reformatting of
the e-mail body which usually break your patches.

Make sure that you tested your changes on various setups before
sending us the patch. Try several different configuration options (in particular
multiple architectures and platforms) and library combinations. During
development, disable `OPTIMIZE_DEADELIM`
(Build Options -> Drop unused functions and data)
so that all of your code is covered by the compiler and linker.


Coding Style
------------

The coding style is documented in `CODING_STYLE.md`. Please have a
look there before committing.


Commit message
--------------

In order to simplify reading and searching the patch history, please
use the following format for the short commit message:

	[selector]/[component name]: [Your short message]

Where `[selector]` can be one of the following:

* `arch`: Patch for the architecture code in `arch/`,
          `[component]` is the architecture (e.g, `x86`)
          applies also for corresponding headers in `include/uk/arch/`
* `plat`: Patch for one of the platform libraries in `plat/`,
          `[component]` is the platform (e.g, `linuxu`). This
          applies also for corresponding headers in `include/uk/plat/`
* `include`: Changes to general Unikraft headers in `include/`, `include/uk`
* `lib`: Patch for one of the Unikraft base libraries (not external) in `lib/`,
          `[component]` is the library name without lib prefix (e.g, `fdt`)
* `doc`: Changes to the documentation in `doc/`,
         `[component]` is the corresponding guide (e.g., `developers`)
* `build`: Changes to build system or generic configurations,
           `[component]` is optional

If no `[selector]` applies, define your own and cross your fingers that the
reviewer(s) do(es) not complain. :-)

Sometimes a single change required multiple commit identifiers. In general this
should be avoided by splitting a patch into multiple ones. But for the rare
case use a comma separated list of identifiers and/or use an asterisk for
`[component]` (according to the sense). For instance:

	lib/nolibc, plat/xen: Add support for foobar

	arch/*: Add spinlocks

The short message part should start with a capital and be formulated in simple
present.

The actual subject line of the patch email should be prefixed with
`[UNIKRAFT PATCH]` (use `--subject-prefix 'UNIKRAFT PATCH'` for
`git format-patch`). Re-submissions append a version number:
For instance `[UNIKRAFT PATCH v2]` for the first re-submission (use
`--subject-prefix 'UNIKRAFT PATCH v2'`). Patch series have to have a
cover-letter (use `--cover-letter` when creating patch series with
`git-format-patch`). We highly recommend using `git send-email`
to send out your patches. Please check out the git documentation for setting
up email connectivity.

The long message part is pretty free form but should be used to
explain the reasons for the patch, what has been changed and why. It
is important to provide enough information to allow reviewers and other
developers to understand the patch's purpose.

### Signing off

Please note that all patch that you send out __must__ __be__
__signed__ __off__.  This is required so that you certify that you
submitted the patch under the [Developer's Certificate of
Origin](https://www.kernel.org/doc/html/latest/process/submitting-patches.html#developer-s-certificate-of-origin-1-1).

Signing off is done by adding the following line after the long commit message:
 `Signed-off-by: [your name] <[your email]>`
You can also use the `--signoff` or `-s`  parameter of `git commit` when
creating commit messages.

	Developer's Certificate of Origin 1.1

	By making a contribution to this project, I certify that:
	(a) The contribution was created in whole or in part by me and I
	    have the right to submit it under the open source license
	    indicated in the file; or

	(b) The contribution is based upon previous work that, to the best
	    of my knowledge, is covered under an appropriate open source
	    license and I have the right under that license to submit that
	    work with modifications, whether created in whole or in part
	    by me, under the same open source license (unless I am
	    permitted to submit under a different license), as indicated
	    in the file; or

	(c) The contribution was provided directly to me by some other
	    person who certified (a), (b) or (c) and I have not modified
	    it.

        (d) I understand and agree that this project and the contribution
	    are public and that a record of the contribution (including all
	    personal information I submit with it, including my sign-off) is
	    maintained indefinitely and may be redistributed consistent with
	    this project or the open source license(s) involved.

### Patch for a Repository other than `unikraft/unikraft.git`

Since we use the same mailing list also for repositories of external libraries
(e.g., `newlib`, `lwip`), the subject prefix has to include the name of the
library instead of `UNIKRAFT`. As example: for `unikraft/libs/lwip.git`
use `LWIP`; the actual subject line of the patch emails should be prefixed with
`[LWIP PATCH]` (use `--subject-prefix 'LWIP PATCH'`). Re-submissions append
a version number: For instance `[LWIP PATCH v2]` for the first re-submission.
Patch series have to have a cover-letter (use `--cover-letter` when creating
patch series with `git-format-patch`). Once again, we highly recommend using
`git send-email` to send out your patches.

The format of the short and the long messages are free-form as long as the
corresponding library does not define anything. However, it is also here
important to provide enough information to allow reviewers and other developers
to understand the patch's purpose.

### Examples of subject lines:
- A patch for the Xen platform library:
  `[UNIKRAFT PATCH] plat/xen: Add support for ARM64`
- A patch for libukboot:
  `[UNIKRAFT PATCH] lib/ukboot: Shutdown system after main() returns`
- A patch for the external library newlib:
  `[NEWLIB PATCH] Implement glue for pthread_create()`

### Example of a commit message

	[UNIKRAFT PATCH] lib/ukdebug: Add new trondle calls

	Add some new trondle calls to the foobar interface to support
	the new zot feature.

	Signed-off-by: Joe Smith <joe.smith@citrix.com>


Maintainers
-----------

Maintainers are listed in the `MAINTAINERS.md` file which you can find in the
base folder. Each external library should have its own `MAINTAINERS` file.

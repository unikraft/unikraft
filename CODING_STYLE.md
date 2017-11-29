Coding Style
============

The Unikraft project uses the Linux kernel coding style which is mostly
true for the Unikraft base libraries, new source files, and KConfig
files. Please note that ported libraries from existing sources may use
different style definitions (e.g., `lib/fdt`). Please follow the
appropriate style depending on where you want to modify or introduce
new code.

You can find the documentation of the Linux kernel style here:
<https://www.kernel.org/doc/html/latest/process/coding-style.html>

To support you in checking your coding style, we provide a copy of the
Linux kernel's `checkpatch.pl` script in
`support/scripts/checkpatch.pl`.  You should run this from the root of
the Unikraft repository because it contains a `.checkpatch.conf` that
disables some tests that we consider irrelevant for Unikraft. Please
run this script on all patches you are about to submit. Do your best
to fix all errors and warnings; if you decide to ignore some, be
prepared to have a good reason for each warning, and a really good
reason for each error.

You can also use
[`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) to
check your patches. Most code closely follows an
automatically-formatted style defined by the `.clang-format` file in
the repository's root directory. However, since this is an automated
formatting tool, it's not perfect, and the Unikraft code can deviate
from the `clang-format`'s programmatically created output (especially
in the aesthetics of assignment alignments). Use `clang-format` as a
helpful tool, but with a grain of salt. Again, note that some parts of
the code (e.g., `lib/fdt`) follow different coding styles that you
should follow if you change this code. In the future, me might provide
`.clang-format` files appropriate for those libraries. We would also
be very happy to accept `.clang-format` definitions for those.

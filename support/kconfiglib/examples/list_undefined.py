# Prints a list of symbols that are referenced in the Kconfig files of some
# architecture but not defined by the Kconfig files of any architecture.
#
# A Kconfig file might be shared between many architectures and legitimately
# reference undefined symbols for some of them, but if no architecture defines
# the symbol, it usually indicates a problem or potential cleanup.
#
# This script could be sped up a lot if needed. See the comment near the
# referencing_nodes() call.
#
# Run with the following command in the kernel root:
#
#   $ python(3) Kconfiglib/examples/list_undefined.py
#
# Example output:
#
#   Registering defined and undefined symbols for all arches
#     Processing mips
#     Processing ia64
#     Processing metag
#     ...
#
#   Finding references to each undefined symbol
#     Processing mips
#     Processing ia64
#     Processing metag
#     ...
#
#   The following globally undefined symbols were found, listed here
#   together with the locations of the items that reference them.
#   References might come from enclosing menus and ifs.
#
#     ARM_ERRATA_753970: arch/arm/mach-mvebu/Kconfig:56, arch/arm/mach-mvebu/Kconfig:39
#     SUNXI_CCU_MP: drivers/clk/sunxi-ng/Kconfig:14
#     SUNXI_CCU_DIV: drivers/clk/sunxi-ng/Kconfig:14
#     AC97: sound/ac97/Kconfig:6
#     ...

import os
import subprocess

from kconfiglib import Kconfig


# Referenced inside the Kconfig files
os.environ["KERNELVERSION"] = str(
    subprocess.check_output(("make", "kernelversion")).decode("utf-8").rstrip()
)


def all_arch_srcarch_pairs():
    """
    Generates all valid (ARCH, SRCARCH) tuples for the kernel, corresponding to
    different architectures. SRCARCH holds the arch/ subdirectory.
    """
    for srcarch in os.listdir("arch"):
        # Each subdirectory of arch/ containing a Kconfig file corresponds to
        # an architecture
        if os.path.exists(os.path.join("arch", srcarch, "Kconfig")):
            yield (srcarch, srcarch)

    # Some architectures define additional ARCH settings with ARCH != SRCARCH
    # (search for "Additional ARCH settings for" in the top-level Makefile)

    yield ("i386", "x86")
    yield ("x86_64", "x86")

    yield ("sparc32", "sparc")
    yield ("sparc64", "sparc")

    yield ("sh64", "sh")

    yield ("um", "um")


def all_arch_srcarch_kconfigs():
    """
    Generates Kconfig instances for all the architectures in the kernel
    """

    os.environ["srctree"] = "."
    os.environ["HOSTCC"] = "gcc"
    os.environ["HOSTCXX"] = "g++"
    os.environ["CC"] = "gcc"
    os.environ["LD"] = "ld"

    for arch, srcarch in all_arch_srcarch_pairs():
        print("  Processing " + arch)

        os.environ["ARCH"] = arch
        os.environ["SRCARCH"] = srcarch

        # um (User Mode Linux) uses a different base Kconfig file
        yield Kconfig("Kconfig" if arch != "um" else "arch/x86/um/Kconfig",
                      warn=False)


print("Registering defined and undefined symbols for all arches")

# Sets holding the names of all defined and undefined symbols, for all
# architectures
defined = set()
undefined = set()

for kconf in all_arch_srcarch_kconfigs():
    for name, sym in kconf.syms.items():
        if sym.nodes:
            # If the symbol has a menu node, it is defined
            defined.add(name)
        else:
            # Undefined symbol. We skip some of the uninteresting ones.

            # Due to how Kconfig works, integer literals show up as symbols
            # (from e.g. 'default 1'). Skip those.
            try:
                int(name, 0)
                continue
            except ValueError:
                # Interesting undefined symbol
                undefined.add(name)


print("\nFinding references to each undefined symbol")

def referencing_nodes(kconf, name):
    # Returns a list of all menu nodes that reference a symbol named 'name' in
    # any of their properties or property conditions
    res = []

    for node in kconf.node_iter():
        for ref in node.referenced:
            if ref.name == name:
                res.append(node)

    return res


# Maps each globally undefined symbol to the menu nodes that reference it
undef_sym_refs = [(name, set()) for name in undefined - defined]

for kconf in all_arch_srcarch_kconfigs():
    for name, refs in undef_sym_refs:
        # This means that we search the entire configuration tree for each
        # undefined symbol, which is terribly inefficient. We could speed
        # things up by tweaking referencing_nodes() to compare each symbol to
        # multiple symbols while walking the configuration tree.
        for node in referencing_nodes(kconf, name):
            refs.add("{}:{}".format(node.filename, node.linenr))


print("\nThe following globally undefined symbols were found, listed here\n"
      "together with the locations of the items that reference them.\n"
      "References might come from enclosing menus and ifs.\n")

for name, refs in undef_sym_refs:
    print("  {}: {}".format(name, ", ".join(refs)))

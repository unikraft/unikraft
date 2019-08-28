# Prints all menu nodes that reference a given symbol any of their properties
# or property conditions, along with their parent menu nodes.
#
# Usage:
#
#   $ make [ARCH=<arch>] scriptconfig SCRIPT=Kconfiglib/examples/find_symbol.py SCRIPT_ARG=<name>
#
# Example output for SCRIPT_ARG=X86:
#
#   Found 470 locations that reference X86:
#
#   ========== Location 1 (init/Kconfig:1108) ==========
#
#   config SGETMASK_SYSCALL
#   	bool
#   	prompt "sgetmask/ssetmask syscalls support" if EXPERT
#   	default PARISC || M68K || PPC || MIPS || X86 || SPARC || MICROBLAZE || SUPERH
#   	help
#   	  sys_sgetmask and sys_ssetmask are obsolete system calls
#   	  no longer supported in libc but still enabled by default in some
#   	  architectures.
#
#   	  If unsure, leave the default option here.
#
#   ---------- Parent 1 (init/Kconfig:1077)  ----------
#
#   menuconfig EXPERT
#   	bool
#   	prompt "Configure standard kernel features (expert users)"
#   	select DEBUG_KERNEL
#   	help
#   	  This option allows certain base kernel options and settings
#   	  to be disabled or tweaked. This is for specialized
#   	  environments which can tolerate a "non-standard" kernel.
#   	  Only use this if you really know what you are doing.
#
#   ---------- Parent 2 (init/Kconfig:39)  ----------
#
#   menu "General setup"
#
#   ========== Location 2 (arch/Kconfig:29) ==========
#
#   config OPROFILE_EVENT_MULTIPLEX
#   	bool
#   	prompt "OProfile multiplexing support (EXPERIMENTAL)"
#   	default "n"
#   	depends on OPROFILE && X86
#   	help
#   	  The number of hardware counters is limited. The multiplexing
#   	  feature enables OProfile to gather more events than counters
#   	  are provided by the hardware. This is realized by switching
#   	  between events at a user specified time interval.
#
#   	  If unsure, say N.
#
#   ---------- Parent 1 (arch/Kconfig:16)  ----------
#
#   config OPROFILE
#   	tristate
#   	prompt "OProfile system profiling"
#   	select RING_BUFFER
#   	select RING_BUFFER_ALLOW_SWAP
#   	depends on PROFILING && HAVE_OPROFILE
#   	help
#   	  OProfile is a profiling system capable of profiling the
#   	  whole system, include the kernel, kernel modules, libraries,
#   	  and applications.
#
#   	  If unsure, say N.
#
#   ---------- Parent 2 (init/Kconfig:39)  ----------
#
#   menu "General setup"
#
#   ... (tons more)

import sys

import kconfiglib


if len(sys.argv) < 3:
    sys.exit('Pass symbol name (without "CONFIG_" prefix) with SCRIPT_ARG=<name>')

kconf = kconfiglib.Kconfig(sys.argv[1])
sym_name = sys.argv[2]
if sym_name not in kconf.syms:
    print("No symbol {} exists in the configuration".format(sym_name))
    sys.exit(0)

referencing = [node for node in kconf.node_iter()
               if kconf.syms[sym_name] in node.referenced]
if not referencing:
    print("No references to {} found".format(sym_name))
    sys.exit(0)

print("Found {} locations that reference {}:\n"
      .format(len(referencing), sym_name))

for i, node in enumerate(referencing, 1):
    print("========== Location {} ({}:{}) ==========\n\n{}"
          .format(i, node.filename, node.linenr, node))

    # Print the parents of the menu node too

    node = node.parent
    parent_i = 1
    while node is not kconf.top_node:
        print("---------- Parent {} ({}:{})  ----------\n\n{}"
              .format(parent_i, node.filename, node.linenr, node))
        node = node.parent
        parent_i += 1

# Loads a Kconfig and a .config and prints a symbol.
#
# Usage:
#
#   $ make [ARCH=<arch>] scriptconfig SCRIPT=Kconfiglib/examples/print_sym_info.py SCRIPT_ARG=<name>
#
# Example output for SCRIPT_ARG=MODULES:
#
# menuconfig MODULES
# 	bool
# 	prompt "Enable loadable module support"
# 	option modules
# 	help
# 	  Kernel modules are small pieces of compiled code which can
# 	  be inserted in the running kernel, rather than being
# 	  permanently built into the kernel.  You use the "modprobe"
# 	  tool to add (and sometimes remove) them.  If you say Y here,
# 	  many parts of the kernel can be built as modules (by
# 	  answering M instead of Y where indicated): this is most
# 	  useful for infrequently used options which are not required
# 	  for booting.  For more information, see the man pages for
# 	  modprobe, lsmod, modinfo, insmod and rmmod.
#
# 	  If you say Y here, you will need to run "make
# 	  modules_install" to put the modules under /lib/modules/
# 	  where modprobe can find them (you may need to be root to do
# 	  this).
#
# 	  If unsure, say Y.
#
# value = n
# visibility = y
# currently assignable values: n, y
# defined at init/Kconfig:1674

import sys

from kconfiglib import Kconfig, TRI_TO_STR


if len(sys.argv) < 3:
    sys.exit('Pass symbol name (without "CONFIG_" prefix) with SCRIPT_ARG=<name>')

kconf = Kconfig(sys.argv[1])
sym = kconf.syms[sys.argv[2]]

print(sym)
print("value = " + sym.str_value)
print("visibility = " + TRI_TO_STR[sym.visibility])
print("currently assignable values: " +
      ", ".join([TRI_TO_STR[v] for v in sym.assignable]))

for node in sym.nodes:
    print("defined at {}:{}".format(node.filename, node.linenr))

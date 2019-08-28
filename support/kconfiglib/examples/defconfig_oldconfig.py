# Produces exactly the same output as the following script:
#
# make defconfig
# echo CONFIG_ETHERNET=n >> .config
# make oldconfig
# echo CONFIG_ETHERNET=y >> .config
# yes n | make oldconfig
#
# This came up in https://github.com/ulfalizer/Kconfiglib/issues/15.
#
# Usage:
#
#   $ make [ARCH=<arch>] scriptconfig SCRIPT=Kconfiglib/examples/defconfig_oldconfig.py

import sys

import kconfiglib


kconf = kconfiglib.Kconfig(sys.argv[1])

# Mirrors defconfig
kconf.load_config("arch/x86/configs/x86_64_defconfig")
kconf.write_config()

# Mirrors the first oldconfig
kconf.load_config()
kconf.syms["ETHERNET"].set_value(0)
kconf.write_config()

# Mirrors the second oldconfig
kconf.load_config()
kconf.syms["ETHERNET"].set_value(2)
for s in kconf.unique_defined_syms:
    if s.user_value is None and 0 in s.assignable:
        s.set_value(0)

# Write the final configuration
print(kconf.write_config())

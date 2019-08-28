#!/usr/bin/env python

# Copyright (c) 2018-2019, Ulf Magnusson
# SPDX-License-Identifier: ISC

"""
Writes a configuration file where as many symbols as possible are set to 'n'.

The default output filename is '.config'. A different filename can be passed
in the KCONFIG_CONFIG environment variable.

Usage for the Linux kernel:

  $ make [ARCH=<arch>] scriptconfig SCRIPT=Kconfiglib/examples/allmodconfig.py

See the examples/allnoconfig_walk.py example script for another way to
implement this script.
"""
import kconfiglib


def main():
    kconf = kconfiglib.standard_kconfig()

    # Avoid warnings that would otherwise get printed by Kconfiglib for the
    # following:
    #
    # 1. Assigning a value to a symbol without a prompt, which never has any
    #    effect
    #
    # 2. Assigning values invalid for the type (only bool/tristate symbols
    #    accept 0/1/2, for n/m/y). The assignments will be ignored for other
    #    symbol types, which is what we want.
    kconf.warn = False
    for sym in kconf.unique_defined_syms:
        sym.set_value(2 if sym.is_allnoconfig_y else 0)
    kconf.warn = True

    kconfiglib.load_allconfig(kconf, "allno.config")

    print(kconf.write_config())


if __name__ == "__main__":
    main()

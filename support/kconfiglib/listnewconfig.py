#!/usr/bin/env python

# Copyright (c) 2018-2019, Ulf Magnusson
# SPDX-License-Identifier: ISC

"""
List all user-modifiable symbols that are not given a value in the configuration
file. Usually, these are new symbols that have been added to the Kconfig files.

The default configuration filename is '.config'. A different filename can be
passed in the KCONFIG_CONFIG environment variable.
"""
import sys

from kconfiglib import standard_kconfig, BOOL, TRISTATE, INT, HEX, STRING, \
                       TRI_TO_STR


def main():
    kconf = standard_kconfig()
    # Make it possible to filter this message out
    sys.stderr.write(kconf.load_config() + "\n")

    for sym in kconf.unique_defined_syms:
        # Only show symbols that can be toggled. Choice symbols are a special
        # case in that sym.assignable will be (2,) (length 1) for visible
        # symbols in choices in y mode, but they can still be toggled by
        # selecting some other symbol.
        if sym.user_value is None and \
           (len(sym.assignable) > 1 or
            (sym.visibility and (sym.orig_type in (INT, HEX, STRING) or
                                 sym.choice))):

            # Don't reuse the 'config_string' format for bool/tristate symbols,
            # to show n-valued symbols as 'CONFIG_FOO=n' instead of
            # '# CONFIG_FOO is not set'. This matches the C tools.
            if sym.orig_type in (BOOL, TRISTATE):
                s = "{}{}={}\n".format(kconf.config_prefix, sym.name,
                                       TRI_TO_STR[sym.tri_value])
            else:
                s = sym.config_string

            sys.stdout.write(s)


if __name__ == "__main__":
    main()

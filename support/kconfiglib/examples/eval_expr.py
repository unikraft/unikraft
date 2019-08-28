# Evaluates an expression (e.g. "X86_64 || (X86_32 && X86_LOCAL_APIC)") in the
# context of a configuration. Note that this always yields a tristate value (n,
# m, or y).
#
# Usage:
#
#   $ make [ARCH=<arch>] scriptconfig SCRIPT=Kconfiglib/examples/eval_expr.py SCRIPT_ARG=<expr>

import sys

import kconfiglib


if len(sys.argv) < 3:
    sys.exit("Pass the expression to evaluate with SCRIPT_ARG=<expression>")

kconf = kconfiglib.Kconfig(sys.argv[1])
expr = sys.argv[2]

# Enable modules so that m doesn't get demoted to n
kconf.modules.set_value(2)

print("the expression '{}' evaluates to {}"
      .format(expr, kconf.eval_string(expr)))

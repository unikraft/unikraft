#!/usr/bin/env python

# Implements a simple configuration interface on top of Kconfiglib to
# demonstrate concepts for building a menuconfig-like. Emulates how the
# standard menuconfig prints menu entries.
#
# Always displays the entire Kconfig tree to keep things as simple as possible
# (all symbols, choices, menus, and comments).
#
# Usage:
#
#   $ python(3) Kconfiglib/examples/menuconfig.py <Kconfig file>
#
# A sample Kconfig is available in Kconfiglib/examples/Kmenuconfig.
#
# Here's a notation guide. The notation matches the one used by menuconfig
# (scripts/kconfig/mconf):
#
#   [ ] prompt      - Bool
#   < > prompt      - Tristate
#   {M} prompt      - Tristate selected to m. Can only be set to m or y.
#   -*- prompt      - Bool/tristate selected to y, pinning it
#   -M- prompt      - Tristate selected to m that also has m visibility,
#                     pinning it to m
#   (foo) prompt    - String/int/hex symbol with value "foo"
#   --> prompt      - The selected symbol in a choice in y mode. This
#                     syntax is unique to this example.
#
# When modules are disabled, the .type attribute of TRISTATE symbols and
# choices automatically changes to BOOL. This trick is used by the C
# implementation as well, and gives the expected behavior without having to do
# anything extra here. The original type is available in .orig_type if needed.
#
# The Kconfiglib/examples/Kmenuconfig example uses named choices to be able to
# refer to choices by name. Named choices are supported in the C tools too, but
# I don't think I've ever seen them used in the wild.
#
# Sample session:
#
#   $ python Kconfiglib/examples/menuconfig.py Kconfiglib/examples/Kmenuconfig
#
#   ======== Example Kconfig configuration ========
#
#   [*] Enable loadable module support (MODULES)
#       Bool and tristate symbols
#           [*] Bool symbol (BOOL)
#                   [ ] Dependent bool symbol (BOOL_DEP)
#                   < > Dependent tristate symbol (TRI_DEP)
#                   [ ] First prompt (TWO_MENU_NODES)
#           < > Tristate symbol (TRI)
#           [ ] Second prompt (TWO_MENU_NODES)
#               *** These are selected by TRI_DEP ***
#           < > Tristate selected by TRI_DEP (SELECTED_BY_TRI_DEP)
#           < > Tristate implied by TRI_DEP (IMPLIED_BY_TRI_DEP)
#       String, int, and hex symbols
#           (foo) String symbol (STRING)
#           (747) Int symbol (INT)
#           (0xABC) Hex symbol (HEX)
#       Various choices
#           -*- Bool choice (BOOL_CHOICE)
#                   --> Bool choice sym 1 (BOOL_CHOICE_SYM_1)
#                       Bool choice sym 2 (BOOL_CHOICE_SYM_2)
#           {M} Tristate choice (TRI_CHOICE)
#                   < > Tristate choice sym 1 (TRI_CHOICE_SYM_1)
#                   < > Tristate choice sym 2 (TRI_CHOICE_SYM_2)
#           [ ] Optional bool choice (OPT_BOOL_CHOICE)
#
#   Enter a symbol/choice name, "load_config", or "write_config" (or press CTRL+D to exit): BOOL
#   Value for BOOL (available: n, y): n
#
#   ======== Example Kconfig configuration ========
#
#   [*] Enable loadable module support (MODULES)
#       Bool and tristate symbols
#           [ ] Bool symbol (BOOL)
#           < > Tristate symbol (TRI)
#           [ ] Second prompt (TWO_MENU_NODES)
#               *** These are selected by TRI_DEP ***
#           < > Tristate selected by TRI_DEP (SELECTED_BY_TRI_DEP)
#           < > Tristate implied by TRI_DEP (IMPLIED_BY_TRI_DEP)
#       String, int, and hex symbols
#           (foo) String symbol (STRING)
#           (747) Int symbol (INT)
#           (0xABC) Hex symbol (HEX)
#       Various choices
#           -*- Bool choice (BOOL_CHOICE)
#                   --> Bool choice sym 1 (BOOL_CHOICE_SYM_1)
#                       Bool choice sym 2 (BOOL_CHOICE_SYM_2)
#           {M} Tristate choice (TRI_CHOICE)
#                   < > Tristate choice sym 1 (TRI_CHOICE_SYM_1)
#                   < > Tristate choice sym 2 (TRI_CHOICE_SYM_2)
#          [ ] Optional bool choice (OPT_BOOL_CHOICE)
#
#   Enter a symbol/choice name, "load_config", or "write_config" (or press CTRL+D to exit): MODULES
#   Value for MODULES (available: n, y): n
#
#   ======== Example Kconfig configuration ========
#
#   [ ] Enable loadable module support (MODULES)
#       Bool and tristate symbols
#           [ ] Bool symbol (BOOL)
#           [ ] Tristate symbol (TRI)
#           [ ] Second prompt (TWO_MENU_NODES)
#               *** These are selected by TRI_DEP ***
#           [ ] Tristate selected by TRI_DEP (SELECTED_BY_TRI_DEP)
#           [ ] Tristate implied by TRI_DEP (IMPLIED_BY_TRI_DEP)
#       String, int, and hex symbols
#           (foo) String symbol (STRING)
#           (747) Int symbol (INT)
#           (0xABC) Hex symbol (HEX)
#       Various choices
#           -*- Bool choice (BOOL_CHOICE)
#                   --> Bool choice sym 1 (BOOL_CHOICE_SYM_1)
#                       Bool choice sym 2 (BOOL_CHOICE_SYM_2)
#           -*- Tristate choice (TRI_CHOICE)
#                   --> Tristate choice sym 1 (TRI_CHOICE_SYM_1)
#                       Tristate choice sym 2 (TRI_CHOICE_SYM_2)
#           [ ] Optional bool choice (OPT_BOOL_CHOICE)
#
#   Enter a symbol/choice name, "load_config", or "write_config" (or press CTRL+D to exit): ^D

from __future__ import print_function
import readline
import sys

from kconfiglib import Kconfig, \
                       Symbol, MENU, COMMENT, \
                       BOOL, TRISTATE, STRING, INT, HEX, UNKNOWN, \
                       expr_value, \
                       TRI_TO_STR


# Python 2/3 compatibility hack
if sys.version_info[0] < 3:
    input = raw_input


def indent_print(s, indent):
    print(indent*" " + s)


def value_str(sc):
    """
    Returns the value part ("[*]", "<M>", "(foo)" etc.) of a menu entry.

    sc: Symbol or Choice.
    """
    if sc.type in (STRING, INT, HEX):
        return "({})".format(sc.str_value)

    # BOOL or TRISTATE

    # The choice mode is an upper bound on the visibility of choice symbols, so
    # we can check the choice symbols' own visibility to see if the choice is
    # in y mode
    if isinstance(sc, Symbol) and sc.choice and sc.visibility == 2:
        # For choices in y mode, print '-->' next to the selected symbol
        return "-->" if sc.choice.selection is sc else "   "

    tri_val_str = (" ", "M", "*")[sc.tri_value]

    if len(sc.assignable) == 1:
        # Pinned to a single value
        return "-{}-".format(tri_val_str)

    if sc.type == BOOL:
        return "[{}]".format(tri_val_str)

    if sc.type == TRISTATE:
        if sc.assignable == (1, 2):
            # m and y available
            return "{" + tri_val_str + "}"  # Gets a bit confusing with .format()
        return "<{}>".format(tri_val_str)


def node_str(node):
    """
    Returns the complete menu entry text for a menu node, or "" for invisible
    menu nodes. Invisible menu nodes are those that lack a prompt or that do
    not have a satisfied prompt condition.

    Example return value: "[*] Bool symbol (BOOL)"

    The symbol name is printed in parentheses to the right of the prompt. This
    is so that symbols can easily be referred to in the configuration
    interface.
    """
    if not node.prompt:
        return ""

    # Even for menu nodes for symbols and choices, it's wrong to check
    # Symbol.visibility / Choice.visibility here. The reason is that a symbol
    # (and a choice, in theory) can be defined in multiple locations, giving it
    # multiple menu nodes, which do not necessarily all have the same prompt
    # visibility. Symbol.visibility / Choice.visibility is calculated as the OR
    # of the visibility of all the prompts.
    prompt, prompt_cond = node.prompt
    if not expr_value(prompt_cond):
        return ""

    if node.item == MENU:
        return "    " + prompt

    if node.item == COMMENT:
        return "    *** {} ***".format(prompt)

    # Symbol or Choice

    sc = node.item

    if sc.type == UNKNOWN:
        # Skip symbols defined without a type (these are obscure and generate
        # a warning)
        return ""

    # {:3} sets the field width to three. Gives nice alignment for empty string
    # values.
    res = "{:3} {}".format(value_str(sc), prompt)

    # Don't print the name for unnamed choices (the normal kind)
    if sc.name is not None:
        res += " ({})".format(sc.name)

    return res


def print_menuconfig_nodes(node, indent):
    """
    Prints a tree with all the menu entries rooted at 'node'. Child menu
    entries are indented.
    """
    while node:
        string = node_str(node)
        if string:
            indent_print(string, indent)

        if node.list:
            print_menuconfig_nodes(node.list, indent + 8)

        node = node.next


def print_menuconfig(kconf):
    """
    Prints all menu entries for the configuration.
    """
    # Print the expanded mainmenu text at the top. This is the same as
    # kconf.top_node.prompt[0], but with variable references expanded.
    print("\n======== {} ========\n".format(kconf.mainmenu_text))

    print_menuconfig_nodes(kconf.top_node.list, 0)
    print("")


def get_value_from_user(sc):
    """
    Prompts the user for a value for the symbol or choice 'sc'. For
    bool/tristate symbols and choices, provides a list of all the assignable
    values.
    """
    if not sc.visibility:
        print(sc.name + " is not currently visible")
        return False

    prompt = "Value for {}".format(sc.name)
    if sc.type in (BOOL, TRISTATE):
        prompt += " (available: {})" \
                  .format(", ".join(TRI_TO_STR[val] for val in sc.assignable))
    prompt += ": "

    val = input(prompt)

    # Automatically add a "0x" prefix for hex symbols, like the menuconfig
    # interface does. This isn't done when loading .config files, hence why
    # set_value() doesn't do it automatically.
    if sc.type == HEX and not val.startswith(("0x", "0X")):
        val = "0x" + val

    # Let Kconfiglib itself print a warning here if the value is invalid. We
    # could also disable warnings temporarily with 'kconf.warn = False' and
    # print our own warning.
    return sc.set_value(val)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: menuconfig.py <Kconfig file>")

    # Load Kconfig configuration files
    kconf = Kconfig(sys.argv[1])

    # Print the initial configuration tree
    print_menuconfig(kconf)

    while True:
        try:
            cmd = input('Enter a symbol/choice name, "load_config", or '
                        '"write_config" (or press CTRL+D to exit): ').strip()
        except EOFError:
            print("")
            break

        if cmd == "load_config":
            config_filename = input(".config file to load: ")
            try:
                # Returns a message telling which file got loaded
                print(kconf.load_config(config_filename))
            except EnvironmentError as e:
                print(e, file=sys.stderr)

            print_menuconfig(kconf)
            continue

        if cmd == "write_config":
            config_filename = input("To this file: ")
            try:
                # Returns a message telling which file got saved
                print(kconf.write_config(config_filename))
            except EnvironmentError as e:
                print(e, file=sys.stderr)

            continue

        # Assume 'cmd' is the name of a symbol or choice if it isn't one of the
        # commands above, prompt the user for a value for it, and print the new
        # configuration tree

        if cmd in kconf.syms:
            if get_value_from_user(kconf.syms[cmd]):
                print_menuconfig(kconf)

            continue

        if cmd in kconf.named_choices:
            if get_value_from_user(kconf.named_choices[cmd]):
                print_menuconfig(kconf)

            continue

        print("No symbol/choice named '{}' in the configuration".format(cmd),
              file=sys.stderr)

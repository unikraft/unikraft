# Prints menu entries as a tree with its value in the .config file. This can be
# handy e.g. for diffing between different .config files or versions of Kconfig files.
#
# Usage:
#
#   $ make [ARCH=<arch>] scriptconfig SCRIPT=print_config_tree.py [SCRIPT_ARG=<.config>]
#
#   If the variable WITH_HELP_DESC is modified to 'True', the help is added
#   to the symbols.
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
# Example output:
#
#   $ make scriptconfig SCRIPT=Kconfiglib/examples/print_config_tree.py [SCRIPT_ARG=<.config file>]
#
#   ======== Linux/x86 4.9.82 Kernel Configuration ========
#
#   [*] 64-bit kernel (64BIT)
#       General setup
#          ()  Cross-compiler tool prefix (CROSS_COMPILE)
#          [ ] Compile also drivers which will not load (COMPILE_TEST)
#          ()  Local version - append to kernel release (LOCALVERSION)
#          [*] Automatically append version information to the version string (LOCALVERSION_AUTO)
#          -*- Kernel compression mode
#          ...
#
# With the variable WITH_HELP_DESC modified to 'True':
#
#   ======== Linux/x86 4.9.82 Kernel Configuration ========
#
#   [*] 64-bit kernel - Say yes to build a 64-bit kernel - formerly known as x86_64 Say no to build a 32-bit kernel - formerly known as i386  (64BIT)
#       General setup
#           ()  Cross-compiler tool prefix - Same as running 'make CROSS_COMPILE=prefix-' but stored for default make runs in this kernel build directory.  You don't need to set this unless you want the configured kernel build directory to select the cross-compiler automatically.  (CROSS_COMPILE)
#           [ ] Compile also drivers which will not load - Some drivers can be compiled on a different platform than they are intended to be run on. Despite they cannot be loaded there (or even when they load they cannot be used due to missing HW support), developers still, opposing to distributors, might want to build such drivers to compile-test them.  If you are a developer and want to build everything available, say Y here. If you are a user/distributor, say N here to exclude useless drivers to be distributed.  (COMPILE_TEST)
#           ...

import sys

from kconfiglib import Kconfig, \
                       Symbol, MENU, COMMENT, \
                       BOOL, TRISTATE, STRING, INT, HEX, UNKNOWN, \
                       expr_value


# Add help description to output
WITH_HELP_DESC = False


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

    The symbol name is printed in parentheses to the right of the prompt.
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

    # Add help text
    if WITH_HELP_DESC:
        prompt += ' - ' + str(node.help).replace('\n', ' ').replace('\r', '')

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


if __name__ == "__main__":

    # Load Kconfig configuration files
    kconf = Kconfig(sys.argv[1])

    # Set default .config file or load it from argv
    if len(sys.argv) == 2:
        config_filename = '.config'
    else:
        config_filename = sys.argv[2]

    kconf.load_config(config_filename)

    # Print the configuration tree
    print_menuconfig(kconf)

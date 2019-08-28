# Does a case-insensitive search for a regular expression in the help texts of
# symbols and choices and the prompts of menus and comments. Prints the
# matching items together with their locations and the matching text.
#
# Usage:
#
#   $ make [ARCH=<arch>] scriptconfig SCRIPT=Kconfiglib/examples/help_grep.py SCRIPT_ARG=<regex>
#
# Shortened example output for SCRIPT_ARG=general:
#
#   menu "General setup"
#   location: init/Kconfig:39
#
#   config SYSVIPC
#   	bool
#   	prompt "System V IPC"
#   	help
#   	  ...
#   	  exchange information. It is generally considered to be a good thing,
#   	  ...
#
#   location: init/Kconfig:233
#
#   config BSD_PROCESS_ACCT
#   	bool
#   	prompt "BSD Process Accounting" if MULTIUSER
#   	help
#   	  ...
#   	  information.  This is generally a good idea, so say Y.
#
#   location: init/Kconfig:403
#
#   ...


import re
import sys

from kconfiglib import Kconfig, Symbol, Choice, MENU, COMMENT


if len(sys.argv) < 3:
    sys.exit("Pass the regex with SCRIPT_ARG=<regex>")

search = re.compile(sys.argv[2], re.IGNORECASE).search

for node in Kconfig(sys.argv[1]).node_iter():
    match = False

    if isinstance(node.item, (Symbol, Choice)) and \
       node.help is not None and search(node.help):
        print(node.item)
        match = True

    elif node.item == MENU and search(node.prompt[0]):
        print('menu "{}"'.format(node.prompt[0]))
        match = True

    elif node.item == COMMENT and search(node.prompt[0]):
        print('comment "{}"'.format(node.prompt[0]))
        match = True

    if match:
        print("location: {}:{}\n".format(node.filename, node.linenr))

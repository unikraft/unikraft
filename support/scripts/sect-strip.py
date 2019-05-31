#!/usr/bin/env python3
#
# Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# This nonsense exists because objcopy and strip are complaining if
# the last section within the segment is removed. This is just a
# script which suppresses this message.

import argparse
import subprocess
import sys
import shutil

def format_args(args):
    def format_arg(arg):
        if ' ' in arg:
            return '"%s"' % arg
        return arg

    return ' '.join(map(format_arg, args))

def main():
    parser = argparse.ArgumentParser(description="Strip sections gracefully")
    parser.add_argument("src", help="Source elf to strip")
    parser.add_argument("-o", "--output", help="Target elf file", required=True)
    parser.add_argument("-R", "--remove-section", action='append', default=[],
                        help="Sections to be removed (patterns allowed)")
    parser.add_argument("--with-objcopy", default="objcopy",
                        help="Specify if a specific objcopy binary should be used")
    parser.add_argument("--dry-run", action="store_true",
                        default=False,
                        help="do not change anything, print objcopy command")
    opt = parser.parse_args()

    cmd = [opt.with_objcopy,
           opt.src,
           opt.output,
           ]

    if len(opt.remove_section) == 0:
        if not opt.dry_run:
            shutil.copy(opt.src, opt.output)
        return

    for i in opt.remove_section:
        cmd += ['-R', i]

    if opt.dry_run:
        print(format_args(cmd))
        return

    proc = subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    _stdout, _ = proc.communicate()
    _stdout = _stdout.decode().strip()

    if proc.returncode:
        print("Error occurred while objcopy:", file=sys.stderr)
        print(_stdout, file=sys.stderr)
        sys.exit(1)

    filter_str = "warning: Empty loadable segment detected, is this intentional"

    # If nothing was on stdout, split will produce a list consisting
    # of one element - empty string. We an want empty list though.
    if len(_stdout) == 0:
        lines = []
    else:
        lines = _stdout.split('\n')

    lines = filter(lambda a : filter_str not in a, lines)
    for i in lines:
        print(i, file=sys.stderr)

if __name__ == '__main__':
    main()

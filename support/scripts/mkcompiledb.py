#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import argparse
import glob
import os
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "path",
        help="The path to search for compile db files. "
        "This should normally be your project's build directory.",
    )
    opt = parser.parse_args()

    search_path = opt.path
    if not os.path.isdir(search_path):
        print("{}: Invalid path or not a directory".format(search_path))
        exit(1)

    files = glob.glob(search_path + "/**/*.ukcmpdb.json", recursive=True)

    sys.stdout.write("[\n")
    if files:
        content = "".join(open(f).read() for f in files)
        sys.stdout.write(content.rstrip(",\n"))
    sys.stdout.write("\n]\n")


if __name__ == "__main__":
    main()

#!/usr/bin/python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the MIT License (the "License", see COPYING.md).
# You may not use this file except in compliance with the License.
import sys
import argparse

GCOV_BEGIN = "GCOV_DUMP_INFO_SERIAL:"
GCOV_END = "GCOV_DUMP_INFO_SERIAL_END"


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--filename",
        required=True,
        type=str,
        help="The path of the console output file",
    )
    parser.add_argument(
        "--output", required=True, type=str, help="The path of the desired output file"
    )
    args = parser.parse_args()

    return args


def can_decode(x):
    return x.isdigit() or (x >= "a" and x <= "f")


def main():
    args = parse_args()
    dump_filename = args.filename
    output_path = args.output

    with open(dump_filename, "r") as f:
        # read the whole file
        lines = f.readlines()

        # find the start and end line
        start_line = -1
        end_line = -1
        for i, line in enumerate(lines):
            if GCOV_BEGIN in line:
                start_line = i + 1
            if GCOV_END in line:
                end_line = i - 1
                break

        if start_line > end_line:
            print("Error: start line > end line")
            sys.exit(1)

        # go through all the lines, decode each byte and write to the output file
        with open(output_path, "wb") as g:
            first = True
            c = 0
            for line in lines[start_line:end_line]:
                for i, x in enumerate(line):
                    if x == "\n":
                        continue
                    if can_decode(x):
                        if first:
                            c = int(x, 16)  # Decode the first hex digit
                        else:
                            g.write(
                                bytes([c + 16 * int(x, 16)])
                            )  # Decode the second hex digit and combine with the first
                        first = not first
                    else:
                        first = True


if __name__ == "__main__":
    main()

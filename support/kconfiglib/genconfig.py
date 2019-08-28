#!/usr/bin/env python

# Copyright (c) 2018-2019, Ulf Magnusson
# SPDX-License-Identifier: ISC

"""
Generates a header file with #defines from the configuration, matching the
format of include/generated/autoconf.h in the Linux kernel.

Optionally, also writes the configuration output as a .config file. See
--config-out.

The --sync-deps, --file-list, and --env-list options generate information that
can be used to avoid needless rebuilds/reconfigurations.

Before writing a header or configuration file, Kconfiglib compares the old
contents of the file against the new contents. If there's no change, the write
is skipped. This avoids updating file metadata like the modification time, and
might save work depending on your build setup.

By default, the configuration is generated from '.config'. A different
configuration file can be passed in the KCONFIG_CONFIG environment variable.
"""
import argparse
import os
import sys

import kconfiglib


DEFAULT_HEADER_PATH = "config.h"
DEFAULT_SYNC_DEPS_PATH = "deps/"


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument(
        "--header-path",
        metavar="HEADER_FILE",
        default=DEFAULT_HEADER_PATH,
        help="Path for the generated header file (default: {})"
             .format(DEFAULT_HEADER_PATH))

    parser.add_argument(
        "--config-out",
        metavar="CONFIG_FILE",
        help="""
Write the configuration to CONFIG_FILE. This is useful if you include .config
files in Makefiles, as the generated configuration file will be a full .config
file even if .config is outdated. The generated configuration matches what
olddefconfig would produce. If you use sync-deps, you can include
deps/auto.conf instead. --config-out is meant for cases where incremental build
information isn't needed.
""")

    parser.add_argument(
        "--sync-deps",
        metavar="OUTPUT_DIR",
        nargs="?",
        const=DEFAULT_SYNC_DEPS_PATH,
        help="""
Enable generation of symbol dependency information for incremental builds,
optionally specifying the output directory (default: {}). See the docstring of
Kconfig.sync_deps() in Kconfiglib for more information.
""".format(DEFAULT_SYNC_DEPS_PATH))

    parser.add_argument(
        "--file-list",
        metavar="OUTPUT_FILE",
        help="""
Write a list of all Kconfig files to OUTPUT_FILE, with one file per line. The
paths are relative to $srctree (or to the current directory if $srctree is
unset). Files appear in the order they're 'source'd.
""")

    parser.add_argument(
        "--env-list",
        metavar="OUTPUT_FILE",
        help="""
Write a list of all environment variables referenced in Kconfig files to
OUTPUT_FILE, with one variable per line. Each line has the format NAME=VALUE.
Only environment variables referenced with the preprocessor $(VAR) syntax are
included, and not variables referenced with the older $VAR syntax (which is
only supported for backwards compatibility).
""")

    parser.add_argument(
        "kconfig_filename",
        metavar="KCONFIG_FILENAME",
        nargs="?",
        default="Kconfig",
        help="Top-level Kconfig file (default: Kconfig)")

    args = parser.parse_args()


    kconf = kconfiglib.Kconfig(args.kconfig_filename)
    kconf.load_config()

    kconf.write_autoconf(args.header_path)

    if args.config_out is not None:
        kconf.write_config(args.config_out, save_old=False)

    if args.sync_deps is not None:
        kconf.sync_deps(args.sync_deps)

    if args.file_list is not None:
        with _open_write(args.file_list) as f:
            for path in kconf.kconfig_filenames:
                f.write(path + "\n")

    if args.env_list is not None:
        with _open_write(args.env_list) as f:
            for env_var in kconf.env_vars:
                f.write("{}={}\n".format(env_var, os.environ[env_var]))


def _open_write(path):
    # Python 2/3 compatibility. io.open() is available on both, but makes
    # write() expect 'unicode' strings on Python 2.

    if sys.version_info[0] < 3:
        return open(path, "w")
    return open(path, "w", encoding="utf-8")


if __name__ == "__main__":
    main()

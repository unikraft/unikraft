#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import argparse
import subprocess
import re
import os

SECINFO_EXP = r'^\s*\d+\s+\.uk_bootinfo\s+([0-9,a-f]+)'
PHDRS_EXP = r'^\s+LOAD.+vaddr\s(0x[0-9,a-f]+).+\n.+memsz\s(0x[0-9,a-f]+)\sflags\s([rwx|-]{3})$'

# Memory region types (see include/uk/plat/memory.h)
UKPLAT_MEMRT_KERNEL     = 0x0004   # Kernel binary segment

# Memory region flags (see include/uk/plat/memory.h)
UKPLAT_MEMRF_READ       = 0x0001   # Region is readable
UKPLAT_MEMRF_WRITE      = 0x0002   # Region is writable
UKPLAT_MEMRF_EXECUTE    = 0x0004   # Region is executable

UKPLAT_MEMR_NAME_LEN    = 36

# Boot info structure (see include/uk/plat/common/bootinfo.h)
UKPLAT_BOOTINFO_SIZE    = 80

UKPLAT_BOOTINFO_MAGIC   = 0xB007B0B0 # Boot Bobo
UKPLAT_BOOTINFO_VERSION = 0x01

PAGE_SIZE = 4096

def main():
    parser = argparse.ArgumentParser(
        description='Creates the bootinfo of a unikernel binary')
    parser.add_argument('kernel', help='path to unikernel binary to update')
    parser.add_argument('output', help='path to bootinfo binary blob')
    parser.add_argument('-a', '--arch', default="x86_64",
                        help='architecture of the unikernel')
    parser.add_argument('-b', '--big', action="store_true",
                        help='use big endianness')
    parser.add_argument('-n', '--names', action="store_true",
                        help='embed region names')
    parser.add_argument('--min-address', metavar="ADDR", default=0,
                        type=lambda x: int(x, 0),
                        help='exclude segments below this virtual address')
    opt = parser.parse_args()

    if (opt.arch != 'x86_64') and (opt.arch != 'arm64'):
        raise Exception("Unsupported architecture: {}".format(opt.arch))

    endianness = 'big' if opt.big else 'little'
    mrd_size  = 64 if opt.names else 32

    # Extract name of the unikernel binary for KERNEL segments
    if opt.names:
        name = os.path.splitext(os.path.basename(opt.kernel))[0]
        name = bytes(name, 'ASCII')[:(UKPLAT_MEMR_NAME_LEN - 1)]
        name = name + b'\0' * (UKPLAT_MEMR_NAME_LEN - len(name))
    else:
        name = b'\0' * 4 # padding

    # Check for the presence and size of the .uk_bootinfo section. We want
    # to create a binary blob that has exactly this size so we can replace it
    # in the binary.
    out = subprocess.check_output(["objdump", "-h", opt.kernel])
    match = re.findall(SECINFO_EXP, out.decode('utf-8'), re.MULTILINE)

    if len(match) != 1:
        raise Exception(".uk_bootinfo section not found")

    bootsec_size = int(match[0], 16)

    # Retrieve info about ELF segments in unikernel
    out = subprocess.check_output(["objdump", "-p", opt.kernel])
    phdrs = re.findall(PHDRS_EXP, out.decode('utf-8'), re.MULTILINE)

    # Make sure they are sorted by their addresses
    phdrs = sorted(phdrs, key=lambda x: x[0])

    # The boot info is a struct ukplat_bootinfo
    # (see plat/common/include/uk/plat/common/bootinfo.h) followed by a list of
    # struct ukplat_memregion_desc (see include/uk/plat/memory.h).
    with open(opt.output, 'wb') as secobj:
        nsecs      = 0

        cap = bootsec_size
        cap = cap - UKPLAT_BOOTINFO_SIZE
        cap = cap // mrd_size

        secobj.write(UKPLAT_BOOTINFO_MAGIC.to_bytes(4, endianness)) # magic
        secobj.write(UKPLAT_BOOTINFO_VERSION.to_bytes(1, endianness)) # version
        secobj.write(b'\0' * 3) # _pad0
        secobj.write(b'\0' * 16) # bootloader
        secobj.write(b'\0' * 16) # bootprotocol
        secobj.write(b'\0' * 8) # cmdline
        secobj.write(b'\0' * 8) # cmdline_len
        secobj.write(b'\0' * 8) # dtb
        secobj.write(b'\0' * 8) # efi_st
        secobj.write(cap.to_bytes(4, endianness)) # mrds.capacity
        secobj.write(b'\0' * 4) # mrds.count

        assert secobj.tell() == UKPLAT_BOOTINFO_SIZE

        for phdr in phdrs:
            if len(phdr) != 3:
                continue

            pbase = int(phdr[0], base=16)
            if pbase < opt.min_address:
                continue

            flags = 0
            if phdr[2][0] == 'r':
                flags |= UKPLAT_MEMRF_READ
            if phdr[2][1] == 'w':
                flags |= UKPLAT_MEMRF_WRITE
            if phdr[2][2] == 'x':
                flags |= UKPLAT_MEMRF_EXECUTE

            # We have 1:1 mapping
            vbase = pbase

            # Align size up to page size
            size = (int(phdr[1], base=16) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1)
            if size == 0:
                continue

            assert nsecs < cap
            nsecs += 1

            secobj.write(pbase.to_bytes(8, endianness)) # pbase
            secobj.write(vbase.to_bytes(8, endianness)) # vbase
            secobj.write(size.to_bytes(8, endianness)) # len
            secobj.write(UKPLAT_MEMRT_KERNEL.to_bytes(2, endianness)) # type
            secobj.write(flags.to_bytes(2, endianness)) # flags
            secobj.write(name) # name or padding

        # Update the number of memory regions
        secobj.seek(UKPLAT_BOOTINFO_SIZE - 4, os.SEEK_SET)
        secobj.write(nsecs.to_bytes(4, endianness)) # mrds.count

        # Jump to end of section to fill remaining space with 0s
        secobj.truncate(bootsec_size)


if __name__ == '__main__':
    main()

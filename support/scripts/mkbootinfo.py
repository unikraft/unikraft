#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import argparse
import os
import struct
from elftools.elf.elffile import ELFFile
from elftools.elf.constants import P_FLAGS

# Memory region types (see include/uk/plat/memory.h)
UKPLAT_MEMRT_KERNEL     = 0x0004   # Kernel binary segment

# Memory region flags (see include/uk/plat/memory.h)
UKPLAT_MEMRF_READ       = 0x0001   # Region is readable
UKPLAT_MEMRF_WRITE      = 0x0002   # Region is writable
UKPLAT_MEMRF_EXECUTE    = 0x0004   # Region is executable

UKPLAT_MEMR_NAME_LEN    = 36

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

    endianness = '>' if opt.big else '<'
    # Use "sx" to null-terminate strings
    struct_ukplat_bootinfo = endianness + "IB3x15sx15sxQII"
    struct_ukplat_memregion_desc = endianness + "QQQHH35sx" if opt.names else endianness + "QQQHH4x"

    # Extract name of the unikernel binary for KERNEL segments
    if opt.names:
        name = os.path.splitext(os.path.basename(opt.kernel))[0]
        name = bytes(name, 'ASCII')[:(UKPLAT_MEMR_NAME_LEN - 1)]
    else:
        name = None

    with open(opt.kernel, 'rb') as kernelfile:
        elf = ELFFile(kernelfile)
        bootsec = elf.get_section_by_name(".uk_bootinfo")
        if bootsec is None:
            raise Exception(".uk_bootinfo section not found")
        bootsec_size = bootsec.data_size

        # The boot info is a struct ukplat_bootinfo
        # (see plat/common/include/uk/plat/common/bootinfo.h) followed by a list of
        # struct ukplat_memregion_desc (see include/uk/plat/memory.h).
        with open(opt.output, 'wb') as secobj:
            last_pbase = 0

            cap = bootsec_size
            cap = cap - struct.calcsize(struct_ukplat_bootinfo)
            cap = cap // struct.calcsize(struct_ukplat_memregion_desc)

            bootinfo = [UKPLAT_BOOTINFO_MAGIC, UKPLAT_BOOTINFO_VERSION, b'', b'', 0, cap]
            secdata = []

            for phdr in elf.iter_segments():
                if phdr['p_type'] != 'PT_LOAD':
                    continue

                pbase = phdr['p_vaddr']
                if pbase < opt.min_address:
                    continue

                if pbase != (pbase & ~(PAGE_SIZE - 1)):
                    raise Exception("Segment base address 0x{:x} is not page-aligned".format(pbase))

                flags = 0
                if phdr['p_flags'] & P_FLAGS.PF_R:
                    flags |= UKPLAT_MEMRF_READ
                if phdr['p_flags'] & P_FLAGS.PF_W:
                    flags |= UKPLAT_MEMRF_WRITE
                if phdr['p_flags'] & P_FLAGS.PF_X:
                    flags |= UKPLAT_MEMRF_EXECUTE

                # We have 1:1 mapping
                vbase = pbase

                # Align size up to page size
                size = (phdr['p_memsz'] + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1)

                assert len(secdata) < cap
                assert pbase >= last_pbase
                last_pbase = pbase

                secdata.append([pbase, vbase, size, UKPLAT_MEMRT_KERNEL, flags])

            # Update the number of memory regions
            bootinfo.append(len(secdata))

            secobj.write(struct.pack(struct_ukplat_bootinfo, *bootinfo))
            for sec in secdata:
                if opt.names:
                    sec.append(name)
                secobj.write(struct.pack(struct_ukplat_memregion_desc, *sec))

            # Fill remaining space with 0s
            secobj.truncate(bootsec_size)


if __name__ == '__main__':
    main()

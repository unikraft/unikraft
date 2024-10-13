#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import os
import argparse
from struct import unpack

# Default global values
ELF_MACHINE = {
    "i386": 3,  # EM_386
    "x86_64": 62,  # EM_X86_64
}

ELF64_EHDR_LEN = 64
ELF64_PHDR_LEN = 56
ELF64_SHDR_LEN = 64

# The fixed-size part of the Multiboot header is of size 16
# (Magic(4) + Architecture(4) + Header length(4) + Checksum(4)).
# The rest of the header is variable in size depending on the chosen tags
MULTIBOOT_HEADER_BASE_SIZE = 16

MULTIBOOT_HEADER_MAGIC = 0xE85250D6
MULTIBOOT_ARCHITECTURE_I386 = 0

# Used to set the value of 'flags' field in the Multiboot2 header tags
# to indicate the bootloader may ignore the tag if it lacks relevant support
MULTIBOOT_HEADER_TAG_OPTIONAL = 1
# Although not imposed by the specification, the Multiboot2 include an end tag
# which GRUB uses to determine the end of the header when parsing the tags
MULTIBOOT_HEADER_TAG_END = 0
MULTIBOOT_HEADER_TAG_MODULE_ALIGN = 6

class Elf64Editor:
    endian = "<"

    def __init__(self, elf64):
        self.elf64 = elf64
        self.file_sz = os.path.getsize(elf64)

        with open(elf64, "rb") as f:
            self.elf64_ehdr = f.read(ELF64_EHDR_LEN)

        # EI_CLASS must be 2 for 64-bit
        if self.elf64_ehdr[4] != 2:
            raise Exception("File format is not ELF64")

        # Check EI_DATA for endianness
        if self.elf64_ehdr[5] == 1:
            self.endian = "little"
        else:
            self.endian = "big"
        _endian = ">" if self.endian == "big" else "<"

        # Check e_machine
        if sum(self.elf64_ehdr[18:20]) != ELF_MACHINE["x86_64"]:
            raise Exception("Expected x86_64 ELF64. Wrong image format.")

        # Gather whatever is needed in order to parse the Program Headers and
        # the Section Headers. No need to store the original e_phentsize
        # or e_shentsize since we will only ever use them here. Other than that,
        # the file offsets should stay exactly the same.
        self.e_phoff = unpack(_endian + "Q", self.elf64_ehdr[32:40])[0]
        self.e_shoff = unpack(_endian + "Q", self.elf64_ehdr[40:48])[0]
        ehdr64_e_phentsize = unpack(_endian + "h", self.elf64_ehdr[54:56])[0]
        self.e_phnum = unpack(_endian + "H", self.elf64_ehdr[56:58])[0]
        ehdr64_e_shentsize = unpack(_endian + "h", self.elf64_ehdr[58:60])[0]
        self.e_shnum = unpack(_endian + "H", self.elf64_ehdr[60:62])[0]

        with open(elf64, "rb") as f:
            f.seek(self.e_phoff)
            self.elf64_phdrs = []
            for _ in range(self.e_phnum):
                self.elf64_phdrs.append(f.read(ehdr64_e_phentsize))

            f.seek(self.e_shoff)
            self.elf64_shdrs = []
            for _ in range(self.e_shnum):
                self.elf64_shdrs.append(f.read(ehdr64_e_shentsize))

    # We use this function to adjust offsets w.r.t. the prepended ELF64 header,
    # the Multiboot header and the Program headers.
    def prpnd_off(self, barray, mb2_header_end_off):
        sz = len(barray)
        old_off = int.from_bytes(barray, self.endian)

        new_off = mb2_header_end_off + ELF64_PHDR_LEN * self.e_phnum + old_off
        if sz < (new_off.bit_length() + 7) // 8:
            raise Exception("New size exceeds initial byte-length.")

        return new_off.to_bytes(sz, self.endian)

    def get_ehdr(self, mb2_header_end_off):
        elf64_ehdr = bytearray(ELF64_EHDR_LEN)
        elf64_ehdr[0:ELF64_EHDR_LEN] = self.elf64_ehdr[0:ELF64_EHDR_LEN]

        # The offset to the ELF64 Program/Sections Headers are at
        # the end of the Multiboot Header and the end of the file respectively,
        # so that we do not mess up the Multiboot2 header, which must exist
        # in the first 8192 bytes. Although the specification only enforces
        # the Multiboot2 header to be contained completely within the first
        # 32768 bytes, GRUB, for whatever reason, wants the Program Headers
        # to also be placed in the first 8192 bytes (see commit 9a5c1ad).
        # Therefore the final image will have, in this order:
        # ELF64 header
        # Multiboot2 header
        # ELF64 Program headers
        # Original ELF64 binary
        # ELF64 Section headers (these are too many so place them at the end)
        elf64_phoff = mb2_header_end_off
        elf64_shoff = mb2_header_end_off + ELF64_PHDR_LEN * self.e_phnum + self.file_sz

        elf64_ehdr[32:40] = elf64_phoff.to_bytes(8, self.endian)  # e_phoff
        elf64_ehdr[40:48] = elf64_shoff.to_bytes(8, self.endian)  # e_shoff

        return elf64_ehdr

    def get_phdrs(self, mb2_header_end_off):
        def update_phdr(phdr):
            new_phdr = bytearray(ELF64_PHDR_LEN)

            new_phdr[0:ELF64_PHDR_LEN] = phdr[0:ELF64_PHDR_LEN]  # keep the rest
            new_phdr[8:16] = self.prpnd_off(phdr[8:16], mb2_header_end_off)  # p_offset

            return new_phdr

        return [update_phdr(p) for p in self.elf64_phdrs]

    def get_shdrs(self, mb2_header_end_off):
        def update_shdr(shdr):
            new_shdr = bytearray(ELF64_SHDR_LEN)

            new_shdr[0:ELF64_SHDR_LEN] = shdr[0:ELF64_SHDR_LEN]  # keep the rest
            new_shdr[24:32] = self.prpnd_off(
                shdr[24:32], mb2_header_end_off
            )  # sh_offset

            return new_shdr

        return [update_shdr(s) for s in self.elf64_shdrs]

    def get_endian(self):
        return self.endian

def multiboot2_tag_end(endian):
    return (
        MULTIBOOT_HEADER_TAG_END.to_bytes(2, endian) +
        MULTIBOOT_HEADER_TAG_OPTIONAL.to_bytes(2, endian) +
        (8).to_bytes(4, endian)
    )

def multiboot2_tag_module_align(endian):
    return (
        MULTIBOOT_HEADER_TAG_MODULE_ALIGN.to_bytes(2, endian) +
        MULTIBOOT_HEADER_TAG_OPTIONAL.to_bytes(2, endian) +
        (8).to_bytes(4, endian)
    )

def get_multiboot2_hdr(endian):
    mb2_magic = MULTIBOOT_HEADER_MAGIC
    mb2_architecture = MULTIBOOT_ARCHITECTURE_I386

    tags_hdr = multiboot2_tag_module_align(endian)
    tags_hdr += multiboot2_tag_end(endian)
    tags_size = len(tags_hdr)

    mb2_header_length = MULTIBOOT_HEADER_BASE_SIZE + tags_size
    # We need to convert checksum to 4-byte signed integer range to avoid overflow
    mb2_checksum = -(mb2_magic + mb2_architecture + mb2_header_length)
    mb2_checksum = (mb2_checksum + (2 ** 32)) % (2 ** 32)
    if mb2_checksum >= (2 ** 31):
        mb2_checksum -= 2 ** 32

    return (
        mb2_magic.to_bytes(4, endian)
        + mb2_architecture.to_bytes(4, endian)
        + mb2_header_length.to_bytes(4, endian)
        + mb2_checksum.to_bytes(4, endian, signed=True)
        + tags_hdr
    )

def main():
    parser = argparse.ArgumentParser(
        description="Appends the Multiboot2 header to the beginning of an ELF64 Binary."
    )
    parser.add_argument("elf64", help="path to ELF64 binary to update")
    opt = parser.parse_args()

    elf64 = Elf64Editor(opt.elf64)

    endian = elf64.get_endian()

    mb2_hdr = get_multiboot2_hdr(endian)
    mb2_header_end_off = ELF64_EHDR_LEN + len(mb2_hdr)

    elf64_ehdr = elf64.get_ehdr(mb2_header_end_off)
    elf64_phdrs = elf64.get_phdrs(mb2_header_end_off)
    elf64_shdrs = elf64.get_shdrs(mb2_header_end_off)

    with open(opt.elf64, "r+b") as new_elf:
        # Save original ELF content
        elf64_buf = new_elf.read()

        # Go back to the beginning
        new_elf.seek(0)

        # Write ELF64 header with updated offsets
        new_elf.write(elf64_ehdr)

        # Append Multiboot2 header
        # NOTE: Multiboot2 header must be 8-byte aligned - luckily that is the
        # case with the size of the ELF64 header (64 bytes)
        if new_elf.tell() & 0b111:
            raise Exception("ELF64 header is not a multiple of 8 bytes in size")

        new_elf.write(mb2_hdr)

        # Write ELF64 Program Headers
        for p in elf64_phdrs:
            new_elf.write(p)

        # Append the original ELF64 binary
        new_elf.write(elf64_buf)

        # Write ELF64 Section Headers
        for s in elf64_shdrs:
            new_elf.write(s)

if __name__ == "__main__":
    main()

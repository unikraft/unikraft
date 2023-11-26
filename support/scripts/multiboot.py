#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import os
import argparse
from struct import unpack

# Default global values
ELF_MAGIC = {
    'i386':
    b'\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00',
    'x86_64':
    b'\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00',
}

ELF_MACHINE = {
    'i386': 3,                                                     # EM_386
    'x86_64': 62,                                                  # EM_X86_64
}

ELF32_EHDR_LEN = 52
ELF64_EHDR_LEN = 64

ELF32_PHDR_LEN = 32
ELF64_PHDR_LEN = 56

ELF32_SHDR_LEN = 40
ELF64_SHDR_LEN = 64

MULTIBOOT_HEADER_MAGIC = 0x1BADB002
MULTIBOOT_PAGE_ALIGN = 0x00000001
MULTIBOOT_MEMORY_INFO = 0x00000002
MULTIBOOT_SEARCH = 8192
# our Multiboot header is of size 12 (Magic(4) + Flags(4) + Checksum(4))
MULTIBOOT_HEADER_END_OFF = ELF32_EHDR_LEN + 12

class Elf64_to_32:
    arch32 = 'i386'
    endian = '<'

    def __init__(self, elf64):
        self.elf64 = elf64
        self.file_sz = os.path.getsize(elf64)

        with open(elf64, 'rb') as f:
            self.elf64_ehdr = f.read(ELF64_EHDR_LEN)

        # EI_CLASS must be 2 for 64-bit
        if self.elf64_ehdr[4] != 2:
            raise Exception('File format is not ELF64')

        # Check EI_DATA for endianness
        if self.elf64_ehdr[5] == 1:
            self.endian = 'little'
        else:
            self.endian = 'big'
        _endian = '>' if self.endian == 'big' else '<'

        # Check e_machine
        if sum(self.elf64_ehdr[18:20]) != ELF_MACHINE['x86_64']:
            raise Exception("Expected x86_64 ELF64. Wrong image format.")

        # Gather whatever is needed in order to parse the Program Headers and
        # the Section Headers. No need to store the original e_phentsize
        # or e_shentsize since we will only ever use them here. Other than that,
        # the file offsets should stay exactly the same.
        self.e_phoff = unpack(_endian + 'Q', self.elf64_ehdr[32:40])[0]
        self.e_shoff = unpack(_endian + 'Q', self.elf64_ehdr[40:48])[0]
        ehdr64_e_phentsize = unpack(_endian + 'h', self.elf64_ehdr[54:56])[0]
        self.e_phnum = unpack(_endian + 'h', self.elf64_ehdr[56:58])[0]
        ehdr64_e_shentsize = unpack(_endian + 'h', self.elf64_ehdr[58:60])[0]
        self.e_shnum = unpack(_endian + 'h', self.elf64_ehdr[60:62])[0]

        with open(elf64, 'rb') as f:
            f.seek(self.e_phoff)
            self.elf64_phdrs = []
            for _ in range(self.e_phnum):
                self.elf64_phdrs.append(f.read(ehdr64_e_phentsize))

            f.seek(self.e_shoff)
            self.elf64_shdrs = []
            for _ in range(self.e_shnum):
                self.elf64_shdrs.append(f.read(ehdr64_e_shentsize))

    # We use this function to adjust offsets w.r.t. the prepended ELF32 header,
    # the Multiboot header and the Program headers.
    def prpnd_off(self, barray):
        sz = len(barray)
        old_off = int.from_bytes(barray, self.endian)

        new_off = (MULTIBOOT_HEADER_END_OFF +
                   ELF32_PHDR_LEN * self.e_phnum +
                   old_off)
        if sz < (new_off.bit_length() + 7) // 8:
            raise Exception("New size exceeds initial byte-length.")

        return new_off.to_bytes(sz, self.endian)

    def get_elf32_ehdr(self):
        elf32_ehdr = bytearray(ELF32_EHDR_LEN)
        elf32_em = ELF_MACHINE[self.arch32]

        # The offset to the equivalent ELF32 Program/Sections Headers are at
        # the end of the Multiboot Header and the end of the file respectively,
        # so that we do not mess up the Multiboot header, which must exist
        # in the first 8192 bytes. Although the specification does not enforce
        # this, GRUB, for whatever reason, wants the Program Headers to also be
        # be placed in the first 8192 bytes (see commit 9a5c1ad).
        # Therefore the final image will have, in this order:
        # ELF32 header
        # ALIGN(4)
        # Multiboot header
        # ELF32 Program headers
        # Original ELF64 binary
        # ELF32 Section headers (these are too many so place them at the end)
        elf32_phoff = MULTIBOOT_HEADER_END_OFF.to_bytes(4, self.endian)
        elf32_shoff = (MULTIBOOT_HEADER_END_OFF +
                       ELF32_PHDR_LEN * self.e_phnum +
                       self.file_sz).to_bytes(4, self.endian)

        elf32_ehdr[0:16] = ELF_MAGIC[self.arch32]
        elf32_ehdr[16:18] = self.elf64_ehdr[16:18]                  # e_type
        elf32_ehdr[18:20] = elf32_em.to_bytes(2, self.endian)       # e_machine
        elf32_ehdr[20:24] = self.elf64_ehdr[20:24]                  # e_version
        elf32_ehdr[24:28] = self.elf64_ehdr[24:28]                  # e_entry
        elf32_ehdr[28:32] = elf32_phoff                             # e_phoff
        elf32_ehdr[32:36] = elf32_shoff                             # e_shoff
        elf32_ehdr[36:40] = self.elf64_ehdr[48:52]                  # e_flags
        elf32_ehdr[40:42] = ELF32_EHDR_LEN.to_bytes(2, self.endian) # e_ehsize
        elf32_ehdr[42:44] = ELF32_PHDR_LEN.to_bytes(2, self.endian) # e_phentsize
        elf32_ehdr[44:46] = self.elf64_ehdr[56:58]                  # e_phnum
        elf32_ehdr[46:48] = ELF32_SHDR_LEN.to_bytes(2, self.endian) # e_shentsize
        elf32_ehdr[48:50] = self.elf64_ehdr[60:62]                  # e_shnum
        elf32_ehdr[50:52] = self.elf64_ehdr[62:64]                  # e_shstrndx

        return elf32_ehdr

    def get_elf32_phdrs(self):
        def elf64_to_32_phdr(elf64_phdr):
            elf32_phdr = bytearray(ELF32_PHDR_LEN)

            elf32_phdr[0:4] = elf64_phdr[0:4]                       # p_type
            elf32_phdr[4:8] = self.prpnd_off(elf64_phdr[8:12])      # p_offset
            elf32_phdr[8:12] = elf64_phdr[16:20]                    # p_vaddr
            elf32_phdr[12:16] = elf64_phdr[24:28]                   # p_paddr
            elf32_phdr[16:20] = elf64_phdr[32:36]                   # p_filesz
            elf32_phdr[20:24] = elf64_phdr[40:44]                   # p_memsz
            elf32_phdr[24:28] = elf64_phdr[4:8]                     # p_flags
            elf32_phdr[28:32] = elf64_phdr[48:52]                   # p_align

            return elf32_phdr

        return [elf64_to_32_phdr(p) for p in self.elf64_phdrs]


    def get_elf32_shdrs(self):
        def elf64_to_32_shdr(elf64_shdr):
            elf32_shdr = bytearray(ELF32_PHDR_LEN)

            elf32_shdr[0:4] = elf64_shdr[0:4]                       # sh_name
            elf32_shdr[4:8] = elf64_shdr[4:8]                       # sh_type
            elf32_shdr[8:12] = elf64_shdr[8:12]                     # sh_flags
            elf32_shdr[12:16] = elf64_shdr[16:20]                   # sh_addr
            elf32_shdr[16:20] = self.prpnd_off(elf64_shdr[24:28])   # sh_offset
            elf32_shdr[20:24] = elf64_shdr[32:36]                   # sh_size
            elf32_shdr[24:28] = elf64_shdr[40:44]                   # sh_link
            elf32_shdr[28:32] = elf64_shdr[44:48]                   # sh_info
            elf32_shdr[32:36] = elf64_shdr[48:52]                   # sh_addralign
            elf32_shdr[36:40] = elf64_shdr[56:60]                   # sh_entsize

            return elf32_shdr

        return [elf64_to_32_shdr(p) for p in self.elf64_shdrs]

def get_multiboot_hdr():
    mb_magic = MULTIBOOT_HEADER_MAGIC
    mb_flags = MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
    mb_checksum = -(mb_magic + mb_flags)

    return (mb_magic.to_bytes(4, 'little') +
		    mb_flags.to_bytes(4, 'little') +
		    mb_checksum.to_bytes(4, 'little', signed=True))

def main():
    parser = argparse.ArgumentParser(
        description='Appends the equivalent ELF32 Headers to an ELF64 Binary.')
    parser.add_argument('elf64', help='path to ELF64 binary to update')
    opt = parser.parse_args()

    elf64 = Elf64_to_32(opt.elf64)

    elf32_ehdr = elf64.get_elf32_ehdr()
    elf32_phdrs = elf64.get_elf32_phdrs()
    elf32_shdrs = elf64.get_elf32_shdrs()
    mb_hdr = get_multiboot_hdr()

    with open(opt.elf64, 'r+b') as elf32:
        # Save previous ELF64
        elf64_buf = elf32.read()

        # Go back to the beginning
        elf32.seek(0)

        # Write equivalent ELF32 header
        elf32.write(elf32_ehdr)

        # Append the Multiboot header
        # NOTE: Multiboot header must be 4-byte aligned - luckily that is the
        # case with the size of the ELF32 header (40 bytes)
        if elf32.tell() & 0b11:
            raise Exception("ELF32 header is not a multiple of 4 bytes in size")

        elf32.write(mb_hdr)

        # Write the ELF32 equivalent Program Headers after the ELF32 header
        for p in elf32_phdrs:
            elf32.write(p)

        # Append the original ELF64 binary
        elf32.write(elf64_buf)

        # Write the ELF32 equivalent Section Headers at the end of the binary
        for s in elf32_shdrs:
            elf32.write(s)

if __name__ == '__main__':
    main()

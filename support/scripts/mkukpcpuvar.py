#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").

import os
import argparse
import subprocess
import re
from struct import unpack

ELF64_EHDR_LEN = 64
ELF64_SHDR_LEN = 64


def get_sym_val(elf, sym):
    """Get the absolute value of symbol, as seen through `nm`"""
    exp = r"^\s*" + r"([a-f0-9]+)" + r"\s+[A-Za-z]\s+" + sym + r"$"
    out = subprocess.check_output(["nm", elf])  # nosec

    re_out = re.findall(exp, out.decode("ASCII"), re.MULTILINE)
    if len(re_out) != 1:
        raise Exception("Found no " + sym + " symbol.")

    return int(re_out[0], 16)


def align_up(value, alignment):
    """Align value up to alignment boundary"""
    return (value + alignment - 1) & ~(alignment - 1)


class Elf64:
    def __init__(self, elf_path):
        self.elf_path = elf_path
        self.file_sz = os.path.getsize(elf_path)

        # Parse ELF header
        with open(elf_path, "rb") as f:
            elf_ehdr = f.read(ELF64_EHDR_LEN)

        # Check EI_CLASS (must be 2 for 64-bit)
        if elf_ehdr[4] != 2:
            raise Exception("File format is not ELF64")

        # Check endianness
        self.endian = "<" if elf_ehdr[5] == 1 else ">"

        self.ehdr = {
            "endian": self.endian,
            "e_shoff": unpack(self.endian + "Q", elf_ehdr[40:48])[0],
            "e_shentsize": unpack(self.endian + "H", elf_ehdr[58:60])[0],
            "e_shnum": unpack(self.endian + "H", elf_ehdr[60:62])[0],
            "e_shstrndx": unpack(self.endian + "H", elf_ehdr[62:64])[0],
        }

        # Parse all section headers
        self.shdrs = []
        with open(elf_path, "rb") as f:
            for idx in range(self.ehdr["e_shnum"]):
                f.seek(self.ehdr["e_shoff"] + idx * self.ehdr["e_shentsize"])
                shdr_bytes = f.read(self.ehdr["e_shentsize"])
                self.shdrs.append(self._parse_shdr(shdr_bytes))

        # Read string table
        shstrtab_shdr = self.shdrs[self.ehdr["e_shstrndx"]]
        with open(elf_path, "rb") as f:
            f.seek(shstrtab_shdr["sh_offset"])
            self.shstrtab = f.read(shstrtab_shdr["sh_size"])

    def _parse_shdr(self, shdr_bytes):
        """Parse a section header"""
        return {
            "sh_name": unpack(self.endian + "I", shdr_bytes[0:4])[0],
            "sh_type": unpack(self.endian + "I", shdr_bytes[4:8])[0],
            "sh_flags": unpack(self.endian + "Q", shdr_bytes[8:16])[0],
            "sh_addr": unpack(self.endian + "Q", shdr_bytes[16:24])[0],
            "sh_offset": unpack(self.endian + "Q", shdr_bytes[24:32])[0],
            "sh_size": unpack(self.endian + "Q", shdr_bytes[32:40])[0],
            "sh_link": unpack(self.endian + "I", shdr_bytes[40:44])[0],
            "sh_info": unpack(self.endian + "I", shdr_bytes[44:48])[0],
            "sh_addralign": unpack(self.endian + "Q", shdr_bytes[48:56])[0],
            "sh_entsize": unpack(self.endian + "Q", shdr_bytes[56:64])[0],
        }

    def get_section_name(self, shdr):
        """Extract section name from string table"""
        end = self.shstrtab.find(b"\x00", shdr["sh_name"])
        return self.shstrtab[shdr["sh_name"] : end].decode("ascii")

    def find_and_validate_pcpuvar_section(self, total_size):
        """
        Find .uk_pcpuvar section, validate size, and check available space.
        Returns the section header if valid.
        """
        pcpuvar_shdr = None

        for shdr in self.shdrs:
            name = self.get_section_name(shdr)

            if name != ".uk_pcpuvar":
                continue

            pcpuvar_shdr = shdr

            # Check if section size is sufficient
            if total_size > shdr["sh_size"]:
                raise Exception(
                    f"Not enough space in .uk_pcpuvar section. "
                    f"Need {total_size} bytes but section is {shdr['sh_size']} bytes. "
                    f"Check your linker script FILL() calculation."
                )

            # Find the section with the smallest file offset greater than
            # pcpuvar's offset to determine available room in the file.
            # Note: section header table ordering does not imply file offset
            # ordering, so we must search rather than taking shdrs[idx + 1].
            next_shdr = min(
                (
                    s
                    for s in self.shdrs
                    if s["sh_offset"] > pcpuvar_shdr["sh_offset"]
                ),
                key=lambda s: s["sh_offset"],
                default=None,
            )
            if next_shdr is None:
                available_space = self.file_sz - pcpuvar_shdr["sh_offset"]
            else:
                available_space = (
                    next_shdr["sh_offset"] - pcpuvar_shdr["sh_offset"]
                )

            if available_space < total_size:
                raise Exception(
                    f"Not enough room in file. Need {total_size} bytes "
                    f"but only {available_space} bytes available before next section."
                )

            break

        if pcpuvar_shdr is None:
            raise Exception("Section .uk_pcpuvar not found")

        return pcpuvar_shdr

    def duplicate_pcpuvar(self, max_cpus):
        """Duplicate the per-CPU variable template across all CPU slots"""
        # Find template boundaries and alignment using symbols
        base_addr = get_sym_val(self.elf_path, r"_uk_pcpuvar_base")
        tmpl_start_addr = get_sym_val(self.elf_path, r"_uk_pcpuvar_tmpl_start")
        tmpl_end_addr = get_sym_val(self.elf_path, r"_uk_pcpuvar_tmpl_end")
        cache_line_size = get_sym_val(self.elf_path, r"_uk_pcpuvar_align")

        # Calculate sizes
        tmpl_size = tmpl_end_addr - tmpl_start_addr
        aligned_size = align_up(tmpl_size, cache_line_size)
        total_size = aligned_size * max_cpus

        # Find and validate the .uk_pcpuvar section
        pcpuvar_shdr = self.find_and_validate_pcpuvar_section(total_size)

        # Read the template data
        with open(self.elf_path, "rb") as f:
            tmpl_offset = pcpuvar_shdr["sh_offset"] + (
                tmpl_start_addr - base_addr
            )
            f.seek(tmpl_offset)
            template = f.read(tmpl_size)

        # Create the full per-CPU data with padding
        pcpuvar_data = bytearray()
        for _ in range(max_cpus):
            pcpuvar_data.extend(template)
            pcpuvar_data.extend(b"\x00" * (aligned_size - tmpl_size))

        # Write back to the file
        with open(self.elf_path, "r+b") as f:
            f.seek(pcpuvar_shdr["sh_offset"])
            f.write(pcpuvar_data)


def main():
    parser = argparse.ArgumentParser(
        description="Duplicate per-CPU variable template across all CPU slots"
    )
    parser.add_argument("elf", help="path to ELF64 binary to process")
    parser.add_argument(
        "--max-cpus",
        type=int,
        required=True,
        help="maximum number of CPUs (CONFIG_UKPLAT_CPU_MAXCOUNT)",
    )

    opt = parser.parse_args()

    elf = Elf64(opt.elf)
    elf.duplicate_pcpuvar(opt.max_cpus)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import argparse
import subprocess
import re

# Generic regex for the type of an ELF section's entry
TYPE_EXP = r"[a-zA-Z0-9_]+"

# Generic regex for the name of an ELF section
SEC_NAME_EXP = r"[a-zA-Z0-9_.\[\]]*"

# Generic regex for a number in hexadecimal base
HEXNUM_EXP = r"[a-f0-9]+"

# Generic regex for a decimal number used as an index
INDEX_EXP = r"[0-9]+"

# Generic regex for a column separator
SEP = r"\s*"

# generic regex for a possible ELF symbol
SYM_EXP = r"[a-zA-z0-9._]+"

# Default values for out load address and the signature of the .uk_reloc
# section respectively (see reloc.h)
BASE_ADDR = 0x0
UKRELOC_SIGNATURE = 0x0BADB0B0

# Rela Entries format from `readelf -r`:
# Offset | Info | Type | Sym. Value | Sym. Name + Addend
# However, since this is linked with -static-pie, all that we need can be
# extracted from the .rela.dyn section(hopefully no .rel section since this
# script does not account for those) as indicated by .dynamic. In our case
# this section is expected to be filled with R_X86_64_RELATIVE or
# R_AARCH64_RELATIVE (in the case of the latter, the E may not fit in the
# command's output...).
# Luckily we do not need to deal with .symtab since `readelf -r` nicely shows
# the corresponding st_value.
R_RELATIVE_TYPE_EXP = r"R_(?:X86_64|AARCH64)_RELATIV[E]{0,1}"


def get_rela_dyn_sec_exp():
    return (
        r"^"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + R_RELATIVE_TYPE_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"$"
    )


# Return a dictionary only with the relevant fields described above
# In the case of .rela.dyn, Sym. Name + Addend represents one single
# value in hexadecimal.
def get_rela_dyn_secs(elf):
    rela_dyn_sec_exp = get_rela_dyn_sec_exp()
    out = subprocess.check_output(["readelf", "-r", elf])
    re_out = re.findall(rela_dyn_sec_exp, out.decode("ASCII"), re.MULTILINE)

    rela_dyn_secs = []
    for rds in re_out:
        rela_dyn_secs.append(
            {
                "Offset": rds[0],
                "Info": rds[1],
                "Type": rds[2],
                "Sym. Name + Addend": rds[3],
            }
        )

    return rela_dyn_secs


# Section Header Entry format from  `readelf -S`:
# Name | Type | Address | Offset |
# Size | EntSize | Flags | Link | Info | Align
def get_shdr_exp():
    return (
        r"^"
        + SEP
        + r"\["
        + SEP
        + r"("
        + INDEX_EXP
        + r")"
        + r"\]"
        + SEP
        + r"("
        + SEC_NAME_EXP
        + r")"
        + SEP
        + r"("
        + TYPE_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + TYPE_EXP
        + SEP
        + r")"
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"$"
    )


# Return a dictionary only with the relevant fields described above
def get_shdrs(elf):
    sh_exp = get_shdr_exp()
    out = subprocess.check_output(["readelf", "-S", elf])
    re_out = re.findall(sh_exp, out.decode("ASCII"), re.MULTILINE)

    shdrs = []
    for s in re_out:
        shdrs.append(
            {
                "Nr": int(s[0]),
                "Name": s[1],
                "Type": s[2],
                "Address": int(s[3], 16),
                "Offset": int(s[4], 16),
                "Size": int(s[5], 16),
                "EntSize": int(s[6]),
                "Flags": s[7],
                "Link": int(s[8]),
            }
        )

    return shdrs


# Dynamic Section format from  `readelf -d`:
# Tag | (Type) | Name/Value
def get_dyn_sec_exp():
    return (
        r"^"
        + SEP
        + r"0x"
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + r"\("
        + r"("
        + TYPE_EXP
        + r")"
        + r"\)"
        + SEP
        + r"("
        + r"[A-Za-z0-9\(\)\:_ ]*"
        + r")"
        + SEP
        + r"$"
    )


# Return a dictionary only with the relevant fields described above
def get_dyn_secs(elf):
    dyn_sec_exp = get_dyn_sec_exp()
    out = subprocess.check_output(["readelf", "-d", elf])
    re_out = re.findall(dyn_sec_exp, out.decode("ASCII"), re.MULTILINE)

    dyn_secs = []
    for ds in re_out:
        dyn_secs.append({"Tag": ds[0], "Type": ds[1], "Name/Value": ds[2]})

    return dyn_secs


# A nm_sym has the following format:
# (Value, Sym. Name)
def get_nm_sym_exp(sym):
    return (
        r"^"
        + SEP
        + r"("
        + HEXNUM_EXP
        + r")"
        + SEP
        + TYPE_EXP
        + SEP
        + r"("
        + sym
        + r")"
        + SEP
        + r"$"
    )


# Return a dictionary only with the relevant fields described above
# In the case of x86, we must take care in avoiding the _start16 symbols
# that are used for the bootstrap code of the Application Processors
# which will be relocated at runtime during SMP setup only, separately
# from the early self relocator.
x86_ignore_sym_substring = "_start16"


def get_nm_syms(elf, sym):
    nm_sym_exp = get_nm_sym_exp(sym)
    out = subprocess.check_output(["nm", elf])

    _nm_syms = re.findall(nm_sym_exp, out.decode("ASCII"), re.MULTILINE)

    return [s for s in _nm_syms if x86_ignore_sym_substring not in s[1]]


def rela_to_uk_reloc(rela):
    # Offset and Sym. Name + Addend already have the link time address added
    offset = int(rela["Offset"], 16) - BASE_ADDR
    value = int(rela["Sym. Name + Addend"], 16) - BASE_ADDR

    # We cannot have a 64-bit PIE with 32-bit or 16-bit relocations,
    # so only 8 bytes per relocation. Furthermore, all auto-generated
    # 64-bit relocations are based off a reference virtual address so no
    # need for a UKRELOC_FLAGS_PHYS_REL flag
    # (see reloc.h).
    return [offset, value, 8, 0]


# Generic Regex for a uk_reloc generated symbol.
# If there is a _phys suffix, then the relocation is to be done against
# a physical address, otherwise, we do not care what suffix the user
# provided (see reloc.h).
RELOC_ADDR_TYPE = r"[_phys].*"


def get_uk_reloc_sym_exp(_type):
    return (
        r"("
        + SYM_EXP
        + r")"
        + r"_uk_reloc_"
        + r"("
        + _type
        + r")"
        + r"("
        + r"[0-9]{1,2}"
        + r")"
        + r"("
        + RELOC_ADDR_TYPE
        + r")"
    )


# uk_reloc_sym has the following format, as parsed through `nm`:
# (Offset in memory to apply relocation, Name of the in-binary uk_reloc symbol,
# Name of the symbol whose value to relocate, Size of the relocation in bits,
# Optionally, a _phys suffix to represent a UKRELOC_FLAGS_PHYS_REL relocation,
# any other suffix is ignored and taken as without UKRELOC_FLAGS_PHYS_REL).
UKRELOC_FLAGS_PHYS_REL = 1 << 0


def uk_reloc_sym_to_struct(elf, uk_reloc_sym):
    # Offset already has the link time address added to it.
    offset = int(uk_reloc_sym[0], 16) - BASE_ADDR

    # Get the symbol entry of the symbol we are actually supposed to resolve.
    rel_sym = uk_reloc_sym[2]
    nm_syms = get_nm_syms(elf, rel_sym)

    # If the symbol has the `_phys` suffix then it means it must be resolved
    # against the physical base address - helpful information for the self
    # relocator.
    flags = 0
    if "_phys" in uk_reloc_sym[1]:
        flags |= UKRELOC_FLAGS_PHYS_REL

    # Now we get the value of the symbol we are supposed to relocate.
    # Value already has the link time address added to it.
    value = int(nm_syms[0][0], 16) - BASE_ADDR

    # We must also check whether this is a relocatable Page Table Entry.
    # If it is, we must add the PTE attributes to the value to properly
    # resolve the final value of the symbol. So look for an additional
    # symbol with the `pte_attr` suffix.
    for pte_attr in uk_reloc_sym_to_struct.pte_attr_syms:
        if uk_reloc_sym[2] == pte_attr[2]:
            value += int(pte_attr[0], 16)
            flags |= UKRELOC_FLAGS_PHYS_REL

    # Get the size in bytes of the relocation.
    size = int(uk_reloc_sym[4])

    return [offset, value, size, flags]


# A uk_reloc entry has the same definition as `struct uk_reloc`.
# See reloc.h.
def build_uk_relocs(elf, rela_dyn_secs, max_r_mem_off):
    uk_relocs = [rela_to_uk_reloc(r) for r in rela_dyn_secs]
    uk_reloc_syms = get_nm_syms(elf, get_uk_reloc_sym_exp(r"data"))
    uk_reloc_syms += get_nm_syms(elf, get_uk_reloc_sym_exp(r"imm"))

    # Also gather all of the PTE uk_reloc's for use against relocatable
    # PTE's.
    uk_reloc_sym_to_struct.pte_attr_syms = get_nm_syms(
        elf, get_uk_reloc_sym_exp(r"pte_attr")
    )

    for s in uk_reloc_syms:
        uk_reloc_en = uk_reloc_sym_to_struct(elf, s)

        for ur in uk_relocs:
            if ur[0] == uk_reloc_en[0]:
                uk_relocs.remove(ur)
                break

        uk_relocs.append(uk_reloc_en)

    # Filer out relocations beyond desired memory offset
    return [ur for ur in uk_relocs if ur[0] < max_r_mem_off]


def main():
    parser = argparse.ArgumentParser(
        description="Builds the .uk_reloc section off the .rela. and .symtab"
        "ELF sections"
    )
    parser.add_argument("elf", help="path to ELF binary to process")
    parser.add_argument("-b", "--big", action="store_true", help="use big endianness")
    opt = parser.parse_args()

    if opt.big:
        endianness = "big"
    else:
        endianness = "little"

    # Update BASE_ADDR globally by grabbing the `_base_addr` symbol's value.
    # Since it is the first symbol in the loaded executable, we assume it
    # to be the base load address.
    global BASE_ADDR
    BASE_ADDR = int(get_nm_syms(opt.elf, r"_base_addr")[0][0], 16)

    # Make sure that all 5 obligatory sections are present for this to work
    shdrs = get_shdrs(opt.elf)
    bss_shdr = None
    sh_found = 0
    for s in shdrs:
        sh_name = s["Name"]
        if sh_name == ".dynamic":
            sh_found += 1
        elif sh_name == ".rela.dyn":
            sh_found += 1
        elif sh_name == ".text":
            sh_found += 1
        elif sh_name == ".uk_reloc":
            sh_found += 1
        elif sh_name == ".bss":
            bss_shdr = s  # We will use this later
            sh_found += 1

    if sh_found != 5:
        raise Exception(
            "Found more or less than 5 of the required sections: "
            ".dynamic, .rela.dyn, .text, .uk_reloc, .bss. Are "
            "there sections with the same names?"
        )

    # Get the RELACOUNT value to compare against found entries in .rela.dyn
    # as a sanity check.
    dyn_secs = get_dyn_secs(opt.elf)
    relacount = 0
    for d in dyn_secs:
        if d["Type"] == "RELACOUNT":
            relacount = int(d["Name/Value"])
            break

    if relacount == 0:
        raise Exception("Could not find count of relocation entries.")

    # Now gather all the entries in .rela.dyn section and make sure we found
    # as many as the .dynamic section tells us there are.
    rela_dyn_secs = get_rela_dyn_secs(opt.elf)
    if len(rela_dyn_secs) != relacount:
        raise Exception(
            "Found " + str(len(rela_dyn_secs)) + " .rela.dyn "
            "entries but .dynamic section shows " + str(relacount)
        )

    # Finally, gather all of the uk_reloc entries we will write to binary file.
    # Make sure we discard any relocations beyond .bss, since that is the last
    # loaded section. This is to avoid adding relocations caused by dummy
    # sections beyond .comment (e.g. .uk_trace_keyvals)
    uk_relocs = build_uk_relocs(opt.elf, rela_dyn_secs, bss_shdr["Address"] - BASE_ADDR)

    uk_reloc_start = int(get_nm_syms(opt.elf, r"_uk_reloc_start")[0][0], 16)
    uk_reloc_end = int(get_nm_syms(opt.elf, r"_uk_reloc_end")[0][0], 16)
    uk_reloc_sz = 24
    if uk_reloc_end - uk_reloc_start < uk_reloc_sz * len(uk_relocs):
        raise Exception(
            "There are " + str(len(uk_relocs)) + " struct uk_reloc"
            "entries but the maximum amount is of "
            + str(int((uk_reloc_end - uk_reloc_start) / uk_reloc_sz))
            + " entries."
        )

    # Write the binary blob with `struct uk_reloc` entries
    with open(opt.elf + ".uk_reloc.bin", "wb") as ur_bin:
        ur_bin.write(UKRELOC_SIGNATURE.to_bytes(4, endianness))  # signature

        def write_uk_reloc_to_ur_bin(ur):
            ur_bin.write(ur[0].to_bytes(8, endianness))  # r_mem_off
            ur_bin.write(ur[1].to_bytes(8, endianness, signed=True))  # r_addr
            ur_bin.write(ur[2].to_bytes(4, endianness))  # r_sz
            ur_bin.write(ur[3].to_bytes(4, endianness))  # flags

        for ur in uk_relocs:
            write_uk_reloc_to_ur_bin(ur)

        # Now write the sentinel (a zeroed out entry, but only r_sz is required
        # to be 0 anyway)
        write_uk_reloc_to_ur_bin([0, 0, 0, 0])


if __name__ == "__main__":
    main()

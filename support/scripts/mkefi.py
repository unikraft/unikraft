#!/usr/bin/env python3

import argparse
import subprocess
import re

ELF_MACHINE = {"x86_64": 62, "arm64": 183}  # EM_X86_64  # EM_AARCH64

PE_MACHINE = {
    "x86_64": 0x8664,  # IMAGE_FILE_MACHINE_AMD64
    "arm64": 0xAA64,  # IMAGE_FILE_MACHINE_ARM64
}

PE_SEC_SZ = 40

# COFF Characteristics
COFF_HDR_CHRS = (
    0x0002
    | 0x0004  # IMAGE_FILE_EXECUTABLE_IMAGE
    | 0x0200  # IMAGE_FILE_LINE_NUMS_STRIPPED  # IMAGE_FILE_DEBUG_STRIPPED
)

# All sections that we obtain from the PT_LOAD Program Headers must be
# writable, since we have to apply initial relocations. We are going to
# change permissions ourselves afterwards anyway.
PE_SEC_CHRS = (
    0x00000040
    | 0x00D00000  # IMAGE_SCN_CNT_INITIALIZED_DATA
    | 0x20000000  # IMAGE_SCN_ALIGN_4096BYTES
    | 0x40000000  # IMAGE_SCN_MEM_EXECUTE
    | 0x80000000  # IMAGE_SCN_MEM_READ  # IMAGE_SCN_MEM_WRITE
)

MZ_PE_HDR = {
    # MS-DOS Stub (Image Only)
    "MZ_MAGIC": (0x0000000000005A4D, 0x02),  # Offset   0
    "MZ_PAD": (0x0000000000000000, 0x3A),  # Offset   2
    "PE_HDR_OFF": (0x0000000000000040, 0x04),  # Offset  60
    # Signature (Image Only)
    "PE_MAGIC": (0x0000000000004550, 0x04),  # Offset  64
    # COFF File Header (Object and Image)
    "Machine": [0x0000000000000000, 0x02],  # Offset  68
    "NumberOfSections": [0x0000000000000000, 0x02],  # Offset  70
    "TimeDateStamp": (0x0000000000000000, 0x04),  # Offset  72
    "PointerToSymbolTable": (0x0000000000000000, 0x04),  # Offset  76
    "NumberOfSymbols": (0x0000000000000000, 0x04),  # Offset  80
    "SizeOfOptionalHeader": (0x0000000000000070, 0x02),  # Offset  84
    "Characteristics": (COFF_HDR_CHRS, 0x02),  # Offset  86
    # Optional Header Standard Fields (Image Only)
    "Magic": (0x000000000000020B, 0x02),  # Offset  88
    "MajorLinkerVersion": (0x0000000000000000, 0x01),  # Offset  90
    "MinorLinkerVersion": (0x0000000000000000, 0x01),  # Offset  91
    "SizeOfCode": [0x0000000000000000, 0x04],  # Offset  92
    "SizeOfInitializedData": [0x0000000000000000, 0x04],  # Offset  96
    "SizeOfUninitializedData": [0x0000000000000000, 0x04],  # Offset 100
    "AddressOfEntryPoint": [0x0000000000000000, 0x04],  # Offset 104
    "BaseOfCode": [0x0000000000000000, 0x04],  # Offset 108
    # Optional Header Windows-Specific Fields (Image Only)
    "ImageBase": [0x0000000000000000, 0x08],  # Offset 112
    "SectionAlignment": (0x0000000000001000, 0x04),  # Offset 120
    "FileAlignment": (0x0000000000001000, 0x04),  # Offset 124
    "MajorOperatingSystemVersion": (0x0000000000000000, 0x02),  # Offset 128
    "MinorOperatingSystemVersion": (0x0000000000000000, 0x02),  # Offset 130
    "MajorImageVersion": (0x0000000000000000, 0x02),  # Offset 132
    "MinorImageVersion": (0x0000000000000000, 0x02),  # Offset 134
    "MajorSubsystemVersion": (0x0000000000000000, 0x02),  # Offset 136
    "MinorSubsystemVersion": (0x0000000000000000, 0x02),  # Offset 138
    "Win32VersionValue": (0x0000000000000000, 0x04),  # Offset 140
    "SizeOfImage": [0x0000000000000000, 0x04],  # Offset 144
    "SizeOfHeaders": [0x0000000000000000, 0x04],  # Offset 148
    "CheckSum": (0x0000000000000000, 0x04),  # Offset 152
    "Subsystem": (0x000000000000000A, 0x02),  # Offset 156
    "DllCharacteristics": (0x0000000000000000, 0x02),  # Offset 158
    "SizeOfStackReserve": (0x0000000000000000, 0x08),  # Offset 160
    "SizeOfStackCommit": (0x0000000000000000, 0x08),  # Offset 168
    "SizeOfHeapReserve": (0x0000000000000000, 0x08),  # Offset 176
    "SizeOfHeapCommit": (0x0000000000000000, 0x08),  # Offset 184
    "LoaderFlags": (0x0000000000000000, 0x04),  # Offset 192
    "NumberOfRvaAndSizes": (0x0000000000000000, 0x04),  # Offset 196
    # Section Table (Section Headers)
    "SectionHeaders": [],  # Offset 200
}


def elf_phdr_to_pe_sec(base_addr, phdr):
    return {
        "Name": (0x554B5F5048445200, 0x8),  # UK_PHDR
        "VirtualSize": (phdr["MemSiz"], 0x4),
        "VirtualAddress": (
            phdr["VirtAddr"] - base_addr + MZ_PE_HDR["SizeOfHeaders"][0],
            0x4,
        ),
        "SizeOfRawData": (phdr["FileSiz"], 0x4),
        "PointerToRawData": (phdr["Offset"] + MZ_PE_HDR["SizeOfHeaders"][0], 0x4),
        "PointerToRelocations": (0x0, 0x4),
        "PointerToLinenumbers": (0x0, 0x4),
        "NumberOfRelocations": (0x0, 0x2),
        "NumberOfLinenumbers": (0x0, 0x2),
        "Characteristics": (PE_SEC_CHRS, 0x4),
    }


# Get the absolute value of symbol, as seen through `nm`
def get_sym_val(elf, sym):
    exp = r"^\s*" + r"([a-f0-9]+)" + r"\s+[A-Za-z]\s+" + sym + r"$"
    out = subprocess.check_output(["nm", elf])

    re_out = re.findall(exp, out.decode("ASCII"), re.MULTILINE)
    if len(re_out) != 1:
        raise Exception("Found no " + sym + " symbol.")

    return int(re_out[0], 16)


# Get a list of all the PT_LOAD Program Headers
HEXNUM_EXP = r"0x[a-f0-9]+"


def get_loadable_phdrs(elf):
    exp = (
        r"^\s*"
        + r"LOAD"
        + r"\s*"
        + r"("
        + HEXNUM_EXP
        + r")"
        + r"\s*"
        + r"("
        + HEXNUM_EXP
        + r")"
        + r"\s*"
        + HEXNUM_EXP
        + r"\s*"
        + r"("
        + HEXNUM_EXP
        + r")"
        + r"\s*"
        + r"("
        + HEXNUM_EXP
        + r")"
        + r"\s*"
        + r"("
        + r"[RWE ]+"
        + r")"
        + r"\s*"
        + HEXNUM_EXP
        + r"$"
    )
    out = subprocess.check_output(["readelf", "-l", elf], stderr=subprocess.DEVNULL)
    re_out = re.findall(exp, out.decode("ASCII"), re.MULTILINE)

    return [
        {
            "Offset": int(r[0], 16),
            "VirtAddr": int(r[1], 16),
            "FileSiz": int(r[2], 16),
            "MemSiz": int(r[3], 16),
            "Flags": r[4].replace(" ", ""),
        }
        for r in re_out
    ]


def main():
    parser = argparse.ArgumentParser(
        description="Update the fake PE32 header with the required metadata to"
        "be bootable by a UEFI environment and overwrite the ELF64"
        "Header with an empty MS-DOS stub."
    )
    parser.add_argument("elf", help="path to ELF64 binary to process")
    opt = parser.parse_args()

    # We need to operate on the debug image for symbol values. But as far as
    # ELF sections go, using the final image is enough.
    elf_dbg = opt.elf + ".dbg"

    # Fetch base address, end address and bss start address from ld script
    base_addr = get_sym_val(elf_dbg, r"_base_addr")
    bss_addr = get_sym_val(elf_dbg, r"__bss_start")

    # Consider the first PT_LOAD Segment as the first in the file.
    ld_phdrs = get_loadable_phdrs(opt.elf)

    # Make sure they are sorted by their addresses
    ld_phdrs = sorted(ld_phdrs, key=lambda x: x["VirtAddr"] + x["MemSiz"])
    end_addr = ld_phdrs[-1]["VirtAddr"] + ld_phdrs[-1]["MemSiz"]

    # Again, all addresses relative to the very base of the file, because PE
    # loading considers the MZ/PE/COFF headers as the first thing loaded.
    # Use the first function in our EFI stub as the entry point.
    entry_addr = get_sym_val(elf_dbg, r"uk_efi_entry64")

    # PE is loaded by sections, thus the fake PE Header encodes the PT_LOAD's
    # as PE sections with all permissions enabled (RWX)
    with open(opt.elf, "r+b") as f:
        elf_file = f.read()

        # Get Machine
        if sum(elf_file[18:20]) == ELF_MACHINE["arm64"]:
            MZ_PE_HDR["Machine"][0] = PE_MACHINE["arm64"]
        elif sum(elf_file[18:20]) == ELF_MACHINE["x86_64"]:
            MZ_PE_HDR["Machine"][0] = PE_MACHINE["x86_64"]
        else:
            raise Exception("Unknown architecture. See ELF_MACHINE/PE_MACHINE")

        MZ_PE_HDR["NumberOfSections"][0] = len(ld_phdrs)

        MZ_PE_HDR["ImageBase"][0] = base_addr

        # SizeOfHeaders should be everything in the binary before the actual
        # loadable PE sections start. So:
        # 1. Size of all enumerated loadable PE section headers obtained from
        # loadable ELF segments
        MZ_PE_HDR["SizeOfHeaders"][0] = PE_SEC_SZ * (len(ld_phdrs) + 1)
        # 2. Size of all PE/COFF headers fields from the specification
        MZ_PE_HDR["SizeOfHeaders"][0] += 200  # See MZ_PE_HDR Offset comments
        # 3. Respect SectionAlignment PE/COFF field: PAGE_ALIGN_DOWN
        MZ_PE_HDR["SizeOfHeaders"][0] = MZ_PE_HDR["SizeOfHeaders"][0] & ~0xFFF
        # 4. add + PAGE_SIZE to previous PAGE_ALIGN_DOWN
        MZ_PE_HDR["SizeOfHeaders"][0] += 0x1000

        # Everything is relative to the image because the whole PE is loaded
        # into memory including its headers. So, previously, end_addr was the
        # last relative address of the last loadable PE section. Now update it
        # to also include the headers that will also be loaded into memory.
        end_addr += MZ_PE_HDR["SizeOfHeaders"][0]

        # Same story for entry_addr
        entry_addr += MZ_PE_HDR["SizeOfHeaders"][0]

        MZ_PE_HDR["AddressOfEntryPoint"][0] = entry_addr - base_addr

        MZ_PE_HDR["SizeOfUninitializedData"][0] = end_addr - bss_addr

        MZ_PE_HDR["SizeOfImage"][0] = MZ_PE_HDR["SizeOfHeaders"][0] + (
            end_addr - base_addr
        )

        for lp in ld_phdrs:
            if "E" in lp["Flags"] and "R" in lp["Flags"]:
                MZ_PE_HDR["SizeOfCode"][0] += lp["MemSiz"]
                MZ_PE_HDR["BaseOfCode"][0] = (
                    lp["VirtAddr"] - base_addr + MZ_PE_HDR["SizeOfHeaders"][0]
                )
            elif "R" in lp["Flags"]:
                MZ_PE_HDR["SizeOfInitializedData"][0] += lp["MemSiz"]

        # The loop above also added the virtual size of the loadable segment
        # containing the .bss section, which is uninitialized.
        # Therefore, subtract .bss size.
        MZ_PE_HDR["SizeOfInitializedData"][0] -= end_addr - bss_addr

        for ld_phdr in ld_phdrs:
            MZ_PE_HDR["SectionHeaders"].append(elf_phdr_to_pe_sec(base_addr, ld_phdr))

        # Write the MS-DOS signature and the rest of the PE/COFF header fields
        f.seek(0)
        for field in [k for k in MZ_PE_HDR.keys() if k != "SectionHeaders"]:
            f.write(MZ_PE_HDR[field][0].to_bytes(MZ_PE_HDR[field][1], "little"))

        for s in MZ_PE_HDR["SectionHeaders"]:
            for field in s.keys():
                f.write(s[field][0].to_bytes(s[field][1], "little"))

        # Go to the end of the PE/COFF headers and append the original ELF64
        f.seek(MZ_PE_HDR["SizeOfHeaders"][0])
        f.write(elf_file)


if __name__ == "__main__":
    main()

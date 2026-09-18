#!/usr/bin/env python3

import argparse
import os
import re
import subprocess
from struct import unpack

ELF_MAGIC = {
    "i386": b"\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "x86_64": b"\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00",
}

ELF_MACHINE = {
    "i386": 3,  # EM_386
    "x86_64": 62,  # EM_X86_64
}

ELF64_EHDR_LEN = 64

ELF64_PHDR_LEN = 56

ELF64_SHDR_LEN = 64

# Linux x86 boot header
# https://www.kernel.org/doc/html/v5.4/x86/boot.html
LINUX_x86_64_HDR = {
    "SETUP_SECTS": (0x0000000000000000, 0x01),  # Offset   0x1f1
    "ROOT_FLAGS": (0x0000000000000000, 0x02),  # Offset   0x1f2
    "SYSSIZE": (0x0000000000000000, 0x04),  # Offset   0x1f4
    "RAM_SIZE": (0x0000000000000000, 0x02),  # Offset  0x1f8
    "VID_MODE": (0x0000000000000000, 0x02),  # Offset  0x1fa
    "ROOT_DEV": (0x0000000000000000, 0x02),  # Offset  0x1fc
    "BOOT_FLAG": (0x000000000000AA55, 0x02),  # Offset  0x1fe
    "JUMP": (0x00000000000066EB, 0x02),  # Offset  0x200
    "HEADER": (0x0000000053726448, 0x04),  # Offset  0x202
    "VERSION": (0x000000000000020D, 0x02),  # Offset  0x206
    "REALMODE_SWTCH": (0x0000000000000000, 0x04),  # Offset  0x208
    "START_SYS_SEG": (0x0000000000000000, 0x02),  # Offset  0x20c
    "KERNEL_VERSION": (0x0000000000000000, 0x02),  # Offset  0x20e
    "TYPE_OF_LOADER": (0x0000000000000000, 0x01),  # Offset  0x210
    "LOADFLAGS": (0x0000000000000001, 0x01),  # Offset  0x211
    "SETUP_MOVE_SIZE": (0x0000000000000000, 0x02),  # Offset  0x212
    "CODE32_START": [0x0000000000000000, 0x04],  # Offset  0x214
    "RAMDISK_IMAGE": (0x0000000000000000, 0x04),  # Offset  0x218
    "RAMDISK_SIZE": (0x0000000000000000, 0x04),  # Offset  0x21c
    "BOOTSECT_KLUDGE": (0x0000000000000000, 0x04),  # Offset  0x220
    "HEAP_END_PTR": (0x0000000000000000, 0x02),  # Offset  0x224
    "EXT_LOADER_VER": (0x0000000000000000, 0x01),  # Offset  0x226
    "EXT_LOADER_TYPE": (0x0000000000000000, 0x01),  # Offset  0x227
    "CMD_LINE_PTR": (0x0000000000000000, 0x04),  # Offset  0x228
    "INITRD_ADDR_MAX": (0x0000000037FFFFFF, 0x04),  # Offset  0x22c
    "KERNEL_ALIGNMENT": (0x0000000000000000, 0x04),  # Offset  0x230
    "RELOCATABLE_KERNEL": (0x0000000000000000, 0x01),  # Offset  0x234
    "MIN_ALIGNMENT": (0x0000000000000000, 0x01),  # Offset  0x235
    "XLOADFLAGS": (0x0000000000000000, 0x02),  # Offset  0x236
    "CMDLINE_SIZE": (0x0000000000000000, 0x04),  # Offset  0x238
    "HARDWARE_SUBARCH": (0x0000000000000000, 0x04),  # Offset  0x23c
    "HARDWARE_SUBARCH_DATA": (0x0000000000000000, 0x08),  # Offset  0x240
    "PAYLOAD_OFFSET": (0x0000000000000000, 0x04),  # Offset  0x248
    "PAYLOAD_LENGTH": (0x0000000000000000, 0x04),  # Offset  0x24c
    "SETUP_DATA": (0x0000000000000000, 0x08),  # Offset  0x250
    "PREF_ADDRESS": (0x0000000000000000, 0x08),  # Offset  0x258
    "INIT_SIZE": [0x0000000000000000, 0x04],  # Offset  0x260
    "HANDOVER_OFFSET": (0x0000000000000000, 0x04),  # Offset  0x264
}
LINUX_x86_64_HDR_SIZE = 119

# surely there's a pythonic way for that
def align_up(v, a):
    return ((v) + (a) - 1) & ~((a) - 1)


def autoint(x):
    return int(x, 0)


# Get the absolute value of symbol, as seen through `nm`
def get_sym_val(elf, sym):
    exp = r"^\s*" + r"([a-f0-9]+)" + r"\s+[A-Za-z]\s+" + sym + r"$"
    out = subprocess.check_output(["nm", elf])  # nosec

    re_out = re.findall(exp, out.decode("ASCII"), re.MULTILINE)
    if len(re_out) != 1:
        raise Exception("Found no " + sym + " symbol.")

    return int(re_out[0], 16)

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
    # the Lxboot header, the original binary and the Program headers.
    def prpnd_off(self, barray, lxboot_header_end_off):
        sz = len(barray)
        old_off = int.from_bytes(barray, self.endian)

        new_off = (
            lxboot_header_end_off + old_off
        )
        if sz < (new_off.bit_length() + 7) // 8:
            raise Exception("New size exceeds initial byte-length.")

        return new_off.to_bytes(sz, self.endian)

    def get_phdrs(self, lxboot_header_end_off):
        def update_phdr(phdr):
            new_phdr = bytearray(ELF64_PHDR_LEN)

            new_phdr[0:ELF64_PHDR_LEN] = phdr[0:ELF64_PHDR_LEN] # keep the rest
            new_phdr[8:16] = self.prpnd_off(phdr[8:16], lxboot_header_end_off)  # p_offset

            return new_phdr

        return [update_phdr(p) for p in self.elf64_phdrs]

def main():
    parser = argparse.ArgumentParser()
    # description = "Prepends image with x86 linux boot header."
    parser.add_argument("bin", help="Raw binary to prepend header with.")
    parser.add_argument(
        "elf",
        help="The ELF image the raw binary was derived from. "
        "Required for resolving symbols used in the header.",
    )
    opt = parser.parse_args()

    code32_start = get_sym_val(opt.elf, r"_lxboot_entry32")
    LINUX_x86_64_HDR["CODE32_START"][0] = code32_start

    # Create final image

    # x = padding to lxboot header
    # y = lxboot header
    # z = padding to alignment
    # binary
    total = 3072

    elf64 = Elf64Editor(opt.elf)
    with open(opt.bin, "r+b") as f:
        img = f.read()
        f.seek(0)

        lxboot_header_end_off = LINUX_x86_64_HDR_SIZE + 0x1f1

        phdrs = elf64.get_phdrs(lxboot_header_end_off)

        # Add padding
        x = 0x1f1
        for i in range (0x1f1):
            f.write(b'\x00')
        
        # Add Lxboot header
        y = LINUX_x86_64_HDR_SIZE
        for field in [k for k in LINUX_x86_64_HDR.keys()]:
            f.write(
                LINUX_x86_64_HDR[field][0].to_bytes(
                    LINUX_x86_64_HDR[field][1], "little"
                )
            )

        # Padding to alignment

        lcpu_start32 = get_sym_val(opt.elf, r"lcpu_start32")
        byte_array = lcpu_start32.to_bytes(4, byteorder='little')

                                                                         # .section .text
                                                                         # .org = 0
                                                                         #
                                                                         # .code16
                                                                         # start:
        trampoline = b'\xEB\x18'                                         #      jmp lxboot_start16
                                                                         #
                                                                         # .globl gdt32
                                                                         # gdt32:
                                                                         # /* We can repurpose the null segment to encode the GDT pointer because
                                                                         #  * the null segment is not accessed by the processor in 32-bit mode. We also
                                                                         #  * want the GDT in front of the code so that we can more easily reference the
                                                                         #  * symbols. So we use the two spare bytes to just jump over the GDT.
                                                                         #  */
                                                                         # gdt32_null:
        trampoline += b'\x00\x00'                                        #     .word    0x0000
                                                                         # .globl gdt32_ptr
                                                                         # gdt32_ptr:
        trampoline += b'\x17\x00'                                        #     .word    (gdt32_end - gdt32 - 1)    /* size - 1    */
        trampoline += b'\x6A\x02\x01\x00'                                #     .long 0x1026a    /* GDT address    */
                                                                         # gdt32_cs:
        trampoline += b'\xFF\xFF\x00\x00\x00\x9B\xCF\x00'                #     .quad    0x00cf9b000000ffff    /* 32-bit CS    */
                                                                         # gdt32_ds:
        trampoline += b'\xFF\xFF\x00\x00\x00\x93\xCF\x00'                #     .quad    0x00cf93000000ffff    /* DS        */
                                                                         # gdt32_end:

                                                                         # .code16
                                                                         # lxboot_start16:
                                                                         #     /* Disable interrupts */
        trampoline += b'\xFA'                                            #     cli

                                                                         #     /* Setup protected mode */
        trampoline += b'\x66\xB8\x01\x00\x00\x00'                        #     movl    $1, %eax
        trampoline += b'\x0F\x22\xC0'                                    #     movl    %eax, %cr0

                                                                         #     /* Load 32-bit GDT and jump into 32-bit code segment */
        trampoline += b'\x66\x0F\x01\x16\x6C\x02'                        #     lgdtl 0x26c

        trampoline += b'\x66\xB8\x10\x00\x00\x00'                        #     movl    $(gdt32_ds - gdt32), %eax
        trampoline += b'\x8E\xD8'                                        #     movl    %eax, %ds
        trampoline += b'\x8E\xD0'                                        #     movl    %eax, %ss
        trampoline += b'\x8E\xC0'                                        #     movl    %eax, %es
        trampoline += b'\x66\x31\xC0'                                    #     xorl    %eax, %eax
        trampoline += b'\x8E\xE0'                                        #     movl    %eax, %fs
        trampoline += b'\x8E\xE8'                                        #     movl    %eax, %gs

        trampoline = trampoline + b'\x66\xEA' + byte_array + b'\x08\x00' #     data32 ljmp $(gdt32_cs - gdt32), $<addr of lcpu32>

        f.write(trampoline)
        for i in range (total - x - y - len(trampoline)):
            f.write(b'\x00')

        # Binary

        f.write(img)

        # init_size

        init_size = 0
        first_vaddr = -1
        last_vaddr = -1
        last_memsz = -1
        for p in phdrs:
            if (int.from_bytes(p[0:4], byteorder='little', signed=False) == 0x1):
                if (first_vaddr == -1):
                    first_vaddr = int.from_bytes(p[16:24], byteorder='little', signed=False)
                last_vaddr = int.from_bytes(p[16:24], byteorder='little', signed=False)
                last_memsz = int.from_bytes(p[40:48], byteorder='little', signed=False)
        
        init_size = last_vaddr + last_memsz - first_vaddr
        LINUX_x86_64_HDR["INIT_SIZE"][0] = init_size

if __name__ == "__main__":
    main()

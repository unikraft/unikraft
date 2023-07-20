#!/usr/bin/env python3

import argparse
import os
import sys

# Linux arm64 boot header
# https://www.kernel.org/doc/Documentation/arm64/booting.txt
LINUX_ARM64_HDR = {
    "CODE0"                   : [0x0000000091005a4d, 0x04],  # Offset   0
    "CODE1"                   : [0x0000000000000000, 0x04],  # Offset   4
    "LOAD_OFFS"               : [0x0000000000000000, 0x08],  # Offset   8
    "IMAGE_SIZE"              : [0x0000000000000000, 0x08],  # Offset  16
    "KERNEL_FLAGS"            : [0x0000000000000000, 0x08],  # Offset  24
    "RES2"                    : [0x0000000000000000, 0x08],  # Offset  32
    "RES3"                    : [0x0000000000000000, 0x08],  # Offset  40
    "RES4"                    : [0x0000000000000000, 0x08],  # Offset  48
    "MAGIC"                   : [0x00000000644d5241, 0x04],  # Offset  56
    "RES5"                    : [0x0000000000000040, 0x04],  # Offset  60
}
LINUX_ARM64_HDR_SIZE = 64

# surely there's a pythonic way for that
def align_up(v, a):
    return (((v) + (a)-1) & ~((a)-1))

def autoint(x):
    return int(x, 0)

def main():
    parser = argparse.ArgumentParser()
    description='Prepends image with linux arm64 boot header'
    parser.add_argument("ram_base", type=autoint, help='ram base')
    parser.add_argument("img_base", type=autoint, help='image base')
    parser.add_argument("entry", type=autoint, help='entry point')
    parser.add_argument('bin', help='path to raw arm64 binary')
    opt = parser.parse_args()

    # code1
    #
    # This contains an unconditional branch to the entry point.
    # The encoding of the branch instruction according to the
    # Arm ARM (DDI0487I.a) is:
    #
    # 31           25                            0
    # ┌───────────┬───────────────────────────────┐
    # │0 0 0 1 0 1│           imm26               │
    # └───────────┴───────────────────────────────┘
    #
    # b <offs>
    #
    # where offs is encoded as "imm26" times 4, relatively
    # to the branch instruction
    #
    pc = opt.img_base - LINUX_ARM64_HDR_SIZE + 4
    offs = opt.entry - pc

    # offset must be <= +128M. We don't accept negative offsets
    # here as the header always prepends the image.
    assert((offs) <= ((1 << 26) / 2 - 1))

    LINUX_ARM64_HDR["CODE1"][0] = ((0b101 << 26) | (int(offs / 4)))

    # load_offset
    #
    # This includes the header, so we subtract the header size
    LINUX_ARM64_HDR["LOAD_OFFS"][0] = (opt.img_base - opt.ram_base) - LINUX_ARM64_HDR_SIZE

    # kernel_flags
    #
    # For now assume little-endian, 4KiB pages, image anywhere in memory
    LINUX_ARM64_HDR["KERNEL_FLAGS"][0] = 0xa

    # image_size
    #
    # We arbitrarily set this to header size + image size aligned up to 2MiB
    total_size = os.path.getsize(opt.bin) + LINUX_ARM64_HDR_SIZE
    LINUX_ARM64_HDR["IMAGE_SIZE"][0] = align_up(total_size, 2 * 1024 * 1024)

    # Create img
    with open(opt.bin, 'r+b') as f:
        img = f.read()
        f.seek(0)
        for field in [k for k in LINUX_ARM64_HDR.keys() if k != "SectionHeaders"]:
            f.write(LINUX_ARM64_HDR[field][0].to_bytes(LINUX_ARM64_HDR[field][1], 'little'))
        f.write(img)

if __name__ == '__main__':
    main()

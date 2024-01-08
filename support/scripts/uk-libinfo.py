#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

# NOTE: The script requires pyelftools (pip3 install)
import sys
import argparse
from elftools.elf.elffile import ELFFile


class UKLibraryInfoRecord:
    def __init__(self, type, data, byteorder):
        self._tnum = int(type)
        self._tname = None
        self._tdesc = None
        self._data = None  # Tuple: (is_raw, decoded_data)
        match int(self._tnum):
            #
            # NOTE: This list needs to kept in sync with
            #       lib/uklibid/include/uk/libid/info.h
            #
            case 0x0001:
                self._tname = "LIBNAME"
                self._tdesc = "Library name"
                self._data = self._data_decode(data, "ascii")
            case 0x0002:
                self._tname = "COMMENT"
                self._tdesc = "Comment"
                self._data = self._data_decode(data, "ascii")
            case 0x0003:
                self._tname = "VERSION"
                self._tdesc = "Version"
                self._data = self._data_decode(data, "ascii")
            case 0x0004:
                self._tname = "LICENSE"
                self._tdesc = "License"
                self._data = self._data_decode(data, "ascii")
            case 0x0005:
                self._tname = "GITDESC"
                self._tdesc = "Git version"
                self._data = self._data_decode(data, "ascii")
            case 0x0006:
                self._tname = "UKVERSION"
                self._tdesc = "Unikraft version"
                self._data = self._data_decode(data, "ascii")
            case 0x0007:
                self._tname = "UKFULLVERSION"
                self._tdesc = "Unikraft full version"
                self._data = self._data_decode(data, "ascii")
            case 0x0008:
                self._tname = "UKCODENAME"
                self._tdesc = "Unikraft codename"
                self._data = self._data_decode(data, "ascii")
            case 0x0009:
                self._tname = "UKCONFIG"
                self._tdesc = "Unikraft .config"
                self._data = (True, data)
            case 0x000A:
                self._tname = "UKCONFIGGZ"
                self._tdesc = "Unikraft .config.gz"
                self._data = (True, data)
            case 0x000B:
                self._tname = "COMPILER"
                self._tdesc = "Compiler (CC)"
                self._data = self._data_decode(data, "ascii")
            case 0x000C:
                self._tname = "COMPILEDATE"
                self._tdesc = "Compile date"
                self._data = self._data_decode(data, "ascii")
            case 0x000D:
                self._tname = "COMPILEDBY"
                self._tdesc = "Compiled by"
                self._data = self._data_decode(data, "ascii")
            case 0x000E:
                self._tname = "COMPILEDBYASSOC"
                self._tdesc = "Compiled by (association)"
                self._data = self._data_decode(data, "ascii")
            case 0x000F:
                self._tname = "COMPILEOPTS"
                self._tdesc = "Compile options"
                flags = int.from_bytes(data, byteorder)
                decflags = []

                # Decode UKLI_REC_CO_* flags
                if self._flag_isset(flags, 0):
                    flags = self._flag_unset(flags, 0)
                    decflags.append("PIE")
                if self._flag_isset(flags, 1):
                    flags = self._flag_unset(flags, 1)
                    decflags.append("DCE")
                if self._flag_isset(flags, 2):
                    flags = self._flag_unset(flags, 2)
                    decflags.append("LTO")
                if flags != 0x0:
                    decflags.append("UNKNOWN-0x{:01x}".format(flags))
                self._data = (False, ", ".join(decflags))
            case _:
                self._tname = int(type)
                self._tdesc = "Unknown record"
                self._data = (True, data)

    def _data_decode(self, data, encoding="ascii"):
        try:
            return (False, data.decode(encoding))
        except UnicodeDecodeError:
            # decoding failed, treat as raw data
            return (True, data)

    def _flag_isset(self, flags, flagpos):
        return flags & (0x1 << flagpos)

    def _flag_unset(self, flags, flagpos):
        return flags ^ (0x1 << flagpos)

    def type(self):
        return self._tnum

    def type_name(self):
        return self._tname

    def type_description(self):
        return self._tdesc

    def data(self):
        return self._data[1]

    def data_is_raw(self):
        return self._data[0]

    def str_data(self):
        if self.data_is_raw():
            str_data = "<binary data>"
        else:
            str_data = str(self.data())
        return str_data

    def __str__(self):
        return "{:}: {:}".format(self._tdesc, self.str_data())


class UKLibraryInfoHeader:
    def __init__(self, version, byteorder):
        self._version = version
        self._libname = None
        self._recs = []

    def add_rec(self, rec):
        if self._libname is None and rec.type() == 0x0001:  # LIBNAME
            self._libname = rec.data()
        else:
            self._recs.append(rec)

    def __str__(self):
        ret = ""
        if self._libname is None:
            for rec in self._recs:
                str_type = "{:}:".format(rec.type_description())
                ret += "{:30}{:}\n".format(str_type, rec.str_data())
        else:
            ret = "{:}:\n".format(self._libname)
            for rec in self._recs:
                str_type = "{:}:".format(rec.type_description())
                ret += "  {:28}{:}\n".format(str_type, rec.str_data())
        return ret

    def get_recs(self):
        return self._recs

    def libname(self):
        return self._libname


def parse_uklibinfo(data, byteorder=sys.byteorder):
    if not data:
        raise Exception("No uklibinfo data found for parsing")
    hdr_hdr_len = 4 + 2  # sizeof(__u32) + sizeof(__u16)
    rec_hdr_len = 2 + 4  # sizeof(__u16) + sizeof(__u32)
    seek = 0
    left = len(data)
    found = []

    # We seek and remember the number of left bytes so that
    # we avoid copying sub data
    while left >= hdr_hdr_len:
        # struct uk_libid_info_hdr
        hdr_len = int.from_bytes(data[seek : seek + 4], byteorder)
        seek += 4  # sizeof(__u32 len)
        if hdr_len > left:
            raise Exception("Invalid header size at byte position {}".format(seek))
        hdr_version = int.from_bytes(data[seek : seek + 2], byteorder)
        seek += 2  # sizeof(__u16 version)
        left -= hdr_len
        rec_seek = seek
        rec_left = hdr_len - hdr_hdr_len
        seek += rec_left

        if not hdr_version == 1:
            sys.stderr.write(
                "Skipping unsupported library information version {}\n".format(
                    hdr_version
                )
            )
            continue

        hdr = UKLibraryInfoHeader(hdr_version, byteorder)
        found.append(hdr)

        # struct uk_libid_info_rec
        while rec_left >= rec_hdr_len:
            rec_type = int.from_bytes(data[rec_seek : rec_seek + 2], byteorder)
            rec_seek += 2  # sizeof(__u16 type)
            rec_len = int.from_bytes(data[rec_seek : rec_seek + 4], byteorder)
            rec_seek += 4  # sizeof(__u32 len)
            if rec_len > rec_left:
                raise Exception(
                    "Invalid record size at byte position {}".format(rec_seek)
                )
            rec_data = data[rec_seek : rec_seek + (rec_len - rec_hdr_len)]
            rec_seek += len(rec_data)
            rec_left -= rec_len

            hdr.add_rec(UKLibraryInfoRecord(rec_type, rec_data, byteorder))
    return found


# Load uklibinfo data from ELF section
def uklibinfo_elf_load(path):
    elf_sec_name = ".uk_libinfo"
    fd = open(path, "rb")
    elffile = ELFFile(fd)
    section = None
    for s in elffile.iter_sections():
        if s.name == elf_sec_name:
            section = s
            break
    if not section:
        raise Exception("Section " + elf_sec_name + " not found")
    uklibinfo = section.data()
    fd.close()
    return uklibinfo


##
## MAIN
##
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Unikraft library metadata tool")
    parser.add_argument("file", help="Kernel or library to investigate")
    (opt, rem_args) = parser.parse_known_args()

    try:
        uklibinfo = uklibinfo_elf_load(opt.file)
    except Exception as e:
        print("{}: Failed to load uklibinfo data: {}".format(opt.file, str(e)))
        sys.exit(1)

    try:
        #
        # FIXME: Derive int sizes and byteorder from ELF headers
        #
        hdrs = parse_uklibinfo(uklibinfo)
    except Exception as e:
        print("{}: Failed to parse uklibinfo data: {}".format(opt.file, str(e)))
        sys.exit(1)

    # Print first global information (header without libname)
    for hdr in hdrs:
        if hdr.libname() is None:
            print(hdr)

    # Then print list of libraries
    for hdr in hdrs:
        if hdr.libname() is not None:
            print(hdr)

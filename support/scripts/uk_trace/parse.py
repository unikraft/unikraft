# SPDX-License-Identifier: BSD-3-Clause */
#
# Authors: Yuri Volchkov <yuri.volchkov@neclab.eu>
#
# Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import binascii
import logging
import re
import struct
import subprocess
from dataclasses import dataclass

logger = logging.getLogger(__name__)

TP_HEADER_MAGIC = b"TRhd"
TP_DEF_MAGIC = b"TPde"
UK_TRACE_ARG_INT = 0
UK_TRACE_ARG_STRING = 1

FORMAT_VERSION = 1

# Per printf(3): %[flags][width][.precision][length]specifier
# Length modifiers: hh, h, l, ll, z, t, j, q
# Signed specifiers: d, i
# Unsigned specifiers: u, o, x, X
# Pointer: p (always unsigned, size = ptr_size)
PRINTF_FMT_SPEC = re.compile(
    r"%%|%"
    r"(?P<flags>[#0\- +']*)"
    r"(?P<width>\*|\d+)?"
    r"(?P<precision>\.(?:\*|\d+))?"
    r"(?P<len>hh|h|ll|l|z|t|j|q)?"
    r"(?P<conv>[cdiouxXsp])"
)


def align_down(v, alignment):
    return v & ~(alignment - 1)


def align_up(v, alignment):
    return align_down(v + alignment - 1, alignment)


@dataclass(frozen=True)
class tp_sample:
    tp: tp_definition
    time: int
    args: list[int | str]

    @property
    def formatted_args(self):
        try:
            return self.tp.fmt % tuple(self.args)
        except TypeError:
            # Mismatch between format string and actual arguments
            return f"{self.args!r} (format error {self.tp.fmt!r})"

    def __str__(self):
        return f"{self.time:016d} {self.tp.name}: {self.formatted_args}"

    def tabulate_fmt(self):
        return [self.time, self.tp.name, self.formatted_args]


class EndOfBuffer(Exception):
    pass


# Parsing of trace buffer is designed to be used without gdb. If ever
# Unikraft will have a bare metal port, it would be complicated to use
# gdb for fetching and parsing runtime data.
#
# However, it is possible anyways to get an address of the statically
# allocated trace buffer using gdb/nm/objdump. And we can use some
# different mechanism to copy memory from the running instance of
# Unikraft (which is yet to be designed).
#
# Similar considerations are applied to parsing trace point
# definitions. We can do that disregarding if it is possible to attach
# gdb to a running instance or not
def sample_parser(keyvals, tp_defs_data, trace_buff, ptr_size):
    if (v := int(keyvals["format_version"])) > FORMAT_VERSION:
        logger.warning(f"Trace in format v{v} (with parser v{FORMAT_VERSION})")

    data = unpacker(trace_buff)
    definitions = get_tp_definitions(tp_defs_data, ptr_size)

    while data.seek_to_pattern(TP_HEADER_MAGIC):
        try:
            _, size, time, cookie = data.unpack("4sLQQ")
            if cookie in definitions:
                yield definitions[cookie].parse_sample(time, data)
            else:
                logger.warning(f"No definition for {cookie=:x}")
                data.read(size)
        except EndOfBuffer:
            break


class unpacker:
    def __init__(self, data):
        self.data = data
        self.pos = 0
        self.endian = "<"

    def __len__(self):
        return len(self.data) - self.pos

    def seek_to_pattern(self, pattern):
        position = self.data.find(pattern, self.pos)
        offset = position - self.pos
        if position < 0 or offset < 0:
            return False

        self.pos += offset
        return True

    def read(self, size):
        if self.pos + size > len(self.data):
            raise EndOfBuffer(f"Buffer too short to read {size} bytes")
        res = self.data[self.pos : self.pos + size]
        self.pos += size
        return res

    def unpack(self, fmt):
        fmt = self.endian + fmt
        size = struct.calcsize(fmt)
        return struct.unpack(fmt, self.read(size))

    def unpack_string(self):
        (strlen,) = self.read(1)
        return self.read(strlen).decode()

    def unpack_int(self, size, signed=False):
        return int.from_bytes(
            self.read(size),
            byteorder="little" if self.endian == "<" else "big",
            signed=signed,
        )

    def align_pos(self, alignment):
        self.pos = align_up(self.pos, alignment)


@dataclass(frozen=True)
class tp_definition:
    name: str
    args_nr: int
    fmt: str
    sizes: list[int]
    types: list[int]
    signed: list[bool] | None = None

    def __post_init__(self):
        if self.signed is None:
            self.signed = [False] * self.args_nr

    def __str__(self):
        return f"{self.name} {self.fmt}"

    def parse_sample(self, time, data):
        args = [
            (
                data.unpack_string()
                if typ == UK_TRACE_ARG_STRING
                else data.unpack_int(size, signed)
            )
            for (typ, size, signed) in zip(
                self.types, self.sizes, self.signed, strict=True
            )
        ]
        return tp_sample(self, time, args)


def parse_tp_definition(data, ptr_size):
    magic = data.read(len(TP_DEF_MAGIC))
    if magic != TP_DEF_MAGIC:
        raise ValueError(f"Expected tp def magic {TP_DEF_MAGIC!r}")

    _, cookie, args_nr, name_len, fmt_len = data.unpack("IQBBB")

    sizes = list(data.read(args_nr))
    types = list(data.read(args_nr))
    # Strange, but terminating '\0' makes a problem if the script
    # is running in the gdb
    name = data.read(name_len).rstrip(b"\x00").decode()
    fmt = data.read(fmt_len).rstrip(b"\x00").decode()

    signs = [False] * args_nr
    args_found = 0

    def pythonize_fmt(m):
        """
        This is a callback for regexp substitution that does 3 things in
        a single pass of re.sub:
        - Find the number of specified format arguments
        - Find sign-ness of integer arguments
        - Tweak format specifiers from C printf to Python
            - %p -> %N0x (where N is the machine size)
            - %'d -> %d (thousands separator is not supported in Python)
        """
        nonlocal args_found

        if m.group() == "%%":
            return "%%"

        if m["width"] == "*":
            # %*d consumes one more argument for the width specifier
            args_found += 1

        if m["precision"] == ".*":
            # %.*d consumes one more argument for the precision specifier
            args_found += 1

        if args_found < args_nr:
            # Get signed-ness from conversion specifier
            signs[args_found] = m["conv"] and m["conv"] in "di"

            if types[args_found] == UK_TRACE_ARG_STRING and m["conv"] != "s":
                logger.warning(
                    f"{name}: Expected string formatter for "
                    f"arg[{1 + args_found}], got {m.group()} instead"
                )
            elif types[args_found] == UK_TRACE_ARG_INT and m["conv"] == "s":
                logger.warning(
                    f"{name}: Expected int formatter for "
                    f"arg[{1 + args_found}], got {m.group()} instead"
                )

        args_found += 1

        # Python does not support "%p", resort to hex at machine ptr size
        if m.group() == "%p":
            return f"0x%0{2*ptr_size}x"

        # Python does not support %'d flag (thousands separator)
        flags = (m["flags"] or "").replace("'", "")

        # Custom pointer specs convert to "%<spec>x"
        if m["conv"] == "p":
            width = m["width"] or f"0{2*ptr_size}"
            return f"%{flags}{width}x"

        # Otherwise return spec
        return f"%{flags}{m['width'] or ''}{m['precision'] or ''}{m['conv']}"

    fmt = re.sub(PRINTF_FMT_SPEC, pythonize_fmt, fmt)
    if args_found != args_nr:
        logger.warning(
            f"{name}: Expected to find {args_nr} argument "
            f"format specifiers, got {args_found} instead"
        )

    return cookie, tp_definition(name, args_nr, fmt, sizes, types, signs)


def get_tp_definitions(tp_data, ptr_size):
    data = unpacker(tp_data)
    ret = {}

    while data.seek_to_pattern(TP_DEF_MAGIC):
        try:
            cookie, tpdef = parse_tp_definition(data, ptr_size)
            ret[cookie] = tpdef
        except EndOfBuffer:
            break

    return ret


def get_tp_sections(elf):
    hexdump_reg = re.compile(r" *0x[0-9a-f]+(?P<hex>( [0-9a-f]{8}){1,4})")
    readelf_cmd = ["readelf", "-x", ".uk_tracepoints_list", elf]
    readelf_output = subprocess.check_output(readelf_cmd).decode()

    hex = "".join(
        m["hex"]
        for line in readelf_output.splitlines()
        if (m := hexdump_reg.match(line))
    ).replace(" ", "")
    return binascii.unhexlify(hex)


def get_keyvals(elf):
    readelf_cmd = ["readelf", "-p", ".uk_trace_keyvals", elf]
    raw_data = subprocess.check_output(readelf_cmd).decode()
    filtered = re.findall(
        r"^\s*\[ *\d+\]\s+(.+) = (.+)$", raw_data, re.MULTILINE
    )
    return dict(filtered)

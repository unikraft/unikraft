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

import struct
import sys
import subprocess
import re
import tempfile
import logging

logger = logging.getLogger(__name__)

TP_HEADER_MAGIC = b"TRhd"
TP_DEF_MAGIC = b"TPde"
UK_TRACE_ARG_INT = 0
UK_TRACE_ARG_STRING = 1

FORMAT_VERSION = 1


def align_down(v, alignment):
    return v & ~(alignment - 1)


def align_up(v, alignment):
    return align_down(v + alignment - 1, alignment)


class tp_sample:
    def __init__(self, tp, time, args):
        self.tp = tp
        self.args = args
        self.time = time

    def __str__(self):
        return ("%016d %s: " % (self.time, self.tp.name)) + (
            self.tp.fmt % self.args
        )

    def tabulate_fmt(self):
        return [self.time, self.tp.name, (self.tp.fmt % self.args)]


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
class sample_parser:
    def __init__(self, keyvals, tp_defs_data, trace_buff, ptr_size):
        if int(keyvals["format_version"]) > FORMAT_VERSION:
            print(
                "Warning: Version of trace format is more recent",
                file=sys.stderr,
            )
        self.data = unpacker(trace_buff)
        self.tps = get_tp_definitions(tp_defs_data, ptr_size)

    def __iter__(self):
        self.data.pos = 0
        return self

    def __next__(self):
        try:
            # TODO: generate format. Cookie can be 4 bytes long on other
            # platforms
            magic, _, time, cookie = self.data.unpack("4sLQQ")
        except EndOfBuffer:
            raise StopIteration

        if magic != TP_HEADER_MAGIC:
            raise StopIteration

        return self.tps[cookie].parse_sample(time, self.data)


class unpacker:
    def __init__(self, data):
        self.data = data
        self.pos = 0
        self.endian = "<"

    def seek_to_pattern(self, pattern):
        position = self.data[self.pos :].find(pattern)
        if position < 0:
            raise ValueError(f"{pattern!r} not found")
        self.pos += position

    def read(self, size):
        res = self.data[self.pos : self.pos + size]
        self.pos += size
        return res

    def unpack(self, fmt):
        fmt = self.endian + fmt
        size = struct.calcsize(fmt)
        if size > len(self.data) - self.pos:
            raise EndOfBuffer("No data in buffer for unpacking %s bytes" % size)
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


class tp_definition:
    def __init__(self, name, args_nr, fmt, sizes, types, signed=None):
        self.name = name
        self.args_nr = args_nr
        self.fmt = fmt
        self.sizes = sizes
        self.types = types
        self.signed = signed if signed is not None else [False] * args_nr

    def __str__(self):
        return "%s %s" % (self.name, self.fmt)

    def parse_sample(self, time, data):
        args = tuple(
            (
                data.unpack_string()
                if typ == UK_TRACE_ARG_STRING
                else data.unpack_int(size, signed)
            )
            for (typ, size, signed) in zip(
                self.types, self.sizes, self.signed, strict=True
            )
        )
        return tp_sample(self, time, args)


def get_tp_definitions(tp_data, ptr_size):
    ptr_fmt = "0x%0" + "%dx" % (ptr_size * 2)
    data = unpacker(tp_data)

    ret = dict()

    data.seek_to_pattern(TP_DEF_MAGIC)

    while True:
        entry_start = data.pos

        try:
            magic, size, cookie, args_nr, name_len, fmt_len = data.unpack(
                "4sIQBBB"
            )
        except EndOfBuffer:
            break

        if magic != TP_DEF_MAGIC:
            raise ValueError("Wrong tracepoint definition magic")

        sizes = data.unpack("B" * args_nr)
        types = data.unpack("B" * args_nr)
        # Strange, but terminating '\0' makes a problem if the script
        # is running in the gdb
        name = data.read(name_len).rstrip(b"\x00").decode()
        fmt = data.read(fmt_len).rstrip(b"\x00").decode()

        # Determine signed-ness of each integer argument from the format string
        # Per printf(3): %[flags][width][.precision][length]specifier
        #
        # Length modifiers: hh, h, l, ll, z, t, j, q
        # Signed specifiers: d, i
        # Unsigned specifiers: u, o, x, X
        # Pointer: p (always unsigned, size = ptr_size)

        _FLAGS = r"(?P<flags>[#0\- +']*)"
        _WIDTH = r"(?P<width>\*|\d+)?"
        _PRECISION = r"(?:\.(?:\*|\d+))?"
        _LEN = r"(?P<len>hh|h|ll|l|z|t|j|q)?"
        _CONV = r"(?P<conv>[diouxXs])"

        # fmt: off
        _FMT_SPEC_RE = re.compile(
            # literal %% (skip, consumes no arg)
            r'%%' +
            # General case: %... specifiers
            r"|%" + _FLAGS + _WIDTH + _PRECISION + _LEN + _CONV +
            # pointer (no length modifier)
            r'|%p'
        )
        # fmt: on

        signs = [False] * args_nr
        args_found = 0

        def pythonize_fmt(m, signs=signs, types=types):
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

            if m.group() == "%p":
                # %p is a special case without modifiers
                args_found += 1
                return ptr_fmt

            if m["width"] == "*":
                # %*d consumes one more argument for the width specifier
                args_found += 1

            signs[args_found] = m["conv"] == "d" or m["conv"] == "i"

            if types[args_found] == UK_TRACE_ARG_STRING and m["conv"] != "s":
                logger.warning(
                    f"Expected string formatter for arg[{1 + args_found}], "
                    f"got {m.group()} instead"
                )

            args_found += 1

            # Python does not support %'d flag (thousands separator)
            flags = (m["flags"] or "").replace("'", "")
            return f"%{flags}{m['width'] or ''}{m['conv']}"

        fmt = re.sub(_FMT_SPEC_RE, pythonize_fmt, fmt)
        if args_found != args_nr:
            logger.warning(
                f"Expected to find {args_nr} argument format specifiers, "
                f"got {args_found} instead"
            )

        ret[cookie] = tp_definition(name, args_nr, fmt, sizes, types, signs)

        # Use size field to jump to next entry
        data.pos = entry_start + size

        # Skip any zero-padding (compiler alignment between entries)
        while data.pos < len(data.data) and data.data[data.pos] == 0:
            data.pos += 1

    return ret


def get_tp_sections(elf):
    # NOTE: When reading a cross-architecture ELF file, make sure the objcopy utility supports that architecture.
    return subprocess.check_output(
        [
            "objcopy",
            "--dump-section",
            ".uk_tracepoints_list=/dev/stdout",
            elf,
            "/dev/null",
        ]
    )


def get_keyvals(elf):
    readelf_cmd = ["readelf", "-p", ".uk_trace_keyvals", elf]
    raw_data = subprocess.check_output(readelf_cmd).decode()
    filtered = re.findall(
        r"^\s*\[ *\d+\]\s+(.+) = (.+)$", raw_data, re.MULTILINE
    )
    return dict(filtered)

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

TP_HEADER_MAGIC = "TRhd"
TP_DEF_MAGIC = "TPde"
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

        magic = magic.decode()
        if magic != TP_HEADER_MAGIC:
            raise StopIteration

        tp = self.tps[cookie]
        args = []
        for i in range(tp.args_nr):
            if tp.types[i] == UK_TRACE_ARG_STRING:
                args += [self.data.unpack_string()]
            else:
                args += [self.data.unpack_int(tp.sizes[i], tp.signed[i])]

        return tp_sample(tp, time, tuple(args))


class unpacker:
    def __init__(self, data):
        self.data = data
        self.pos = 0
        self.endian = "<"

    def unpack(self, fmt):
        fmt = self.endian + fmt
        size = struct.calcsize(fmt)
        if size > len(self.data) - self.pos:
            raise EndOfBuffer("No data in buffer for unpacking %s bytes" % size)
        cur = self.data[self.pos : self.pos + size]
        self.pos += size
        return struct.unpack(fmt, cur)

    def unpack_string(self):
        (strlen,) = self.unpack("B")
        fmt = "%ds" % strlen
        (ret,) = self.unpack(fmt)
        return ret.decode()

    def unpack_int(self, size, signed=False):
        if size == 1:
            fmt = "b" if signed else "B"
        elif size == 2:
            fmt = "h" if signed else "H"
        elif size == 4:
            fmt = "i" if signed else "I"
        elif size == 8:
            fmt = "q" if signed else "Q"
        (ret,) = self.unpack(fmt)
        return ret

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


def get_tp_definitions(tp_data, ptr_size):
    ptr_fmt = "0x%0" + "%dx" % (ptr_size * 2)
    data = unpacker(tp_data)

    ret = dict()

    # Find the first entry by scanning for the TP_DEF_MAGIC string
    while data.pos < len(data.data) - 4:
        if data.data[data.pos:data.pos+4] == TP_DEF_MAGIC.encode():
            break
        data.pos += 1

    while True:
        entry_start = data.pos

        try:
            magic, size, cookie, args_nr, name_len, fmt_len = data.unpack(
                "4sIQBBB"
            )
        except EndOfBuffer:
            break

        magic = magic.decode()
        if magic != TP_DEF_MAGIC:
            raise Exception("Wrong tracepoint definition magic")

        sizes = data.unpack("B" * args_nr)
        types = data.unpack("B" * args_nr)
        name, fmt = data.unpack("%ds%ds" % (name_len, fmt_len))
        # Strange, but terminating '\0' makes a problem if the script
        # is running in the gdb
        name = name[:-1].decode()
        fmt = fmt[:-1].decode()

        # Determine signed-ness of each integer argument from the format string
        # Per printf(3): %[flags][width][.precision][length]specifier
        #
        # Length modifiers: hh, h, l, ll, z, t, j, q
        # Signed specifiers: d, i
        # Unsigned specifiers: u, o, x, X
        # Pointer: p (always unsigned, size = ptr_size)

        _FLAGS = r"[#0\- +']*"
        _WIDTH = r'(?:\*|\d+)?'
        _PRECISION  = r'(?:\.(?:\*|\d+))?'
        _LEN   = r'(?:hh|h|ll|l|z|t|j|q)?'
        _FWP   = _FLAGS + _WIDTH + _PRECISION  # flags + width + precision (optional)

        _FMT_SPEC_RE = re.compile(
            r'%%'                       # literal %% (skip, consumes no arg) (group 0)
            r'|%' + _FWP +             # optional (group 0)
            r'(hh|h|ll|l|z|t|j|q)?'    # length modifier (group 1)
            r'([diouxXs])'              # conversion specifier (group 2)
            r'|%p'                      # pointer (no length modifier) (group 0)
        )

        # Validate: the number of format specifiers must match args_nr
        num_specifiers = sum(
            1 for m in _FMT_SPEC_RE.finditer(fmt)
            if m.group(0) != '%%'
        )
        assert num_specifiers == args_nr, (
            "Format specifier count mismatch: fmt has %d, args_nr is %d for '%s'"
            % (num_specifiers, args_nr, name)
        )

        # Now safely determine signed-ness for each argument
        is_arg_signed = [False for _ in range(args_nr)]
        fmt_tmp = fmt
        for i in range(args_nr):
            # Skip any literal %% before looking for the real specifier
            while True:
                m = _FMT_SPEC_RE.search(fmt_tmp)
                if not m:
                    break
                if m.group(0) == '%%':
                    fmt_tmp = fmt_tmp[m.end():]
                    continue
                break

            if types[i] == UK_TRACE_ARG_STRING:
                if m and m.group(2) == 's':
                    fmt_tmp = fmt_tmp[m.end():]
            else:
                if m:
                    conv = m.group(2)  # None if matched %p branch
                    if conv and conv != 's':
                        is_arg_signed[i] = conv in ('d', 'i')
                    else: # %p case
                        is_arg_signed[i] = False
                    fmt_tmp = fmt_tmp[m.end():]

        # Convert C printf format specifiers into Python equivalents.
        # Strip length modifiers while preserving flags, width, and precision.
        _FWP_CAP = '(' + _FLAGS + _WIDTH + _PRECISION + ')'  # capturing version

        # Hex/octal: strip length modifier, keep the rest
        fmt = re.sub(r'%' + _FWP_CAP + _LEN + r'([xXo])', r'%\1\2', fmt)

        # Decimal (d, i, u): strip length modifier, normalize to %d
        fmt = re.sub(r'%' + _FWP_CAP + _LEN + r'[diu]', r'%\1d', fmt)

        # %p has no length modifier per spec; simple replace at the end
        fmt = fmt.replace("%p", ptr_fmt)

        ret[cookie] = tp_definition(name, args_nr, fmt, sizes, types,
                                    is_arg_signed)

        # Use size field to jump to next entry
        data.pos = entry_start + size

        # Skip any zero-padding (compiler alignment between entries)
        while data.pos < len(data.data) and data.data[data.pos] == 0:
            data.pos += 1

    return ret


def get_tp_sections(elf):
    f = tempfile.NamedTemporaryFile()
    # NOTE: When reading a cross-architecture ELF file, make sure the objcopy utility supports that architecture.
    objcopy_cmd = ["objcopy",
                   "--dump-section", ".uk_tracepoints_list=" + f.name,
                   elf, "/dev/null"]
    subprocess.check_call(objcopy_cmd)
    return f.read()


def get_keyvals(elf):
    readelf_cmd = "readelf -p .uk_trace_keyvals %s" % elf
    readelf_cmd = readelf_cmd.split()
    raw_data = subprocess.check_output(readelf_cmd).decode()
    filtered = re.findall(
        r"^\s*\[ *\d+\]\s+(.+) = (.+)$", raw_data, re.MULTILINE
    )

    ret = dict()
    for key, val in filtered:
        ret[key] = val

    return ret

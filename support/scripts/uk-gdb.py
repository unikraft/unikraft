#!/user/bin/env python3
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
#
# THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.

import gdb
import pickle
import os, sys
import tempfile, shutil

scripts_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(scripts_dir)

import uk_trace.parse as parse

type_char = gdb.lookup_type('char')
type_void = gdb.lookup_type('void')

PTR_SIZE = type_void.pointer().sizeof

def get_trace_buffer():
    inf = gdb.selected_inferior()

    try:
        trace_buff = gdb.parse_and_eval('uk_trace_buffer')
        trace_buff_size = trace_buff.type.sizeof
        trace_buff_addr = int(trace_buff.address)
        trace_buff_writep = int(gdb.parse_and_eval('uk_trace_buffer_writep'))
    except gdb.error:
        gdb.write("Error getting the trace buffer. Is tracing enabled?\n")
        raise gdb.error

    if (trace_buff_writep == 0):
        # This can happen as effect of compile optimization if none of
        # tracepoints were called
        used = 0
    else:
        used = trace_buff_writep - trace_buff_addr

    return bytes(inf.read_memory(trace_buff_addr, used))

def save_traces(out):
    elf = gdb.current_progspace().filename

    pickler = pickle.Pickler(out)

    # keyvals need to go first, because they have format_version
    # key. Even if the format is changed we must guarantee that at
    # least keyvals are always stored first. However, ideally, next
    # versions should just have modifications at the very end to keep
    # compatibility with previously collected data.
    pickler.dump(parse.get_keyvals(elf))
    pickler.dump(elf)
    pickler.dump(PTR_SIZE)
    # We are saving raw trace buffer here. Another option is to pickle
    # already parsed samples. But in the chosen case it is a lot
    # easier to debug the parser, because python in gdb is not very
    # convenient for development.
    pickler.dump(parse.get_tp_sections(elf))
    pickler.dump(get_trace_buffer())

class uk(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, 'uk',
                             gdb.COMMAND_USER, gdb.COMPLETE_COMMAND, True)

class uk_trace(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, 'uk trace',
                             gdb.COMMAND_USER, gdb.COMPLETE_COMMAND, True)
    def invoke(self, arg, from_tty):
        elf = gdb.current_progspace().filename
        samples = parse.sample_parser(parse.get_keyvals(elf),
                                      parse.get_tp_sections(elf),
                                      get_trace_buffer(), PTR_SIZE)
        for sample in samples:
            print(sample)


class uk_trace_save(gdb.Command):
    def __init__(self):
        gdb.Command.__init__(self, 'uk trace save',
                             gdb.COMMAND_USER, gdb.COMPLETE_COMMAND)
    def invoke(self, arg, from_tty):
        if not arg:
            gdb.write('Missing argument. Usage: uk trace save <filename>\n')
            return

        gdb.write('Saving traces to %s ...\n' % arg)

        with tempfile.NamedTemporaryFile() as out:
            save_traces(out)
            out.flush()
            shutil.copyfile(out.name, arg)

uk()
uk_trace()
uk_trace_save()

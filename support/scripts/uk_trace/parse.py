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

import subprocess
import re
import tempfile

def get_tp_sections(elf):
    f = tempfile.NamedTemporaryFile()
    objcopy_cmd  = 'objcopy -O binary %s ' % elf
    objcopy_cmd += '--only-section=.uk_tracepoints_list ' + f.name
    objcopy_cmd = objcopy_cmd.split()
    subprocess.check_call(objcopy_cmd)
    return f.read()

def get_keyvals(elf):
    readelf_cmd = 'readelf -p .uk_trace_keyvals %s' % elf
    readelf_cmd = readelf_cmd.split()
    raw_data = subprocess.check_output(readelf_cmd).decode()
    filtered = re.findall(r'^\s*\[ *\d+\]\s+(.+) = (.+)$', raw_data,
                          re.MULTILINE)

    ret = dict()
    for key,val in filtered:
        ret[key] = val

    return ret

#!/usr/bin/env python3
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

import click
import os, sys
import pickle
import subprocess
from tabulate import tabulate

import parse

@click.group()
def cli():
    pass

def parse_tf(trace_file):
    try:
        with open(trace_file, 'rb') as tf:
            unpickler = pickle.Unpickler(tf)

            keyvals = unpickler.load()
            elf = unpickler.load()
            ptr_size = unpickler.load()
            tp_defs = unpickler.load()
            trace_buff = unpickler.load()
    except EOFError:
        print("Unexpected end of trace file", file=sys.stderr)
        quit(-1)
    except Exception as inst:
        print("Problem occurred during reading the tracefile: %s" % str(inst))
        quit(-1)

    return parse.sample_parser(keyvals, tp_defs, trace_buff, ptr_size)

@cli.command()
@click.argument('trace_file', type=click.Path(exists=True), default='tracefile')
@click.option('--no-tabulate', is_flag=True,
              help='No pretty printing')
def list(trace_file, no_tabulate):
    """Parse binary trace file fetched from Unikraft"""
    if not no_tabulate:
        print_data = [x.tabulate_fmt() for x in parse_tf(trace_file)]
        print(tabulate(print_data, headers=['time', 'tp_name', 'msg']))
    else:
        for i in parse_tf(trace_file):
            print(i)

@cli.command()
@click.argument('uk_img', type=click.Path(exists=True))
@click.option('--out', '-o', type=click.Path(),
              default='tracefile', show_default=True,
              help='Output binary file')
@click.option('--remote', '-r', type=click.STRING,
              default=':1234', show_default=True,
              help='How to connect to the gdb session '+
              '(parameters for "target remote" command)')
@click.option('--list', 'do_list', is_flag=True,
              default=False,
              help='Parse the fetched tracefile and list events')
@click.option('--verbose', is_flag=True, default=False)
def fetch(uk_img, out, remote, do_list, verbose):
    """Fetch binary trace file from Unikraft (using gdb)"""

    if os.path.exists(out):
        os.remove(out)

    helper_path = os.path.abspath(uk_img) + '-gdb.py'
    gdb_cmd = ['gdb', '-nh', '-batch',
               click.format_filename(uk_img),
               '-iex', 'add-auto-load-safe-path ' + helper_path,
               '-ex', 'target remote ' + remote,
               '-ex', 'uk trace save ' + out
    ]

    proc = subprocess.Popen(gdb_cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT
    )
    _stdout, _ = proc.communicate()
    _stdout = _stdout.decode()
    if proc.returncode or not os.path.exists(out):
        print(_stdout)
        sys.exit(1)
    if verbose:
        print(_stdout)

    if do_list:
        for i in parse_tf(out):
            print(i)

if __name__ == '__main__':
    cli()

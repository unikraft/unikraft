#===============================================================================
# Copyright 2017-2021 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#===============================================================================

#
# Intel(R) Integrated Performance Primitives Cryptography (Intel(R) IPP Cryptography)
#

import re
import sys
import os
import hashlib
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--header', action='store', required=True, help='Intel IPP Cryptography dispatcher will be generated for fucntions in Header')
parser.add_argument('-o', '--out-directory', action='store', required=True, help='Output folder for generated files')
parser.add_argument('-l', '--cpu-list', action='store', required=True, help='Actual CPU list: semicolon separated string')
parser.add_argument('-c', '--compiler', action='store', required=True, help='Compiler')
args = parser.parse_args()
Header = args.header
OutDir = args.out_directory
cpulist = args.cpu_list.split(';')
compiler = args.compiler

headerID= False      ## Header ID define to avoid multiple include like: #if !defined( __IPPCP_H__ )

from gen_disp_common import readNextFunction

HDR= open( Header, 'r' )
h= HDR.readlines()
HDR.close()


## keep filename only
(incdir, Header)= os.path.split(Header)

## original header name to declare external functions as internal for dispatcher
OrgH= Header

isFunctionFound = True
curLine = 0
FunType = ""
FunName = ""
FunArg = ""

if(compiler == "GNU" or compiler == "Clang"):

    while (isFunctionFound == True):

        result = readNextFunction(h, curLine, headerID)

        curLine         = result['curLine']
        FunType         = result['FunType']
        FunName         = result['FunName']
        FunArg          = result['FunArg']
        isFunctionFound = result['success']

        if (isFunctionFound == True):

            ##################################################
            ## create dispatcher files ASM
            ##################################################
            ASMDISP= open( os.sep.join([OutDir, "jmp_" + FunName+"_" + hashlib.sha512(FunName.encode('utf-8')).hexdigest()[:8] +".asm"]), 'w' )

            # Symbol type setting for extern functions initially appeared in version 2.15
            ASMDISP.write("%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))\n");
            ASMDISP.write("  %xdefine elf_symbol_type :function\n");
            ASMDISP.write("%else\n");
            ASMDISP.write("  %xdefine elf_symbol_type\n");
            ASMDISP.write("%endif\n");
            for cpu in cpulist:
                ASMDISP.write("extern "+cpu+"_"+FunName+"%+elf_symbol_type\n")

            ASMDISP.write("extern ippcpJumpIndexForMergedLibs\n")
            ASMDISP.write("extern ippcpSafeInit%+elf_symbol_type\n\n")
            ASMDISP.write("""
segment .data
align 8
dq  .Lin_{FunName}
.Larraddr_{FunName}:
""".format(FunName=FunName))

            for cpu in cpulist:
                ASMDISP.write("    dq "+cpu+"_"+FunName+"\n")

            ASMDISP.write("""
segment .text
global {FunName}:function ({FunName}.LEnd{FunName} - {FunName})
.Lin_{FunName}:
    {endbr64}
    call ippcpSafeInit wrt ..plt
    align 16

{FunName}:
    {endbr64}
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_{FunName}]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEnd{FunName}:
""".format(FunName=FunName, endbr64='db 0xf3, 0x0f, 0x1e, 0xfa'))
            ASMDISP.close()

else:
    while (isFunctionFound == True):

        result = readNextFunction(h, curLine, headerID)

        curLine         = result['curLine']
        FunType         = result['FunType']
        FunName         = result['FunName']
        FunArg          = result['FunArg']
        isFunctionFound = result['success']

        if (isFunctionFound == True):

            ##################################################
            ## create dispatcher files: C file with inline asm
            ##################################################
            DISP= open( os.sep.join([OutDir, "jmp_"+FunName+"_" + hashlib.sha512(FunName.encode('utf-8')).hexdigest()[:8] + ".c"]), 'w' )

            DISP.write("""#include "ippcp.h"\n\n#pragma warning(disable : 1478 1786) // deprecated\n\n""")

            DISP.write("typedef "+FunType+" (*IPP_PROC)"+FunArg+";\n\n")
            DISP.write("extern int ippcpJumpIndexForMergedLibs;\n")
            DISP.write("extern IppStatus ippcpSafeInit( void );\n\n")

            DISP.write("extern "+FunType+" in_"+FunName+FunArg+";\n")

            for cpu in cpulist:
                DISP.write("extern "+FunType+" "+cpu+"_"+FunName+FunArg+";\n")

            DISP.write("static IPP_PROC arraddr[] =\n{{\n	(IPP_PROC)in_{}".format(FunName))

            for cpu in cpulist:
                DISP.write(",\n	(IPP_PROC)"+cpu+"_"+FunName+"")

            DISP.write("\n};")

            DISP.write("""
#undef  IPPAPI
#define IPPAPI(type,name,arg) __declspec(naked) type name arg
IPPAPI({FunType}, {FunName},{FunArg})
{{
    register unsigned long long i __asm__("rax");
    i = (unsigned long long)arraddr[ippcpJumpIndexForMergedLibs+1];
    _asm{{ jmp rax }}
}};
IPPAPI({FunType}, in_{FunName},{FunArg})
{{
   __asm{{
        call ippcpSafeInit
        mov  rax, qword ptr [{FunName}]
        jmp  rax
  }}
}};
""".format(FunType=FunType, FunName=FunName, FunArg=FunArg))

            DISP.close()

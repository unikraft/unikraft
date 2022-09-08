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
FunName = ""
FunArg = ""

if(compiler == "GNU" or compiler == "Clang"):
      while (isFunctionFound == True):

            result = readNextFunction(h, curLine, headerID)

            curLine         = result['curLine']
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
                  ASMDISP.write("extern ippcpInit%+elf_symbol_type\n\n")
                  ASMDISP.write("""
segment .data
align 4

dd  in_{FunName}
arraddr_{FunName}:

""".format(FunName=FunName))
                  size = 4
                  for cpu in cpulist:
                        size = size + 4
                        ASMDISP.write("    dd "+cpu+"_"+FunName+"\n")

                  ASMDISP.write("""

segment .text
global {FunName}:function ({FunName}.LEnd{FunName} - {FunName})
in_{FunName}:
    {endbr32}
    call  ippcpInit
    align 16
{FunName}:
    {endbr32}
    mov   eax, dword [ippcpJumpIndexForMergedLibs]
    jmp   dword [rel arraddr_{FunName} + eax*4]
.LEnd{FunName}:
""".format(FunName=FunName, size=size, endbr32='db 0xf3, 0x0f, 0x1e, 0xfb'))
            ASMDISP.close()
else:
      while (isFunctionFound == True):

            result = readNextFunction(h, curLine, headerID)

            curLine         = result['curLine']
            FunName         = result['FunName']
            FunArg          = result['FunArg']
            isFunctionFound = result['success']

            if (isFunctionFound == True):

                  ##################################################
                  ## create dispatcher files: C file with inline asm
                  ##################################################
                  DISP= open( os.sep.join([OutDir, "jmp_"+FunName+"_" + hashlib.sha512(FunName.encode('utf-8')).hexdigest()[:8] + ".c"]), 'w' )

                  DISP.write("""#include "ippcpdefs.h"\n\n""")

                  DISP.write("typedef void (*IPP_PROC)(void);\n\n")
                  DISP.write("extern int ippcpJumpIndexForMergedLibs;\n")
                  DISP.write("extern IPP_CALL ippcpInit();\n\n")

                  DISP.write("extern IppStatus in_"+FunName+FunArg+";\n")

                  for cpu in cpulist:
                        DISP.write("extern IppStatus "+cpu+"_"+FunName+FunArg+";\n")

                  DISP.write("""
__asm( "  .data");
__asm( "    .align 4");
__asm( "arraddr:");
__asm( "    .long	in_{FunName}");""".format(FunName=FunName))
                  size = 4
                  for cpu in cpulist:
                        size = size + 4
                        DISP.write("""\n__asm( "    .long	{cpu}_{FunName}");""".format(FunName=FunName, cpu=cpu))

                  DISP.write("""
__asm( "    .type	arraddr,@object");
__asm( "    .size	arraddr,{size}");
__asm( "  .data");\n""".format(size=size))

                  DISP.write("""
#undef  IPPAPI
#define IPPAPI(type,name,arg) __declspec(naked) void name arg
IPPAPI(IppStatus, {FunName},{FunArg})
{{
    __asm( ".L0: mov ippcpJumpIndexForMergedLibs, %eax");
    __asm( "mov (arraddr+4)(,%eax,4), %eax" );
    __asm( "jmp *%eax" );
    __asm( ".global in_{FunName}" );
    __asm( "in_{FunName}:" );
    {endbr32}
    __asm( "call ippcpInit" );
    __asm( "jmp .L0" );
}};
""".format(FunName=FunName, FunArg=FunArg, endbr32='__asm( ".byte 0xf3, 0x0f, 0x1e, 0xfb" );'))

            DISP.close()


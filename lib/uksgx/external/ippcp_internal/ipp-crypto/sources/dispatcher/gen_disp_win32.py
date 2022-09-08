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
parser.add_argument('-c', '--compiler', action='store', help='Compiler') # is not used
args = parser.parse_args()
Header = args.header
OutDir = args.out_directory
cpulist = args.cpu_list.split(';')

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
FunType = ""
FunArg = ""

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

      DISP.write("""#include "ippcpdefs.h" """)

      DISP.write("\n\ntypedef " + FunType + "(*IPP_PROC)(void);\n\n")
      DISP.write("extern int ippcpJumpIndexForMergedLibs;\n")
      DISP.write("extern IppStatus IPP_CALL ippcpInit();\n\n")

      DISP.write("extern " + FunType + " IPP_CALL in_"+FunName+FunArg+";\n")

      for cpu in cpulist:
         DISP.write("extern " + FunType + " IPP_CALL "+cpu+"_"+FunName+FunArg+";\n")
 
      DISP.write("static IPP_PROC arraddr[] =\n{{\n	(IPP_PROC)in_{}".format(FunName))

      for cpu in cpulist:
         DISP.write(",\n	(IPP_PROC)"+cpu+"_"+FunName+"")

      DISP.write("\n};")

      DISP.write("""
#ifdef _MSC_VER
#pragma warning(disable: 4100) // for MSVC, unreferenced param
#endif
#undef  IPPAPI
#define IPPAPI(type,name,arg) __declspec(naked) type __stdcall name arg
IPPAPI({FunType}, {FunName},{FunArg})
{{
  __asm {{mov eax, dword ptr ippcpJumpIndexForMergedLibs}}
  __asm {{mov eax, arraddr[eax*4+4]}}
  __asm {{jmp eax}}
}};

IPPAPI({FunType}, in_{FunName},{FunArg})
{{
  __asm {{call ippcpInit}}
  __asm {{mov eax, dword ptr ippcpJumpIndexForMergedLibs }}
  __asm {{mov eax, arraddr[eax*4+4]}}
  __asm {{jmp eax}}
}};
""".format(FunType=FunType, FunName=FunName, FunArg=FunArg))

      DISP.close()


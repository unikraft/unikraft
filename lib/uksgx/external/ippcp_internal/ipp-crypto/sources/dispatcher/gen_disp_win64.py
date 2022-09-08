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
# Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
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
FunArg = ""

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
    filename = "jmp_{}_{}".format(FunName, hashlib.sha512(FunName.encode('utf-8')).hexdigest()[:8])
    
    DISP= open( os.sep.join([OutDir, filename + ".asm"]), 'w' )
    
    for cpu in cpulist:
       DISP.write("extern "+cpu+"_"+FunName+"\n")
    
    DISP.write("extern ippcpJumpIndexForMergedLibs\n")
    DISP.write("extern ippcpSafeInit\n\n")
    
    DISP.write("segment data\n\n")
    
    DISP.write("    DQ in_"+FunName+"\n")
    DISP.write(FunName+"_arraddr:\n")
    
    for cpu in cpulist:
       DISP.write("    DQ "+cpu+"_"+FunName+"\n")
    
    DISP.write("""

segment text

global {FunName}

in_{FunName}:
   {endbr64}
   call    ippcpSafeInit
   align 16
{FunName}:
   {endbr64}
   movsxd  rax, dword [rel ippcpJumpIndexForMergedLibs]
   lea     r10, [rel {FunName}_arraddr]
   mov     r10, qword [r10+rax*8]
   jmp     r10

""".format(FunName=FunName, endbr64='db 0xf3, 0x0f, 0x1e, 0xfa'))

    DISP.close()

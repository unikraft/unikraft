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

def readNextFunction(header, curLine, headerID):    ## read next function with arguments
  ## find header ID macros
  FunName  = ''
  FunArg   = ''
  FunType  = ''
  success = False
  while (curLine < len(header) and success == False):
    if not headerID and re.match( '\s*#\s*if\s*!\s*defined\s*\(\s*__IPP', header[curLine]):
      headerID= re.sub( '.*__IPP', '__IPP', header[curLine] )
      headerID= re.sub( "\)", '', headerID)
      headerID= re.sub( '[\n\s]', '', headerID )
    
    if re.match( '^\s*IPPAPI\s*\(.*', header[curLine] ) :
      FunStr= header[curLine];
      FunStr= re.sub('\n','',FunStr)   ## remove EOL symbols
  
      while not re.match('.*\)\s*\)\s*$', FunStr):   ## concatinate strinng if string is not completed
        curLine= curLine+1
        FunStr= FunStr+header[curLine]
        FunStr= re.sub('\n','',FunStr)   ## remove EOL symbols
    
      FunStr= re.sub('\s+', ' ', FunStr)
    
      s= FunStr.split(',')
    
      ## Extract funtion name
      FunName= s[1]
      FunName= re.sub('\s', '', FunName)
    
      ## Extract function type
      FunType= re.sub( '.*\(', '', s[0] )
      #FunType= re.sub(' ', '', FunType )
    
      ## Extract function arguments
      FunArg= re.sub('.*\(.*,.+,\s*\(', '(', FunStr)
      FunArg= re.sub('\)\s*\)', ')', FunArg)
      success = True

    curLine = curLine + 1

  return {'curLine':curLine, 'FunType':FunType, 'FunName':FunName, 'FunArg':FunArg, 'success':success }
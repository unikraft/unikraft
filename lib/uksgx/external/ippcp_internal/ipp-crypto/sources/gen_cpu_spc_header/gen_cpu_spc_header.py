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

import sys
import os
import datetime

sys.path.append(os.path.join(sys.path[0], '../dispatcher'))

from gen_disp_common import readNextFunction

Header  = sys.argv[1]
OutDir  = sys.argv[2]

Header = os.path.abspath(Header)
Filename = ""

HDR= open(Header, 'r')
h= HDR.readlines()
HDR.close()

headerID= False
FunName = ""

if not os.path.exists(OutDir):
  os.makedirs(OutDir)

Filename = "ippcp_cpuspc"
OutFile  = os.sep.join([OutDir, Filename + ".h"])

OUT= open( OutFile, 'w' )
OUT.write("""/*******************************************************************************
* Copyright {year} Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

""".format(year=datetime.datetime.today().year))

curLine = 0
isFunctionFound = True

while (isFunctionFound):

	result = readNextFunction(h, curLine, headerID)

	curLine         = result['curLine']
	FunName         = result['FunName']
	isFunctionFound = result['success']

	if (isFunctionFound):
		OUT.write("#define " + FunName + " " + "OWNAPI(" + FunName + ")\n")
OUT.close()

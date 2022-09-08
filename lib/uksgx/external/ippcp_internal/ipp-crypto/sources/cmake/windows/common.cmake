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

set(OS_DEFAULT_COMPILER Intel19.0.0)

if(${ARCH} MATCHES "ia32")
  set(LIBRARY_DEFINES "${LIBRARY_DEFINES} -DWIN32 -D_ARCH_IA32") # _WIN32 is defined by a compiler
else()
  set(LIBRARY_DEFINES "${LIBRARY_DEFINES} -DWIN32E -D_ARCH_EM64T") # _WIN32 and _WIN64 are defined by a compiler
endif(${ARCH} MATCHES "ia32")

#set(LIBRARY_DEFINES "${LIBRARY_DEFINES} -DBN_OPENSSL_DISABLE")
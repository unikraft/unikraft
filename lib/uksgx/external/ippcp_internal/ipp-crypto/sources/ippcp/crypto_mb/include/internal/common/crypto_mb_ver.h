/*******************************************************************************
* Copyright 2021 Intel Corporation
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

#include <crypto_mb/version.h>

#define STR2(x)           #x
#define STR(x)       STR2(x)

#define MBX_LIB_LONGNAME()     "Crypto multi-buffer"
#define MBX_LIB_SHORTNAME()    "crypto_mb"

#ifndef MBX_BASE_VERSION
#define MBX_BASE_VERSION() MBX_VER_MAJOR,MBX_VER_MINOR,MBX_VER_REV
#endif

#define MBX_BUILD() 1043
#define MBX_VERSION() MBX_BASE_VERSION(),MBX_BUILD()

#ifndef MBX_STR_VERSION
#define MBX_STR_VERSION() STR(MBX_VER_MAJOR) "." STR(MBX_VER_MINOR) "." STR(MBX_VER_REV) " (" STR(MBX_INTERFACE_VERSION_MAJOR) "." STR(MBX_INTERFACE_VERSION_MINOR) ")"
#endif

#ifndef CRYPTO_MB_STR_VERSION
 #ifdef MBX_REVISION
  #define CRYPTO_MB_STR_VERSION() MBX_STR_VERSION() " (r" STR( MBX_REVISION ) ")"
 #else
  #define CRYPTO_MB_STR_VERSION() MBX_STR_VERSION() " (-)"
 #endif
#endif


/* ////////////////////////////// End of file /////////////////////////////// */

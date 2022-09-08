/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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

/*
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
//
*/

#ifndef __PCPNAME_H__
#define __PCPNAME_H__

/*
   The prefix of library without the quotes. The prefix is directly used to generate the
   GetLibVersion function. It is used to generate the names in the dispatcher code, in
   the version description and in the resource file.
*/
#define LIB_PREFIX          ippcp

/*
   Names of library. It is used in the resource file and is used to generate the names
   in the dispatcher code.
*/
#define IPP_LIB_LONGNAME()     "Cryptography"
#define IPP_LIB_SHORTNAME()    "ippCP"


#define GET_STR2(x)      #x
#define GET_STR(x)       GET_STR2(x)
#define IPP_LIB_PREFIX() GET_STR(LIB_PREFIX)

#define IPP_INC_NAME()   "ippcp.h"

#endif /* __PCPNAME_H__ */
/* ///////////////////////// End of file "pcpname.h" ///////////////////////// */

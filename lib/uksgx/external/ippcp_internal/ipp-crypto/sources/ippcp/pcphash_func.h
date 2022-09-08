/*******************************************************************************
* Copyright 2014-2021 Intel Corporation
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
// 
//  Purpose:
//     Cryptography Primitive.
//     Security Hash Standard
//     Internal Definitions and Internal Functions Prototypes
// 
// 
*/

#if !defined(_PCP_HASH_FUNC_H)
#define _PCP_HASH_FUNC_H

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"

/*
// hash alg default processing functions and opt argument
*/
static cpHashProc cpHashProcFunc[] = {
   (cpHashProc)NULL,

   #if defined(_ENABLE_ALG_SHA1_)
      #if(_SHA_NI_ENABLING_==_FEATURE_ON_)
      UpdateSHA1ni,
      #else
      UpdateSHA1,
      #endif
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SHA256_)
      #if(_SHA_NI_ENABLING_==_FEATURE_ON_)
      UpdateSHA256ni,
      #else
      UpdateSHA256,
      #endif
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SHA224_)
      #if(_SHA_NI_ENABLING_==_FEATURE_ON_)
      UpdateSHA256ni,
      #else
      UpdateSHA256,
      #endif
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SHA512_)
   UpdateSHA512,
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SHA384_)
   UpdateSHA512,
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_MD5_)
   UpdateMD5,
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SM3_)
   UpdateSM3,
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SHA512_224_)
   UpdateSHA512,
   #else
   NULL,
   #endif

   #if defined(_ENABLE_ALG_SHA512_256_)
   UpdateSHA512,
   #else
   NULL,
   #endif
};

#endif /* _PCP_HASH_FUNC_H */

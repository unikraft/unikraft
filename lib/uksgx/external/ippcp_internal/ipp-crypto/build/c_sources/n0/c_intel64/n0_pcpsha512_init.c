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
// 
//  Purpose:
//     Cryptography Primitive.
//     SHA512 message digest
// 
//  Contents:
//        InitSHA512()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha512stuff.h"

IPP_OWN_DEFN (IppStatus, InitSHA512, (IppsSHA512State* pState, const DigestSHA512 IV))
{
   /* test state pointer */
   IPP_BAD_PTR1_RET(pState);

   /* set state ID */
   HASH_SET_ID(pState, idCtxSHA512);
   /* zeros message length */
   HASH_LENLO(pState) = 0;
   HASH_LENHI(pState) = 0;
   /* message buffer is free */
   HAHS_BUFFIDX(pState) = 0;
   /* setup initial digest */
   hashInit(HASH_VALUE(pState), IV);

   return ippStsNoErr;
}

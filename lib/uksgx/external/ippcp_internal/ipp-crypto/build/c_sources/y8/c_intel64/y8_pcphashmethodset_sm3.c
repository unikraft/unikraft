/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     Digesting message according to SM3
// 
//  Contents:
//        ippsHashMethodSet_SM3()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsm3stuff.h"

/*F*
//    Name: ippsHashMethodSet_SM3
//
// Purpose: Setup SM3 method.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMethod == NULL
//    ippStsNoErr             no errors
//
*F*/
IPPFUN( IppStatus, ippsHashMethodSet_SM3, (IppsHashMethod* pMethod) )
{
   /* test pointers */
   IPP_BAD_PTR1_RET(pMethod);

   pMethod->hashAlgId     = ippHashAlg_SM3;
   pMethod->hashLen       = IPP_SM3_DIGEST_BITSIZE/8;
   pMethod->msgBlkSize    = MBS_SM3;
   pMethod->msgLenRepSize = MLR_SM3;
   pMethod->hashInit      = sm3_hashInit;
   pMethod->hashUpdate    = sm3_hashUpdate;
   pMethod->hashOctStr    = sm3_hashOctString;
   pMethod->msgLenRep     = sm3_msgRep;

   return ippStsNoErr;
}

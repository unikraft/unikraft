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
//        ippsHashMethodSet_SHA512_256()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha512stuff.h"

/*F*
//    Name: ippsHashMethodSet_SHA512_256
//
// Purpose: Setup SHA512_256 method.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMethod == NULL
//    ippStsNoErr             no errors
//
*F*/

IPPFUN( IppStatus, ippsHashMethodSet_SHA512_256, (IppsHashMethod* pMethod) )
{
   /* test pointers */
   IPP_BAD_PTR1_RET(pMethod);

   pMethod->hashAlgId     = ippHashAlg_SHA512_256;
   pMethod->hashLen       = IPP_SHA256_DIGEST_BITSIZE/8;
   pMethod->msgBlkSize    = MBS_SHA512;
   pMethod->msgLenRepSize = MLR_SHA512;
   pMethod->hashInit      = sha512_256_hashInit;
   pMethod->hashUpdate    = sha512_hashUpdate;
   pMethod->hashOctStr    = sha512_256_hashOctString;
   pMethod->msgLenRep     = sha512_msgRep;

   return ippStsNoErr;
}

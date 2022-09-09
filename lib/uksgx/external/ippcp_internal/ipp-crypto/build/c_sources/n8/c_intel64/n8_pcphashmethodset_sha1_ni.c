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
//     Digesting message according to SHA1
// 
//  Contents:
//        ippsHashMethodSet_SHA1_NI()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha1stuff.h"

/*F*
//    Name: ippsHashMethodSet_SHA1_NI
//
// Purpose: Setup SHA1 method (using the SHA-NI instruction set).
//
// Returns:                Reason:
//    ippStsNullPtrErr           pMethod == NULL
//    ippStsNotSupportedModeErr  mode disabled by configuration
//    ippStsNoErr                no errors
//
*F*/

IPPFUN( IppStatus, ippsHashMethodSet_SHA1_NI, (IppsHashMethod* pMethod) )
{
   /* test pointers */
   IPP_BAD_PTR1_RET(pMethod);

#if (_SHA_NI_ENABLING_==_FEATURE_TICKTOCK_ || _SHA_NI_ENABLING_==_FEATURE_ON_)
   pMethod->hashAlgId     = ippHashAlg_SHA1;
   pMethod->hashLen       = IPP_SHA1_DIGEST_BITSIZE/8;
   pMethod->msgBlkSize    = MBS_SHA1;
   pMethod->msgLenRepSize = MLR_SHA1;
   pMethod->hashInit      = sha1_hashInit;
   pMethod->hashUpdate    = sha1_ni_hashUpdate;
   pMethod->hashOctStr    = sha1_hashOctString;
   pMethod->msgLenRep     = sha1_msgRep;

   return ippStsNoErr;
#else
   pMethod->hashAlgId     = ippHashAlg_Unknown;
   pMethod->hashLen       = 0;
   pMethod->msgBlkSize    = 0;
   pMethod->msgLenRepSize = 0;
   pMethod->hashInit      = 0;
   pMethod->hashUpdate    = 0;
   pMethod->hashOctStr    = 0;
   pMethod->msgLenRep     = 0;

   return ippStsNotSupportedModeErr;
#endif
}

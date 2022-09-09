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
//     Digesting message according to MD5
//     (derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm)
// 
//     Equivalent code is available from RFC 1321.
// 
//  Contents:
//        ippsHashMethodSet_MD5()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpmd5stuff.h"

/*F*
//    Name: ippsHashMethodSet_MD5
//
// Purpose: Setup MD5 method.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMethod == NULL
//    ippStsNoErr             no errors
//
*F*/
IPPFUN( IppStatus, ippsHashMethodSet_MD5, (IppsHashMethod* pMethod) )
{
   /* test pointers */
   IPP_BAD_PTR1_RET(pMethod);

   pMethod->hashAlgId     = ippHashAlg_MD5;
   pMethod->hashLen       = IPP_MD5_DIGEST_BITSIZE/8;
   pMethod->msgBlkSize    = MBS_MD5;
   pMethod->msgLenRepSize = MLR_MD5;
   pMethod->hashInit      = md5_hashInit;
   pMethod->hashUpdate    = md5_hashUpdate;
   pMethod->hashOctStr    = md5_hashOctString;
   pMethod->msgLenRep     = md5_msgRep;

   return ippStsNoErr;
}

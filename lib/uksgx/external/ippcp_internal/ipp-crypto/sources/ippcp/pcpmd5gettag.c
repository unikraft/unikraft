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
//        ippsMD5GetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpmd5stuff.h"

/*F*
//    Name: ippsMD5GetTag
//
// Purpose: Compute digest based on current state.
//          Note, that futher digest update is possible
//
// Returns:                Reason:
//    ippStsNullPtrErr        pTag == NULL
//                            pState == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxMD5
//    ippStsLengthErr         max_MD5_digestLen < tagLen <1
//    ippStsNoErr             no errors
//
// Parameters:
//    pTag        address of the output digest
//    tagLen      length of digest
//    pState      pointer to the MD5 state
//
*F*/
IPPFUN(IppStatus, ippsMD5GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsMD5State* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxMD5), ippStsContextMatchErr);

   /* test digest pointer */
   IPP_BAD_PTR1_RET(pTag);
   IPP_BADARG_RET((tagLen<1)||(sizeof(DigestMD5)<tagLen), ippStsLengthErr);

   {
      DigestMD5 digest;
      CopyBlock(HASH_VALUE(pState), digest, sizeof(DigestMD5));
      cpFinalizeMD5(digest, HASH_BUFF(pState), HAHS_BUFFIDX(pState), HASH_LENLO(pState));
      CopyBlock(digest, pTag, (cpSize)tagLen);

      return ippStsNoErr;
   }
}

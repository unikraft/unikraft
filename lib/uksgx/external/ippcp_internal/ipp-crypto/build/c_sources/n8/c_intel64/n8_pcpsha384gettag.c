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
//        ippsSHA384GetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha512stuff.h"

/*F*
//    Name: ippsSHA384GetTag
//
// Purpose: Compute digest based on current state.
//          Note, that futher digest update is possible
//
// Returns:                Reason:
//    ippStsNullPtrErr        pTag == NULL
//                            pState == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxSHA512
//    ippStsLengthErr         max_SHA_digestLen < tagLen <1
//    ippStsNoErr             no errors
//
// Parameters:
//    pTag        address of the output digest
//    tagLen      length of digest
//    pState      pointer to the SHA384 state
//
*F*/

IPPFUN(IppStatus, ippsSHA384GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSHA384State* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxSHA512), ippStsContextMatchErr);

   /* test digest pointer */
   IPP_BAD_PTR1_RET(pTag);
   IPP_BADARG_RET((tagLen<1)||(sizeof(DigestSHA384)<tagLen), ippStsLengthErr);

   {
      DigestSHA512 digest;
      CopyBlock(HASH_VALUE(pState), digest, sizeof(DigestSHA512));
      cpFinalizeSHA512(digest,
                       HASH_BUFF(pState), HAHS_BUFFIDX(pState),
                       HASH_LENLO(pState), HASH_LENHI(pState));
      digest[0] = ENDIANNESS64(digest[0]);
      digest[1] = ENDIANNESS64(digest[1]);
      digest[2] = ENDIANNESS64(digest[2]);
      digest[3] = ENDIANNESS64(digest[3]);
      digest[4] = ENDIANNESS64(digest[4]);
      digest[5] = ENDIANNESS64(digest[5]);
      CopyBlock(digest, pTag, (cpSize)tagLen);

      return ippStsNoErr;
   }
}

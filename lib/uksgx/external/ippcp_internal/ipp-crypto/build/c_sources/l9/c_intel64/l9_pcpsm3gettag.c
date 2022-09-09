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
//        ippsSM3GetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsm3stuff.h"

/*F*
//    Name: ippsSM3GetTag
//
// Purpose: Compute digest based on current state.
//          Note, that futher digest update is possible
//
// Returns:                Reason:
//    ippStsNullPtrErr        pTag == NULL
//                            pState == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxSM3
//    ippStsLengthErr         max_SHA_digestLen < tagLen <1
//    ippStsNoErr             no errors
//
// Parameters:
//    pTag        address of the output digest
//    tagLen      length of digest
//    pState      pointer to the SHS state
//
*F*/
IPPFUN(IppStatus, ippsSM3GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSM3State* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxSM3), ippStsContextMatchErr);

   /* test digest pointer */
   IPP_BAD_PTR1_RET(pTag);
   IPP_BADARG_RET((tagLen<1)||(sizeof(DigestSM3)<tagLen), ippStsLengthErr);

   {
      DigestSM3 digest;
      CopyBlock(HASH_VALUE(pState), digest, sizeof(DigestSM3));
      cpFinalizeSM3(digest, HASH_BUFF(pState), HAHS_BUFFIDX(pState), HASH_LENLO(pState));
      digest[0] = ENDIANNESS32(digest[0]);
      digest[1] = ENDIANNESS32(digest[1]);
      digest[2] = ENDIANNESS32(digest[2]);
      digest[3] = ENDIANNESS32(digest[3]);
      digest[4] = ENDIANNESS32(digest[4]);
      digest[5] = ENDIANNESS32(digest[5]);
      digest[6] = ENDIANNESS32(digest[6]);
      digest[7] = ENDIANNESS32(digest[7]);
      CopyBlock(digest, pTag, (cpSize)tagLen);

      return ippStsNoErr;
   }
}

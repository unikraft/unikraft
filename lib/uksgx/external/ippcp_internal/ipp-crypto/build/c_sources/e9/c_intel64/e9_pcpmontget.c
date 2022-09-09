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
//  Contents:
//        ippsMontGet()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcptool.h"

/*F*
// Name: ippsMontGet
//
// Purpose: Extracts modulus.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           pMont==NULL
//                               pModulus==NULL
//                               pSize==NULL
//    ippStsContextMatchErr      !MNT_VALID_ID()
//    ippStsNoErr                no errors
//
// Parameters:
//    pModulus    pointer to the modulus buffer
//    pSize       pointer to the modulus length (in Ipp32u chunks).
//    pMont       pointer to the context
*F*/
IPPFUN(IppStatus, ippsMontGet,(Ipp32u* pModulus, int* pSize, const IppsMontState* pMont))
{
   IPP_BAD_PTR3_RET(pMont, pModulus, pSize);

   IPP_BADARG_RET(!MNT_VALID_ID(pMont), ippStsContextMatchErr);

   {
      cpSize len32 = MOD_LEN(MNT_ENGINE(pMont))*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
      Ipp32u* bnData = (Ipp32u*) MOD_MODULUS( MNT_ENGINE(pMont) );

      FIX_BNU32(bnData, len32);
      COPY_BNU(pModulus, bnData, len32);
      *pSize = len32;

      return ippStsNoErr;
   }
}

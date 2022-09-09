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
//     HMAC General Functionality
// 
//  Contents:
//        ippsHMAC_GetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphmac.h"
#include "pcptool.h"

/*F*
//    Name: ippsHMAC_GetTag
//
// Purpose: Compute digest with further digesting ability.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMD == NULL
//                            pState == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxHMAC
//    ippStsLengthErr         size_of_digest < mdLen <1
//    ippStsNoErr             no errors
//
// Parameters:
//    pMD         address of the output digest
//    mdLen       length of the digest
//    pState      pointer to the HMAC state
//
*F*/
IPPFUN(IppStatus, ippsHMAC_GetTag,(Ipp8u* pMD, int mdLen, const IppsHMACState* pCtx))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!HMAC_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test MD pointer */
   IPP_BAD_PTR1_RET(pMD);

   { /* TBD: consider implementation without copy of context */
      IppStatus sts;
      IppsHMACState tmpCtx;
      ippsHMAC_Duplicate(pCtx, &tmpCtx);
      sts = ippsHMAC_Final(pMD, mdLen, &tmpCtx);

      PurgeBlock(&tmpCtx, sizeof(IppsHMACState));
      return sts;
   }
}

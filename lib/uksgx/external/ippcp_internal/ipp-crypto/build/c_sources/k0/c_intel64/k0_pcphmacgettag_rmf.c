/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//        ippsHMACGetTag_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphmac.h"
#include "pcphmac_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsHMACGetTag_rmf
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
IPPFUN(IppStatus, ippsHMACGetTag_rmf,(Ipp8u* pMD, int mdLen, const IppsHMACState_rmf* pCtx))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!HMAC_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test MD pointer */
   IPP_BAD_PTR1_RET(pMD);

   { /* TBD: consider implementation without copy of context */
      IppStatus sts;
      IppsHMACState_rmf tmpCtx;
      ippsHMACDuplicate_rmf(pCtx, &tmpCtx);

      sts = ippsHMACFinal_rmf(pMD, mdLen, &tmpCtx);

      PurgeBlock(&tmpCtx, sizeof(IppsHMACState_rmf));
      return sts;
   }
}

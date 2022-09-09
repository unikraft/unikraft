/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     EC over Prime Finite Field (EC Key Generation)
// 
//  Contents:
//     ippsECCPSetKeyPair()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"


/*F*
//    Name: ippsECCPSetKeyPair
//
// Purpose: Generate (private,public) Key Pair
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pPrivate
//                               NULL == pPublic
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               illegal pPrivate->idCtx
//                               illegal pPublic->idCtx
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pPrivate    pointer to the private key
//    pPublic     pointer to the public  key
//    regular     flag regular/ephemeral keys
//    pEC        pointer to the ECCP context
//
*F*/
IPPFUN(IppStatus, ippsECCPSetKeyPair, (const IppsBigNumState* pPrivate, const IppsECCPPointState* pPublic,
                                       IppBool regular,
                                       IppsECCPState* pEC))
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   {
      BNU_CHUNK_T* targetPrivate;
      BNU_CHUNK_T* targetPublic;

      if(regular) {
         targetPrivate = ECP_PRIVAT(pEC);
         targetPublic  = ECP_PUBLIC(pEC);
      }
      else {
         targetPrivate = ECP_PRIVAT_E(pEC);
         targetPublic  = ECP_PUBLIC_E(pEC);
      }

      /* set up private key request */
      if( pPrivate ) {
         IPP_BADARG_RET(!BN_VALID_ID(pPrivate), ippStsContextMatchErr);
         {
            int privateLen = BITS_BNU_CHUNK(ECP_ORDBITSIZE(pEC));
            cpGFpElementCopyPad(targetPrivate, privateLen, BN_NUMBER(pPrivate), BN_SIZE(pPrivate));
         }
      }

      /* set up public  key request */
      if( pPublic ) {
         IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPublic), ippStsContextMatchErr );
         {
            BNU_CHUNK_T* targetPublicX = targetPublic;
            BNU_CHUNK_T* targetPublicY = targetPublic+ECP_POINT_FELEN(pPublic);
            gfec_GetPoint(targetPublicX, targetPublicY, pPublic, pEC);
            gfec_SetPoint(targetPublic, targetPublicX, targetPublicY, pEC);

            //cpGFpElementCopy(targetPublic, ECP_POINT_DATA(pPublic), publicLen);
         }
      }

      return ippStsNoErr;
   }
}

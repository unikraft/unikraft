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
//        ippsHMAC_Final()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphmac.h"
#include "pcptool.h"

/*F*
//    Name: ippsHMAC_Final
//
// Purpose: Stop message digesting and return digest.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMD == NULL
//                            pState == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxHMAC
//    ippStsLengthErr         sizeof(DigestMD5) < mdLen <1
//    ippStsNoErr             no errors
//
// Parameters:
//    pMD         address of the output digest
//    pState      pointer to the HMAC state
//
*F*/
IPPFUN(IppStatus, ippsHMAC_Final,(Ipp8u* pMD, int mdLen, IppsHMACState* pCtx))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!HMAC_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test MD pointer and length */
   IPP_BAD_PTR1_RET(pMD);
   IPP_BADARG_RET(mdLen<=0, ippStsLengthErr);

   {
      /* hash specific */
      IppsHashState* pHashCtx = &HASH_CTX(pCtx);
      int mbs = cpHashMBS(HASH_ALG_ID(pHashCtx));
      int hashSize = cpHashSize(HASH_ALG_ID(pHashCtx));
      if(mdLen>hashSize)
         IPP_ERROR_RET(ippStsLengthErr);

      /*
      // finalize hmac
      */
      {
         /* finalize 1-st step */
         Ipp8u md[IPP_SHA512_DIGEST_BITSIZE/8];
         IppStatus sts = ippsHashFinal(md, pHashCtx);

         if(ippStsNoErr==sts) {
            /* perform outer hash */
            ippsHashUpdate(pCtx->opadKey, mbs, pHashCtx);
            ippsHashUpdate(md, hashSize, pHashCtx);

            /* complete HMAC */
            ippsHashFinal(md, pHashCtx);
            CopyBlock(md, pMD, IPP_MIN(hashSize, mdLen));

            /* ready to the next HMAC computation */
            ippsHashUpdate(pCtx->ipadKey, mbs, pHashCtx);
         }

         return sts;
      }
   }
}

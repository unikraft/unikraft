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
//        ippsHMACFinal_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphmac.h"
#include "pcphmac_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsHMACFinal_rmf
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
IPPFUN(IppStatus, ippsHMACFinal_rmf,(Ipp8u* pMD, int mdLen, IppsHMACState_rmf* pCtx))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!HMAC_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test MD pointer and length */
   IPP_BAD_PTR1_RET(pMD);
   IPP_BADARG_RET(mdLen<=0, ippStsLengthErr);

   {
      /* hash specific */
      IppsHashState_rmf* pHashCtx = &HASH_CTX(pCtx);
      const IppsHashMethod* method = HASH_METHOD(pHashCtx);
      int mbs = method->msgBlkSize;
      int hashSize = method->hashLen;
      if(mdLen>hashSize)
         IPP_ERROR_RET(ippStsLengthErr);

      /*
      // finalize hmac
      */
      {
         /* finalize 1-st step */
         Ipp8u md[IPP_SHA512_DIGEST_BITSIZE/8];
         IppStatus sts = ippsHashFinal_rmf(md, pHashCtx);

         if(ippStsNoErr==sts) {
            /* perform outer hash */
            ippsHashUpdate_rmf(pCtx->opadKey, mbs, pHashCtx);
            ippsHashUpdate_rmf(md, hashSize, pHashCtx);

            /* complete HMAC */
            ippsHashFinal_rmf(md, pHashCtx);
            CopyBlock(md, pMD, IPP_MIN(hashSize, mdLen));

            /* ready to the next HMAC computation */
            ippsHashUpdate_rmf(pCtx->ipadKey, mbs, pHashCtx);
         }

         return sts;
      }
   }
}

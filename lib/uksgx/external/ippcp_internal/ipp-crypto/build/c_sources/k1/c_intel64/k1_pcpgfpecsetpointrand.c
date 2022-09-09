/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p) Operations
//
//     Context:
//        ippsGFpECSetPointRandom()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcphash.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsGFpECSetPointRandom
//
// Purpose: Sets the coordinates of a point on an elliptic curve to random values
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pPoint == NULL
//                              pEC == NULL
//                              pScratchBuffer == NULL
//
//    ippStsContextMatchErr     invalid pEC->idCtx
//                              NULL == pEC->subgroup
//                              invalid pPoint->idCtx
//
//    ippStsOutOfRangeErr       ECP_POINT_FELEN(pPoint)!=GFP_FELEN()
//
//    ippStsNoErr               no error
//
// Parameters:
//    pPoint           Pointer to the IppsGFpECPoint context
//    pEC              Pointer to the context of the elliptic curve
//    rndFunc          Pesudorandom number generator
//    pRndParam        Pointer to the pseudorandom number generator context
//    pScratchBuffer   Pointer to the scratch buffer
//
// Note:
//    Is not a fact that computed point belongs to BP-related subgroup BP
//
*F*/

IPPFUN(IppStatus, ippsGFpECSetPointRandom,(IppsGFpECPoint* pPoint, IppsGFpECState* pEC,
                                           IppBitSupplier rndFunc, void* pRndParam,
                                           Ipp8u* pScratchBuffer))
{
   IppsGFpState*  pGF;
   gsModEngine* pGFE;

   IPP_BAD_PTR3_RET(pPoint, pEC, pScratchBuffer);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPoint), ippStsContextMatchErr );

   pGF = ECP_GFP(pEC);
   pGFE = GFP_PMA(pGF);

   IPP_BADARG_RET( ECP_POINT_FELEN(pPoint)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);

   IPP_BAD_PTR2_RET(rndFunc, pRndParam);

   {
      int internal_err;

      if( GFP_IS_BASIC(pGFE) ) {
         BNU_CHUNK_T* pElm = cpGFpGetPool(1, pGFE);

         do { /* get random X */
            internal_err = NULL==cpGFpRand(pElm, pGFE, rndFunc, pRndParam);
         } while( !internal_err && !gfec_MakePoint(pPoint, pElm, pEC) );

         cpGFpReleasePool(1, pGFE);

         /* R = [cofactor]R */
         if(!internal_err && ECP_SUBGROUP(pEC)) {
            BNU_CHUNK_T* pCofactor = ECP_COFACTOR(pEC);
            int cofactorLen = GFP_FELEN(pGFE);
            if(!GFP_IS_ONE(pCofactor, cofactorLen))
               gfec_MulPoint(pPoint, pPoint, ECP_COFACTOR(pEC), GFP_FELEN(pGFE), /*0,*/ pEC, pScratchBuffer);
         }
      }

      else {
         IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);
         {
            /* number of bits being generated */
            int generatedBits = ECP_ORDBITSIZE(pEC) + GFP_RAND_ADD_BITS;
            int generatedLen = BITS_BNU_CHUNK(generatedBits);

            /* allocate random exponent */
            int poolElements = (generatedLen + GFP_PELEN(pGFE) -1) / GFP_PELEN(pGFE);
            BNU_CHUNK_T* pExp = cpGFpGetPool(poolElements, pGFE);

            /* setup copy of the base point */
            IppsGFpECPoint G;
            cpEcGFpInitPoint(&G, ECP_G(pEC),ECP_AFFINE_POINT|ECP_FINITE_POINT, pEC);

            /* get random bits */
            internal_err = ippStsNoErr != rndFunc((Ipp32u*)pExp, generatedBits, pRndParam);

            if(!internal_err) {
               /* reduce with respect to order value */
               int nsE = cpMod_BNU(pExp, generatedLen, MOD_MODULUS(ECP_MONT_R(pEC)), BITS_BNU_CHUNK(ECP_ORDBITSIZE(pEC)));
               /* compute random point */
               gfec_MulPoint(pPoint, &G, pExp, nsE, pEC, pScratchBuffer);
            }

            cpGFpReleasePool(poolElements, pGFE);
         }
      }

      return internal_err? ippStsErr : ippStsNoErr;
   }
}

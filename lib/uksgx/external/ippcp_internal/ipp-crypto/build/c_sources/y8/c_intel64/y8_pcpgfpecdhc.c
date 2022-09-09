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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        ippsGFpECSharedSecretDHC()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"



/*F*
//    Name: ippsGFpECSharedSecretDHC
//
// Purpose: Compute Shared Secret (Diffie-Hellman with cofactor)
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pEC
//                               NULL == pPrivateA
//                               NULL == pPublicB
//                               NULL == pShare
//
//    ippStsContextMatchErr      illegal pEC->idCtx
//                               pEC->subgroup == NULL
//                               illegal pPrivateA->idCtx
//                               illegal pPublicB->idCtx
//                               illegal pShare->idCtx
//
//    ippStsRangeErr             not enough room for share key
//
//    ippStsShareKeyErr          (infinity) => z
//
//    ippStsNoErr                no errors
//
// Parameters:
//    pPrivateA   pointer to own   private key
//    pPublicB    pointer to alien public  key
//    pShare      pointer to the shared secret value
//    pEC         pointer to the EC context
//
*F*/
IPPFUN(IppStatus, ippsGFpECSharedSecretDHC,(const IppsBigNumState* pPrivateA, const IppsGFpECPoint* pPublicB,
                                            IppsBigNumState* pShare,
                                            IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
{
   IppsGFpState*  pGF;
   gsModEngine* pGFE;

   /* EC context and buffer */
   IPP_BAD_PTR2_RET(pEC, pScratchBuffer);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   pGF = ECP_GFP(pEC);
   pGFE = GFP_PMA(pGF);

   /* test private (own) key */
   IPP_BAD_PTR1_RET(pPrivateA);
   IPP_BADARG_RET(!BN_VALID_ID(pPrivateA), ippStsContextMatchErr);

   /* test public (other party) key */
   IPP_BAD_PTR1_RET(pPublicB);
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pPublicB), ippStsContextMatchErr );

   /* test share key */
   IPP_BAD_PTR1_RET(pShare);
   IPP_BADARG_RET(!BN_VALID_ID(pShare), ippStsContextMatchErr);
   IPP_BADARG_RET((BN_ROOM(pShare)<GFP_FELEN(pGFE)), ippStsRangeErr);

   {
      int elmLen = GFP_FELEN(pGFE);

      IppsGFpElement elm;
      IppsGFpECPoint T;
      int finite_point;

      gsModEngine* montR = ECP_MONT_R(pEC);
      int nsR = MOD_LEN(montR);

      BNU_CHUNK_T* F = cpGFpGetPool(2, pGFE);

      /* compute factored secret F = coFactor*privateA */
      BNU_CHUNK_T* pCofactor = ECP_COFACTOR(pEC);
      int cofactorLen = GFP_FELEN(pGFE);
      cofactorLen = cpGFpElementLen(pCofactor, cofactorLen);

      if(GFP_IS_ONE(pCofactor, cofactorLen))
         cpGFpElementCopyPad(F, nsR, BN_NUMBER(pPrivateA), BN_SIZE(pPrivateA));
      else {
         cpMontEnc_BNU_EX(F, BN_NUMBER(pPrivateA), BN_SIZE(pPrivateA), montR);
         cpMontMul_BNU_EX(F, F, nsR, pCofactor, cofactorLen, montR);
      }
      /* T = [F]pPublicB */
      cpEcGFpInitPoint(&T, cpEcGFpGetPool(1, pEC),0, pEC);
      gfec_MulPoint(&T, pPublicB, F, nsR, /*ECP_ORDBITSIZE(pEC),*/ pEC, pScratchBuffer);

      /* share = T.x */
      cpGFpElementConstruct(&elm, F, elmLen);
      finite_point = gfec_GetPoint(GFPE_DATA(&elm), NULL, &T, pEC);
      if(finite_point) {
         BNU_CHUNK_T* pShareData = BN_NUMBER(pShare);
         int nsShare = BN_ROOM(pShare);
         /* share = decode(T.x) */
         GFP_METHOD(pGFE)->decode(pShareData, GFPE_DATA(&elm), pGFE);
         cpGFpElementPad(pShareData+elmLen, nsShare-elmLen, 0);

         BN_SIGN(pShare) = ippBigNumPOS;
         FIX_BNU(pShareData, nsShare);
         BN_SIZE(pShare) = nsShare;
      }

      cpGFpReleasePool(2, pGFE);
      cpEcGFpReleasePool(1, pEC);

      return finite_point? ippStsNoErr : ippStsShareKeyErr;
   }
}

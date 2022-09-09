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
//        ippsGFpECMulPoint()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"

/*F*
// Name: ippsGFpECMulPoint
//
// Purpose: Multiplies a point on an elliptic curve by a scalar
//
// Returns:                   Reason:
//    ippStsNullPtrErr               pP == NULL
//                                   pQ == NULL
//                                   pR == NULL
//                                   pEC == NULL
//
//    ippStsContextMatchErr          invalid pEC->idCtx
//                                   pEC->subgroup == NULL
//                                   invalid pP->idCtx
//                                   invalid pQ->idCtx
//                                   invalid pR->idCtx
//
//    ippStsOutOfRangeErr            ECP_POINT_FELEN(pP)!=GFP_FELEN()
//                                   ECP_POINT_FELEN(pR)!=GFP_FELEN()
//
//    ippStsBadArgErr                pN is negative
//                                   pN > MOD_MODULUS(ECP_MONT_R(pEC))
//
//    ippStsNoErr                    no error
//
// Parameters:
//    pP              Pointer to the context of the given point on the elliptic curve
//    pN              Pointer to the Big Number context storing the scalar value
//    pR              Pointer to the context of the resulting elliptic curve point
//    pEC             Pointer to the context of the elliptic curve
//    pScratchBuffer  Pointer to the scratch buffer
//
//  Note:
//    computes [N]*P, 0 < N < order
//
*F*/
#if 0
IPPFUN(IppStatus, ippsGFpECMulPoint,(const IppsGFpECPoint* pP,
                                     const IppsBigNumState* pN,
                                     IppsGFpECPoint* pR,
                                     IppsGFpECState* pEC,
                                     Ipp8u* pScratchBuffer))
{
   IPP_BAD_PTR4_RET(pP, pR, pEC, pScratchBuffer);
   IPP_BADARG_RET( !VALID_ECP_ID(pEC), ippStsContextMatchErr );
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pP), ippStsContextMatchErr );
   IPP_BADARG_RET( !ECP_POINT_VALID_ID(pR), ippStsContextMatchErr );

   IPP_BADARG_RET( ECP_POINT_FELEN(pP)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET( ECP_POINT_FELEN(pR)!=GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   IPP_BAD_PTR1_RET(pN);
   IPP_BADARG_RET(!BN_VALID_ID(pN), ippStsContextMatchErr );
   IPP_BADARG_RET( BN_NEGATIVE(pN), ippStsBadArgErr );

   {
      BNU_CHUNK_T* pScalar = BN_NUMBER(pN);
      int scalarLen = BN_SIZE(pN);
      IPP_BADARG_RET(0<cpCmp_BNU(pScalar, scalarLen, MOD_MODULUS(ECP_MONT_R(pEC)), MOD_LEN(ECP_MONT_R(pEC))), ippStsBadArgErr);

      gfec_MulPoint(pR, pP, pScalar, scalarLen, pEC, pScratchBuffer);
      return ippStsNoErr;
   }
}
#endif
IPPFUN(IppStatus, ippsGFpECMulPoint, (const IppsGFpECPoint* pP,
   const IppsBigNumState* pN,
   IppsGFpECPoint* pR,
   IppsGFpECState* pEC,
   Ipp8u* pScratchBuffer))
{
   IPP_BAD_PTR4_RET(pP, pR, pEC, pScratchBuffer);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   //IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   IPP_BADARG_RET(!ECP_POINT_VALID_ID(pP), ippStsContextMatchErr);
   IPP_BADARG_RET(!ECP_POINT_VALID_ID(pR), ippStsContextMatchErr);

   IPP_BADARG_RET(ECP_POINT_FELEN(pP) != GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);
   IPP_BADARG_RET(ECP_POINT_FELEN(pR) != GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsOutOfRangeErr);

   IPP_BAD_PTR1_RET(pN);
   IPP_BADARG_RET(!BN_VALID_ID(pN), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pN), ippStsBadArgErr);

   {
      BNU_CHUNK_T* pScalar = BN_NUMBER(pN);
      int scalarLen = BN_SIZE(pN);
      IPP_BADARG_RET(0<cpCmp_BNU(pScalar, scalarLen, MOD_MODULUS(ECP_MONT_R(pEC)), MOD_LEN(ECP_MONT_R(pEC))), ippStsBadArgErr);

      gfec_MulPoint(pR, pP, pScalar, scalarLen, pEC, pScratchBuffer);
      return ippStsNoErr;
   }
}

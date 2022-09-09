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
//     EC over Prime Finite Field (setup/retrieve domain parameters)
// 
//  Contents:
//        ippsECCPGet()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPGet
//
// Purpose: Retrieve ECC Domain Parameter.
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pPrime
//                            NULL == pA
//                            NULL == pB
//                            NULL == pGX
//                            NULL == pGY
//                            NULL == pOrder
//                            NULL == cofactor
//                            NULL == pEC
//
//    ippStsContextMatchErr   illegal pPrime->idCtx
//                            illegal pA->idCtx
//                            illegal pB->idCtx
//                            illegal pGX->idCtx
//                            illegal pGY->idCtx
//                            illegal pOrder->idCtx
//                            illegal pEC->idCtx
//
//    ippStsRangeErr          not enough room for:
//                            pPrime
//                            pA, pB,
//                            pGX,pGY
//                            pOrder
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pPrime   pointer to the retrieval prime (specify FG(p))
//    pA       pointer to the retrieval A coefficient of EC equation
//    pB       pointer to the retrieval B coefficient of EC equation
//    pGX,pGY  pointer to the retrieval Base Point (x and y coordinates) of EC
//    pOrder   pointer to the retrieval Base Point order
//    cofactor pointer to the retrieval cofactor value
//    pEC     pointer to the ECC context
//
*F*/
IPPFUN(IppStatus, ippsECCPGet, (IppsBigNumState* pPrime,
                                IppsBigNumState* pA, IppsBigNumState* pB,
                                IppsBigNumState* pGX,IppsBigNumState* pGY,
                                IppsBigNumState* pOrder, int* cofactor,
                                IppsECCPState* pEC))
{
   IppsGFpState*  pGF;
   gsModEngine* pGFE;

   /* test pEC */
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   pGF = ECP_GFP(pEC);
   pGFE = GFP_PMA(pGF);

   /* test pPrime */
   IPP_BAD_PTR1_RET(pPrime);
   IPP_BADARG_RET(!BN_VALID_ID(pPrime), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_ROOM(pPrime)<GFP_FELEN(pGFE), ippStsRangeErr);

   /* test pA and pB */
   IPP_BAD_PTR2_RET(pA,pB);
   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pB), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_ROOM(pA)<GFP_FELEN(pGFE), ippStsRangeErr);
   IPP_BADARG_RET(BN_ROOM(pB)<GFP_FELEN(pGFE), ippStsRangeErr);

   /* test pG and pGorder pointers */
   IPP_BAD_PTR3_RET(pGX,pGY, pOrder);
   IPP_BADARG_RET(!BN_VALID_ID(pGX),    ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pGY),    ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pOrder), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_ROOM(pGX)<GFP_FELEN(pGFE),    ippStsRangeErr);
   IPP_BADARG_RET(BN_ROOM(pGY)<GFP_FELEN(pGFE),    ippStsRangeErr);
   IPP_BADARG_RET((BN_ROOM(pOrder)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)), ippStsRangeErr);

   /* test cofactor */
   IPP_BAD_PTR1_RET(cofactor);

   {
      mod_decode  decode = GFP_METHOD(pGFE)->decode;  /* gf decode method  */
      BNU_CHUNK_T* tmp = cpGFpGetPool(1, pGFE);

      /* retrieve EC parameter */
      ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)GFP_MODULUS(pGFE), pPrime);

      decode(tmp, ECP_A(pEC), pGFE);
      ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)tmp, pA);
      decode(tmp, ECP_B(pEC), pGFE);
      ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)tmp, pB);

      decode(tmp, ECP_G(pEC), pGFE);
      ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)tmp, pGX);
      decode(tmp, ECP_G(pEC)+GFP_FELEN(pGFE), pGFE);
      ippsSet_BN(ippBigNumPOS, GFP_FELEN32(pGFE), (Ipp32u*)tmp, pGY);

      {
         gsModEngine* pR = ECP_MONT_R(pEC);
         ippsSet_BN(ippBigNumPOS, MOD_LEN(pR)*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)), (Ipp32u*)MOD_MODULUS(pR), pOrder);
      }

      *cofactor = (int)ECP_COFACTOR(pEC)[0];

      cpGFpReleasePool(1, pGFE);
      return ippStsNoErr;
   }
}

/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Operations over GF(p).
// 
//     Context:
//        ippsGFpMultiExp()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: ippsGFpMultiExp
//
// Purpose: Multiplies exponents of GF elements
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pGFp
//                                  NULL == ppElmA
//                                  NULL == pR
//                                  NULL == ppE
//
//    ippStsContextMatchErr         invalid pGFp->idCtx
//                                  invalid ppElmA[i]->idCtx
//                                  invalid pR->idCtx
//                                  invalid ppE[i]->idCtx
//
//    ippStsOutOfRangeErr           GFPE_ROOM() != GFP_FELEN()
//
//    ippStsBadArgErr               1>nItems
//                                  nItems>6
//
//    ippStsNoErr                   no error
//
// Parameters:
//    ppElmA          Pointer to the array of contexts of the finite field elements representing the base of the exponentiation.
//    ppE             Pointer to the array of the Big Number contexts storing the exponents.
//    nItems          Number of exponents.
//    pR              Pointer to the context of the resulting element of the finite field.
//    pGFp            Pointer to the context of the finite field.
//    pScratchBuffer  Pointer to the scratch buffer.
//
*F*/

IPPFUN(IppStatus, ippsGFpMultiExp,(const IppsGFpElement* const ppElmA[], const IppsBigNumState* const ppE[], int nItems,
                                    IppsGFpElement* pR, IppsGFpState* pGFp,
                                    Ipp8u* pScratchBuffer))
{
   IPP_BAD_PTR2_RET(ppElmA, ppE);

   if(nItems==1)
      return ippsGFpExp(ppElmA[0], ppE[0], pR, pGFp, pScratchBuffer);

   else {
      /* test number of exponents */
      IPP_BADARG_RET(1>nItems || nItems>IPP_MAX_EXPONENT_NUM, ippStsBadArgErr);

      IPP_BAD_PTR2_RET(pR, pGFp);

      IPP_BADARG_RET( !GFP_VALID_ID(pGFp), ippStsContextMatchErr );
      IPP_BADARG_RET( !GFPE_VALID_ID(pR), ippStsContextMatchErr );
      {
         int n;

         gsModEngine* pGFE = GFP_PMA(pGFp);
         IPP_BADARG_RET( GFPE_ROOM(pR)!=GFP_FELEN(pGFE), ippStsOutOfRangeErr);

         /* test all ppElmA[] and ppE[] pairs */
         for(n=0; n<nItems; n++) {
            const IppsGFpElement* pElmA = ppElmA[n];
            const IppsBigNumState* pE = ppE[n];
            IPP_BAD_PTR2_RET(pElmA, pE);

            IPP_BADARG_RET( !GFPE_VALID_ID(pElmA), ippStsContextMatchErr );
            IPP_BADARG_RET( !BN_VALID_ID(pE), ippStsContextMatchErr );

            IPP_BADARG_RET( (GFPE_ROOM(pElmA)!=GFP_FELEN(pGFE)) || (GFPE_ROOM(pR)!=GFP_FELEN(pGFE)), ippStsOutOfRangeErr);
         }

         if(NULL==pScratchBuffer) {
            mod_mul mulF = GFP_METHOD(pGFE)->mul;

            BNU_CHUNK_T* pTmpR = cpGFpGetPool(1, pGFE);
            //tbcd: temporary excluded: assert(NULL!=pTmpR);

            cpGFpxExp(GFPE_DATA(pR), GFPE_DATA(ppElmA[0]), BN_NUMBER(ppE[0]), BN_SIZE(ppE[0]), pGFE, 0);
            for(n=1; n<nItems; n++) {
               cpGFpxExp(pTmpR, GFPE_DATA(ppElmA[n]), BN_NUMBER(ppE[n]), BN_SIZE(ppE[n]), pGFE, 0);
               mulF(GFPE_DATA(pR), GFPE_DATA(pR), pTmpR, pGFE);
            }
   
            cpGFpReleasePool(1, pGFE);
         }

         else {
            const BNU_CHUNK_T* ppAdata[IPP_MAX_EXPONENT_NUM];
            const BNU_CHUNK_T* ppEdata[IPP_MAX_EXPONENT_NUM];
            int nsEdataLen[IPP_MAX_EXPONENT_NUM];
            for(n=0; n<nItems; n++) {
               ppAdata[n] = GFPE_DATA(ppElmA[n]);
               ppEdata[n] = BN_NUMBER(ppE[n]);
               nsEdataLen[n] = BN_SIZE(ppE[n]);
            }
            cpGFpxMultiExp(GFPE_DATA(pR), ppAdata, ppEdata, nsEdataLen, nItems, pGFE, pScratchBuffer);
         }

         return ippStsNoErr;
      }
   }
}

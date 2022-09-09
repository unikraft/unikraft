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
//     GF(p^d) methods
//
*/
#include "owncp.h"

#include "pcpgfpxstuff.h"
#include "pcpgfpxmethod_com.h"

//tbcd: temporary excluded: #include <assert.h>

IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxMul_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
{
   int extDegree = GFP_EXTDEGREE(pGFEx);

    BNU_CHUNK_T* pGFpolynomial = GFP_MODULUS(pGFEx);
    int degR = extDegree-1;
    int elemLen= GFP_FELEN(pGFEx);

    int degB = degR;
    BNU_CHUNK_T* pTmpProduct = cpGFpGetPool(2, pGFEx);
    BNU_CHUNK_T* pTmpResult = pTmpProduct + GFP_PELEN(pGFEx);

    gsEngine* pGroundGFE = GFP_PARENT(pGFEx);
    BNU_CHUNK_T* r = cpGFpGetPool(1, pGroundGFE);
    int groundElemLen = GFP_FELEN(pGroundGFE);

    const BNU_CHUNK_T* pTmpB = GFPX_IDX_ELEMENT(pB, degB, groundElemLen);

    //tbcd: temporary excluded: assert(NULL!=pTmpProduct && NULL!=r);

    /* clear temporary */
    cpGFpElementPad(pTmpProduct, elemLen, 0);

    /* R = A * B[degB-1] */
    cpGFpxMul_GFE(pTmpResult, pA, pTmpB, pGFEx);

    for(degB-=1; degB>=0; degB--) {
      /* save R[degR-1] */
      cpGFpElementCopy(r, GFPX_IDX_ELEMENT(pTmpResult, degR, groundElemLen), groundElemLen);

      { /* R = R * x */
         int j;
         for (j=degR; j>=1; j--)
            cpGFpElementCopy(GFPX_IDX_ELEMENT(pTmpResult, j, groundElemLen), GFPX_IDX_ELEMENT(pTmpResult, j-1, groundElemLen), groundElemLen);
         cpGFpElementPad(pTmpResult, groundElemLen, 0);
      }

      cpGFpxMul_GFE(pTmpProduct, pGFpolynomial, r, pGFEx);
      GFP_METHOD(pGFEx)->sub(pTmpResult, pTmpResult, pTmpProduct, pGFEx);

      /* B[degB-i] */
      pTmpB -= groundElemLen;
      cpGFpxMul_GFE(pTmpProduct, pA, pTmpB, pGFEx);
      GFP_METHOD(pGFEx)->add(pTmpResult, pTmpResult, pTmpProduct, pGFEx);
   }

   /* copy result */
   cpGFpElementCopy(pR, pTmpResult, elemLen);

   /* release pools */
   cpGFpReleasePool(1, pGroundGFE);
   cpGFpReleasePool(2, pGFEx);

   return pR;
}

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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal operations over GF(p) extension.
// 
//     Context:
//        cpGFpxInv()
//
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpgfpxstuff.h"
#include "gsscramble.h"

//tbcd: temporary excluded: #include <assert.h>

static BNU_CHUNK_T* gfpxPolyDiv(BNU_CHUNK_T* pQ, BNU_CHUNK_T* pR,
                        const BNU_CHUNK_T* pA,
                        const BNU_CHUNK_T* pB,
                        gsModEngine* pGFEx)
{
   if( GFP_IS_BASIC(pGFEx) )
      return NULL;

   else {
      int elemLen = GFP_FELEN(pGFEx);
      gsModEngine* pGroundGFE = GFP_PARENT(pGFEx);
      int termLen = GFP_FELEN(pGroundGFE);

      int degA = degree(pA, pGFEx);
      int degB = degree(pB, pGFEx);

      if(degB==0) {
         if( GFP_IS_ZERO(pB, termLen) )
            return NULL;
         else {
            gsModEngine* pBasicGFE = cpGFpBasic(pGroundGFE);

            cpGFpInv(pR, pB, pBasicGFE);
            cpGFpElementPad(pR+GFP_FELEN(pGroundGFE), termLen-GFP_FELEN(pGroundGFE), 0);
            cpGFpxMul_GFE(pQ, pA, pR, pGFEx);
            cpGFpElementPad(pR, elemLen, 0);
            return pR;
         }
      }

      if(degA < degB) {
         cpGFpElementPad(pQ, elemLen, 0);
         cpGFpElementCopyPad(pR, elemLen, pA, (degA+1)*termLen);
         return pR;
      }

      else {
         mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
         mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

         int i, j;
         BNU_CHUNK_T* pProduct = cpGFpGetPool(2, pGroundGFE);
         BNU_CHUNK_T* pInvB = pProduct + GFP_PELEN(pGroundGFE);
         //tbcd: temporary excluded: assert(NULL!=pProduct);

         cpGFpElementCopyPad(pR, elemLen, pA, (degA+1)*termLen);
         cpGFpElementPad(pQ, elemLen, 0);

         cpGFpxInv(pInvB, GFPX_IDX_ELEMENT(pB, degB, termLen), pGroundGFE);

         for(i=0; i<=degA-degB && !GFP_IS_ZERO(GFPX_IDX_ELEMENT(pR, degA-i, termLen), termLen); i++) {
            /* compute q term */
            mulF(GFPX_IDX_ELEMENT(pQ, degA-degB-i, termLen),
                 GFPX_IDX_ELEMENT(pR, degA-i, termLen),
                 pInvB,
                 pGroundGFE);

            /* R -= B * q */
            cpGFpElementPad(GFPX_IDX_ELEMENT(pR, degA-i, termLen), termLen, 0);
            for(j=0; j<degB; j++) {
               mulF(pProduct,
                    GFPX_IDX_ELEMENT(pB, j ,termLen),
                    GFPX_IDX_ELEMENT(pQ, degA-degB-i, termLen),
                    pGroundGFE);
               subF(GFPX_IDX_ELEMENT(pR, degA-degB-i+j, termLen),
                    GFPX_IDX_ELEMENT(pR, degA-degB-i+j, termLen),
                    pProduct,
                    pGroundGFE);
            }
         }

         cpGFpReleasePool(2, pGroundGFE);
         return pR;
      }
   }
}


static BNU_CHUNK_T* gfpxGeneratorDiv(BNU_CHUNK_T* pQ, BNU_CHUNK_T* pR, const BNU_CHUNK_T* pB, gsModEngine* pGFEx)
{
   if( GFP_IS_BASIC(pGFEx) )
      return NULL;

   else {
      int elemLen = GFP_FELEN(pGFEx);

      gsModEngine* pGroundGFE = GFP_PARENT(pGFEx);
      mod_mul mulF = GFP_METHOD(pGroundGFE)->mul;
      mod_sub subF = GFP_METHOD(pGroundGFE)->sub;

      int termLen = GFP_FELEN(pGroundGFE);

      BNU_CHUNK_T* pInvB = cpGFpGetPool(2, pGroundGFE);
      BNU_CHUNK_T* pTmp  = pInvB + GFP_PELEN(pGroundGFE);

      int degB = degree(pB, pGFEx);
      int i;

      //tbcd: temporary excluded: assert(NULL!=pInvB);

      cpGFpElementCopy(pR, GFP_MODULUS(pGFEx), elemLen);
      cpGFpElementPad(pQ, elemLen, 0);

      cpGFpxInv(pInvB, GFPX_IDX_ELEMENT(pB, degB, termLen), pGroundGFE);

      for(i=0; i<degB; i++) {
         BNU_CHUNK_T* ptr;
         mulF(pTmp, pInvB, GFPX_IDX_ELEMENT(pB, i, termLen), pGroundGFE);
         ptr = GFPX_IDX_ELEMENT(pR, GFP_EXTDEGREE(pGFEx)-degB+i, termLen);
         subF(ptr, ptr, pTmp, pGroundGFE);
      }

      gfpxPolyDiv(pQ, pR, pR, pB, pGFEx);

      cpGFpElementCopy(GFPX_IDX_ELEMENT(pQ, GFP_EXTDEGREE(pGFEx)-degB, termLen), pInvB, termLen);

      cpGFpReleasePool(2, pGroundGFE);
      return pR;
   }
}

IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxInv, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFEx))
{
   if( GFP_IS_BASIC(pGFEx) )
      return cpGFpInv(pR, pA, pGFEx);

   if(0==degree(pA, pGFEx)) {
      gsModEngine* pGroundGFE = GFP_PARENT(pGFEx);
      BNU_CHUNK_T* tmpR = cpGFpGetPool(1, pGroundGFE);
      //tbcd: temporary excluded: assert(NULL!=tmpR);

      cpGFpxInv(tmpR, pA, pGroundGFE);

      cpGFpElementCopyPad(pR, GFP_FELEN(pGFEx), tmpR, GFP_FELEN(pGroundGFE));
      cpGFpReleasePool(1, pGroundGFE);
      return pR;
   }

   else {
      int elemLen = GFP_FELEN(pGFEx);
      gsModEngine* pGroundGFE = GFP_PARENT(pGFEx);
      gsModEngine* pBasicGFE = cpGFpBasic(pGFEx);

      int pxVars = 6;
      int pelemLen = GFP_PELEN(pGFEx);
      BNU_CHUNK_T* lastrem = cpGFpGetPool(pxVars, pGFEx);
      BNU_CHUNK_T* rem     = lastrem + pelemLen;
      BNU_CHUNK_T* quo     = rem +  pelemLen;
      BNU_CHUNK_T* lastaux = quo + pelemLen;
      BNU_CHUNK_T* aux     = lastaux + pelemLen;
      BNU_CHUNK_T* temp    = aux + pelemLen;
      //tbcd: temporary excluded: assert(NULL!=lastrem);

      cpGFpElementCopy(lastrem, pA, elemLen);
      cpGFpElementCopyPad(lastaux, elemLen, GFP_MNT_R(pBasicGFE), GFP_FELEN(pBasicGFE));

      gfpxGeneratorDiv(quo, rem, pA, pGFEx);
      cpGFpxNeg(aux, quo, pGFEx);

      while(degree(rem, pGFEx) > 0) {
         gfpxPolyDiv(quo, temp, lastrem, rem, pGFEx);
         SWAP_PTR(BNU_CHUNK_T, rem, lastrem); //
         SWAP_PTR(BNU_CHUNK_T, temp, rem);

         GFP_METHOD(pGFEx)->neg(quo, quo, pGFEx);
         GFP_METHOD(pGFEx)->mul(temp, quo, aux, pGFEx);
         GFP_METHOD(pGFEx)->add(temp, lastaux, temp, pGFEx);
         SWAP_PTR(BNU_CHUNK_T, aux, lastaux);
         SWAP_PTR(BNU_CHUNK_T, temp, aux);
      }
      if (GFP_IS_ZERO(rem, elemLen)) { /* gcd != 1 */
         cpGFpReleasePool(pxVars, pGFEx);
         return NULL;
      }

      {
         BNU_CHUNK_T* invRem  = cpGFpGetPool(1, pGroundGFE);
         //tbcd: temporary excluded: assert(NULL!=invRem);

         cpGFpxInv(invRem, rem, pGroundGFE);
         cpGFpxMul_GFE(pR, aux, invRem, pGFEx);

         cpGFpReleasePool(1, pGroundGFE);
      }

      cpGFpReleasePool(pxVars, pGFEx);

      return pR;
   }
}

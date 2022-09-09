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
//        cpGFpxMultiExp()
//
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpgfpxstuff.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"

//tbcd: temporary excluded: #include <assert.h>

static int GetIndex(const BNU_CHUNK_T* ppE[], int nItems, int nBit)
{
   int shift = nBit%BYTESIZE;
   int offset= nBit/BYTESIZE;
   int index = 0;

   int n;
   for(n=nItems; n>0; n--) {
      const Ipp8u* pE = ((Ipp8u*)ppE[n-1]) + offset;
      Ipp8u e = pE[0];
      index <<= 1;
      index += (e>>shift) &1;
   }
   return index;
}


static void cpPrecomputeMultiExp(BNU_CHUNK_T* pTable, const BNU_CHUNK_T* ppA[], int nItems, gsModEngine* pGFEx)
{
   gsModEngine* pBasicGFE = cpGFpBasic(pGFEx);

   //int nPrecomputed = 1<<nItems;

   /* length of element (BNU_CHUNK_T) */
   int elmLen = GFP_FELEN(pGFEx);

   /* get resource */
   BNU_CHUNK_T* pT = cpGFpGetPool(1, pGFEx);
   //tbcd: temporary excluded: assert(NULL!=pT);

   /* pTable[0] = 1 */
   cpGFpElementCopyPad(pT, elmLen, GFP_MNT_R(pBasicGFE), GFP_FELEN(pBasicGFE));
   //cpScramblePut(pTable+0, nPrecomputed, (Ipp8u*)pT, elmDataSize);
   gsScramblePut(pTable, 0, pT, elmLen, nItems);
   /* pTable[1] = A[0] */
   //cpScramblePut(pTable+1, nPrecomputed, (Ipp8u*)(ppA[0]), elmDataSize);
   gsScramblePut(pTable, 1, ppA[0], elmLen, nItems);

   {
      mod_mul mulF = GFP_METHOD(pGFEx)->mul;  /* mul method */

      int i, baseIdx;
      for(i=1, baseIdx=2; i<nItems; i++, baseIdx*=2) {
         /* pTable[baseIdx] = A[i] */
         //cpScramblePut(pTable+baseIdx, nPrecomputed, (Ipp8u*)(ppA[i]), elmDataSize);
         gsScramblePut(pTable, baseIdx, ppA[i], elmLen, nItems);

         {
            int nPasses = 1;
            int step = baseIdx/2;

            int k;
            for(k=i-1; k>=0; k--) {
               int tblIdx = baseIdx;

               int n;
               for(n=0; n<nPasses; n++, tblIdx+=2*step) {
                  /* use pre-computed value */
                  //cpScrambleGet((Ipp8u*)pT, elmDataSize, pTable+tblIdx, nPrecomputed);
                  gsScrambleGet(pT, elmLen, pTable, tblIdx, nItems);
                  mulF(pT, pT, ppA[k], pGFEx);
                  //cpScramblePut(pTable+tblIdx+step, nPrecomputed, (Ipp8u*)pT, elmDataSize);
                  gsScramblePut(pTable, tblIdx+step, pT, elmLen, nItems);
               }

               nPasses *= 2;
               step /= 2;
            }
         }
      }
   }

   /* release resourse */
   cpGFpReleasePool(1, pGFEx);
}


static int cpGetMaxBitsizeExponent(const BNU_CHUNK_T* ppE[], int nsE[], int nItems)
{
   int n, tmp;
   Ipp32u mask;
   /* find out the longest exponent */
   int expBitSize = BITSIZE_BNU(ppE[0], nsE[0]);
   for(n=1; n<nItems; n++) {
      tmp = BITSIZE_BNU(ppE[n], nsE[n]);
      mask = (Ipp32u)cpIsMsb_ct((BNU_CHUNK_T)(expBitSize - tmp));
      expBitSize = (int)(((BNU_CHUNK_T)expBitSize & ~mask) | ((BNU_CHUNK_T)tmp & mask));
   }
   return expBitSize;
}

/* sscm version */
IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpxMultiExp, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* ppA[], const BNU_CHUNK_T* ppE[], int nsE[], int nItems, gsModEngine* pGFEx, Ipp8u* pScratchBuffer))
{
   /* align scratch buffer */
   BNU_CHUNK_T* pTable = (BNU_CHUNK_T*)( IPP_ALIGNED_PTR(pScratchBuffer, CACHE_LINE_SIZE) );
   /* pre-compute table */
   cpPrecomputeMultiExp(pTable, ppA, nItems, pGFEx);

   {
      mod_mul mulF = GFP_METHOD(pGFEx)->mul;  /* mul and sqr methods and parameter */
      mod_sqr sqrF = GFP_METHOD(pGFEx)->sqr;
      int elmLen = GFP_FELEN(pGFEx);

      /* find out the longest exponent */
      int expBitSize = cpGetMaxBitsizeExponent(ppE, nsE, nItems);

      /* allocate resource and copy expanded exponents into */
      const BNU_CHUNK_T* ppExponent[IPP_MAX_EXPONENT_NUM];
      {
         int n;
         for(n=0; n<nItems; n++) {
            BNU_CHUNK_T* pData = cpGFpGetPool(1, pGFEx);
            //tbcd: temporary excluded: assert(NULL!=pData);
            cpGFpElementCopyPad(pData, elmLen, ppE[n], nsE[n]);
            ppExponent[n] = pData;
         }
      }

      /* multiexponentiation */
      {
         /* get temporary */
         BNU_CHUNK_T* pT = cpGFpGetPool(1, pGFEx);

         /* init result */
         int tblIdx = GetIndex(ppExponent, nItems, --expBitSize);
         //cpScrambleGet((Ipp8u*)pR, elmDataSize, pScratchBuffer+tblIdx, nPrecomputed);
         gsScrambleGet_sscm(pR, elmLen, pTable, tblIdx, nItems);

         //tbcd: temporary excluded: assert(NULL!=pT);

         /* compute the rest: square and multiply */
         for(--expBitSize; expBitSize>=0; expBitSize--) {
            sqrF(pR, pR, pGFEx);
            tblIdx = GetIndex(ppExponent, nItems, expBitSize);
            //cpScrambleGet((Ipp8u*)pT, elmDataSize, pScratchBuffer+tblIdx, nPrecomputed);
            gsScrambleGet_sscm(pT, elmLen, pTable, tblIdx, nItems);
            mulF(pR, pR, pT, pGFEx);
         }

         /* release resourse */
         cpGFpReleasePool(1, pGFEx);
      }

      /* release resourse */
      cpGFpReleasePool(nItems, pGFEx);

      return pR;
   }
}

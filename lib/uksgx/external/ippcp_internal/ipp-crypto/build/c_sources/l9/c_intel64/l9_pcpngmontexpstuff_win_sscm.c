/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     Modular Exponentiation (windowed "safe" version)
*/

#include "owncp.h"

#if defined(_USE_WINDOW_EXP_)

#include "pcpngmontexpstuff.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"

/*
// "safe" fixed-size window montgomery exponentiation
//
// - input/output are in Montgomery Domain
// - possible inplace mode
//
// scratch buffer structure:
//    precomuted table of multipliers[(1<<w)*nsM]
//    RR[nsM]   tmp result if inplace operation
//    TT[nsM]  unscrmbled table entry
//    EE[nsM+1] power expasin
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataExp, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   cpSize nsM = MOD_LEN(pMont);
   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   /*
   // test for special cases:
   //    x^0 = 1
   //    0^e = 0
   */
   if( cpEqu_BNU_CHUNK(dataExp, nsE, 0) ) {
      COPY_BNU(dataY, MOD_MNT_R(pMont), nsM);
   }
   else if( cpEqu_BNU_CHUNK(dataX, nsX, 0) ) {
      ZEXPAND_BNU(dataY, 0, nsM);
   }

   /* general case */
   else {
      cpSize winSize = gsMontExp_WinSize(bitsizeE);
      cpSize nPrecomute= 1<<winSize;
      BNU_CHUNK_T mask = (BNU_CHUNK_T)(nPrecomute -1);
      int n;

      BNU_CHUNK_T* pTable = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pBuffer), CACHE_LINE_SIZE));
      BNU_CHUNK_T* dataTT = pTable + gsGetScrambleBufferSize(nsM, winSize);
      BNU_CHUNK_T* dataRR = dataTT + nsM;
      BNU_CHUNK_T* dataEE = dataRR;

      /* copy and expand base to the modulus length */
      ZEXPAND_COPY_BNU(dataTT, nsM, dataX, nsX);

      /* initialize recource */
      gsScramblePut(pTable, 0, MOD_MNT_R(pMont), nsM, winSize);
      COPY_BNU(dataRR, dataTT, nsM);
      gsScramblePut(pTable, 1, dataTT, nsM, winSize);
      for(n=2; n<nPrecomute; n++) {
         MOD_METHOD( pMont )->mul(dataTT, dataTT, dataRR, pMont);
         gsScramblePut(pTable, n, dataTT, nsM, winSize);
      }

      /* expand exponent*/
      ZEXPAND_COPY_BNU(dataEE, nsM+1, dataExp, nsE);
      bitsizeE = ((bitsizeE+winSize-1)/winSize) *winSize;

      /* exponentiation */
      {
         /* position of the 1-st (left) window */
         int eBit = bitsizeE-winSize;

         /* extract 1-st window value */
         Ipp32u eChunk = *((Ipp32u*)((Ipp16u*)dataEE + eBit/BITSIZE(Ipp16u)));
         int shift = eBit & 0xF;
         Ipp32u winVal = (eChunk>>shift) &mask;

         /* initialize result */
         gsScrambleGet_sscm(dataY, nsM, pTable, (int)winVal, winSize);

         for(eBit-=winSize; eBit>=0; eBit-=winSize) {
            /* do square window times */
            for(n=0,winVal=0; n<winSize; n++) {
               MOD_METHOD( pMont )->sqr(dataY, dataY, pMont);
            }

            /* extract next window value */
            eChunk = *((Ipp32u*)((Ipp16u*)dataEE + eBit/BITSIZE(Ipp16u)));
            shift = eBit & 0xF;
            winVal = (eChunk>>shift) &mask;

            /* exptact precomputed value and muptiply */
            gsScrambleGet_sscm(dataTT, nsM, pTable, (int)winVal, winSize);

            MOD_METHOD( pMont )->mul(dataY, dataY, dataTT, pMont);
         }
      }
   }

   return nsM;
}

/*
// "safe" fixed-size window exponentiation
// - input/output are in Regular Domain
// - possible inplace mode
*/
IPP_OWN_DEFN (cpSize, gsModExpWin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataExp, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   cpSize nsM = MOD_LEN(pMont);

   /* copy and expand base to the modulus length */
   ZEXPAND_COPY_BNU(dataY, nsM, dataX, nsX);

   /* convert base to Montgomery domain */
   MOD_METHOD(pMont)->encode(dataY, dataY, pMont);

   /* exponentiation */
   gsMontExpWin_BNU_sscm(dataY, dataY, nsM, dataExp, bitsizeE, pMont, pBuffer);

   /* convert result back to regular domain */
   MOD_METHOD(pMont)->decode(dataY, dataY, pMont);

   return nsM;
}

#endif /* _USE_WINDOW_EXP_ */

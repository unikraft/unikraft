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
//  Contents:
//        cpMontExpWin_BN_sscm()
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "gsscramble.h"

//tbcd: temporary excluded: #include <assert.h>

#if defined(_USE_WINDOW_EXP_)

IPP_OWN_DEFN (static void, gsMul_school, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pX, const BNU_CHUNK_T* pY, cpSize len, BNU_CHUNK_T* pKbuffer))
{
   IPP_UNREFERENCED_PARAMETER(pKbuffer);
   cpMul_BNU_school(pR, pX, len, pY, len);
}
IPP_OWN_DEFN (static void, gsSqr_school, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pX, cpSize len, BNU_CHUNK_T* pKbuffer))
{
   IPP_UNREFERENCED_PARAMETER(pKbuffer);
   cpSqr_BNU_school(pR, pX, len);
}

IPP_OWN_FUNPTR (void, gsMul, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pX, const BNU_CHUNK_T* pY, cpSize len, BNU_CHUNK_T* pKbuffer))
IPP_OWN_FUNPTR (void, gsSqr, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pX, cpSize len, BNU_CHUNK_T* pKbuffer))

/*F*
// Name: cpMontExpWin_BN_sscm
//
// Purpose: Binary method of Exponentiation
//
//
// Parameters:
//      pX           big number integer of Montgomery form within the
//                      range [0,m-1]
//      pE           big number exponent
//      pMont        Montgomery modulus of IppsMontState.
/       pY           the Montgomery exponentation result.
//
*F*/

IPP_OWN_DEFN (void, cpMontExpWin_BN_sscm, (IppsBigNumState* pY, const IppsBigNumState* pX, const IppsBigNumState* pE, gsModEngine* pMont, BNU_CHUNK_T* pResource))
{
   BNU_CHUNK_T* dataX = BN_NUMBER(pX);
   cpSize nsX = BN_SIZE(pX);

   BNU_CHUNK_T* dataE = BN_NUMBER(pE);
   cpSize nsE = BN_SIZE(pE);

   BNU_CHUNK_T* dataY = BN_NUMBER(pY);
   cpSize nsM = MOD_LEN(pMont);

   /*
   // test for special cases:
   //    x^0 = 1
   //    0^e = 0
   */
   if( cpEqu_BNU_CHUNK(dataE, nsE, 0) ) {
      COPY_BNU(dataY, MOD_MNT_R(pMont), nsM);
   }
   else if( cpEqu_BNU_CHUNK(dataX, nsX, 0) ) {
      ZEXPAND_BNU(dataY, 0, nsM);
   }

   /* general case */
   else {
      /* Montgomery engine buffers */
      const int usedPoolLen = 2;
      BNU_CHUNK_T* pBuffer  = gsModPoolAlloc(pMont, usedPoolLen);

      BNU_CHUNK_T* pKBuffer = pBuffer + nsM;

      /* mul & sqr functions */
      gsMul mulFun = gsMul_school;
      gsSqr sqrFun = gsSqr_school;

      /* fixed window param */
      cpSize bitsizeE = BITSIZE_BNU(dataE, nsE);
      cpSize window = cpMontExp_WinSize(bitsizeE);
      BNU_CHUNK_T mask = (1<<window) -1;
      cpSize nPrecomute= 1<<window;
      //cpSize chunkSize = CACHE_LINE_SIZE/nPrecomute;
      int n;

      /* expand base */
      BNU_CHUNK_T* dataT = BN_BUFFER(pY);
      ZEXPAND_COPY_BNU(dataY, nsM, dataX, nsX);

      /* initialize recource */
      pResource = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pResource, CACHE_LINE_SIZE));

      //tbcd: temporary excluded: assert(NULL!=pBuffer);

      //cpScramblePut(((Ipp8u*)pResource)+0, chunkSize, (Ipp32u*)MOD_MNT_R(pMont), nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
      gsScramblePut(pResource, 0, MOD_MNT_R(pMont), nsM, window);
      ZEXPAND_COPY_BNU(dataT, nsM, dataX, nsX);
      //cpScramblePut(((Ipp8u*)pResource)+chunkSize, chunkSize, (Ipp32u*)dataT, nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
      gsScramblePut(pResource, 1, dataT, nsM, window);
      for(n=2; n<nPrecomute; n++) {
         mulFun(pBuffer, dataT, dataY, nsM, pKBuffer);
         cpMontRed_BNU(dataT, pBuffer, pMont);
         //cpScramblePut(((Ipp8u*)pResource)+n*chunkSize, chunkSize, (Ipp32u*)dataT, nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
         gsScramblePut(pResource, n, dataT, nsM, window);
      }

      /* expand exponent*/
      dataE[nsE] = 0;
      bitsizeE = ((bitsizeE+window-1)/window) *window;

      /* exponentiation */
      {
         /* position of the 1-st (left) window */
         int eBit = bitsizeE-window;

         /* extract 1-st window value */
         Ipp32u eChunk = *((Ipp32u*)((Ipp16u*)dataE + eBit/BITSIZE(Ipp16u)));
         int shift = eBit & 0xF;
         cpSize windowVal = (cpSize)((eChunk>>shift) &mask);

         /* initialize result */
         //cpScrambleGet((Ipp32u*)dataY, nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u), ((Ipp8u*)pResource)+windowVal*chunkSize, chunkSize);
         gsScrambleGet_sscm(dataY, nsM, pResource, windowVal, window);

         for(eBit-=window; eBit>=0; eBit-=window) {
            /* do squaring window-times */
            for(n=0,windowVal=0; n<window; n++) {
               sqrFun(pBuffer, dataY, nsM, pKBuffer);
               cpMontRed_BNU(dataY, pBuffer, pMont);
            }

            /* extract next window value */
            eChunk = *((Ipp32u*)((Ipp16u*)dataE + eBit/BITSIZE(Ipp16u)));
            shift = eBit & 0xF;
            windowVal = (cpSize)((eChunk>>shift) &mask);

            /* exptact precomputed value and muptiply */
            //cpScrambleGet((Ipp32u*)dataT, nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u), ((Ipp8u*)pResource)+windowVal*chunkSize, chunkSize);
            gsScrambleGet_sscm(dataT, nsM, pResource, windowVal, window);

            mulFun(pBuffer, dataY, dataT, nsM, pKBuffer);
            cpMontRed_BNU(dataY, pBuffer, pMont);
         }
      }

      gsModPoolFree(pMont, usedPoolLen);
   }

   FIX_BNU(dataY, nsM);
   BN_SIZE(pY) = nsM;
   BN_SIGN(pY) = ippBigNumPOS;
}

#endif /* _USE_WINDOW_EXP_ */

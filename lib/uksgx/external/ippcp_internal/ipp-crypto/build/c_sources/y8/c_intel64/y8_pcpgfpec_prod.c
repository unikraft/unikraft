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
//     Internal EC over GF(p^m) basic Definitions & Function Prototypes
// 
//     Context:
//        gfec_point_prod()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"


IPP_OWN_DEFN (void, gfec_point_prod, (BNU_CHUNK_T* pointR, const BNU_CHUNK_T* pointA, const Ipp8u* scalarA, const BNU_CHUNK_T* pointB, const Ipp8u* scalarB, int scalarBitSize, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
{
   int pointLen = ECP_POINTLEN(pEC);

   /* optimal size of window */
   const int window_size = 5;
   /* number of table entries */
   const int tableLen = 1<<(window_size-1);

   /* aligned pre-computed tables */
   BNU_CHUNK_T* pTableA = (BNU_CHUNK_T*)IPP_ALIGNED_PTR(pScratchBuffer, CACHE_LINE_SIZE);
   BNU_CHUNK_T* pTableB = pTableA+pointLen*tableLen;

   setupTable(pTableA, pointA, pEC);
   setupTable(pTableB, pointB, pEC);

   {
      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      int elemLen = GFP_FELEN(pGFE);

      mod_neg negF = GFP_METHOD(pGFE)->neg;

      BNU_CHUNK_T* pHy = cpGFpGetPool(1, pGFE);

      BNU_CHUNK_T* pTdata = cpEcGFpGetPool(1, pEC); /* points from the pool */
      BNU_CHUNK_T* pHdata = cpEcGFpGetPool(1, pEC);

      int wvalue;
      Ipp8u digit, sign;
      int mask = (1<<(window_size+1)) -1;
      int bit = scalarBitSize-(scalarBitSize%window_size);

      /* first window */
      if(bit) {
         wvalue = *((Ipp16u*)&scalarA[(bit-1)/8]);
         wvalue = (wvalue>> ((bit-1)%8)) & mask;
      }
      else
         wvalue = 0;
      booth_recode(&sign, &digit, (Ipp8u)wvalue, window_size);
      gsScrambleGet_sscm(pTdata, pointLen, pTableA, digit-1, 5-1);

      if(bit) {
         wvalue = *((Ipp16u*)&scalarB[(bit-1)/8]);
         wvalue = (wvalue>> ((bit-1)%8)) & mask;
      }
      else
         wvalue = 0;
      booth_recode(&sign, &digit, (Ipp8u)wvalue, window_size);
      gsScrambleGet_sscm(pHdata, pointLen, pTableB, digit-1, 5-1);

      gfec_point_add(pTdata, pTdata, pHdata, pEC);

      for(bit-=window_size; bit>=window_size; bit-=window_size) {
         gfec_point_double(pTdata, pTdata, pEC);
         gfec_point_double(pTdata, pTdata, pEC);
         gfec_point_double(pTdata, pTdata, pEC);
         gfec_point_double(pTdata, pTdata, pEC);
         gfec_point_double(pTdata, pTdata, pEC);

         wvalue = *((Ipp16u*)&scalarA[(bit-1)/8]);
         wvalue = (wvalue>> ((bit-1)%8)) & mask;
         booth_recode(&sign, &digit, (Ipp8u)wvalue, window_size);
         gsScrambleGet_sscm(pHdata, pointLen, pTableA, digit-1, 5-1);

         negF(pHy, pHdata+elemLen, pGFE);
         cpMaskedReplace_ct(pHdata+elemLen, pHy, elemLen, ~cpIsZero_ct(sign));
         gfec_point_add(pTdata, pTdata, pHdata, pEC);

         wvalue = *((Ipp16u*)&scalarB[(bit-1)/8]);
         wvalue = (wvalue>> ((bit-1)%8)) & mask;
         booth_recode(&sign, &digit, (Ipp8u)wvalue, window_size);
         gsScrambleGet_sscm(pHdata, pointLen, pTableB, digit-1, 5-1);

         negF(pHy, pHdata+elemLen, pGFE);
         cpMaskedReplace_ct(pHdata+elemLen, pHy, elemLen, ~cpIsZero_ct(sign));
         gfec_point_add(pTdata, pTdata, pHdata, pEC);
      }
      /* last window */
      gfec_point_double(pTdata, pTdata, pEC);
      gfec_point_double(pTdata, pTdata, pEC);
      gfec_point_double(pTdata, pTdata, pEC);
      gfec_point_double(pTdata, pTdata, pEC);
      gfec_point_double(pTdata, pTdata, pEC);

      wvalue = *((Ipp16u*)&scalarA[0]);
      wvalue = (wvalue << 1) & mask;
      booth_recode(&sign, &digit, (Ipp8u)wvalue, window_size);
      gsScrambleGet_sscm(pHdata, pointLen, pTableA, digit-1, 5-1);

      negF(pHy, pHdata+elemLen, pGFE);
      cpMaskedReplace_ct(pHdata+elemLen, pHy, elemLen, ~cpIsZero_ct(sign));
      gfec_point_add(pTdata, pTdata, pHdata, pEC);

      wvalue = *((Ipp16u*)&scalarB[0]);
      wvalue = (wvalue << 1) & mask;
      booth_recode(&sign, &digit, (Ipp8u)wvalue, window_size);
      gsScrambleGet_sscm(pHdata, pointLen, pTableB, digit-1, 5-1);

      negF(pHy, pHdata+elemLen, pGFE);
      cpMaskedReplace_ct(pHdata+elemLen, pHy, elemLen, ~cpIsZero_ct(sign));
      gfec_point_add(pTdata, pTdata, pHdata, pEC);

      cpGFpElementCopy(pointR, pTdata, pointLen);

      cpEcGFpReleasePool(2, pEC);
      cpGFpReleasePool(1, pGFE);
   }
}

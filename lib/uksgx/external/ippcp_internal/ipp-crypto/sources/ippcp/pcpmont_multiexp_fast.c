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
// 
//     Context:
//        cpFastMontMultiExp()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"


static cpSize GetIndex(const Ipp8u** ppE, cpSize numItems, cpSize nBit)
{
   cpSize shift = nBit%BYTESIZE;
   cpSize offset= nBit/BYTESIZE;
   cpSize index = 0;

   cpSize n;
   for(n=numItems; n>0; n--) {
      const Ipp8u* pE = ppE[n-1] + offset;
      Ipp8u e = pE[0];
      index <<= 1;
      index += (e>>shift) &1;
   }
   return index;
}

/*
// Computes multi-exponentiation
//    y = x[0]^e[0] * x[1]^e[1] *...* x[numItems-1]^e[numItems-1] mod M
//
// Input:
//    - table pTbl of precomuted values pTbl[i] = x[0]^i[0] * x[1]^i[1] *...* x[numItems-1]^i[numItems-1] mod M,
//      where i[0], i[1], ..., i[numItems-1] are bits of i value;
//      each entry has sizeM length (i.e. equial to modulo M size)
//    - array of pointers to the BNU exponents e[0], e[1],...,e[numItems-1]
//    - pointer to the Montgomery engine
*/
IPP_OWN_DEFN (void, cpFastMontMultiExp, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pPrecomTbl, const Ipp8u** ppE, cpSize eItemBitSize, cpSize numItems, gsModEngine* pModEngine))
{
   cpSize sizeM = MOD_LEN(pModEngine);

   /* find 1-st non zero index */
   cpSize eBitNumber;
   cpSize tblIdx;
   for(eBitNumber=eItemBitSize-1, tblIdx=0; !tblIdx && eBitNumber>=0; eBitNumber--)
      tblIdx =GetIndex(ppE, numItems, eBitNumber);

   COPY_BNU(pY, pPrecomTbl+tblIdx*sizeM, sizeM);

   for(; eBitNumber>=0; eBitNumber--) {
      cpMontMul_BNU(pY,
                    pY,
                    pY,
                    pModEngine);

      tblIdx = GetIndex(ppE, numItems, eBitNumber);

      if(tblIdx)
         cpMontMul_BNU(pY,
                       pY,
                       pPrecomTbl+tblIdx*sizeM,
                       pModEngine);
   }
}

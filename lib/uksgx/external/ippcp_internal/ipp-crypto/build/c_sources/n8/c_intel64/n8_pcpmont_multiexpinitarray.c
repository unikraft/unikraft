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
//        cpMontMultiExpInitArray()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"

/* 
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
// 
// 
*/
/*
// Initialize multi-exponentiation computation
//    y = x[0]^e[0] * x[1]^e[1] *...* x[numItems-1]^e[numItems-1] mod M
//
// Output:
//    - table pTbl of precomuted values pTbl[i] = x[0]^i[0] * x[1]^i[1] *...* x[numItems-1]^i[numItems-1] mod M,
//      where i[0], i[1], ..., i[numItems-1] are bits of i value;
//
// Input:
//    - array of pointers to the BNU bases x[0], x[1],...,x[numItems-1]
//    - pointer to the Montgomery engine
*/
IPP_OWN_DEFN (void, cpMontMultiExpInitArray, (BNU_CHUNK_T* pPrecomTbl, const BNU_CHUNK_T** ppX, cpSize xItemBitSize, cpSize numItems, gsModEngine* pModEngine))
{
   cpSize sizeM = MOD_LEN(pModEngine);

   cpSize i, base;
   cpSize sizeX = BITS_BNU_CHUNK(xItemBitSize);

   /* buff[0] = mont(1) */
   COPY_BNU(pPrecomTbl, MOD_MNT_R(pModEngine), sizeM);
   /* buff[1] = X[0] */
   ZEXPAND_COPY_BNU(pPrecomTbl+sizeM, sizeM, ppX[0], sizeX);

   for(i=1,base=2*sizeM; i<numItems; i++,base*=2) {
      /* buf[base] = X[i] */
      ZEXPAND_COPY_BNU(pPrecomTbl+base, sizeM, ppX[i], sizeX);

      {
         int nPasses = 1;
         int step = base/2;

         int k;
         for(k=i-1; k>=0; k--) {
            const BNU_CHUNK_T* pXterm = ppX[k];

            BNU_CHUNK_T* pBufferBase = pPrecomTbl+base;
            int n;
            for(n=1; n<=nPasses; n++, pBufferBase+=2*step) {
               cpMontMul_BNU_EX(pBufferBase+step,
                                pBufferBase,      sizeM,
                                pXterm,           sizeX,
                                pModEngine);
            }

            nPasses *= 2;
            step /= 2;
         }
      }
   }
}

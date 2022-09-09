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
//     Internal operations over prime GF(p).
//
//     Context:
//        cpGFpRand
//
*/
#include "owncp.h"

#include "pcpbn.h"
#include "pcpgfpstuff.h"

//tbcd: temporary excluded: #include <assert.h>

IPP_OWN_DEFN (BNU_CHUNK_T*, cpGFpRand, (BNU_CHUNK_T* pR, gsModEngine* pGFE, IppBitSupplier rndFunc, void* pRndParam))
{
   int elemLen = GFP_FELEN(pGFE);
   int reqBitSize = GFP_FEBITLEN(pGFE)+GFP_RAND_ADD_BITS;
   int nsR = (reqBitSize +BITSIZE(BNU_CHUNK_T)-1)/BITSIZE(BNU_CHUNK_T);

   int internal_err;

   BNU_CHUNK_T* pPool = cpGFpGetPool(2, pGFE);
   //tbcd: temporary excluded: assert(pPool!=NULL);

   cpGFpElementPad(pPool, nsR, 0);

   internal_err = ippStsNoErr != rndFunc((Ipp32u*)pPool, reqBitSize, pRndParam);

   if(!internal_err) {
      nsR = cpMod_BNU(pPool, nsR, GFP_MODULUS(pGFE), elemLen);
      cpGFpElementPad(pPool+nsR, elemLen-nsR, 0);
      GFP_METHOD(pGFE)->encode(pR, pPool, pGFE);
   }

   cpGFpReleasePool(2, pGFE);
   return internal_err? NULL : pR;
}

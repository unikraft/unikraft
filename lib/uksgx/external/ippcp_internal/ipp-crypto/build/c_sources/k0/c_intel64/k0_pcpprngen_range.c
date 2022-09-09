/*******************************************************************************
* Copyright 2004-2021 Intel Corporation
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
//     PRNG Functions
// 
//  Contents:
//        cpPRNGenRange()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcphash.h"
#include "pcpprng.h"
#include "pcptool.h"

/* generates random string of specified bitSize length
   within specified ragnge lo < r < Hi
   returns:
      0 random bit string not generated
      1 random bit string generated
     -1 detected internal error (ippStsNoErr != rndFunc())
*/
IPP_OWN_DEFN (int, cpPRNGenRange, (BNU_CHUNK_T* pRand, const BNU_CHUNK_T* pLo, cpSize loLen, const BNU_CHUNK_T* pHi, cpSize hiLen, IppBitSupplier rndFunc, void* pRndParam))
{
   int bitSize = BITSIZE_BNU(pHi,hiLen);
   BNU_CHUNK_T topMask = MASK_BNU_CHUNK(bitSize);

   #define MAX_COUNT (1000)
   int n;
   for(n=0; n<MAX_COUNT; n++) {
      cpSize randLen;
      IppStatus sts = rndFunc((Ipp32u*)pRand, bitSize, pRndParam);
      if(ippStsNoErr!=sts) return -1;

      pRand[hiLen-1] &= topMask;
      randLen = cpFix_BNU(pRand, hiLen);
      if((0 < cpCmp_BNU(pRand, randLen, pLo, loLen)) && (0 < cpCmp_BNU(pHi, hiLen, pRand, randLen)))
         return 1;
   }
   #undef MAX_COUNT

   return 0; /* no random matched to (Lo,Hi) */
}

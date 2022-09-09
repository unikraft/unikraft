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
//        cpPRNGenPattern()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcphash.h"
#include "pcpprng.h"
#include "pcptool.h"

/* generates random string of specified bitSize length
   returns:
      1 random bit string generated
     -1 detected internal error (ippStsNoErr != rndFunc())
*/
IPP_OWN_DEFN (int, cpPRNGenPattern, (BNU_CHUNK_T* pRand, int bitSize, BNU_CHUNK_T botPattern, BNU_CHUNK_T topPattern, IppBitSupplier rndFunc, void* pRndParam))
{
   BNU_CHUNK_T topMask = MASK_BNU_CHUNK(bitSize);
   cpSize randLen = BITS_BNU_CHUNK(bitSize);

   IppStatus sts = rndFunc((Ipp32u*)pRand, bitSize, pRndParam);
   if(ippStsNoErr!=sts) return -1;

   pRand[randLen-1] &= topMask;
   pRand[0] |= botPattern;
   pRand[randLen-1] |= topPattern;
   return 1;
}

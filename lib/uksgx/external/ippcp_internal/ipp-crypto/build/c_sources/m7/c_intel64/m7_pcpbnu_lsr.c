/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal Unsigned BNU misc functionality
// 
//  Contents:
//     cpLSL_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"


/*F*
//    Name: cpLSR_BNU
//
// Purpose: Logical shift right (including inplace).
//
// Returns:
//       new length
//
// Parameters:
//    pA          BigNum A
//    pR          result BigNum
//    nsA         size of A
//    nBits       size of shift in bits 
*F*/

IPP_OWN_DEFN (cpSize, cpLSR_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize nsA, cpSize nBits))
{
   cpSize nw = nBits/BNU_CHUNK_BITS;
   cpSize n;

   pA += nw;
   nsA -= nw;

   nBits %= BNU_CHUNK_BITS;
   if(nBits) {
      BNU_CHUNK_T hi;
      BNU_CHUNK_T lo = pA[0];

      for(n=0; n<(nsA-1); n++) {
         hi = pA[n+1];
         pR[n] = (lo>>nBits) | (hi<<(BNU_CHUNK_BITS-nBits));
         lo = hi;
      }
      pR[nsA-1] = (lo>>nBits);
   }
   else {
      for(n=0; n<nsA; n++)
         pR[n] = pA[n];
   }

   for(n=0; n<nw; n++)
      pR[nsA+n] = 0;

   return nsA+nw;
}

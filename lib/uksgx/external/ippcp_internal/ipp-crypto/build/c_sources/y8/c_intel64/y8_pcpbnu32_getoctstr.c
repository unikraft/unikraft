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
//     Unsigned internal BNU32 misc functionality
// 
//  Contents:
//     cpToOctStr_BNU32()
// 
*/

#include "owncp.h"
#include "pcpbnuimpl.h"
#include "pcpbnumisc.h"
#include "pcpbnu32misc.h"

/*F*
//    Name: cpToOctStr_BNU32
//
// Purpose: Convert BNU into HexString representation.
//
// Returns:
//       length of the string or 0 if no success
//
// Parameters:
//    pBNU        pointer to the source BN
//    bnuSize     size of BN
//    pStr        pointer to the target octet string
//    strLen      octet string length
*F*/

IPP_OWN_DEFN (cpSize, cpToOctStr_BNU32, (Ipp8u* pStr, cpSize strLen, const Ipp32u* pBNU, cpSize bnuSize))
{
   FIX_BNU32(pBNU, bnuSize);
   {
      int bnuBitSize = BITSIZE_BNU32(pBNU, bnuSize);
      if(bnuBitSize <= strLen*BYTESIZE) {
         Ipp32u x = pBNU[bnuSize-1];

         ZEXPAND_BNU(pStr, 0, strLen);
         pStr += strLen - BITS2WORD8_SIZE(bnuBitSize);

         if(x) {
            int nb;
            for(nb=cpNLZ_BNU32(x)/BYTESIZE; nb<4; nb++)
               *pStr++ = EBYTE(x,3-nb);

            for(--bnuSize; bnuSize>0; bnuSize--) {
               x = pBNU[bnuSize-1];
               *pStr++ = EBYTE(x,3);
               *pStr++ = EBYTE(x,2);
               *pStr++ = EBYTE(x,1);
               *pStr++ = EBYTE(x,0);
            }
         }
         return strLen;
      }
      else
         return 0;
   }
}

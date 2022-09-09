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
//     cpToOctStr_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"


/*
// Convert BNU into HexString representation
//
// Returns length of the string or 0 if no success
*/

/*F*
//    Name: cpToOctStr_BNU
//
// Purpose: Convert BNU into HexString representation.
//
// Returns:  
//       length of the string or 0 if no success
//
// Parameters:
//    pA          pointer to the source BN A
//    nsA         size of A
//    pStr        pointer to the target octet string
//    strLen      octet string length
*F*/

IPP_OWN_DEFN (cpSize, cpToOctStr_BNU, (Ipp8u* pStr, cpSize strLen, const BNU_CHUNK_T* pA, cpSize nsA))
{
   FIX_BNU(pA, nsA);
   {
      cpSize bnuBitSize = BITSIZE_BNU(pA, nsA);
      if(bnuBitSize <= strLen*BYTESIZE) {
         int cnvLen = 0;
         BNU_CHUNK_T x = pA[nsA-1];

         ZEXPAND_BNU(pStr, 0, strLen);
         pStr += strLen - BITS2WORD8_SIZE(bnuBitSize);

         if(x) {
            //int nb;
            cpSize nb;
            for(nb=cpNLZ_BNU(x)/BYTESIZE; nb<(cpSize)(sizeof(BNU_CHUNK_T)); cnvLen++, nb++)
               *pStr++ = EBYTE(x, (Ipp32s)sizeof(BNU_CHUNK_T)-1-nb);

            for(--nsA; nsA>0; cnvLen+=(Ipp32s)sizeof(BNU_CHUNK_T), nsA--) {
               x = pA[nsA-1];
               #if (BNU_CHUNK_BITS==BNU_CHUNK_64BIT)
               *pStr++ = EBYTE(x,7);
               *pStr++ = EBYTE(x,6);
               *pStr++ = EBYTE(x,5);
               *pStr++ = EBYTE(x,4);
               #endif
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

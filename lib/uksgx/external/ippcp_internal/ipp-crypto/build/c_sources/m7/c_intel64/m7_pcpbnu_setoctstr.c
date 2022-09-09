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
//     cpFromOctStr_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"

/*F*
//    Name: cpFromOctStr_BNU
//
// Purpose: Convert Oct String into BNU representation.
//
// Returns:                 
//          size of BNU in BNU_CHUNK_T chunks
//
// Parameters:
//    pStr        pointer to the source octet string
//    strLen      octet string length
//    pA          pointer to the target BN
//
*F*/

IPP_OWN_DEFN (cpSize, cpFromOctStr_BNU, (BNU_CHUNK_T* pA, const Ipp8u* pStr, cpSize strLen))
{
   int nsA =0;

   /* start from the end of string */
   for(; strLen>=(int)sizeof(BNU_CHUNK_T); nsA++,strLen-=(int)(sizeof(BNU_CHUNK_T))) {
      /* pack sizeof(BNU_CHUNK_T) bytes into single BNU_CHUNK_T value*/
      *pA++ =
         #if (BNU_CHUNK_BITS==BNU_CHUNK_64BIT)
         +( (BNU_CHUNK_T)pStr[strLen-8]<<(8*7) )
         +( (BNU_CHUNK_T)pStr[strLen-7]<<(8*6) )
         +( (BNU_CHUNK_T)pStr[strLen-6]<<(8*5) )
         +( (BNU_CHUNK_T)pStr[strLen-5]<<(8*4) )
         #endif
         +( (BNU_CHUNK_T)pStr[strLen-4]<<(8*3) )
         +( (BNU_CHUNK_T)pStr[strLen-3]<<(8*2) )
         +( (BNU_CHUNK_T)pStr[strLen-2]<<(8*1) )
         +  (BNU_CHUNK_T)pStr[strLen-1];
   }

   /* convert the beginning of the string */
   if(strLen) {
      BNU_CHUNK_T x = 0;
      for(x=0; strLen>0; strLen--) {
         BNU_CHUNK_T d = *pStr++;
         x = (x<<8) + d;
       }
       *pA++ = x;
       nsA++;
   }

   return nsA;
}

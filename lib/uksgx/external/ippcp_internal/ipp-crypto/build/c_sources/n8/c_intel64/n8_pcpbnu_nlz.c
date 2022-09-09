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
//     cpNLZ_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpmask_ct.h"

/*F*
//    Name: cpNLZ_BNU
//
// Purpose: Returns number of leading zeros of the BNU.
//
// Returns:
//       number of leading zeros of the BNU
//
// Parameters:
//    x         BigNum x
//
*F*/

#if !((_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9))
#if 0
   IPP_OWN_DEFN (cpSize, cpNLZ_BNU, (BNU_CHUNK_T x))
   {
      cpSize nlz = BNU_CHUNK_BITS;
      if(x) {
         nlz = 0;
         #if (BNU_CHUNK_BITS == BNU_CHUNK_64BIT)
         if( 0==(x & 0xFFFFFFFF00000000) ) { nlz +=32; x<<=32; }
         if( 0==(x & 0xFFFF000000000000) ) { nlz +=16; x<<=16; }
         if( 0==(x & 0xFF00000000000000) ) { nlz += 8; x<<= 8; }
         if( 0==(x & 0xF000000000000000) ) { nlz += 4; x<<= 4; }
         if( 0==(x & 0xC000000000000000) ) { nlz += 2; x<<= 2; }
         if( 0==(x & 0x8000000000000000) ) { nlz++; }
         #else
         if( 0==(x & 0xFFFF0000) ) { nlz +=16; x<<=16; }
         if( 0==(x & 0xFF000000) ) { nlz += 8; x<<= 8; }
         if( 0==(x & 0xF0000000) ) { nlz += 4; x<<= 4; }
         if( 0==(x & 0xC0000000) ) { nlz += 2; x<<= 2; }
         if( 0==(x & 0x80000000) ) { nlz++; }
         #endif
      }
      return nlz;
   }
#endif
/* cte version */
IPP_OWN_DEFN (cpSize, cpNLZ_BNU, (BNU_CHUNK_T x))
{
   cpSize nlz = 0;
   BNU_CHUNK_T
   #if (BNU_CHUNK_BITS == BNU_CHUNK_64BIT)
   mask = cpIsZero_ct(x & 0xFFFFFFFF00000000);
   nlz += 32 & mask; x = ((x<<32) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xFFFF000000000000);
   nlz += 16 & mask; x = ((x<<16) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xFF00000000000000);
   nlz += 8 & mask; x = ((x << 8) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xF000000000000000);
   nlz += 4 & mask; x = ((x << 4) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xC000000000000000);
   nlz += 2 & mask; x = ((x << 2) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0x8000000000000000);
   nlz += 1 & mask; x = ((x << 1) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0x8000000000000000);
   nlz += 1 & mask;
#else
   mask = cpIsZero_ct(x & 0xFFFF0000);
   nlz += 16 & mask; x = ((x << 16) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xFF000000);
   nlz += 8 & mask; x = ((x << 8) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xF0000000);
   nlz += 4 & mask; x = ((x << 4) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0xC0000000);
   nlz += 2 & mask; x = ((x << 2) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0x80000000);
   nlz += 1 & mask; x = ((x << 1) & mask) | (x & ~mask);

   mask = cpIsZero_ct(x & 0x80000000);
   nlz += 1 & mask;
#endif
   return nlz;
}

#endif


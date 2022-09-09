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
//     cpNTZ_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"


/*F*
//    Name: cpNTZ_BNU
//
// Purpose: Returns number of trailing zeros of the BNU.
//
// Returns:
//       number of trailing zeros of the BNU
//
// Parameters:
//    x         BigNum x
//
*F*/

IPP_OWN_DEFN (cpSize, cpNTZ_BNU, (BNU_CHUNK_T x))
{
   cpSize ntz = BNU_CHUNK_BITS;
   if(x) {
      ntz = 0;
      #if (BNU_CHUNK_BITS==BNU_CHUNK_64BIT)
      if( 0==(x & 0x00000000FFFFFFFF) ) { ntz+=32; x>>=32; }
      if( 0==(x & 0x000000000000FFFF) ) { ntz+=16; x>>=16; }
      if( 0==(x & 0x00000000000000FF) ) { ntz+= 8; x>>= 8; }
      if( 0==(x & 0x000000000000000F) ) { ntz+= 4; x>>= 4; }
      if( 0==(x & 0x0000000000000003) ) { ntz+= 2; x>>= 2; }
      if( 0==(x & 0x0000000000000001) ) { ntz++; }
      #else
      if( 0==(x & 0x0000FFFF) )         { ntz+=16; x>>=16; }
      if( 0==(x & 0x000000FF) )         { ntz+= 8; x>>= 8; }
      if( 0==(x & 0x0000000F) )         { ntz+= 4; x>>= 4; }
      if( 0==(x & 0x00000003) )         { ntz+= 2; x>>= 2; }
      if( 0==(x & 0x00000001) )         { ntz++; }
      #endif
   }
   return ntz;
}

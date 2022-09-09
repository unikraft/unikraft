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
//     cpNLZ_BNU32()
// 
*/

#include "owncp.h"
#include "pcpbnuimpl.h"
#include "pcpbnumisc.h"
#include "pcpbnu32misc.h"

/*F*
//    Name: cpNLZ_BNU32
//
// Purpose: Returns number of leading zeros of the 32-bit BN chunk.
//
// Returns:
//       number of leading zeros of the 32-bit BN chunk
//
// Parameters:
//    x         BigNum x
//
*F*/

#if (_IPP < _IPP_H9)
   IPP_OWN_DEFN (cpSize, cpNLZ_BNU32, (Ipp32u x))
   {
      cpSize nlz = BITSIZE(Ipp32u);
      if(x) {
         nlz = 0;
         if( 0==(x & 0xFFFF0000) ) { nlz +=16; x<<=16; }
         if( 0==(x & 0xFF000000) ) { nlz += 8; x<<= 8; }
         if( 0==(x & 0xF0000000) ) { nlz += 4; x<<= 4; }
         if( 0==(x & 0xC0000000) ) { nlz += 2; x<<= 2; }
         if( 0==(x & 0x80000000) ) { nlz++; }
      }
      return nlz;
   }
#endif

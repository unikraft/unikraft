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
//     cpMSBit_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"

/*F*
//    Name: cpMSBit_BNU
//
// Purpose: Returns Most Significant Bit of the BNU.
//
// Returns:
//       Most Significant Bit of the BNU
//
// Parameters:
//    pA          BigNum A
//    nsA         size of A
//
// Note:
//    if BNU==0, -1 will return
*F*/

IPP_OWN_DEFN (int, cpMSBit_BNU, (const BNU_CHUNK_T* pA, cpSize nsA))
{
   int msb;
   FIX_BNU(pA, nsA);
   msb  = nsA*BNU_CHUNK_BITS - cpNLZ_BNU(pA[nsA-1]) -1;
   return msb;
}

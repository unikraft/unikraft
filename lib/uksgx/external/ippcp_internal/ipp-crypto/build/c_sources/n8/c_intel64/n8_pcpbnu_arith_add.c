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
//     Internal Unsigned arithmetic
// 
//  Contents:
//     cpAdd_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnuarith.h"
#include "pcpbnumisc.h"


/*F*
//    Name: cpAdd_BNU
//
// Purpose: Addition of two BigNums.
//
// Returns:
//    carry of result of add two BigNums.
//
// Parameters:
//    pA    source BigNum A
//    pB    source BigNum B
//    pR    resultant BigNum
//    ns    size of BigNums
*F*/
#if !((_IPP==_IPP_W7) || \
      (_IPP==_IPP_T7) || \
      (_IPP==_IPP_V8) || \
      (_IPP==_IPP_P8) || \
      (_IPP>=_IPP_G9) || \
      (_IPP==_IPP_S8) || \
      (_IPP32E==_IPP32E_M7) || \
      (_IPP32E==_IPP32E_U8) || \
      (_IPP32E==_IPP32E_Y8) || \
      (_IPP32E>=_IPP32E_E9) || \
      (_IPP32E==_IPP32E_N8)) || \
      defined(_USE_C_cpAdd_BNU_)
IPP_OWN_DEFN (BNU_CHUNK_T, cpAdd_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, cpSize ns))
{
   BNU_CHUNK_T carry = 0;
   cpSize i;
   for(i=0; i<ns; i++) {
      ADD_ABC(carry, pR[i], pA[i],pB[i], carry);
   }
   return carry;
}
#endif

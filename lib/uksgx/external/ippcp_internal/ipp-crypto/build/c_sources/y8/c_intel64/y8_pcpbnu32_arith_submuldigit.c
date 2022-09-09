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
//     Internal BNU32 arithmetic.
// 
//  Contents:
//     cpSubMulDgt_BNU32()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnu32misc.h"
#include "pcpbnu32arith.h"

/*F*
//    Name: cpMulDgt_BNU32
//
// Purpose: multiply and subtract BNU32.
//
// Returns:
//    carry
//
// Parameters:
//    pA    source
//    nsA   size of A
//    val   digit to mul
//    pR    result
//
*F*/

#if !((_IPP32E==_IPP32E_M7) || \
      (_IPP32E==_IPP32E_U8) || \
      (_IPP32E==_IPP32E_Y8) || \
      (_IPP32E>=_IPP32E_E9) || \
      (_IPP32E==_IPP32E_N8))
IPP_OWN_DEFN (Ipp32u, cpSubMulDgt_BNU32, (Ipp32u* pR, const Ipp32u* pA, cpSize nsA, Ipp32u val))
{
   Ipp32u carry = 0;
   for(; nsA>0; nsA--) {
      Ipp64u r = (Ipp64u)*pR - (Ipp64u)(*pA++) * val - carry;
      *pR++  = IPP_LODWORD(r);
      carry  = 0-IPP_HIDWORD(r);
   }
   return carry;
}
#endif

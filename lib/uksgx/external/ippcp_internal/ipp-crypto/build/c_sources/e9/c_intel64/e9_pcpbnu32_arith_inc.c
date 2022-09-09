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
//     cpInc_BNU32()
// 
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnu32misc.h"
#include "pcpbnu32arith.h"


/*F*
//    Name: cpInc_BNU32
//
// Purpose: BNU32 increment.
//
// Returns:
//    carry
//
// Parameters:
//    pA    source
//    pR    result
//    ns    size
//    v     borrow
//
*F*/
IPP_OWN_DEFN (Ipp32u, cpInc_BNU32, (Ipp32u* pR, const Ipp32u* pA, cpSize ns, Ipp32u v))
{
   Ipp32u carry = v;
   cpSize i;
   for(i=0; i<ns && carry; i++) {
      Ipp64u t = (Ipp64u)carry +pA[i];
      pR[i] = IPP_LODWORD(t);
      carry = IPP_HIDWORD(t);
   }
   return carry;
}
